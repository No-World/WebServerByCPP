# WebServerByCPP项目Makefile
# 支持Windows (MinGW/MSVC) 和 Unix/Linux 环境

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -I./include

# 目标文件
TARGET = myhttp

# 目录设置
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INCLUDE_DIR = include

# 源文件和目标文件
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# 平台相关设置
UNAME := $(shell uname)
ifeq ($(OS),Windows_NT)
	# Windows系统
	RM = del /Q
	MKDIR = mkdir
	TARGET := $(TARGET).exe
	LDFLAGS = -lws2_32
	PATH_SEP = \\
else
	# Unix/Linux系统
	RM = rm -f
	MKDIR = mkdir -p
	LDFLAGS = -lpthread
	PATH_SEP = /
endif

# 默认目标
all: directories $(BIN_DIR)/$(TARGET)

# 创建必要的目录
directories:
	@if [ ! -d $(OBJ_DIR) ]; then $(MKDIR) $(OBJ_DIR); fi
	@if [ ! -d $(BIN_DIR) ]; then $(MKDIR) $(BIN_DIR); fi

# 链接目标文件生成可执行文件
$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "编译完成：$@"

# 编译源文件为目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	$(RM) $(OBJ_DIR)$(PATH_SEP)*.o
	$(RM) $(BIN_DIR)$(PATH_SEP)$(TARGET)
	@echo "已清理所有目标文件和可执行文件"

# 运行程序
run: all
	@echo "启动HTTP服务器..."
	@cd $(BIN_DIR) && cd .. && ./bin/$(TARGET)

# 调试版本（添加调试信息）
debug: CXXFLAGS += -g -DDEBUG
debug: all
	@echo "以调试模式启动HTTP服务器..."
	@cd $(BIN_DIR) && cd .. && ./bin/$(TARGET)

# 优化版本
release: CXXFLAGS += -O2
release: all

# 显示帮助信息
help:
	@echo "可用的make目标:"
	@echo "  all       - 构建项目 (默认)"
	@echo "  clean     - 删除所有目标文件和可执行文件"
	@echo "  run       - 构建并运行项目"
	@echo "  debug     - 构建调试版本"
	@echo "  release   - 构建优化版本"
	@echo "  help      - 显示帮助信息"

# 声明伪目标
.PHONY: all clean run debug release help directories