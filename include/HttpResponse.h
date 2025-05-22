/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:30:39
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-15 19:40:46
 * @FilePath: \WebServerByCPP\include\HttpResponse.h
 * @Description: HTTP响应类定义，负责创建和发送符合HTTP协议的服务器响应
 * 支持设置状态码、头部信息和响应体，提供了流式API设计
 * 包含常用HTTP状态的工厂方法，简化200/404/400/500等标准响应的创建
 * 实现了文件传输功能，支持高效发送静态文件内容
 * 自动添加标准头信息，确保响应符合HTTP规范要求
 */
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <map>
#include <string>

class HttpResponse
{
  private:
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;

    // 添加标准头部信息
    void addStandardHeaders();

  public:
    // 构造函数
    HttpResponse();

    // 设置状态码和消息
    void setStatus(int code, const std::string &message);

    // 添加头部信息
    void addHeader(const std::string &name, const std::string &value);

    // 设置响应体
    void setBody(const std::string &content);

    // 发送响应
    void send(int client_socket);

    // 工具方法：发送文件内容
    void sendFile(int client_socket, FILE *resource);

    // 预定义常用响应
    static HttpResponse ok();
    static HttpResponse notFound();
    static HttpResponse badRequest();
    static HttpResponse serverError();
    static HttpResponse notImplemented();
};

#endif // HTTP_RESPONSE_H