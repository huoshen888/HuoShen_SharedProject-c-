#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <type_traits>

class SharedMemory {
public:
    // 基本字节集操作
    static bool Write(const std::string& name, const std::vector<uint8_t>& data);
    static std::vector<uint8_t> Read(const std::string& name);

    // 模板函数：支持任意数据类型
    template<typename T>
    static bool Write(const std::string& name, const T& data) {
        static_assert(std::is_trivially_copyable_v<T>,
            "T must be trivially copyable for shared memory");

        std::vector<uint8_t> byteData(sizeof(T));
        memcpy(byteData.data(), &data, sizeof(T));
        return Write(name, byteData);
    }

    // 模板函数：读取任意数据类型
    template<typename T>
    static T Read(const std::string& name, const T& defaultValue = T{}) {
        static_assert(std::is_trivially_copyable_v<T>,
            "T must be trivially copyable for shared memory");

        auto byteData = Read(name);
        if (byteData.size() != sizeof(T)) {
            return defaultValue;
        }

        T result;
        memcpy(&result, byteData.data(), sizeof(T));
        return result;
    }

    // 字符串特化版本
    static bool WriteString(const std::string& name, const std::string& str);
    static std::string ReadString(const std::string& name, const std::string& defaultValue = "");

    // 检查共享内存是否存在
    static bool Exists(const std::string& name);

    // 获取共享内存大小
    static size_t GetSize(const std::string& name);

private:
    SharedMemory() = delete;
    ~SharedMemory() = delete;

    static HANDLE CreateOrOpenMapping(const std::string& name, DWORD size, bool create);
    static void* MapView(HANDLE hMapping, DWORD access);
    static void Cleanup(HANDLE hMapping, void* view);
};