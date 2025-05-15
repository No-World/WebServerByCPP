/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 08:54:57
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-15 09:34:11
 * @FilePath: \WebServerByCPP\include\HttpRequest.h
 * @Description: HTTP请求解析类，负责解析客户端发送的HTTP请求，提取请求方法、URL、路径、查询字符串
 * 和HTTP头信息。支持CGI请求识别，采用封装设计原则，提供安全访问内部数据的getter方法。
 * 包含友元函数用于格式化输出请求信息，便于调试和日志记录。作为HTTP服务器的核心组件，
 * 为请求处理流程提供必要的数据结构和解析功能。
 */
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

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

    // 辅助函数
    static int getLine(int sock, char *buf, int size);

  public:
    // 构造函数
    HttpRequest();

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

    // 友元函数用于调试输出
    friend std::ostream &operator<<(std::ostream &os, const HttpRequest &req); // 重载输出运算符‘<<’用于打印请求信息
};

#endif // HTTP_REQUEST_H