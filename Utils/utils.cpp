#include "utils.h"

#include <tlhelp32.h>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <ctime>

// 窗口函数依赖 user32，注册表依赖 advapi32（与 MD5.h 的做法保持一致）
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

namespace utils {

namespace {

// 多字节 → 宽字符。codePage 传 CP_UTF8 或 CP_ACP。
std::wstring MultiByteToWide(const std::string& src, UINT codePage)
{
    if (src.empty())
        return std::wstring();

    const int n = MultiByteToWideChar(codePage, 0, src.c_str(),
                                      static_cast<int>(src.size()), nullptr, 0);
    if (n <= 0)
        return std::wstring();

    std::wstring dst(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(codePage, 0, src.c_str(), static_cast<int>(src.size()),
                        &dst[0], n);
    return dst;
}

// 宽字符 → 多字节。codePage 传 CP_UTF8 或 CP_ACP。
std::string WideToMultiByte(const std::wstring& src, UINT codePage)
{
    if (src.empty())
        return std::string();

    const int n = WideCharToMultiByte(codePage, 0, src.c_str(),
                                      static_cast<int>(src.size()),
                                      nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return std::string();

    std::string dst(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(codePage, 0, src.c_str(), static_cast<int>(src.size()),
                        &dst[0], n, nullptr, nullptr);
    return dst;
}

// 单个十六进制字符 → 数值，非法返回 -1。
int HexCharValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// 打开注册表子键，失败返回 nullptr。
// 写操作走 RegCreateKeyEx，子键不存在时自动创建（对应易语言「写注册表项」）。
HKEY OpenRegKey(HKEY root, const std::wstring& subKey, REGSAM view, bool forWrite)
{
    HKEY hKey = nullptr;
    const REGSAM access = (forWrite ? KEY_WRITE : KEY_READ) | view;
    LONG ret;
    if (forWrite)
    {
        ret = RegCreateKeyExW(root, subKey.c_str(), 0, nullptr,
                              REG_OPTION_NON_VOLATILE, access, nullptr, &hKey, nullptr);
    }
    else
    {
        ret = RegOpenKeyExW(root, subKey.c_str(), 0, access, &hKey);
    }
    return ret == ERROR_SUCCESS ? hKey : nullptr;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
//  字符串格式化
// ---------------------------------------------------------------------------
std::string Format(const char* fmt, ...)
{
    if (fmt == nullptr)
        return std::string();

    va_list args;
    va_start(args, fmt);

    // 第一遍：只探长度（不写缓冲区）
    va_list argsCopy;
    va_copy(argsCopy, args);
    const int len = vsnprintf(nullptr, 0, fmt, argsCopy);
    va_end(argsCopy);

    if (len <= 0)
    {
        va_end(args);
        return std::string();
    }

    // 第二遍：真正写入。缓冲多留 1 字节给结尾 '\0'
    std::string result(static_cast<size_t>(len) + 1, '\0');
    vsnprintf(&result[0], static_cast<size_t>(len) + 1, fmt, args);
    va_end(args);

    result.resize(static_cast<size_t>(len));
    return result;
}

// ---------------------------------------------------------------------------
//  编码转换
// ---------------------------------------------------------------------------
std::wstring Utf8ToUtf16(const std::string& utf8)
{
    return MultiByteToWide(utf8, CP_UTF8);
}

std::string Utf16ToUtf8(const std::wstring& utf16)
{
    return WideToMultiByte(utf16, CP_UTF8);
}

std::string Utf16ToAnsi(const std::wstring& utf16)
{
    return WideToMultiByte(utf16, CP_ACP);
}

std::wstring AnsiToUtf16(const std::string& ansi)
{
    return MultiByteToWide(ansi, CP_ACP);
}

std::string AnsiToUtf8(const std::string& ansi)
{
    return Utf16ToUtf8(AnsiToUtf16(ansi));
}

std::string Utf8ToAnsi(const std::string& utf8)
{
    return Utf16ToAnsi(Utf8ToUtf16(utf8));
}
std::wstring StringToWstring(const std::string& str)
{
    return MultiByteToWide(str, CP_UTF8);
}
// ---------------------------------------------------------------------------
//  时间
// ---------------------------------------------------------------------------
std::uint64_t GetUnixTime()
{
    return static_cast<std::uint64_t>(std::time(nullptr));
}

std::string SecondsToDayString(std::uint64_t seconds)
{
    const std::uint64_t days  = seconds / 86400;
    const std::uint64_t hours = (seconds % 86400) / 3600;
    const std::uint64_t mins  = (seconds % 3600) / 60;
    const std::uint64_t secs  = seconds % 60;

    return Format("%02llu天%02llu时%02llu分%02llu秒",
                  static_cast<unsigned long long>(days),
                  static_cast<unsigned long long>(hours),
                  static_cast<unsigned long long>(mins),
                  static_cast<unsigned long long>(secs));
}

std::string SecondsToHourString(std::uint64_t seconds)
{
    const std::uint64_t hours = seconds / 3600;
    const std::uint64_t mins  = (seconds % 3600) / 60;
    const std::uint64_t secs  = seconds % 60;

    return Format("%02llu小时%02llu分%02llu秒",
                  static_cast<unsigned long long>(hours),
                  static_cast<unsigned long long>(mins),
                  static_cast<unsigned long long>(secs));
}

// ---------------------------------------------------------------------------
//  进制 / 十六进制
// ---------------------------------------------------------------------------
std::string Int64ToBase36(std::int64_t value)
{
    const char kDigits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    // 取绝对值时避开 INT64_MIN 取反溢出
    const bool negative = value < 0;
    std::uint64_t mag = negative
        ? static_cast<std::uint64_t>(-(value + 1)) + 1
        : static_cast<std::uint64_t>(value);

    std::string result;
    do
    {
        result.insert(result.begin(), kDigits[mag % 36]);
        mag /= 36;
    } while (mag > 0);

    if (negative)
        result.insert(result.begin(), '-');
    return result;
}

std::string BytesToHex(const char* data, int len)
{
    static const char kHex[] = "0123456789abcdef";

    if (data == nullptr || len <= 0)
        return std::string();

    std::string result;
    result.reserve(static_cast<size_t>(len) * 2);
    for (int i = 0; i < len; ++i)
    {
        const unsigned char b = static_cast<unsigned char>(data[i]);
        result.push_back(kHex[b >> 4]);
        result.push_back(kHex[b & 0x0F]);
    }
    return result;
}

bool HexToBytes(const std::string& hex, std::vector<std::uint8_t>& out)
{
    out.clear();

    // 允许字符串开头 / 每个字节前出现 0x 或 0X
    int high = -1;
    for (size_t i = 0; i < hex.size(); ++i)
    {
        const char c = hex[i];

        // 跳过空白分隔
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            continue;

        // 0x 前缀：仅在一个字节的起始位置生效
        if (high < 0 && c == '0' && i + 1 < hex.size()
            && (hex[i + 1] == 'x' || hex[i + 1] == 'X'))
        {
            ++i;
            continue;
        }

        const int v = HexCharValue(c);
        if (v < 0)
        {
            out.clear();
            return false;
        }

        if (high < 0)
        {
            high = v;
        }
        else
        {
            out.push_back(static_cast<std::uint8_t>((high << 4) | v));
            high = -1;
        }
    }

    // 最后还剩半字节 → 十六进制位数为奇数，非法
    if (high >= 0)
    {
        out.clear();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
//  注册表
// ---------------------------------------------------------------------------
bool RegReadString(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                   std::string& out, const std::string& defaultValue, REGSAM view)
{
    out = defaultValue;

    HKEY hKey = OpenRegKey(root, subKey, view, false);
    if (hKey == nullptr)
        return false;

    bool ok = false;
    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, nullptr, &size) == ERROR_SUCCESS
        && (type == REG_SZ || type == REG_EXPAND_SZ)
        && size >= sizeof(wchar_t))
    {
        std::wstring buf(size / sizeof(wchar_t), L'\0');
        if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type,
                             reinterpret_cast<LPBYTE>(&buf[0]), &size) == ERROR_SUCCESS)
        {
            // 去掉 REG_SZ 结尾的 NUL
            size_t len = size / sizeof(wchar_t);
            while (len > 0 && buf[len - 1] == L'\0')
                --len;

            out = Utf16ToUtf8(std::wstring(buf.c_str(), len));
            ok = true;
        }
    }

    RegCloseKey(hKey);
    return ok;
}

bool RegWriteString(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                    const std::string& value, REGSAM view)
{
    HKEY hKey = OpenRegKey(root, subKey, view, true);
    if (hKey == nullptr)
        return false;

    const std::wstring w = Utf8ToUtf16(value);
    const DWORD bytes = static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t));
    const bool ok = RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ,
                                   reinterpret_cast<const BYTE*>(w.c_str()),
                                   bytes) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok;
}

bool RegReadDword(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                  DWORD& out, DWORD defaultValue, REGSAM view)
{
    out = defaultValue;

    HKEY hKey = OpenRegKey(root, subKey, view, false);
    if (hKey == nullptr)
        return false;

    bool ok = false;
    DWORD type = 0;
    DWORD size = sizeof(DWORD);
    DWORD value = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type,
                         reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS
        && type == REG_DWORD && size == sizeof(DWORD))
    {
        out = value;
        ok = true;
    }

