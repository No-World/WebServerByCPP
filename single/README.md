# 单文件版C++ HTTP服务器

## 项目简介

这是一个使用C++14编写的轻量级HTTP服务器的单文件版本，只使用一个源文件实现了完整的HTTP服务器功能。该项目是[WebServerByCPP](https://github.com/No-World/WebServerByCPP)的简化版本，专注于提供最小化的HTTP服务器实现，非常适合学习和理解HTTP服务器的基本工作原理。

## 主要特性

- **单文件实现**：整个HTTP服务器的代码都在一个C++文件中，便于学习和理解
- **多线程处理**：采用std::thread处理并发HTTP请求
- **静态文件服务**：支持静态HTML文件的访问
- **CGI支持**：可以执行简单的CGI脚本
- **现代C++特性**：使用C++14标准，展示现代C++的错误处理和资源管理方法
- **默认端口**：默认使用6379端口提供HTTP服务

## 系统需求

- C++14兼容的编译器（如GCC 5+）
- Unix/Linux系统
- make工具
- pthread库

## 编译与运行

### 编译项目

```bash
# 进入single目录
cd single

# 授予权限
cd  httpdocs
sudo chmod 600 test.html
sudo chmod 600 post.html
sudo chmod +X post.cgi

#返回上一目录
cd ..

# 编译项目
make
```

### 运行服务器

```bash
# 运行编译后的程序
./myhttp
```

服务器将在默认端口6379上启动。你可以通过浏览器访问`http://localhost:6379/test.html`来测试服务器是否正常工作。

## 目录结构

```
single/
├── httpd.cpp    # 服务器源代码
├── Makefile     # 构建脚本
├── myhttp       # 编译后的可执行文件
└── httpdocs/    # 网站文件目录
    ├── post.cgi  # CGI示例脚本
    ├── post.html # 表单提交示例
    └── test.html # 测试页面
```

## 使用说明

1. **访问静态页面**：浏览器中输入`http://localhost:6379/`或`http://localhost:6379/test.html`
2. **表单提交**：访问`http://localhost:6379/post.html`并提交表单，将会通过CGI处理

## 如何修改配置

服务器的默认配置（如端口号）在源代码中定义，你可以通过修改源代码并重新编译来更改这些配置：

```cpp
// 修改默认端口
HTTPServer server(8080); // 将使用8080端口而非默认的6379
```

## 技术说明

此服务器实现了HTTP/1.0协议的基本功能，包括：

- GET/POST请求处理
- 静态文件服务
- 简单的CGI支持
- 错误处理（404、501等）
