/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-24 15:11:23
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-24 15:43:50
 * @FilePath: /WebServerByCPP/single/httpd.cpp
 * @Description: 单文件版C++14 HTTP服务器，实现了基本的HTTP服务器功能，包括静态文件服务和CGI支持。
 * 采用现代C++特性，使用std::thread处理并发请求，提供了精简但完整的HTTP协议处理流程。
 */
#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

// 定义常量
#define SERVER_STRING "Server: SongHao's http/0.1.0\r\n" // 定义个人server名称
#define ISspace(x) isspace(static_cast<int>(x))

class HTTPServer
{
  public:
    // 构造函数
    HTTPServer(unsigned short port = 6379) : port_(port), server_sock_(-1)
    {
    }

    // 启动服务器
    void run()
    {
        server_sock_ = startup(&port_);

        std::cout << "HTTP server_sock is " << server_sock_ << std::endl;
        std::cout << "HTTP running on port " << port_ << std::endl;

        while (true)
        {
            struct sockaddr_in client_name;
            socklen_t client_name_len = sizeof(client_name);
            int client_sock = accept(server_sock_, reinterpret_cast<struct sockaddr *>(&client_name), &client_name_len);

            std::cout << "New connection.... ip: " << inet_ntoa(client_name.sin_addr)
                      << ", port: " << ntohs(client_name.sin_port) << std::endl;

            if (client_sock == -1)
                error_die("accept");

            // 使用C++11线程替代pthread
            std::thread t(&HTTPServer::accept_request, this, client_sock);
            t.detach(); // 线程分离，让它在后台运行
        }

        close(server_sock_);
    }

  private:
    // 处理监听到的 HTTP 请求
    void accept_request(int client)
    {
        char buf[1024];
        int numchars;
        char method[255];
        char url[255];
        char path[512];
        size_t i, j;
        struct stat st;
        bool cgi = false;
        std::string query_string;

        numchars = get_line(client, buf, sizeof(buf));

        i = 0;
        j = 0;
        while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
        {
            // 提取其中的请求方式
            method[i] = buf[j];
            i++;
            j++;
        }
        method[i] = '\0';

        if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
        {
            unimplemented(client);
            return;
        }

        if (strcasecmp(method, "POST") == 0)
            cgi = true;

        i = 0;
        while (ISspace(buf[j]) && (j < sizeof(buf)))
            j++;

        while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
        {
            url[i] = buf[j];
            i++;
            j++;
        }
        url[i] = '\0';

        // GET请求url可能会带有?,有查询参数
        if (strcasecmp(method, "GET") == 0)
        {
            char *query_str = url;
            while ((*query_str != '?') && (*query_str != '\0'))
                query_str++;

            // 如果有?表明是动态请求, 开启cgi
            if (*query_str == '?')
            {
                cgi = true;
                *query_str = '\0';
                query_str++;
                query_string = query_str;
            }
        }

        sprintf(path, "httpdocs%s", url);

        if (path[strlen(path) - 1] == '/')
        {
            strcat(path, "test.html");
        }

        if (stat(path, &st) == -1)
        {
            while ((numchars > 0) && strcmp("\n", buf))
                numchars = get_line(client, buf, sizeof(buf));

            not_found(client);
        }
        else
        {
            if ((st.st_mode & S_IFMT) == S_IFDIR)
            { // S_IFDIR代表目录
                // 如果请求参数为目录, 自动打开test.html
                strcat(path, "/test.html");
            }

            // 文件可执行
            if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
            {
                // S_IXUSR:文件所有者具可执行权限
                // S_IXGRP:用户组具可执行权限
                // S_IXOTH:其他用户具可读取权限
                cgi = true;
            }

            if (!cgi)
                serve_file(client, path);
            else
                execute_cgi(client, path, method, query_string);
        }

        close(client);
        // std::cout << "connection close....client: " << client << std::endl;
    }

    void bad_request(int client)
    {
        std::string response = "HTTP/1.0 400 BAD REQUEST\r\n";
        response += "Content-type: text/html\r\n";
        response += "\r\n";
        response += "<P>Your browser sent a bad request, ";
        response += "such as a POST without a Content-Length.\r\n";

        send(client, response.c_str(), response.size(), 0);
    }

    void cat(int client, std::ifstream &resource)
    {
        // 发送文件的内容
        std::string line;
        while (std::getline(resource, line))
        {
            line += "\n"; // 添加换行符，因为getline会删除它
            send(client, line.c_str(), line.length(), 0);
        }
    }

    void cannot_execute(int client)
    {
        std::string response = "HTTP/1.0 500 Internal Server Error\r\n";
        response += "Content-type: text/html\r\n";
        response += "\r\n";
        response += "<P>Error prohibited CGI execution.\r\n";

        send(client, response.c_str(), response.size(), 0);
    }

    void error_die(const std::string &sc)
    {
        perror(sc.c_str());
        exit(1);
    }

