/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 08:57:17
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 15:31:09
 * @FilePath: \WebServerByCPP\include\RequestHandler.h
 * @Description: 请求处理器类层次结构, 实现了HTTP请求处理的核心功能
 * 采用C++面向对象设计, 通过抽象基类和继承体现多态特性
 * 包含纯虚函数作为接口规范, 强制子类实现特定行为
 * 使用工厂方法模式动态创建适合不同请求类型的处理器实例
 * 主要包含静态文件处理器和CGI处理器两种具体实现
 * 采用智能指针管理内存, 确保资源安全
 * 设计遵循开闭原则, 便于未来扩展更多请求处理类型, 如动态内容生成、API处理等
 */
#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "HttpRequest.h"
#include <memory>
#include <string>

// 前向声明
class HttpResponse;

// 抽象基类 - 体现多态
class RequestHandler
{
  protected:
    std::string doc_root;

  public:
    // 构造函数
    explicit RequestHandler(const std::string &root = "httpdocs");

    // 虚析构函数
    virtual ~RequestHandler() = default;

    // 纯虚函数 - 必须被子类实现
    virtual void handle(const HttpRequest &request, int client_socket) = 0;

    // 工厂方法
    static std::unique_ptr<RequestHandler> createHandler(const HttpRequest &request);
};

// 静态文件处理器
class StaticFileHandler : public RequestHandler
{
  public:
    explicit StaticFileHandler(const std::string &root = "httpdocs");

    // 实现基类的纯虚函数
    void handle(const HttpRequest &request, int client_socket) override;

  private:
    void serveFile(const std::string &path, int client_socket);
};

// CGI处理器
class CgiHandler : public RequestHandler
{
  public:
    explicit CgiHandler(const std::string &root = "httpdocs"); // 初始化CGI处理器, 设置CGI脚本根目录

    // 实现基类的纯虚函数
    void handle(const HttpRequest &request, int client_socket) override;

  private:
    void executeCgi(const HttpRequest &request, int client_socket, std::string path); // CGI脚本执行函数
};

#endif // REQUEST_HANDLER_H