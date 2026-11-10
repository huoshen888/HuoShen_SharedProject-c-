#include "YouDaoNote.h"
#include "WinHttpClient.h"

#include <iostream>
#include <regex>
#include <chrono>
#include <fstream>
#include <utility>

namespace YouDaoNote
{
    /**
     * @brief [内部助手函数] 从分享链接中提取有道云笔记的唯一ID
     * @param shareUrl 有道云分享的短链接
     * @return 成功返回 ID 字符串，失败返回空字符串
     */
    static std::wstring InternalExtractNoteId(const std::wstring& shareUrl)
    {
        WinHttpClient http;
        // 最后一个参数传入 true 代表禁用自动重定向，从而捕获 302 状态码的 Location
        if (!http.Request(shareUrl, L"GET", L"MyClient/1.0", true) || !http.Send()) {
            return L"";
        }

        auto headersMap = http.GetHeadersMap();
        for (const auto& [key, value] : headersMap)
        {
            if (key == L"Location")
            {
                std::wregex pattern(L"id=([^&]+)");
                std::wsmatch matches;
                if (std::regex_search(value, matches, pattern) && matches.size() > 1) {
                    return matches[1].str();
                }
                break;
            }
        }
        return L"";
    }

    std::string GetNoteContent(const std::wstring& shareUrl)
    {
        std::wstring id = InternalExtractNoteId(shareUrl);
        if (id.empty()) return "";

        std::wstring realUrl = L"https://note.youdao.com/yws/api/note/" + id + L"?sev=j1&editorType=1&unloginId=";
        WinHttpClient http;
        if (!http.Request(realUrl, L"GET", L"MyClient/1.0") || !http.Send()) {
            return "";
        }

        std::string body = http.GetResponseBody();
        if (!body.empty())
        {
            std::regex pattern("<text>(.*?)<\\\\/text>");
            std::smatch matches;
            if (std::regex_search(body, matches, pattern) && matches.size() > 1) {
                return matches[1].str();
            }
        }
        return "";
    }

    std::string GetNoteDirectLink(const std::wstring& shareUrl)
    {
        std::wstring id = InternalExtractNoteId(shareUrl);
        if (id.empty()) return "";

        std::wstring realUrl = L"https://note.youdao.com/yws/api/note/" + id + L"?sev=j1&editorType=1&unloginId=";
        WinHttpClient http;
        if (!http.Request(realUrl, L"GET", L"MyClient/1.0") || !http.Send()) {
            return "";
        }

        std::string body = http.GetResponseBody();
        if (!body.empty())
        {
            // 匹配 <resource>...</resource> 节点
            std::regex pattern("<resource>(.*?)<\\\\/resource>");
            std::smatch matches;
            if (std::regex_search(body, matches, pattern) && matches.size() > 1) {
                return matches[1].str();
            }
        }
        return "";
    }

    std::wstring GetRealReferer(const std::wstring& shareUrl)
    {
        WinHttpClient http;
        if (!http.Request(shareUrl, L"GET", L"MyClient/1.0", true) || !http.Send()) {
            return L"";
        }

        auto headersMap = http.GetHeadersMap();
        std::wstring realLocation = L"";
        for (const auto& [key, value] : headersMap)
        {
            if (key == L"Location") {
                realLocation = value;
                break;
            }
        }

        if (realLocation.empty()) {
            return L"";
        }

        // 获取 C++11 标准的 13 位现行时间戳 (毫秒级别数)
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        // 拼接时间戳参数并返回
        return realLocation + L"&_time=" + std::to_wstring(timestamp);
    }

    bool DownloadNetworkFileToMemory(const std::wstring& downloadUrl, std::vector<uint8_t>& outBytes, const std::wstring& referer, const std::wstring& cookie)
    {
        outBytes.clear();

        WinHttpClient http;

        // 1. 初始化 HTTP 请求
        if (!http.Request(downloadUrl, L"GET", L"MyClient/1.0")) {
            return false;
        }

        // 2. 在 Send() 之前，注入自定义的协议头
        // 注入 Referer
        if (!referer.empty())
        {
            std::wstring refererHeader = L"Referer: " + referer;
            if (!http.AddRequestHeader(refererHeader)) {
                std::wcerr << L"Warning: Failed to inject custom Referer header." << std::endl;
            }
        }

        // 注入 Cookie
        if (!cookie.empty())
        {
            std::wstring cookieHeader = L"Cookie: " + cookie;
            if (!http.AddRequestHeader(cookieHeader)) {
                std::wcerr << L"Warning: Failed to inject custom Cookie header." << std::endl;
            }
        }

        // 3. 执行实际的网络操作 (发送请求)
        if (!http.Send()) {
            return false;
        }

        // 4. 判断服务器返回状态码，200 OK 才说明下载成功
        DWORD statusCode = http.GetStatusCode();
        if (statusCode != 200) {
            std::cerr << "Error: Server responded with status code " << statusCode << std::endl;
            return false;
        }

        // 5. 获取下载下来的文件字节流 (使用安全的二进制流获取)
        outBytes = std::move(http.GetResponseBodyBytes());
        if (outBytes.empty()) {
            return false;
        }

        return true;
    }

    bool DownloadNetworkFile(const std::wstring& downloadUrl, const std::wstring& savePath, const std::wstring& referer, const std::wstring& cookie)
    {
        // 1. 先下载到内存
        std::vector<uint8_t> fileBytes;
        if (!DownloadNetworkFileToMemory(downloadUrl, fileBytes, referer, cookie)) {
            return false;
        }

        // 2. 使用标准的 C++ 二进制写文件流保存到硬盘
        std::ofstream outFile(savePath, std::ios::binary);
        if (!outFile.is_open()) {
            std::wcerr << L"Error: Cannot create local file on path: " << savePath << std::endl;
            return false;
        }

        outFile.write(reinterpret_cast<const char*>(fileBytes.data()), fileBytes.size());
        outFile.close();

        return true;
    }
}