    // 执行cgi动态解析
    void execute_cgi(int client, const std::string &path, const std::string &method, const std::string &query_string)
    {
        char buf[1024];
        int cgi_output[2];
        int cgi_input[2];

        pid_t pid;
        int status;

        int i;
        char c;

        int numchars = 1;
        int content_length = -1;
        // 默认字符
        buf[0] = 'A';
        buf[1] = '\0';

        if (strcasecmp(method.c_str(), "GET") == 0)
            while ((numchars > 0) && strcmp("\n", buf))
                numchars = get_line(client, buf, sizeof(buf));
        else
        {
            numchars = get_line(client, buf, sizeof(buf));
            while ((numchars > 0) && strcmp("\n", buf))
            {
                buf[15] = '\0';
                if (strcasecmp(buf, "Content-Length:") == 0)
                    content_length = atoi(&(buf[16]));

                numchars = get_line(client, buf, sizeof(buf));
            }

            if (content_length == -1)
            {
                bad_request(client);
                return;
            }
        }

        sprintf(buf, "HTTP/1.0 200 OK\r\n");
        send(client, buf, strlen(buf), 0);

        if (pipe(cgi_output) < 0)
        {
            cannot_execute(client);
            return;
        }

        if (pipe(cgi_input) < 0)
        {
            cannot_execute(client);
            return;
        }

        if ((pid = fork()) < 0)
        {
            cannot_execute(client);
            return;
        }

        if (pid == 0)
        { // 子进程: 运行CGI 脚本
            char meth_env[255];
            char query_env[255];
            char length_env[255];

            dup2(cgi_output[1], 1);
            dup2(cgi_input[0], 0);

            close(cgi_output[0]); // 关闭了cgi_output中的读通道
            close(cgi_input[1]);  // 关闭了cgi_input中的写通道

            sprintf(meth_env, "REQUEST_METHOD=%s", method.c_str());
            putenv(meth_env);

            if (strcasecmp(method.c_str(), "GET") == 0)
            {
                // 存储QUERY_STRING
                sprintf(query_env, "QUERY_STRING=%s", query_string.c_str());
                putenv(query_env);
            }
            else
            { // POST
                // 存储CONTENT_LENGTH
                sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
                putenv(length_env);
            }

            execl(path.c_str(), path.c_str(), nullptr); // 执行CGI脚本
            exit(0);
        }
        else
        {
            close(cgi_output[1]);
            close(cgi_input[0]);

            if (strcasecmp(method.c_str(), "POST") == 0)
                for (i = 0; i < content_length; i++)
                {
                    recv(client, &c, 1, 0);
                    write(cgi_input[1], &c, 1);
                }

            // 读取cgi脚本返回数据
            while (read(cgi_output[0], &c, 1) > 0)
                // 发送给浏览器
                send(client, &c, 1, 0);

            // 运行结束关闭
            close(cgi_output[0]);
            close(cgi_input[1]);

            waitpid(pid, &status, 0);
        }
    }

    // 解析一行http报文
    int get_line(int sock, char *buf, int size)
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

    void headers(int client, const std::string &filename)
    {
        // 发送HTTP头
        std::string response = "HTTP/1.0 200 OK\r\n";
        response += SERVER_STRING;
        response += "Content-Type: text/html\r\n";
        response += "\r\n";

        send(client, response.c_str(), response.size(), 0);
    }

    // 返回404错误页面，组装信息
    void not_found(int client)
    {
        std::string response = "HTTP/1.0 404 NOT FOUND\r\n";
        response += SERVER_STRING;
        response += "Content-Type: text/html\r\n";
        response += "\r\n";
        response += "<HTML><TITLE>Not Found</TITLE>\r\n";
        response += "<BODY><P>The server could not fulfill\r\n";
        response += "your request because the resource specified\r\n";
        response += "is unavailable or nonexistent.\r\n";
        response += "</BODY></HTML>\r\n";

        send(client, response.c_str(), response.size(), 0);
    }

    // 如果不是CGI文件，也就是静态文件，直接读取文件返回给请求的http客户端即可
    void serve_file(int client, const std::string &filename)
    {
        int numchars = 1;
        char buf[1024];
        buf[0] = 'A';
        buf[1] = '\0';

        while ((numchars > 0) && strcmp("\n", buf))
        {
            numchars = get_line(client, buf, sizeof(buf));
        }

        // 打开文件
        std::ifstream resource(filename);
        if (!resource.is_open())
        {
            not_found(client);
        }
        else
        {
            headers(client, filename);
            cat(client, resource);
        }
    }

    // 启动服务端
    int startup(unsigned short *port)
    {
        int httpd = 0;
        int option;
        struct sockaddr_in name;

        // 设置http socket
        httpd = socket(PF_INET, SOCK_STREAM, 0);
        if (httpd == -1)
            error_die("socket"); // 连接失败

        socklen_t optlen;
        optlen = sizeof(option);
        option = 1;
        setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, static_cast<void *>(&option), optlen);

        memset(&name, 0, sizeof(name));
        name.sin_family = AF_INET;
        name.sin_port = htons(*port);
        name.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(httpd, reinterpret_cast<struct sockaddr *>(&name), sizeof(name)) < 0)
            error_die("bind"); // 绑定失败

        if (*port == 0)
        { // 动态分配一个端口
            socklen_t namelen = sizeof(name);
            if (getsockname(httpd, reinterpret_cast<struct sockaddr *>(&name), &namelen) == -1)
                error_die("getsockname");
            *port = ntohs(name.sin_port);
        }

        if (listen(httpd, 5) < 0)
            error_die("listen");
        return httpd;
    }

    void unimplemented(int client)
    {
        std::string response = "HTTP/1.0 501 Method Not Implemented\r\n";
        response += SERVER_STRING;
        response += "Content-Type: text/html\r\n";
        response += "\r\n";
        response += "<HTML><HEAD><TITLE>Method Not Implemented\r\n";
        response += "</TITLE></HEAD>\r\n";
        response += "<BODY><P>HTTP request method not supported.\r\n";
        response += "</BODY></HTML>\r\n";

        send(client, response.c_str(), response.size(), 0);
    }

    // 成员变量
    unsigned short port_;
    int server_sock_;
};

// 主函数
int main()
{
    // 创建HTTP服务器实例，默认端口6379
    HTTPServer server;
    // 运行服务器
    server.run();
    return 0;
}
