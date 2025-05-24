/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:25:52
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 18:27:23
 * @FilePath: \WebServerByCPP\src\HttpServer.cpp
 * @Description: HTTP服务器核心实现，提供服务器的初始化、启动、停止和请求处理功能
 * 实现了多线程客户端请求处理，提高并发性能，支持短连接和异常处理机制
 * 集成ConfigManager读取配置参数，灵活调整服务器行为
 * 通过组合HttpRequest、HttpResponse和RequestHandler等组件，实现完整的HTTP请求响应流程
 */
#include "../include/HttpServer.h"
#include "../include/ConfigManager.h"
#include "../include/HttpRequest.h"
#include "../include/HttpResponse.h"
#include "../include/RequestHandler.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

// 构造函数
HttpServer::HttpServer(unsigned short port)
    : port(ConfigManager::getInt("port", port)), server_socket(-1), running(false)
{
    doc_root = ConfigManager::getString("document_root", "httpdocs");
    default_document = ConfigManager::getString("default_document", "test.html");
}

// 析构函数
HttpServer::~HttpServer()
{
    stop();
    if (server_socket != -1)
    {
        close(server_socket);
        server_socket = -1;
    }
}

// 初始化socket
void HttpServer::initSocket()
{
    struct sockaddr_in server_addr;

    // 创建socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1)
    {
        throw std::runtime_error("无法创建socket");
    }
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(server_socket);
        throw std::runtime_error("设置socket选项失败");
    }

    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(server_socket);
        throw std::runtime_error("绑定socket失败");
    }

    // 监听
    if (listen(server_socket, 5) < 0)
    {
        close(server_socket);
        throw std::runtime_error("监听socket失败");
    }

    std::cout << "HTTP服务器启动在端口 " << port << '\n';
}

// 启动服务器
void HttpServer::start()
{
    if (running)
        return;

    try
    {
        initSocket();
        running = true;

        struct sockaddr_in client_addr;
#ifdef _WIN32
        int client_addr_len = sizeof(client_addr);
#else
        socklen_t client_addr_len = sizeof(client_addr);
#endif

        std::cout << "服务器等待连接..." << '\n';

        while (running)
        {
            // 接受连接
            int client_sock = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

            if (client_sock == -1)
            {
                if (!running)
                {
                    break;
                }
                std::cerr << "接受客户端连接失败" << '\n';
                continue;
            }

            std::cout << "新连接: IP=" << inet_ntoa(client_addr.sin_addr) << ", 端口=" << ntohs(client_addr.sin_port)
                      << '\n';

            // 创建新线程处理请求
            std::thread client_thread(&HttpServer::handleClient, this, client_sock);
            client_thread.detach();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "服务器错误: " << e.what() << '\n';
        stop();
    }
}

// 停止服务器
void HttpServer::stop()
{
    if (!running)
        return;

    running = false;

    if (server_socket != -1)
    {
        close(server_socket);
        server_socket = -1;
    }

    std::cout << "服务器已停止" << '\n';
}

// 处理客户端请求
void HttpServer::handleClient(int client_sock)
{
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    try
    {
        HttpRequest request(doc_root, default_document);

        // 解析请求
        if (!request.parse(client_sock))
        {
            // 检查error_message判断是文件不存在还是真正的请求格式错误
            if (request.getErrorMessage().find("File not found") != std::string::npos)
            {
                // debug信息
                std::cerr << "========== HttpServer::handleClient error Info ==========" << '\n';
                std::cerr << "URL: " << request.getUrl() << '\n';
                std::cerr << "path: " << request.getPath() << '\n';
                std::cerr << "error message: " << request.getErrorMessage() << '\n';
                std::cerr << "========== HttpServer::handleClient error Info End ==========" << '\n';

                // 文件不存在返回404
                HttpResponse response = HttpResponse::notFound();
                response.send(client_sock);
            }
            else
            {
                // 请求错误返回400
                HttpResponse response = HttpResponse::badRequest();
                response.send(client_sock);
            }
            return;
        }

        // 根据请求类型创建处理器
        auto handler = RequestHandler::createHandler(request);

        // 处理请求
        handler->handle(request, client_sock);
    }
    catch (const std::exception &e)
    {
        std::cerr << "处理请求错误: " << e.what() << '\n';
        try
        {
            // 尝试发送500错误
            HttpResponse response = HttpResponse::serverError();
            response.send(client_sock);
        }
        catch (...)
        {
            // 忽略发送错误响应时的异常
            std::cerr << "发送错误响应失败" << '\n';
        }
    }
    // 关闭连接
    close(client_sock);
}