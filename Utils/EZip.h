#pragma once
// ============================================================================
//  EZip.h —— 易语言 dp1 支持库「压缩数据 / 解压数据」的 C++ 还原
//
//  逆向结论（分析对象：E:\压缩解压Demo.exe，x86 静态编译，内嵌 zlib）：
//
//    磁盘格式（.ezip 字节流）：
//      偏移  长度   含义
//      +0    4      魔数，磁盘字节序 0D 0F 3E 03（小端 u32 = 0x033E0F0D）
//      +4    4      原始数据长度，小端 u32
//      +8    ...    标准 zlib 流（压缩级别 6 = Z_DEFAULT_COMPRESSION，
//                   首两字节 78 9C，末尾 4 字节大端 adler32）
//
//    与易语言实测一致的行为细节：
//      * 压缩输出与 zlib.compress(data, 6) 逐字节一致（三个不同样本验证）
//      * 解压时校验魔数，魔数不符 → 失败（返回空字节集）
//      * 长度字段仅作为 uncompress() 的目标缓冲区大小：
//          - 偏大：仍能解压成功（实测 size=0x100000 时正常）
//          - 偏小 / 为 0：uncompress 返回 Z_BUF_ERROR → 失败
//      * 单个数据块上限 4GB-1（长度字段是 u32）
//
//    依赖：zlib（API 仅用 compress2 / uncompress / compressBound）
// ============================================================================
#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <cstddef>
#include <vector>

namespace ezip {

// 魔数。注意：以小端 u32 读出等于 0x033E0F0D，磁盘上呈 0D 0F 3E 03。
constexpr std::uint32_t kMagic      = 0x033E0F0D;
// 头部字节数：4 字节魔数 + 4 字节原始长度。
constexpr std::size_t   kHeaderSize = 8;

// 压缩：等价于易语言「压缩数据」。
// 成功时 out 为完整的 .ezip 字节流（头 + zlib 流），返回 true。
// 输入为空 / 超 4GB 时返回 false。
bool CompressData(const void* src, std::size_t srcLen,
                  std::vector<std::uint8_t>& out);

// 解压：等价于易语言「解压数据」。
// 数据非法（长度不足 8、魔数不符、zlib 校验失败、长度字段为 0/过小）
// 时返回 false，对应易语言返回空字节集。
bool DecompressData(const void* src, std::size_t srcLen,
                    std::vector<std::uint8_t>& out);

} // namespace ezip
