#pragma once
#include <windows.h>
#include <wincrypt.h>
#include <iostream>
#include <fstream>
#include <string>

// 链接静态库
#pragma comment(lib, "advapi32.lib")

/**
 * @brief 计算文件的 MD5
 */
inline std::string GetFileMD5(const std::wstring& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return ""; 
    }

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string md5Result = "";

    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
        {
            char buffer[8192]; 

            while (file.read(buffer, sizeof(buffer))) {
                CryptHashData(hHash, reinterpret_cast<BYTE*>(buffer), sizeof(buffer), 0);
            }
            if (file.gcount() > 0) {
                CryptHashData(hHash, reinterpret_cast<BYTE*>(buffer), static_cast<DWORD>(file.gcount()), 0);
            }

            BYTE hashBytes[16];
            DWORD hashLen = 16;
            if (CryptGetHashParam(hHash, HP_HASHVAL, hashBytes, &hashLen, 0))
            {
                char hexStr[33];
                for (int i = 0; i < 16; ++i) {
                    sprintf_s(hexStr + (i * 2), 3, "%02x", hashBytes[i]);
                }
                md5Result = hexStr;
            }
            CryptDestroyHash(hHash); 
        }
        CryptReleaseContext(hProv, 0); 
    }

    return md5Result;
}

/**
 * @brief 计算内存数据的 MD5
 * @param data 内存指针 (支持 char*, unsigned char* 等任意类型指针)
 * @param length 数据长度
 */
inline std::string GetMemoryMD5(const void* data, size_t length)
{
    // 防御性编程：检查空指针或长度为0
    if (!data || length == 0) {
        return ""; 
    }

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string md5Result = "";

    // 1. 初始化 Windows 密码学上下文
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        // 2. 创建 MD5 哈希对象
        if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
        {
            // 3. 一次性将整个内存块的数据投入哈希计算
            // CryptHashData 的长度参数需要 DWORD，所以这里使用 static_cast 转换
            if (CryptHashData(hHash, static_cast<const BYTE*>(data), static_cast<DWORD>(length), 0))
            {
                // 4. 获取最终的 MD5 结果 (固定为 16 字节)
                BYTE hashBytes[16];
                DWORD hashLen = 16;
                if (CryptGetHashParam(hHash, HP_HASHVAL, hashBytes, &hashLen, 0))
                {
                    // 5. 将 16 字节的二进制数据格式化为 32 位的十六进制字符串
                    char hexStr[33];
                    for (int i = 0; i < 16; ++i) {
                        sprintf_s(hexStr + (i * 2), 3, "%02x", hashBytes[i]);
                    }
                    md5Result = hexStr;
                }
            }
            CryptDestroyHash(hHash); // 释放哈希对象
        }
        CryptReleaseContext(hProv, 0); // 释放上下文
    }

    return md5Result;
}