/*
 * @Author: No_World 2259881867@qq.com
 * @Date: 2025-05-16 11:00:31
 * @LastEditors: No_World 2259881867@qq.com
 * @LastEditTime: 2025-05-16 11:35:09
 * @FilePath: \WebServerByCPP\include\ConfigManager.h
 * @Description: 配置管理器类，提供配置文件的加载、解析和访问功能。
 * 使用单例模式确保全局配置的一致性，支持多种数据类型的配置项访问。
 */
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <map>
#include <string>

class ConfigManager
{
  private:
    // 存储配置数据的映射表
    static std::map<std::string, std::string> configData;

    // 标记配置是否已加载
    static bool isLoaded;

  public:
    // 从文件加载配置
    static bool loadConfig(const std::string &filename);

    // 重新加载配置文件
    static bool reloadConfig(const std::string &filename);

    // 检查配置项是否存在
    static bool hasKey(const std::string &key);

    // 获取字符串值，可提供默认值
    static std::string getString(const std::string &key, const std::string &defaultValue = "");

    // 获取整数值，可提供默认值
    static int getInt(const std::string &key, int defaultValue = 0);

    // 获取浮点值，可提供默认值
    static double getDouble(const std::string &key, double defaultValue = 0.0);

    // 获取布尔值，可提供默认值
    static bool getBool(const std::string &key, bool defaultValue = false);

    // 设置配置项
    static void setConfig(const std::string &key, const std::string &value);

    // 保存配置到文件
    static bool saveConfig(const std::string &filename);

    // 打印所有配置项(用于调试)
    static void dumpConfig();
};

#endif // CONFIG_MANAGER_H