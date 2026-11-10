#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace YouDaoNote
{
    /**
     * @brief 获取有道云笔记的纯文本内容 (等同于：取有道云笔记)
     * @param shareUrl 有道云分享链接
     * @return 成功返回笔记内容字符串
     */
    std::string GetNoteContent(const std::wstring& shareUrl);

    /**
     * @brief 获取有道云笔记内的附件或资源直链 (等同于：有道云取直链)
     * @param shareUrl 有道云分享链接
     * @return 成功返回附件下载直链
     */
    std::string GetNoteDirectLink(const std::wstring& shareUrl);

    /**
     * @brief 将分享的短链接还原为长链接并附带13位现行时间戳 (等同于：获取真实Referer)
     * @param shareUrl 有道云分享的短链接
     * @return 成功返回可用于附加协议头的真实 Referer 链接
     */
    std::wstring GetRealReferer(const std::wstring& shareUrl);

    /**
     * @brief 基于 WinHttpClient 下载网络文件到内存 (支持 Referer 和 Cookie)
     * @param downloadUrl 文件的下载直链
     * @param outBytes 输出参数，下载成功后文件字节流存入该数组
     * @param referer 附加的 Referer 协议头，如果没有可以传空字符串
     * @param cookie 附加的 Cookie 数据，如果没有可以传空字符串
     * @return 下载成功返回 true，失败返回 false
     */
    bool DownloadNetworkFileToMemory(const std::wstring& downloadUrl, std::vector<uint8_t>& outBytes, const std::wstring& referer = L"", const std::wstring& cookie = L"");

    /**
     * @brief 基于 WinHttpClient 下载网络文件并落盘 (支持 Referer 和 Cookie)
     * @param downloadUrl 文件的下载直链
     * @param savePath 本地保存文件的绝对路径
     * @param referer 附加的 Referer 协议头，如果没有可以传空字符串
     * @param cookie 附加的 Cookie 数据，如果没有可以传空字符串
     * @return 下载并保存成功返回 true，失败返回 false
     */
    bool DownloadNetworkFile(const std::wstring& downloadUrl, const std::wstring& savePath, const std::wstring& referer = L"", const std::wstring& cookie = L"");
}