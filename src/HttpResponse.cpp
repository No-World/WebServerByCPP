/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:26:33
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-15 19:41:22
 * @FilePath: \WebServerByCPP\src\HttpResponse.cpp
 * @Description:
 */
#include "../include/HttpResponse.h"
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>

#else
#include <sys/socket.h>
#include <unistd.h>

#endif

#define SERVER_STRING "Server: NoWorld's http/0.1.0\r\n"

HttpResponse::HttpResponse() : status_code(200), status_message("OK")
{
    addStandardHeaders();
}

void HttpResponse::setStatus(int code, const std::string &message)
{
    status_code = code;
    status_message = message;
}

void HttpResponse::addHeader(const std::string &name, const std::string &value)
{
    headers[name] = value;
}

void HttpResponse::setBody(const std::string &content)
{
    body = content;
    // 更新Content-Length头
    headers["Content-Length"] = std::to_string(body.length());
}

void HttpResponse::addStandardHeaders()
{
    headers["Server"] = "NoWorld's http/0.1.0";
    headers["Content-Type"] = "text/html";
}

void HttpResponse::send(int client_socket)
{
    // 发送响应行
    std::string status_line = "HTTP/1.0 " + std::to_string(status_code) + " " + status_message + "\r\n";
    ::send(client_socket, status_line.c_str(), status_line.length(), 0);

    // 发送头部
    for (const auto &header : headers)
    {
        std::string header_line = header.first + ": " + header.second + "\r\n";
        ::send(client_socket, header_line.c_str(), header_line.length(), 0);
    }

    // 发送空行，表示头部结束
    ::send(client_socket, "\r\n", 2, 0);

    // 发送响应体
    if (!body.empty())
    {
        ::send(client_socket, body.c_str(), body.length(), 0);
    }
}

void HttpResponse::sendFile(int client_socket, FILE *resource)
{
    char buf[1024];

    // 先发送头部
    std::string status_line = "HTTP/1.0 " + std::to_string(status_code) + " " + status_message + "\r\n";
    ::send(client_socket, status_line.c_str(), status_line.length(), 0);

    for (const auto &header : headers)
    {
        std::string header_line = header.first + ": " + header.second + "\r\n";
        ::send(client_socket, header_line.c_str(), header_line.length(), 0);
    }

    // 发送空行
    ::send(client_socket, "\r\n", 2, 0);

    // 读取并发送文件内容
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        ::send(client_socket, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

// 静态方法：创建常用响应
HttpResponse HttpResponse::ok()
{
    HttpResponse response;
    response.setStatus(200, "OK");
    return response;
}

HttpResponse HttpResponse::notFound()
{
    HttpResponse response;
    response.setStatus(404, "NOT FOUND");

    std::string body = "<HTML><TITLE>Not Found</TITLE>\r\n"
                       "<BODY><P>The server could not fulfill\r\n"
                       "your request because the resource specified\r\n"
                       "is unavailable or nonexistent.\r\n"
                       "</BODY></HTML>\r\n";
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::badRequest()
{
    HttpResponse response;
    response.setStatus(400, "BAD REQUEST");

    std::string body = "<P>Your browser sent a bad request, "
                       "such as a POST without a Content-Length.\r\n";
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::serverError()
{
    HttpResponse response;
    response.setStatus(500, "INTERNAL SERVER ERROR");

    std::string body = "<P>Error prohibited CGI execution.\r\n";
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::notImplemented()
{
    HttpResponse response;
    response.setStatus(501, "METHOD NOT IMPLEMENTED");

    std::string body = "<HTML><HEAD><TITLE>Method Not Implemented\r\n"
                       "</TITLE></HEAD>\r\n"
                       "<BODY><P>HTTP request method not supported.\r\n"
                       "</BODY></HTML>\r\n";
    response.setBody(body);
    return response;
}