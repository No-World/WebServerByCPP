/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:26:23
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 16:00:52
 * @FilePath: \WebServerByCPP\src\HttpRequest.cpp
 * @Description: HTTP请求解析实现, 负责从客户端socket读取数据并解析HTTP请求
 * 支持GET和POST请求处理, 包含请求解析、查询字符串提取、文件路径解析和HTTP头解析
 * 实现了跨平台兼容(Windows/Unix)的网络数据读取和字符串处理
 */
#include "../include/HttpRequest.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

// 添加网络编程头文件（跨平台兼容）
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define strcasecmp _stricmp // Windows下strcasecmp的替代
const char PATH_SEP = '\\';

#else
#include <strings.h> // 提供strcasecmp
#include <sys/socket.h>
#include <unistd.h>
const char PATH_SEP = '/';

#endif

// 构造函数初始化
HttpRequest::HttpRequest(const std::string &root, const std::string &default_doc)
    : method(), url(), path(), query_string(), headers(), is_cgi(false), error_message(), DOC_ROOT(root),
      DEFAULT_DOCUMENT(default_doc)
{
    // 从配置参数初始化
}

// 静态方法：从socket读取一行数据
size_t HttpRequest::getLine(int sock, std::string &buf)
{
    // 使用静态缓冲区提高性能
    thread_local static char read_buffer[MAX_LINE_LENGTH];
    thread_local static int buffer_pos = 0;
    thread_local static int bytes_in_buffer = 0;

    buf.clear(); // 清空字符串
    char c = '\0';

    while (buf.length() < MAX_LINE_LENGTH - 1 && c != '\n')
    {
        // 如果缓冲区为空，从socket读取更多数据
        if (buffer_pos >= bytes_in_buffer)
        {
            bytes_in_buffer = recv(sock, read_buffer, sizeof(read_buffer), 0);
            if (bytes_in_buffer <= 0)
                break;
            buffer_pos = 0;
        }

        c = read_buffer[buffer_pos++];

        // 处理CR+LF序列 (Windows风格的行结束)
        if (c == '\r')
        {
            // 检查下一个字符是否为\n
            if (buffer_pos < bytes_in_buffer)
            {
                if (read_buffer[buffer_pos] == '\n')
                {
                    buffer_pos++; // 跳过\n
                }
                c = '\n';
            }
            else
            {
                // 如果\r在缓冲区末尾，读取下一个字符
                char next;
                int peek_result = recv(sock, &next, 1, MSG_PEEK);
                if (peek_result > 0 && next == '\n')
                {
                    recv(sock, &next, 1, 0); // 消费\n
                }
                c = '\n';
            }
        }

        if (c != '\n') // 不存储最后的换行符
        {
            buf.push_back(c);
        }
    }

    return buf.length();
}

// 判断文件是否存在且检查其类型
bool HttpRequest::checkFileAccess()
{
    struct stat st;

    // 如果是根路径请求，直接使用默认文档
    if (url == "/" || url.empty())
    {
        path = DOC_ROOT;
        if (path.back() != PATH_SEP)
            path += PATH_SEP;
        path += DEFAULT_DOCUMENT;
    }

    if (stat(path.c_str(), &st) == -1)
    {
        // 文件不存在或权限不足
        error_message = "File not found: " + path;
        return false;
    }

    // 处理目录
    if ((st.st_mode & S_IFMT) == S_IFDIR)
    {
        if (path.back() != PATH_SEP)
        {
            path += PATH_SEP;
        }
        path += DEFAULT_DOCUMENT;

        // 再次检查文件是否存在
        if (stat(path.c_str(), &st) == -1)
        {
            error_message = "Default document not found: " + path;
            return false;
        }
    }

    // 检查文件是否可执行
    if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
    {
        is_cgi = true;
    }

    return true;
}

