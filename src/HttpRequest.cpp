/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:26:23
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-15 19:42:09
 * @FilePath: \WebServerByCPP\src\HttpRequest.cpp
 * @Description:
 */
#include "../include/HttpRequest.h"
#include <cstring>
#include <iostream>
#include <sys/stat.h>

// 构造函数：初始化成员变量
HttpRequest::HttpRequest() : is_cgi(false)
{
    // 默认初始化
}

// 静态方法：从socket读取一行数据
int HttpRequest::getLine(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);

        if (n > 0)
        {
            if (c == '\r')
            {
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';
    return i;
}

// 解析HTTP请求
bool HttpRequest::parse(int client_socket)
{
    char buf[1024];
    char method_temp[255];
    char url_temp[255];
    char path_temp[512];
    int numchars;
    size_t i, j;
    struct stat st;
    char *query_string_temp = nullptr;

    // 读取第一行，包含请求方法和URL
    numchars = getLine(client_socket, buf, sizeof(buf));
    if (numchars <= 0)
    {
        return false;
    }

    // 解析请求方法
    i = 0;
    j = 0;
    while (!isspace(buf[j]) && (i < sizeof(method_temp) - 1))
    {
        method_temp[i++] = buf[j++];
    }
    method_temp[i] = '\0';
    method = method_temp; // 使用C++的std::string

    // 检查请求方法是否支持
    if (strcasecmp(method.c_str(), "GET") != 0 && strcasecmp(method.c_str(), "POST") != 0)
    {
        // 不支持的方法
        return false;
    }

    // POST请求一定触发CGI
    if (strcasecmp(method.c_str(), "POST") == 0)
    {
        is_cgi = true;
    }

    // 跳过空格
    while (isspace(buf[j]) && (j < sizeof(buf)))
    {
        j++;
    }

    // 解析URL
    i = 0;
    while (!isspace(buf[j]) && (i < sizeof(url_temp) - 1) && (j < sizeof(buf)))
    {
        url_temp[i++] = buf[j++];
    }
    url_temp[i] = '\0';
    url = url_temp; // 使用C++的std::string

    // 处理GET请求的查询字符串
    if (strcasecmp(method.c_str(), "GET") == 0)
    {
        query_string_temp = url_temp;
        while ((*query_string_temp != '?') && (*query_string_temp != '\0'))
        {
            query_string_temp++;
        }

        if (*query_string_temp == '?')
        {
            is_cgi = true;
            *query_string_temp = '\0';
            query_string_temp++;
            query_string = query_string_temp; // 设置查询字符串
        }
    }

    // 构造文件路径
    sprintf(path_temp, "httpdocs%s", url.c_str());
    path = path_temp;

    // 处理目录请求，默认添加test.html
    if (path[path.length() - 1] == '/')
    {
        path += "test.html";
    }

    // 检查文件是否存在
    if (stat(path.c_str(), &st) == -1)
    {
        // 清空剩余的请求头
        while ((numchars > 0) && strcmp("\n", buf))
        {
            numchars = getLine(client_socket, buf, sizeof(buf));
        }
        return false; // 文件不存在
    }

    // 处理目录
    if ((st.st_mode & S_IFMT) == S_IFDIR)
    {
        path += "/test.html";
    }

    // 检查文件是否可执行，如果可执行则触发CGI
    if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
    {
        is_cgi = true;
    }

    // 读取并存储HTTP头信息
    numchars = getLine(client_socket, buf, sizeof(buf));
    while ((numchars > 0) && strcmp("\n", buf))
    {
        buf[numchars - 1] = '\0'; // 移除换行符

        // 解析头部信息
        char *colon = strchr(buf, ':');
        if (colon)
        {
            *colon = '\0';
            std::string header_name = buf;
            std::string header_value = colon + 1;

            // 去除值前面的空格
            while (header_value[0] == ' ')
            {
                header_value.erase(0, 1);
            }

            // 存储头部信息
            headers[header_name] = header_value;
        }

        numchars = getLine(client_socket, buf, sizeof(buf));
    }

    return true; // 解析成功
}

// 友元函数：重载输出运算符
std::ostream &operator<<(std::ostream &os, const HttpRequest &req)
{
    os << "Method: " << req.method << "\n"
       << "URL: " << req.url << "\n"
       << "Path: " << req.path << "\n"
       << "Query: " << req.query_string << "\n"
       << "CGI: " << (req.is_cgi ? "Yes" : "No") << "\n"
       << "Headers:\n";

    for (const auto &header : req.headers)
    {
        os << "  " << header.first << ": " << header.second << "\n";
    }

    return os;
}