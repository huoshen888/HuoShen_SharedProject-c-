#include "EZip.h"

#include <cstring>
#include <new>

//#include "miniz.h"
#include <Zlib/zlib.h>
namespace ezip {

namespace {

// 显式小端读写，避免依赖宿主机字节序（易语言只跑在 x86 小端上，
// 但这份移植代码保持可移植）。
void WriteLE32(std::uint8_t* dst, std::uint32_t v)
{
    dst[0] = static_cast<std::uint8_t>(v & 0xFF);
    dst[1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
    dst[2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
    dst[3] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
}

std::uint32_t ReadLE32(const std::uint8_t* src)
{
    return static_cast<std::uint32_t>(src[0])
         | (static_cast<std::uint32_t>(src[1]) << 8)
         | (static_cast<std::uint32_t>(src[2]) << 16)
         | (static_cast<std::uint32_t>(src[3]) << 24);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// 压缩 —— 对应易语言：处理后数据 = 压缩数据 (原始数据)
//
// 易语言 dp1 内部实现（逆向确认）：
//   输出 = 4 字节魔数 0x033E0F0D + 4 字节原始长度(LE) + compress2(level 6)
// ---------------------------------------------------------------------------
bool CompressData(const void* src, std::size_t srcLen,
                  std::vector<std::uint8_t>& out)
{
    out.clear();

    // 易语言侧由「取字节集长度()=0」在上层拦截空输入；这里同样拒绝。
    // 长度字段是 u32，超过 4GB-1 无法表示。
    if (src == nullptr || srcLen == 0 || srcLen > 0xFFFFFFFFull)
        return false;

    // compressBound 是 zlib 建议的压缩结果上界（含 2 字节头 + 4 字节
    // adler32 的 zlib 封装）。实际写出长度以 compress2 写入的 dstLen 为准。
    const uLong bound = compressBound(static_cast<uLong>(srcLen));
    out.resize(kHeaderSize + static_cast<std::size_t>(bound));

    // 头部：魔数 + 原始长度（小端）
    WriteLE32(out.data() + 0, kMagic);
    WriteLE32(out.data() + 4, static_cast<std::uint32_t>(srcLen));

    // zlib 流。Z_DEFAULT_COMPRESSION = 级别 6，与易语言输出逐字节一致。
    // 若想显式指定，可换成 6，结果相同。
    uLongf dstLen = static_cast<uLongf>(bound);
    const int ret = compress2(out.data() + kHeaderSize, &dstLen,
                              static_cast<const Bytef*>(src),
                              static_cast<uLong>(srcLen),
                              Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
    {
        out.clear();
        return false;
    }

    out.resize(kHeaderSize + static_cast<std::size_t>(dstLen));
    return true;
}

// ---------------------------------------------------------------------------
// 解压 —— 对应易语言：处理后数据 = 解压数据 (原始数据)
//
// 易语言 dp1 内部实现（逆向 + 边缘测试确认）：
//   1. 数据长度 < 8                     → 失败（返回空字节集）
//   2. 前 4 字节 != 0x033E0F0D          → 失败
//   3. 按 +4 处的 u32 分配目标缓冲区
//   4. uncompress(dst, &dstLen=size, src+8, srcLen-8) != Z_OK → 失败
//      （size 为 0 或小于实际解压结果时触发 Z_BUF_ERROR）
//   5. 成功 → 字节集长度取 uncompress 回写的实际 dstLen
// ---------------------------------------------------------------------------
bool DecompressData(const void* src, std::size_t srcLen,
                    std::vector<std::uint8_t>& out)
{
    out.clear();

    if (src == nullptr || srcLen < kHeaderSize)
        return false;

    const std::uint8_t* p = static_cast<const std::uint8_t*>(src);

    // 校验魔数（实测：改掉魔数 → 易语言报「数据处理失败」）
    if (ReadLE32(p + 0) != kMagic)
        return false;

    // 原始长度 → 目标缓冲区大小（实测：size=0 → 失败）
    const std::uint32_t size = ReadLE32(p + 4);
    if (size == 0)
        return false;

    std::vector<std::uint8_t> buf;
    try
    {
        buf.resize(size);
    }
    catch (const std::bad_alloc&)
    {
        return false; // 头部被篡改成超大尺寸时不要崩溃
    }

    uLongf dstLen = size;
    const int ret = uncompress(buf.data(), &dstLen,
                               p + kHeaderSize,
                               static_cast<uLong>(srcLen - kHeaderSize));
    if (ret != Z_OK)
        return false; // 含 zlib adler32 校验失败、缓冲区不足等

    // 与易语言行为一致：即便头部长度字段比实际解压结果大，也算成功，
    // 输出取实际解出的 dstLen 字节（实测 big_size 场景）。
    out.assign(buf.begin(), buf.begin() + dstLen);
    return true;
}

} // namespace ezip
