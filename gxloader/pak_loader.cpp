/// @file pak_loader.cpp
/// @brief GXPAKバンドルローダーの実装

#include "pak_loader.h"
#include <cstdio>
#include <cstring>

// LZ4 for decompression
#include "lz4.h"

namespace gxloader
{

bool PakLoader::Open(const std::string& filePath)
{
    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) return false;

    // Read header
    gxfmt::GxpakHeader header{};
    fread(&header, sizeof(header), 1, f);

    if (header.magic != gxfmt::k_GxpakMagic)
    {
        fclose(f);
        return false;
    }

    m_filePath = filePath;
    m_entries.clear();

    // TOCはファイル末尾に配置されている
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    (void)fileSize;

    _fseeki64(f, static_cast<long long>(header.tocOffset), SEEK_SET);

    // TOCエントリを1つずつ読み込む (可変長シリアライズ)
    for (uint32_t i = 0; i < header.entryCount; ++i)
    {
        // Read path length
        uint32_t pathLen = 0;
        fread(&pathLen, 4, 1, f);

        // Read path
        std::vector<char> pathBuf(pathLen + 1, '\0');
        fread(pathBuf.data(), 1, pathLen, f);

        // Read fixed fields
        gxfmt::GxpakAssetType assetType;
        uint8_t compressed;
        uint8_t pad[2];
        uint64_t dataOffset;
        uint32_t compressedSize;
        uint32_t originalSize;

        fread(&assetType, 1, 1, f);
        fread(&compressed, 1, 1, f);
        fread(pad, 1, 2, f);
        fread(&dataOffset, 8, 1, f);
        fread(&compressedSize, 4, 1, f);
        fread(&originalSize, 4, 1, f);

        gxfmt::GxpakEntry entry{};
        strncpy(entry.path, pathBuf.data(), sizeof(entry.path) - 1);
        entry.assetType = assetType;
        entry.compressed = (compressed != 0);
        entry.dataOffset = dataOffset;
        entry.compressedSize = compressedSize;
        entry.originalSize = originalSize;

        m_entries.push_back(entry);
    }

    fclose(f);
    return true;
}

void PakLoader::Close()
{
    m_entries.clear();
    m_filePath.clear();
}

bool PakLoader::Contains(const std::string& path) const
{
    for (auto& e : m_entries)
        if (path == e.path) return true;
    return false;
}

std::vector<uint8_t> PakLoader::Read(const std::string& path) const
{
    for (auto& entry : m_entries)
    {
        if (path != entry.path) continue;

        FILE* f = fopen(m_filePath.c_str(), "rb");
        if (!f) return {};

        _fseeki64(f, static_cast<long long>(entry.dataOffset), SEEK_SET);

        std::vector<uint8_t> rawData(entry.compressedSize);
        fread(rawData.data(), 1, entry.compressedSize, f);
        fclose(f);

        if (entry.compressed) // LZ4圧縮されたエントリを展開
        {
            std::vector<uint8_t> decompressed(entry.originalSize);
            int result = LZ4_decompress_safe(
                reinterpret_cast<const char*>(rawData.data()),
                reinterpret_cast<char*>(decompressed.data()),
                static_cast<int>(entry.compressedSize),
                static_cast<int>(entry.originalSize));

            if (result < 0) return {};
            return decompressed;
        }

        return rawData;
    }
    return {};
}

std::vector<gxfmt::GxpakEntry> PakLoader::GetEntries() const
{
    return m_entries;
}

std::vector<gxfmt::GxpakEntry> PakLoader::GetEntriesByType(gxfmt::GxpakAssetType type) const
{
    std::vector<gxfmt::GxpakEntry> result;
    for (auto& e : m_entries)
        if (e.assetType == type) result.push_back(e);
    return result;
}

} // namespace gxloader
