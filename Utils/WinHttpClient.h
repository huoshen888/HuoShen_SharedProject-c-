
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>  // 添加这个头文件
#include <cstdint>

#pragma comment(lib, "winhttp.lib")

class WinHttpClient {
private:
    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;

    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = INTERNET_DEFAULT_PORT;
    bool isHttps = false;
    bool requestSent = false;

    // 解析 URL
    bool ParseUrl(const std::wstring& url) {
        URL_COMPONENTS urlComp;
        ZeroMemory(&urlComp, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);

        wchar_t hostName[256] = { 0 };
        wchar_t urlPath[1024] = { 0 };
        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 1024;

        if (!WinHttpCrackUrl(url.c_str(), url.length(), 0, &urlComp)) {
            std::wcerr << L"Error: Invalid URL format (" << GetLastError() << L")" << std::endl;
            return false;
        }

        host = hostName;
        path = urlPath;
        port = urlComp.nPort;
        isHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

        if (path.empty()) {
            path = L"/";
        }

        return true;
    }

    // 确保请求已发送
    bool EnsureRequestSent() {
        if (!requestSent) {
            if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0)) {
                std::wcerr << L"Error: Cannot send request (" << GetLastError() << L")" << std::endl;
                return false;
            }

            if (!WinHttpReceiveResponse(hRequest, NULL)) {
                std::wcerr << L"Error: Cannot receive response (" << GetLastError() << L")" << std::endl;
                return false;
            }

            requestSent = true;
        }
        return true;
    }

    // 清理资源
    void Cleanup() {
        if (hRequest) {
            WinHttpCloseHandle(hRequest);
            hRequest = nullptr;
        }
        if (hConnect) {
            WinHttpCloseHandle(hConnect);
            hConnect = nullptr;
        }
        if (hSession) {
            WinHttpCloseHandle(hSession);
            hSession = nullptr;
        }
        requestSent = false;
    }

