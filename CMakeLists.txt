cmake_minimum_required(VERSION 3.10)
project(WebServerByCPP VERSION 0.1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加包含目录
include_directories(include)

# 收集源文件
file(GLOB SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(myhttp ${SOURCES})

# 链接库 - Windows下需要额外链接winsock
if(WIN32)
    target_link_libraries(myhttp ws2_32)
else()
    target_link_libraries(myhttp pthread)
endif()