#pragma once
// ============================================================================
//  utils.h —— 通用工具函数集（仅依赖 Windows SDK）
//
//  分组：字符串格式化 / 编码转换 / 时间 / 进制与十六进制 / 注册表 /
//        进程 / 窗口 / 文件
//
//  约定：
//    * 位于 namespace utils，函数名英文 PascalCase
//    * 参数能用 const 引用就用 const 引用
//    * 返回 bool 的函数，true = 成功，false = 失败
//    * 输出型参数在失败时保持默认值（或清空）
//    * 涉及易语言同名命令的，在注释里标注易语言名
// ============================================================================
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace utils {

// ---------------------------------------------------------------------------
//  字符串格式化
// ---------------------------------------------------------------------------

// 与 printf 完全一致，返回 std::string。
// 内部用 vsnprintf 两遍探测：第一遍取长度，第二遍真正写入，
// 因此格式串与参数都可以直接使用 UTF-8（含中文）。
std::string Format(const char* fmt, ...);

// ---------------------------------------------------------------------------
//  编码转换
//  统一失败语义：转换失败（非法字节序列 / 空串）返回空串。
// ---------------------------------------------------------------------------

std::wstring Utf8ToUtf16(const std::string& utf8);   // UTF-8  → UTF-16
std::string  Utf16ToUtf8(const std::wstring& utf16); // UTF-16 → UTF-8
std::string  Utf16ToAnsi(const std::wstring& utf16); // UTF-16 → ANSI(CP_ACP)
std::wstring AnsiToUtf16(const std::string& ansi);   // ANSI   → UTF-16
std::string  AnsiToUtf8(const std::string& ansi);    // ANSI   → UTF-8
std::string  Utf8ToAnsi(const std::string& utf8);    // UTF-8  → ANSI
std::wstring StringToWstring(const std::string& str);
// ---------------------------------------------------------------------------
//  时间
// ---------------------------------------------------------------------------

// 取当前 Unix 时间戳（秒）。
std::uint64_t GetUnixTime();

// 秒数 → "xx天xx时xx分xx秒"
std::string SecondsToDayString(std::uint64_t seconds);

// 秒数 → "xx小时xx分xx秒"
std::string SecondsToHourString(std::uint64_t seconds);

// ---------------------------------------------------------------------------
//  进制 / 十六进制
// ---------------------------------------------------------------------------

// 十进制 int64 → 36 进制字符串（字符集 0-9a-z，负数带前导 '-'）。
std::string Int64ToBase36(std::int64_t value);

// 字节数组 → 十六进制文本（小写，无分隔符）。
std::string BytesToHex(const char* data, int len);

// 十六进制文本 → 字节数组。
// 容忍大小写、可选的 0x 前缀、空白分隔（如 "0x0D 0f 3E03"）；
// 出现非法字符或十六进制位数为奇数时返回 false，out 清空。
bool HexToBytes(const std::string& hex, std::vector<std::uint8_t>& out);

// ---------------------------------------------------------------------------
//  注册表
//  root  : 根键，如 HKEY_CURRENT_USER / HKEY_LOCAL_MACHINE
//  subKey: 子键路径，如 L"Software\\MyApp"
//  view  : 32/64 位视图，KEY_WOW64_32KEY 或 KEY_WOW64_64KEY
//  读取失败返回 false；字符串读出后按 UTF-8 返回。
// ---------------------------------------------------------------------------

bool RegReadString(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                   std::string& out, const std::string& defaultValue = "",
                   REGSAM view = KEY_WOW64_64KEY);
bool RegWriteString(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                    const std::string& value, REGSAM view = KEY_WOW64_64KEY);

bool RegReadDword(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                  DWORD& out, DWORD defaultValue = 0, REGSAM view = KEY_WOW64_64KEY);
bool RegWriteDword(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                   DWORD value, REGSAM view = KEY_WOW64_64KEY);

bool RegReadBinary(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                   std::vector<std::uint8_t>& out, REGSAM view = KEY_WOW64_64KEY);
bool RegWriteBinary(HKEY root, const std::wstring& subKey, const std::wstring& valueName,
                    const std::uint8_t* data, DWORD size, REGSAM view = KEY_WOW64_64KEY);

// ---------------------------------------------------------------------------
//  进程
// ---------------------------------------------------------------------------

// 按进程名取 PID —— 对应易语言「进程_名取ID」。
// 用 CreateToolhelp32Snapshot 遍历系统进程，比较不区分大小写，
// processName 带不带 ".exe" 均可。
// 同名多进程时只取第一个命中的；找不到返回 0。
DWORD GetProcessIdByName(const std::wstring& processName);

// ---------------------------------------------------------------------------
//  窗口（全部按 HWND 操作）
// ---------------------------------------------------------------------------

// 取窗口标题（UTF-8）—— 对应易语言「窗口_取标题」。
std::string GetWindowTitle(HWND hwnd);

// 取窗口类名（UTF-8）—— 对应易语言「窗口_取类名」。
std::string GetWindowClassName(HWND hwnd);

// 关闭窗口（向窗口发送 WM_CLOSE）—— 对应易语言「窗口_关闭」。
bool CloseWindow(HWND hwnd);

// 设置窗口大小（保持左上角位置不变）—— 对应易语言「窗口_置尺寸」。
bool SetWindowSize(HWND hwnd, int width, int height);

// 显示 / 隐藏窗口 —— 对应易语言「窗口_显示」「窗口_隐藏」。
bool SetWindowVisible(HWND hwnd, bool visible);

// 判断窗口是否可见 —— 对应易语言「窗口_是否可视」。
bool IsWindowVisibleEx(HWND hwnd);

// ---------------------------------------------------------------------------
//  文件
// ---------------------------------------------------------------------------

// 读入整个文件（二进制）到 out —— 失败返回 false 且 out 清空。
bool ReadFileBytes(const std::string& path, std::vector<std::uint8_t>& out);

// 写入文件（二进制，覆盖已有内容）—— path 按 UTF-8 解释。
bool WriteFileBytes(const std::string& path, const char* data, int len);

// 取运行目录（exe 所在目录，结尾带反斜杠，UTF-8）。
std::string GetRunDirectory();

} // namespace utils
