/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 09:00:09
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 14:37:08
 * @FilePath: \WebServerByCPP\src\main.cpp
 * @Description: HTTP服务器程序入口点，负责服务器初始化、实例创建和信号处理
 * 实现了优雅的启动与关闭机制，通过信号处理（如SIGINT）支持用户中断操作
 * 采用异常处理确保在发生错误时能够正确清理资源
 * 使用跨平台初始化和清理函数，确保在不同操作系统上正常工作
 * 作为C++重构版HTTP服务器的驱动程序，展示了现代C++的错误处理和资源管理方法
 */
#include "../include/ConfigManager.h"
#include "../include/HttpServer.h"
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
        ConfigManager::loadConfig("config/server.conf");
        unsigned short port = ConfigManager::getInt("port", 6379);
        HttpServer server(port); // 创建server对象，设置默认端口6379
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

        // 清理
        HttpServer::platformCleanup();
        return 1;
    }
}