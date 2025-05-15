/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:25:52
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-15 19:41:05
 * @FilePath: \WebServerByCPP\src\HttpServer.cpp
 * @Description:
 */
#include "../include/HttpServer.h"
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
HttpServer::HttpServer(unsigned short port) : port(port), server_socket(-1), running(false)
{
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
#ifdef _WIN32
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
#else
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
#endif

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
    try
    {
        // 创建请求对象
        HttpRequest request;

        // 解析请求
        if (request.parse(client_sock))
        {
            // 根据请求类型创建处理器
            auto handler = RequestHandler::createHandler(request);

            // 处理请求
            handler->handle(request, client_sock);
        }
        else
        {
            // 解析失败，返回400错误
            HttpResponse response = HttpResponse::badRequest();
            response.send(client_sock);
        }
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