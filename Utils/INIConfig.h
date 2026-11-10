#ifndef INICONFIG_H
#define INICONFIG_H

#include <map>
#include <string>
#include <vector>

using namespace std;

class INIConfig
{
public:
    /**
     * 构造函数
     * @param string INI文件路径
     */
    INIConfig(string);

    /**
     * 析构函数
     */
    ~INIConfig();

private:
    // 定义INI文件中一个节(section)的内部存储结构
    // 使用键值对map存储一个节中的所有配置项
    typedef map<string, string> object;

public:
    /**
     * 从INI文件读取配置
     * @return bool 读取成功返回true，失败返回false
     */
    bool ReadINI();

    /**
     * 将当前配置写入INI文件
     * @return bool 写入成功返回true，失败返回false
     */
    bool WriteINI();

    /**
     * 显示当前内存中的所有INI配置内容
     * 用于调试和查看配置
     */
    void ShowINI();

    /**
     * 清空内存中的所有INI配置
     */
    void Clear();

    /**
     * 向指定节添加或修改一个配置项
     * @param string 节名(section name)
     * @param string 键名(key)
     * @param string 键值(value)
     */
    void Append(string, string, string);

    /**
     * 获取指定节和键的配置值
     * @param string 节名
     * @param string 键名
     * @param string 默认值(当键不存在时返回)
     * @return string 配置值或默认值
     */
    string GetConfig(string, string, string = "");

    /**
     * 获取指定节下的所有键名
     * @param string 节名
     * @return vector<string> 该节下所有键名的集合
     */
    vector<string> GetProject(string);
    /**
     * 获取所有的节(section)名称
     * @return vector<string> 所有节名的集合
     */
    vector<string> GetAllSections();
private:
    // 存储INI配置数据的内存结构
    // 外层map的key是节名，value是该节下的键值对map
    map<string, object>* project;

    // INI文件路径
    string path;
};

#endif // INICONFIG_H
