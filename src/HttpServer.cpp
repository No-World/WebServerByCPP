/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:25:52
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 17:45:27
 * @FilePath: \WebServerByCPP\src\HttpServer.cpp
 * @Description: HTTP服务器核心实现，提供服务器的初始化、启动、停止和请求处理功能
 * 采用跨平台设计，通过条件编译支持Windows和Unix/Linux系统的套接字操作差异
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

// 跨平台相关方法
void HttpServer::platformInit()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        throw std::runtime_error("WSAStartup失败");
    }
#endif
}

void HttpServer::platformCleanup()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

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
#ifdef _WIN32
        closesocket(server_socket);
#else
        close(server_socket);
#endif
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
#ifdef _WIN32 // Windows 环境下
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0)
    {
        closesocket(server_socket);
        throw std::runtime_error("设置socket选项失败");
    }
#else
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(server_socket);
        throw std::runtime_error("设置socket选项失败");
    }
#endif

    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
#ifdef _WIN32
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
#ifdef _WIN32
        closesocket(server_socket);
#else
        close(server_socket);
#endif
        throw std::runtime_error("绑定socket失败");
    }

    // 监听
    if (listen(server_socket, 5) < 0)
    {
#ifdef _WIN32
        closesocket(server_socket);
#else
        close(server_socket);
#endif
        throw std::runtime_error("监听socket失败");
    }

    std::cout << "HTTP服务器启动在端口 " << port << std::endl;
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

        std::cout << "服务器等待连接..." << std::endl;

        while (running)
        {
            // 接受连接
            int client_sock = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

            if (client_sock == -1)
            {
                if (!running)
                    break;
                std::cerr << "接受客户端连接失败" << std::endl;
                continue;
            }

            std::cout << "新连接: IP=" << inet_ntoa(client_addr.sin_addr) << ", 端口=" << ntohs(client_addr.sin_port)
                      << std::endl;

            // 创建新线程处理请求
            std::thread client_thread(&HttpServer::handleClient, this, client_sock);
            client_thread.detach();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "服务器错误: " << e.what() << std::endl;
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
#ifdef _WIN32
        closesocket(server_socket);
#else
        close(server_socket);
#endif
        server_socket = -1;
    }

    std::cout << "服务器已停止" << std::endl;
}

// 处理客户端请求
void HttpServer::handleClient(int client_sock)
{
#ifdef _WIN32
    DWORD timeout = 5000; // 5秒
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
    try
    {
        HttpRequest request(doc_root, default_document);

        // 解析请求
        if (!request.parse(client_sock))
        {
            // 检查error_message判断是文件不存在还是真正的请求格式错误
            if (request.getErrorMessage().find("File not found") != std::string::npos)
            {
                // 文件不存在返回404
                HttpResponse response = HttpResponse::notFound();
                std::cout << client_sock << std::endl;
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
        std::cerr << "处理请求错误: " << e.what() << std::endl;
        try
        {
            // 尝试发送500错误
            HttpResponse response = HttpResponse::serverError();
            response.send(client_sock);
        }
        catch (...)
        {
            // 忽略发送错误响应时的异常
        }
    }

    // 关闭连接
#ifdef _WIN32
    closesocket(client_sock);
#else
    close(client_sock);
#endif
}