    RegCloseKey(hKey);
    return ok;
}

bool RegWriteDword(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                   DWORD value, REGSAM view)
{
    HKEY hKey = OpenRegKey(root, subKey, view, true);
    if (hKey == nullptr)
        return false;

    const bool ok = RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD,
                                   reinterpret_cast<const BYTE*>(&value),
                                   sizeof(value)) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok;
}

bool RegReadBinary(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                   std::vector<std::uint8_t>& out, REGSAM view)
{
    out.clear();

    HKEY hKey = OpenRegKey(root, subKey, view, false);
    if (hKey == nullptr)
        return false;

    bool ok = false;
    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, nullptr, &size) == ERROR_SUCCESS
        && type == REG_BINARY && size > 0)
    {
        std::vector<std::uint8_t> buf(size);
        if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type,
                             reinterpret_cast<LPBYTE>(buf.data()), &size) == ERROR_SUCCESS)
        {
            buf.resize(size);
            out.swap(buf);
            ok = true;
        }
    }

    RegCloseKey(hKey);
    return ok;
}

bool RegWriteBinary(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                    const std::uint8_t* data, DWORD size, REGSAM view)
{
    if (data == nullptr || size == 0)
        return false;

    HKEY hKey = OpenRegKey(root, subKey, view, true);
    if (hKey == nullptr)
        return false;

    const bool ok = RegSetValueExW(hKey, valueName.c_str(), 0, REG_BINARY,
                                   reinterpret_cast<const BYTE*>(data),
                                   size) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok;
}

