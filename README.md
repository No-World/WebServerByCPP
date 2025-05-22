# WebServerByCPP

## 项目简介

WebServerByCPP是一个使用 C++14 开发的轻量级HTTP服务器。该项目复刻于[MyPoorWebServer](https://github.com/forthespada/MyPoorWebServer), 原作者是[阿秀@forthespada](https://github.com/forthespada/), 你可以在本项目根目录下的`old`文件夹中找到原项目的文件

## 主要特性

- **多线程处理**：采用多线程模型处理并发HTTP请求
- **优雅的启动与关闭机制**：通过信号处理支持优雅的服务器停止
- **静态文件服务**：支持静态文件的HTTP服务
- **配置灵活**：通过配置文件调整服务器行为
- **现代C++特性**：使用C++14标准，展示现代C++的错误处理和资源管理方法
- **RAII设计原则**：通过构造函数和析构函数自动管理资源
- **线程安全**：使用std::thread和std::atomic实现线程安全的并发控制

## 系统需求

- C++14兼容的编译器（如GCC 5+）
- Unix/Linux系统
- make工具

## 安装与构建

### 获取源码

```bash
git clone https://github.com/No-World/WebServerByCPP.git
cd WebServerByCPP
```

### 编译项目

```bash
# 编译项目
make

# 编译调试版本
make debug

# 编译优化版本
make release
```

### 运行服务器

```bash
# 直接运行
make run

# 或者手动运行编译后的程序
./bin/myhttp
# 注意：手动运行需保证 httpdocs 文件夹与 myhttp 处于同级目录
```

启动服务器后，可以通过浏览器访问`http://localhost:6379/`查看默认页面。

### make命令

更多make命令可以在`Makefile`文件中查看或执行以下命令:

```bash
make help
```

## 配置服务器

服务器配置文件位于`config/server.conf`，可以修改以下参数：

```
# 服务器端口号
port=6379

# 文档根目录
document_root=./httpdocs

# 默认启动网页
default_document=test.html
```


## 目录结构

```
WebServerByCPP/
├── include/           # 头文件
├── src/               # 源文件
├── config/            # 配置文件
├── httpdocs/          # 静态文件目录
├── bin/               # 编译后的可执行文件
├── obj/               # 编译过程中的目标文件
├── Makefile           # 项目构建脚本
└── README.md          # 项目说明文档
```

## 核心组件

- **HttpServer**：服务器核心类，负责socket初始化和客户端连接管理
- **HttpRequest**：HTTP请求解析类，处理客户端请求
- **HttpResponse**：HTTP响应类，生成服务器响应
- **ConfigManager**：配置管理类，读取服务器配置
- **RequestHandler**：请求处理类，负责处理不同类型的HTTP请求

## 运行截图

## 致谢

感谢项目原作者[阿秀@forthespada](https://github.com/forthespada/)。