/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-15 19:26:41
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 18:06:28
 * @FilePath: \WebServerByCPP\src\RequestHandler.cpp
 * @Description: HTTP请求处理器实现，采用策略模式区分静态文件和CGI处理
 * 包含RequestHandler基类及StaticFileHandler和CgiHandler两个子类
 * StaticFileHandler负责读取和发送静态文件内容，实现了基本的HTTP静态资源服务
 * CgiHandler实现了CGI脚本执行机制，支持GET和POST方法，使用管道进行进程间通信
 * 提供了跨平台支持，在Windows和Unix/Linux系统下有不同实现方式
 * 通过工厂方法根据请求类型自动创建合适的处理器实例
 */
#include "../include/RequestHandler.h"
#include "../include/HttpResponse.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
const char PATH_SEP = '\\';

#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
const char PATH_SEP = '/';

#endif

// 基类构造函数
RequestHandler::RequestHandler(const std::string &root) : doc_root(root)
{
}

// 工厂方法：根据请求类型创建处理器
std::unique_ptr<RequestHandler> RequestHandler::createHandler(const HttpRequest &request)
{
    if (request.isCgi())
    {
        return std::make_unique<CgiHandler>(request.getDocRoot());
    }
    else
    {
        return std::make_unique<StaticFileHandler>(request.getDocRoot());
    }
}

// StaticFileHandler实现
StaticFileHandler::StaticFileHandler(const std::string &root) : RequestHandler(root)
{
}

void StaticFileHandler::handle(const HttpRequest &request, int client_socket)
{
    // 构造完整的文件路径
    std::string fullPath;
    if (!doc_root.empty() && doc_root.back() != '/' && !request.getPath().empty() && request.getPath()[0] != PATH_SEP)
        fullPath = doc_root + PATH_SEP + request.getPath();
    else
        fullPath = doc_root + request.getPath();

    // 处理静态文件
    serveFile(fullPath, client_socket);
}

void StaticFileHandler::serveFile(const std::string &path, int client_socket)
{
    FILE *resource = fopen(path.c_str(), "r");
    std::cout << "请求的文件路径: " << path << std::endl;

    if (resource == nullptr)
    {
        // 获取当前工作目录
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != nullptr)
        {
            std::cout << "文件不存在: " << path << std::endl;
            std::cout << "当前工作目录: " << cwd << std::endl;
        }

        // 文件不存在，返回404
        HttpResponse response = HttpResponse::notFound();
        response.send(client_socket);
        return;
    }

    // 文件存在，发送文件内容
    HttpResponse response = HttpResponse::ok();
    response.sendFile(client_socket, resource);

    fclose(resource);
}

// CgiHandler实现
CgiHandler::CgiHandler(const std::string &root) : RequestHandler(root)
{
}

void CgiHandler::handle(const HttpRequest &request, int client_socket)
{
    std::string path = doc_root + request.getPath();
    executeCgi(request, client_socket, path);
}

void CgiHandler::executeCgi(const HttpRequest &request, int client_socket, std::string path)
{
    const std::string &method = request.getMethod();
    const std::string &query_string = request.getQueryString();

    int cgi_output[2];
    int cgi_input[2];

#ifdef _WIN32 // Windows实现CGI执行 - 未实现
    HttpResponse response = HttpResponse::serverError();
    response.setBody("<html><body>"
                     "<h2>CGI Error</h2>"
                     "<p>CGI execution is not fully implemented in Windows version.</p>"
                     "<p>Required feature will be added in future releases.</p>"
                     "<p>Please use Linux/Unix for full CGI support.</p>"
                     "</body></html>");
    response.send(client_socket);

#else // Unix/Linux实现
    pid_t pid;
    int status;
    int content_length = -1;

    // 检查Content-Length（如果是POST请求）
    if (method == "POST")
    {
        std::string contentLength = request.getHeader("Content-Length");
        if (!contentLength.empty())
        {
            content_length = std::stoi(contentLength);
        }
        else
        {
            // 缺少Content-Length，返回400
            HttpResponse response = HttpResponse::badRequest();
            response.send(client_socket);
            return;
        }
    }

    // 发送HTTP响应状态行
    HttpResponse response = HttpResponse::ok();

    // 创建管道
    if (pipe(cgi_output) < 0 || pipe(cgi_input) < 0)
    {
        response = HttpResponse::serverError();
        response.send(client_socket);
        return;
    }

    // 创建子进程
    if ((pid = fork()) < 0)
    {
        // 关闭管道再返回
        close(cgi_output[0]);
        close(cgi_output[1]);
        close(cgi_input[0]);
        close(cgi_input[1]);

        response = HttpResponse::serverError();
        response.send(client_socket);
        return;
    }

    if (pid == 0)
    { // 子进程运行CGI脚本
        std::string meth_env;
        std::string query_env;
        std::string length_env;

        // 重定向标准输入/输出
        dup2(cgi_output[1], STDOUT_FILENO);
        dup2(cgi_input[0], STDIN_FILENO);

        // 关闭不需要的管道端
        close(cgi_output[0]);
        close(cgi_input[1]);

        // 设置环境变量
        meth_env = "REQUEST_METHOD=" + method;
        putenv(strdup(meth_env.c_str()));

        if (method == "GET")
        {
            query_env = "QUERY_STRING=" + query_string;
            putenv(strdup(query_env.c_str()));
        }
        else
        { // POST
            length_env = "CONTENT_LENGTH=" + std::to_string(content_length);
            putenv(strdup(length_env.c_str()));
        }

        // 执行CGI脚本
        execl(path.c_str(), path.c_str(), nullptr);
        exit(0);
    }
    else
    { // 父进程
        close(cgi_output[1]);
        close(cgi_input[0]);

        // 如果是POST请求，将请求体发送给CGI脚本
        if (method == "POST")
        {
            // 这部分需要用真实的请求体数据替换
            // 简化实现，实际中应从socket读取
            char c;
            for (int i = 0; i < content_length; i++)
            {
                if (recv(client_socket, &c, 1, 0) > 0)
                {
                    write(cgi_input[1], &c, 1);
                }
            }
        }

        // 从CGI脚本读取输出并发送给客户端
        // 首先发送HTTP响应头
        std::string status_line = "HTTP/1.0 200 OK\r\n";
        send(client_socket, status_line.c_str(), status_line.length(), 0);

        // 读取CGI输出并发送给客户端
        char c;
        bool headers_sent = false;
        std::string header_buffer;

        while (read(cgi_output[0], &c, 1) > 0)
        {
            if (!headers_sent)
            {
                header_buffer += c;

                // 查找头部结束标记（空行）
                if (header_buffer.find("\r\n\r\n") != std::string::npos ||
                    header_buffer.find("\n\n") != std::string::npos)
                {
                    // 直接发送CGI脚本生成的头部和正文
                    send(client_socket, header_buffer.c_str(), header_buffer.length(), 0);
                    headers_sent = true;
                }
            }
            else
            {
                // 继续发送正文
                send(client_socket, &c, 1, 0);
            }
        }

        // 如果没有找到头部结束标记，将缓冲区内容作为正文发送
        if (!headers_sent && !header_buffer.empty())
        {
            // 添加默认头部和空行
            std::string default_headers = "Content-Type: text/html\r\n\r\n";
            send(client_socket, default_headers.c_str(), default_headers.length(), 0);

            // 发送缓冲区内容作为正文
            send(client_socket, header_buffer.c_str(), header_buffer.length(), 0);
        }

        // 关闭管道
        close(cgi_output[0]);
        close(cgi_input[1]);

        // 等待子进程结束
        waitpid(pid, &status, 0);
    }
#endif
}