// URL解码函数
std::string HttpRequest::urlDecode(const std::string &encoded)
{
    std::string result;
    for (size_t i = 0; i < encoded.length(); ++i)
    {
        if (encoded[i] == '%' && i + 2 < encoded.length())
        {
            int value;
            std::istringstream is(encoded.substr(i + 1, 2));
            if (is >> std::hex >> value)
            {
                result += static_cast<char>(value);
                i += 2;
            }
            else
            {
                result += encoded[i];
            }
        }
        else if (encoded[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += encoded[i];
        }
    }
    return result;
}

// 解析HTTP请求
bool HttpRequest::parse(int client_socket)
{
    // 重置静态缓冲变量
    thread_local static int buffer_pos = 0;
    thread_local static int bytes_in_buffer = 0;
    buffer_pos = 0;
    bytes_in_buffer = 0;

    std::string buf;
    int numchars;

    // 防止恶意请求设置超时
    // (实际实现应考虑设置socket超时选项)

    // 读取第一行，包含请求方法和URL
    numchars = getLine(client_socket, buf);
    if (numchars <= 0)
    {
        error_message = "Empty request";
        return false;
    }

    // 解析请求方法
    size_t start = 0;
    size_t end = buf.find_first_of(" \t");
    if (end != std::string::npos)
    {
        method = buf.substr(start, end - start);
        // 统一转换为大写以便不区分大小写比较
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    }
    else
    {
        error_message = "Invalid request format";
        return false;
    }

    // 检查请求方法是否支持
    if (method != "GET" && method != "POST")
    {
        error_message = "Method not supported: " + method;
        return false;
    }

    // POST请求一定触发CGI
    if (method == "POST")
    {
        is_cgi = true;
    }

    // 解析URL
    start = buf.find_first_not_of(" \t", end);
    if (start == std::string::npos)
    {
        error_message = "URL not found in request";
        return false;
    }

    end = buf.find_first_of(" \t", start);
    url = urlDecode(buf.substr(start, (end != std::string::npos) ? end - start : std::string::npos));

    // 防止目录遍历攻击
    if (url.find("..") != std::string::npos)
    {
        error_message = "Invalid URL path (directory traversal attempt)";
        return false;
    }

    // 如果URL是根路径，使用默认文档
    if (url == "/" || url.empty())
    {
        // 不要修改url，保持为"/"
        // 不要这样做：url = DEFAULT_DOCUMENT;
    }

    // 处理GET请求的查询字符串
    if (method == "GET")
    {
        size_t query_pos = url.find('?');
        if (query_pos != std::string::npos)
        {
            query_string = url.substr(query_pos + 1);
            url = url.substr(0, query_pos);
            is_cgi = true;
        }
    }

    // 构造文件路径
    // 确保DOC_ROOT末尾有斜杠，url开头没有斜杠
    std::string doc_root_path = DOC_ROOT;
    if (!doc_root_path.empty() && doc_root_path.back() != PATH_SEP)
        doc_root_path += PATH_SEP;
    if (!url.empty() && url[0] == PATH_SEP)
        url = url.substr(1);
    path = doc_root_path + url;

    // 规范化路径格式并处理默认文件
#ifdef _WIN32
    std::replace(path.begin(), path.end(), '/', PATH_SEP);
#else
    std::replace(path.begin(), path.end(), '\\', PATH_SEP);
#endif

    // 如果路径以'/'结尾或是根路径，添加默认文档
    if (path.back() == PATH_SEP || url == "/" || url.empty())
    {
        if (path.back() != PATH_SEP)
            path += PATH_SEP;
        path += DEFAULT_DOCUMENT;
    }

    // 检查文件访问权限
    if (!checkFileAccess())
    {
        // 清空剩余的请求头
        while ((numchars > 0) && buf != "\n")
        {
            numchars = getLine(client_socket, buf);
        }
        return false;
    }

    // 读取并存储HTTP头信息
    numchars = getLine(client_socket, buf);
    while ((numchars > 0) && !buf.empty())
    {
        // 移除末尾的\r (如果存在)
        if (!buf.empty() && buf.back() == '\r')
        {
            buf.pop_back();
        }

        // 解析头部信息
        size_t colon_pos = buf.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string header_name = buf.substr(0, colon_pos);
            std::string header_value = buf.substr(colon_pos + 1);

            // 规范化头部名称 (不区分大小写)
            std::transform(header_name.begin(), header_name.end(), header_name.begin(), ::tolower);

            // 去除值前面的空格
            size_t value_start = header_value.find_first_not_of(" \t");
            if (value_start != std::string::npos)
            {
                header_value = header_value.substr(value_start);
            }

            // 存储头部信息
            headers[header_name] = header_value;

            // 检查Content-Length (对POST请求)
            if (method == "POST" && header_name == "content-length")
            {
                // 实际处理应在这里解析并验证内容长度
            }
        }

        numchars = getLine(client_socket, buf);
    }

    return true; // 解析成功
}

// 获取错误信息方法
const std::string &HttpRequest::getErrorMessage() const
{
    return error_message;
}

std::string HttpRequest::getHeader(const std::string &name) const
{
    auto it = headers.find(name);
    if (it != headers.end())
    {
        return it->second;
    }
    return ""; // 未找到则返回空字符串
}

// 友元函数：重载输出运算符
std::ostream &operator<<(std::ostream &os, const HttpRequest &req)
{
    os << "Method: " << req.method << "\n"
       << "URL: " << req.url << "\n"
       << "Path: " << req.path << "\n"
       << "Query String: " << req.query_string << "\n"
       << "CGI Request: " << (req.is_cgi ? "Yes" : "No") << "\n"
       << "Headers:\n";

    // 使用C++11 range-based for循环
    for (const auto &header : req.headers)
    {
        os << "  " << header.first << ": " << header.second << "\n";
    }

    if (!req.error_message.empty())
    {
        os << "Error: " << req.error_message << "\n";
    }

    return os;
}