// ---------------------------------------------------------------------------
//  进程
// ---------------------------------------------------------------------------
DWORD GetProcessIdByName(const std::wstring& processName)
{
    if (processName.empty())
        return 0;

    // 统一按带 .exe 的形式比较，调用方可省略后缀
    std::wstring target = processName;
    const size_t n = target.size();
    if (n < 4 || _wcsicmp(target.c_str() + n - 4, L".exe") != 0)
        target += L".exe";

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    DWORD pid = 0;
    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, target.c_str()) == 0)
            {
                pid = entry.th32ProcessID; // 同名多进程：取第一个命中的
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

// ---------------------------------------------------------------------------
//  窗口
// ---------------------------------------------------------------------------
std::string GetWindowTitle(HWND hwnd)
{
    if (hwnd == nullptr || !IsWindow(hwnd))
        return std::string();

    const int len = GetWindowTextLengthW(hwnd);
    if (len <= 0)
        return std::string();

    std::wstring buf(static_cast<size_t>(len) + 1, L'\0');
    GetWindowTextW(hwnd, &buf[0], len + 1);
    buf.resize(static_cast<size_t>(len));
    return Utf16ToUtf8(buf);
}

std::string GetWindowClassName(HWND hwnd)
{
    if (hwnd == nullptr || !IsWindow(hwnd))
        return std::string();

    wchar_t buf[256] = {};
    const int len = GetClassNameW(hwnd, buf, static_cast<int>(_countof(buf)));
    if (len <= 0)
        return std::string();

    return Utf16ToUtf8(std::wstring(buf, static_cast<size_t>(len)));
}

bool CloseWindow(HWND hwnd)
{
    if (hwnd == nullptr)
        return false;
    return PostMessageW(hwnd, WM_CLOSE, 0, 0) != FALSE;
}

bool SetWindowSize(HWND hwnd, int width, int height)
{
    if (hwnd == nullptr)
        return false;
    return SetWindowPos(hwnd, nullptr, 0, 0, width, height,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

bool SetWindowVisible(HWND hwnd, bool visible)
{
    if (hwnd == nullptr)
        return false;
    return ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE) != FALSE;
}

bool IsWindowVisibleEx(HWND hwnd)
{
    if (hwnd == nullptr)
        return false;
    return IsWindowVisible(hwnd) != FALSE;
}

// ---------------------------------------------------------------------------
//  文件
// ---------------------------------------------------------------------------
bool ReadFileBytes(const std::string& path, std::vector<std::uint8_t>& out)
{
    out.clear();
    if (path.empty())
        return false;

    const std::wstring wpath = Utf8ToUtf16(path);
    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    bool ok = false;
    LARGE_INTEGER size = {};
    if (GetFileSizeEx(hFile, &size) && size.QuadPart >= 0)
    {
        std::vector<std::uint8_t> buf(static_cast<size_t>(size.QuadPart));
        size_t offset = 0;
        ok = true;

        // 分块读，单次不超过 1GB（ReadFile 的长度参数是 DWORD）
        while (offset < buf.size())
        {
            const DWORD toRead = static_cast<DWORD>(
                std::min<size_t>(buf.size() - offset, 0x40000000));
            DWORD read = 0;
            if (!ReadFile(hFile, buf.data() + offset, toRead, &read, nullptr) || read == 0)
            {
                ok = false;
                break;
            }
            offset += read;
        }

        if (ok)
            out.swap(buf);
    }

    CloseHandle(hFile);
    return ok;
}

bool WriteFileBytes(const std::string& path, const char* data, int len)
{
    if (path.empty() || data == nullptr || len < 0)
        return false;

    const std::wstring wpath = Utf8ToUtf16(path);
    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0,
                               nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    bool ok = true;
    size_t offset = 0;
    const size_t total = static_cast<size_t>(len);

    // 分块写，单次不超过 1GB（WriteFile 的长度参数是 DWORD）
    while (offset < total)
    {
        const DWORD toWrite = static_cast<DWORD>(
            std::min<size_t>(total - offset, 0x40000000));
        DWORD written = 0;
        if (!WriteFile(hFile, data + offset, toWrite, &written, nullptr) || written == 0)
        {
            ok = false;
            break;
        }
        offset += written;
    }

    CloseHandle(hFile);
    return ok;
}

std::string GetRunDirectory()
{
    wchar_t buf[32768] = {};
    const DWORD len = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(_countof(buf)));
    if (len == 0 || len >= _countof(buf))
        return std::string();

    std::wstring dir(buf, static_cast<size_t>(len));
    const size_t pos = dir.find_last_of(L'\\');
    if (pos == std::wstring::npos)
        return std::string();

    dir.resize(pos + 1); // 保留结尾反斜杠
    return Utf16ToUtf8(dir);
}

} // namespace utils
