/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:30:39
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-15 19:40:46
 * @FilePath: \WebServerByCPP\include\HttpResponse.h
 * @Description:
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