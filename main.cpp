#include "include/HttpServer.h"
#include <csignal>
#include <iostream>

// 全局服务器指针，用于信号处理
HttpServer *g_server = nullptr;

// 信号处理函数
void signalHandler(int signal)
{
    std::cout << "接收到信号: " << signal << std::endl;
    if (g_server)
    {
        g_server->stop();
    }
}

int main()
{
    try
    {
        // 初始化平台
        HttpServer::platformInit();

        // 创建服务器
        HttpServer server(6379); // 创建server对象，设置默认端口6379
        g_server = &server;

        // 注册信号处理
        std::signal(SIGINT, signalHandler);

        std::cout << "HTTP服务器启动中..." << std::endl;
        server.start(); // 这会阻塞直到服务器停止

        // 清理
        HttpServer::platformCleanup();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "错误: " << e.what() << std::endl;
        HttpServer::platformCleanup();
        return 1;
    }
}