/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-16 11:16:09
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-19 14:41:33
 * @FilePath: \WebServerByCPP\src\ConfigManager.cpp
 * @Description: 配置管理器实现，负责加载和解析配置文件，提供访问配置项的接口
 * 支持字符串、整数、浮点数和布尔值类型的配置读取，采用键值对格式
 * 实现了错误处理和默认值机制，确保配置缺失时程序仍能正常工作
 */
#include "../include/ConfigManager.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

// 静态成员变量初始化
std::map<std::string, std::string> ConfigManager::configData;
bool ConfigManager::isLoaded = false;

// 加载配置文件
bool ConfigManager::loadConfig(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "无法打开配置文件: " << filename << std::endl;
        return false;
    }

    configData.clear();
    std::string line;

    while (std::getline(file, line))
    {
        // 去除前导和尾随空白
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty() || line[0] == '#')
            continue; // 跳过空行和注释行

        // 查找等号位置
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos)
            continue; // 跳过没有等号的行

        // 提取键和值
        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);

        // 去除键和值的前导和尾随空白
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);

        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // 存储配置项
        configData[key] = value;
    }

    file.close();
    isLoaded = true;
    std::cout << "已加载" << configData.size() << "个配置项" << std::endl;
    return true;
}

// 重新加载配置文件
bool ConfigManager::reloadConfig(const std::string &filename)
{
    return loadConfig(filename);
}

// 检查配置项是否存在
bool ConfigManager::hasKey(const std::string &key)
{
    return configData.find(key) != configData.end();
}

// 获取字符串值
std::string ConfigManager::getString(const std::string &key, const std::string &defaultValue)
{
    auto it = configData.find(key);
    if (it != configData.end())
    {
        return it->second;
    }
    return defaultValue;
}

// 获取整数值
int ConfigManager::getInt(const std::string &key, int defaultValue)
{
    auto it = configData.find(key);
    if (it != configData.end())
    {
        try
        {
            return std::stoi(it->second);
        }
        catch (const std::exception &e)
        {
            std::cerr << "配置项 '" << key << "' 值 '" << it->second << "' 无法转换为整数: " << e.what() << std::endl;
        }
    }
    return defaultValue;
}

// 获取浮点值
double ConfigManager::getDouble(const std::string &key, double defaultValue)
{
    auto it = configData.find(key);
    if (it != configData.end())
    {
        try
        {
            return std::stod(it->second);
        }
        catch (const std::exception &e)
        {
            std::cerr << "配置项 '" << key << "' 值 '" << it->second << "' 无法转换为浮点数: " << e.what() << std::endl;
        }
    }
    return defaultValue;
}

// 获取布尔值
bool ConfigManager::getBool(const std::string &key, bool defaultValue)
{
    auto it = configData.find(key);
    if (it != configData.end())
    {
        std::string value = it->second;
        // 转换为小写便于比较
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (value == "true" || value == "yes" || value == "1" || value == "on")
        {
            return true;
        }
        if (value == "false" || value == "no" || value == "0" || value == "off")
        {
            return false;
        }
        std::cerr << "配置项 '" << key << "' 值 '" << it->second << "' 不是有效的布尔值" << std::endl;
    }
    return defaultValue;
}

// 设置配置项
void ConfigManager::setConfig(const std::string &key, const std::string &value)
{
    configData[key] = value;
}

// 保存配置到文件
bool ConfigManager::saveConfig(const std::string &filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "无法打开配置文件进行保存: " << filename << std::endl;
        return false;
    }

    for (const auto &item : configData)
    {
        file << item.first << " = " << item.second << std::endl;
    }

    file.close();
    return true;
}

// 打印所有配置项(用于调试)
void ConfigManager::dumpConfig()
{
    std::cout << "=== 配置项 ===" << std::endl;
    for (const auto &item : configData)
    {
        std::cout << item.first << " = " << item.second << std::endl;
    }
    std::cout << "==============" << std::endl;
}