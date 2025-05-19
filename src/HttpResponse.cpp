/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:26:33
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 18:20:30
 * @FilePath: \WebServerByCPP\src\HttpResponse.cpp
 * @Description: HTTP响应类实现，负责构建和发送HTTP响应，包括状态码、头部和响应体
 * 提供了标准HTTP响应的工厂方法，支持200 OK、404 Not Found、400 Bad Request等常见状态
 * 实现了文件传输功能，能够高效地将文件内容发送给客户端
 * 采用了跨平台设计，在Windows和Unix/Linux系统上提供一致的接口
 * 作为服务器响应处理的核心组件，确保了HTTP协议的正确实现
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

    // 读取文件内容到字符串
    char temp_buf[1024];
    size_t bytes_read;
    while ((bytes_read = fread(temp_buf, 1, sizeof(temp_buf), resource)) > 0)
    {
        ::send(client_socket, temp_buf, bytes_read, 0);
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

    std::string body =
        "<HTML><TITLE>404 Not Found</TITLE>\r\n"
        "<BODY><P>404 Not Found<br>\r\n"
        "The server could not fulfill your request because the resource specified is unavailable or nonexistent.\r\n"
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