public:
    // 构造函数
    WinHttpClient() = default;

    // 析构函数
    ~WinHttpClient() {
        Cleanup();
    }

    // 发送 HTTP 请求
    bool Request(const std::wstring& url,
        const std::wstring& method = L"GET",
        const std::wstring& userAgent = L"WinHTTP Client/1.0",
        bool disableRedirects = false,
        DWORD timeoutMs = 30000) {

        // 清理之前的请求
        Cleanup();

        // 1. 解析 URL
        if (!ParseUrl(url)) {
            return false;
        }

        std::wcout << L"Requesting: " << method << L" " << host << path << std::endl;

        // 2. 初始化会话
        hSession = WinHttpOpen(userAgent.c_str(),
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);

        if (!hSession) {
            std::wcerr << L"Error: WinHttpOpen failed (" << GetLastError() << L")" << std::endl;
            return false;
        }

        // 3. 设置超时
        WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

        // 4. 连接服务器
        hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
        if (!hConnect) {
            std::wcerr << L"Error: Cannot connect to server (" << GetLastError() << L")" << std::endl;
            Cleanup();
            return false;
        }

        // 5. 创建请求
        DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
        hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
            NULL, NULL, NULL, flags);

        if (!hRequest) {
            std::wcerr << L"Error: Cannot create request (" << GetLastError() << L")" << std::endl;
            Cleanup();
            return false;
        }

        // 6. 设置禁止重定向（如果需要）
        if (disableRedirects) {
            DWORD option = WINHTTP_DISABLE_REDIRECTS;
            if (WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &option, sizeof(option))) {
                std::wcout << L"Auto-redirects disabled" << std::endl;
            }
            else {
                std::wcerr << L"Warning: Cannot disable redirects (" << GetLastError() << L")" << std::endl;
            }
        }

        return true;
    }

    // 发送请求（执行实际网络操作）
    bool Send() {
        if (!hRequest) {
            std::wcerr << L"Error: Request not initialized. Call Request() first." << std::endl;
            return false;
        }

        if (requestSent) {
            std::wcout << L"Warning: Request already sent." << std::endl;
            return true;
        }

        if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0)) {
            std::wcerr << L"Error: Cannot send request (" << GetLastError() << L")" << std::endl;
            return false;
        }

        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            std::wcerr << L"Error: Cannot receive response (" << GetLastError() << L")" << std::endl;
            return false;
        }

        requestSent = true;
        return true;
    }

    // 获取状态码
    DWORD GetStatusCode() {
        if (!EnsureRequestSent()) {
            return 0;
        }

        DWORD statusCode = 0;
        DWORD dwSize = sizeof(statusCode);

        if (WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode, &dwSize, NULL)) {
            return statusCode;
        }

        return 0;
    }

    // 获取所有响应头（原始格式）
    std::wstring GetResponseHeaders() {
        if (!EnsureRequestSent()) {
            return L"";
        }

        DWORD dwSize = 0;
        std::wstring headers;

        // 获取需要的缓冲区大小
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_RAW_HEADERS_CRLF,
            WINHTTP_HEADER_NAME_BY_INDEX,
            NULL, &dwSize, NULL);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::vector<wchar_t> buffer(dwSize / sizeof(wchar_t));

            if (WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_RAW_HEADERS_CRLF,
                WINHTTP_HEADER_NAME_BY_INDEX,
                buffer.data(), &dwSize, NULL)) {
                headers = buffer.data();
            }
        }

        return headers;
    }

    // 获取响应头（解析为键值对）
    std::map<std::wstring, std::wstring> GetHeadersMap() {
        std::map<std::wstring, std::wstring> headersMap;
        std::wstring rawHeaders = GetResponseHeaders();

        if (rawHeaders.empty()) {
            return headersMap;
        }

        std::wistringstream stream(rawHeaders);
        std::wstring line;
        bool isFirstLine = true;

        while (std::getline(stream, line)) {
            // 去除回车符
            if (!line.empty() && line.back() == L'\r') {
                line.pop_back();
            }

            if (line.empty()) continue;

            // 第一行是状态行（如 HTTP/1.1 200 OK）
            if (isFirstLine) {
                headersMap[L"Status-Line"] = line;
                isFirstLine = false;
                continue;
            }

            // 解析头部键值对
            size_t colonPos = line.find(L':');
            if (colonPos != std::wstring::npos) {
                std::wstring key = line.substr(0, colonPos);
                std::wstring value;

                // 跳过冒号后的空格
                if (colonPos + 1 < line.length()) {
                    size_t valueStart = line.find_first_not_of(L' ', colonPos + 1);
                    if (valueStart != std::wstring::npos) {
                        value = line.substr(valueStart);
                    }
                }

                headersMap[key] = value;
            }
        }

        return headersMap;
    }

    // 获取特定响应头
    std::wstring GetHeader(const std::wstring& headerName) {
        if (!EnsureRequestSent()) {
            return L"";
        }

        DWORD dwSize = 0;
        std::wstring headerValue;

        // 获取需要的缓冲区大小
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_CUSTOM,
            headerName.c_str(),
            NULL, &dwSize, NULL);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::vector<wchar_t> buffer(dwSize / sizeof(wchar_t));

            if (WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_CUSTOM,
                headerName.c_str(),
                buffer.data(), &dwSize, NULL)) {
                headerValue = buffer.data();
            }
        }

        return headerValue;
    }

    // 获取响应体（字符串形式）
    std::string GetResponseBody() {
        if (!EnsureRequestSent()) {
            return "";
        }

        std::string responseBody;
        DWORD bytesAvailable = 0;
        DWORD bytesRead = 0;
        const DWORD bufferSize = 8192;
        char buffer[bufferSize];

        do {
            // 检查可用的数据
            if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
                std::wcerr << L"Error querying available data (" << GetLastError() << L")" << std::endl;
                break;
            }

            if (bytesAvailable == 0) {
                break;
            }

            // 读取数据
            DWORD toRead = (bytesAvailable > bufferSize) ? bufferSize : bytesAvailable;
            if (!WinHttpReadData(hRequest, buffer, toRead, &bytesRead)) {
                std::wcerr << L"Error reading data (" << GetLastError() << L")" << std::endl;
                break;
            }

            if (bytesRead > 0) {
                responseBody.append(buffer, bytesRead);
            }

        } while (bytesAvailable > 0);

        return responseBody;
    }

    // 获取响应体（字节数组形式）
    std::vector<unsigned char> GetResponseBodyBytes() {
        if (!EnsureRequestSent()) {
            return {};
        }

        std::vector<unsigned char> responseBody;
        DWORD bytesAvailable = 0;
        DWORD bytesRead = 0;
        const DWORD bufferSize = 8192;
        unsigned char buffer[bufferSize];

        do {
            // 检查可用的数据
            if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
                std::wcerr << L"Error querying available data (" << GetLastError() << L")" << std::endl;
                break;
            }

            if (bytesAvailable == 0) {
                break;
            }

            // 读取数据
            DWORD toRead = (bytesAvailable > bufferSize) ? bufferSize : bytesAvailable;
            if (!WinHttpReadData(hRequest, buffer, toRead, &bytesRead)) {
                std::wcerr << L"Error reading data (" << GetLastError() << L")" << std::endl;
                break;
            }

            if (bytesRead > 0) {
                responseBody.insert(responseBody.end(), buffer, buffer + bytesRead);
            }

        } while (bytesAvailable > 0);

        return responseBody;
    }

    // 一次性获取所有信息
    bool GetFullResponse(std::wstring& headers, std::string& body, DWORD& statusCode) {
        if (!Send()) {
            return false;
        }

        statusCode = GetStatusCode();
        headers = GetResponseHeaders();
        body = GetResponseBody();

        return true;
    }

    // 判断是否是重定向状态
    bool IsRedirect() const {
        DWORD statusCode = const_cast<WinHttpClient*>(this)->GetStatusCode();
        return (statusCode >= 300 && statusCode < 400);
    }

    // 获取重定向地址
    std::wstring GetRedirectLocation() {
        return GetHeader(L"Location");
    }

    // 设置自定义请求头
    bool AddRequestHeader(const std::wstring& header) {
        if (!hRequest) {
            std::wcerr << L"Error: Request not initialized. Call Request() first." << std::endl;
            return false;
        }

        if (requestSent) {
            std::wcerr << L"Error: Cannot add headers after sending request." << std::endl;
            return false;
        }

        DWORD flags = WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE;
        if (WinHttpAddRequestHeaders(hRequest, header.c_str(), header.length(), flags)) {
            return true;
        }

        std::wcerr << L"Error: Cannot add request header (" << GetLastError() << L")" << std::endl;
        return false;
    }
};