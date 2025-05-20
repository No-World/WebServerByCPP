/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 08:54:57
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 15:53:25
 * @FilePath: \WebServerByCPP\include\HttpRequest.h
 * @Description: HTTP请求解析类，负责解析客户端发送的HTTP请求，提取请求方法、URL、路径、查询字符串
 * 和HTTP头信息。支持CGI请求识别，采用封装设计原则，提供安全访问内部数据的getter方法。
 * 包含友元函数用于格式化输出请求信息，便于调试和日志记录。作为HTTP服务器的核心组件，
 * 为请求处理流程提供必要的数据结构和解析功能。
 */
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <iostream>
#include <map>
#include <string>

class HttpRequest
{
  private:
    std::string method;
    std::string url;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> headers;
    bool is_cgi;
    std::string error_message; // 存储错误信息

    // 配置参数
    std::string DOC_ROOT;
    std::string DEFAULT_DOCUMENT;

    static constexpr int MAX_LINE_LENGTH = 1024; // 定义最大行长度常量

    // 辅助函数
    static size_t getLine(int sock, std::string &buf);

    // 检查文件访问权限
    bool checkFileAccess();

    // url解析函数
    static std::string urlDecode(const std::string &encoded);

  public:
    // 带配置参数的构造函数
    HttpRequest(const std::string &root = "httpdocs", const std::string &default_doc = "test.html");

    // 解析HTTP请求
    bool parse(int client_socket);

    // Getter方法（体现封装）
    const std::string &getMethod() const // 获取请求方法
    {
        return method;
    }
    const std::string &getUrl() const // 获取请求的URL
    {
        return url;
    }
    const std::string &getPath() const // 获取请求的路径
    {
        return path;
    }
    const std::string &getQueryString() const // 获取查询字符串
    {
        return query_string;
    }
    bool isCgi() const // 判断是否为CGI请求
    {
        return is_cgi;
    }

    // 获取错误信息的方法
    const std::string &getErrorMessage() const;

    // 获取HTTP头的方法
    std::string getHeader(const std::string &name) const;
    const std::map<std::string, std::string> &getHeaders() const
    {
        return headers;
    }

    // 获取配置参数的方法
    const std::string &getDocRoot() const
    {
        return DOC_ROOT;
    }
    // 获取文档根目录
    const std::string &getDefaultDocument() const
    {
        return DEFAULT_DOCUMENT;
    }

    // 友元函数用于调试输出
    friend std::ostream &operator<<(std::ostream &os, const HttpRequest &req); // 重载输出运算符‘<<’用于打印请求信息
};

#endif // HTTP_REQUEST_H