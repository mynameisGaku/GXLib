/// @file Sound.cpp
/// @brief WAVファイル読み込み・PCMデータ管理の実装
#include "pch.h"
#include "Audio/Sound.h"
#include "Core/Logger.h"
#include <fstream>

namespace GX
{

/// WAVファイルのチャンクヘッダー
struct ChunkHeader
{
    uint32_t id;
    uint32_t size;
};

/// RIFFヘッダー
struct RiffHeader
{
    uint32_t riffId;     // "RIFF"
    uint32_t fileSize;
    uint32_t waveId;     // "WAVE"
};

/// 4文字のタグIDを作成
static constexpr uint32_t MakeTag(char a, char b, char c, char d)
{
    return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) |
           (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
}

bool Sound::LoadFromFile(const std::wstring& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        GX_LOG_ERROR("Failed to open WAV file");
        return false;
    }

    // RIFFヘッダー読み込み
    RiffHeader riff = {};
    file.read(reinterpret_cast<char*>(&riff), sizeof(RiffHeader));
    if (!file.good()) return false;

    if (riff.riffId != MakeTag('R', 'I', 'F', 'F') ||
        riff.waveId != MakeTag('W', 'A', 'V', 'E'))
    {
        GX_LOG_ERROR("Invalid WAV file (not RIFF/WAVE)");
        return false;
    }

    // チャンクを順番に読む
    bool foundFmt = false;
    bool foundData = false;

    while (!file.eof() && file.good())
    {
        ChunkHeader chunk = {};
        file.read(reinterpret_cast<char*>(&chunk), sizeof(ChunkHeader));
        if (!file.good()) break;

        if (chunk.id == MakeTag('f', 'm', 't', ' '))
        {
            // fmtチャンク: フォーマット情報
            m_format = {};
            uint32_t readSize = (std::min)(chunk.size, static_cast<uint32_t>(sizeof(WAVEFORMATEX)));
            file.read(reinterpret_cast<char*>(&m_format), readSize);
            if (!file.good()) return false;

            // 残りをスキップ
            if (chunk.size > readSize)
                file.seekg(chunk.size - readSize, std::ios::cur);

            foundFmt = true;
        }
        else if (chunk.id == MakeTag('d', 'a', 't', 'a'))
        {
            // dataチャンク: PCMデータ
            m_pcmData.resize(chunk.size);
            file.read(reinterpret_cast<char*>(m_pcmData.data()), chunk.size);
            if (!file.good()) return false;
            foundData = true;
            break;  // dataが見つかったら終了
        }
        else
        {
            // 不明なチャンクはスキップ（2バイトアライメント）
            uint32_t skipSize = chunk.size + (chunk.size % 2);
            file.seekg(skipSize, std::ios::cur);
        }
    }

    if (!foundFmt || !foundData)
    {
        GX_LOG_ERROR("WAV file missing fmt or data chunk");
        m_pcmData.clear();
        m_format = {};
        return false;
    }

    GX_LOG_INFO("WAV loaded: %uHz, %uch, %ubit, %u bytes",
                m_format.nSamplesPerSec, m_format.nChannels,
                m_format.wBitsPerSample, GetDataSize());
    return true;
}

} // namespace GX
