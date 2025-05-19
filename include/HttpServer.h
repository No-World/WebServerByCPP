/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 08:51:24
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 16:08:48
 * @FilePath: \WebServerByCPP\include\HttpServer.h
 * @Description: HTTP服务器核心类，实现了跨平台(Windows/Unix)的网络服务器功能。
 * 负责socket初始化、客户端连接管理和请求分发
 * 采用多线程模型处理并发请求, 提供优雅的启动和关闭机制
 * 遵循RAII设计原则, 通过构造函数和析构函数自动管理资源
 * 使用C++11标准库特性如std::thread和std::atomic实现线程安全的并发控制
 * 类设计禁止复制, 确保服务器实例的唯一性和资源安全
 */
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <atomic>
#include <string>
#include <thread>
#include <vector>

// 跨平台头文件处理
#ifdef _WIN32 // Windows平台
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#else // Unix/Linux平台
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#endif

class HttpServer
{
  private:
    // 成员变量
    unsigned short port;              // 服务器端口
    int server_socket;                // 服务器socket
    std::atomic<bool> running;        // 运行状态标志
    std::vector<std::thread> threads; // 线程池

    std::string doc_root;         // 文档根目录
    std::string default_document; // 默认文档

    // 私有方法
    void handleClient(int client_socket); // 处理客户端请求
    void initSocket();                    // 初始化socket

    // 阻止复制
    HttpServer(const HttpServer &) = delete;            // 禁止复制构造函数
    HttpServer &operator=(const HttpServer &) = delete; // 禁止赋值操作符

  public:
    // 构造与析构
    explicit HttpServer(unsigned short port = 6379); // 构造函数, 设置默认端口6379
    const std::string &getDocRoot() const            // 获取文档根目录
    {
        return doc_root;
    }
    const std::string &getDefaultDocument() const // 获取默认文档
    {
        return default_document;
    }
    ~HttpServer(); // 析构函数

    // 主要接口
    void start(); // 启动服务器
    void stop();  // 停止服务器

    // 静态平台初始化/清理
    static void platformInit();    // 初始化平台
    static void platformCleanup(); // 清理平台
};

#endif // HTTP_SERVER_H