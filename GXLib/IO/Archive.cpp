#include "pch.h"
#include "IO/Archive.h"
#include "IO/Crypto.h"
#include "Core/Logger.h"
#include "ThirdParty/lz4.h"

namespace GX {

// アーカイブ形式の定数 (識別子やフラグ)
static constexpr uint8_t k_Magic[8] = { 'G','X','A','R','C',0,0,0 };
static constexpr uint32_t k_FlagEncrypted  = 0x01;
static constexpr uint32_t k_FlagCompressed = 0x02;
static constexpr uint8_t  k_EntryFlagCompressed = 0x01;

// ============================================================================
// アーカイブ読み取り (Reader)
// ============================================================================

bool Archive::Open(const std::string& filePath, const std::string& password)
{
    Close();
    m_filePath = filePath;

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        GX_LOG_ERROR("Archive::Open: Failed to open: %s", filePath.c_str());
        return false;
    }

    // マジックを読み込む (形式識別)
    uint8_t magic[8];
    file.read(reinterpret_cast<char*>(magic), 8);
    if (memcmp(magic, k_Magic, 8) != 0)
    {
        GX_LOG_ERROR("Archive::Open: Invalid magic in: %s", filePath.c_str());
        return false;
    }

    // TOCヘッダを読む (目次の情報)
    uint32_t entryCount, tocSize, flags, reserved;
    file.read(reinterpret_cast<char*>(&entryCount), 4);
    file.read(reinterpret_cast<char*>(&tocSize), 4);
    file.read(reinterpret_cast<char*>(&flags), 4);
    file.read(reinterpret_cast<char*>(&reserved), 4);

    m_encrypted = (flags & k_FlagEncrypted) != 0;

    if (m_encrypted)
    {
        if (password.empty())
        {
            GX_LOG_ERROR("Archive::Open: Archive is encrypted but no password provided");
            return false;
        }
        m_key = Crypto::SHA256(password.c_str(), password.size());
    }

    // 初心者向け: TOC(目次)には各ファイルのパス・位置・サイズが入っている
    // TOCデータを読む
    std::vector<uint8_t> tocData(tocSize);
    file.read(reinterpret_cast<char*>(tocData.data()), tocSize);

    // 暗号化されていればTOCを復号する
    if (m_encrypted)
    {
        // IVはTOCデータの先頭16バイト
        if (tocSize <= 16)
        {
            GX_LOG_ERROR("Archive::Open: TOC too small for IV");
            return false;
        }
        uint8_t iv[16];
        memcpy(iv, tocData.data(), 16);
        auto decrypted = Crypto::Decrypt(tocData.data() + 16, tocSize - 16,
                                          m_key.data(), iv);
        if (decrypted.empty())
        {
            GX_LOG_ERROR("Archive::Open: TOC decryption failed (wrong password?)");
            return false;
        }
        tocData = std::move(decrypted);
    }

    // TOCエントリを解析する
    size_t pos = 0;
    m_entries.reserve(entryCount);
    for (uint32_t i = 0; i < entryCount; ++i)
    {
        if (pos + 2 > tocData.size()) break;

        ArchiveEntry entry;
        uint16_t pathLen = *reinterpret_cast<const uint16_t*>(&tocData[pos]);
        pos += 2;

        if (pos + pathLen > tocData.size()) break;
        entry.path.assign(reinterpret_cast<const char*>(&tocData[pos]), pathLen);
        pos += pathLen;

        if (pos + 8 + 4 + 4 + 1 > tocData.size()) break;
        entry.offset = *reinterpret_cast<const uint64_t*>(&tocData[pos]); pos += 8;
        entry.compressedSize = *reinterpret_cast<const uint32_t*>(&tocData[pos]); pos += 4;
        entry.originalSize = *reinterpret_cast<const uint32_t*>(&tocData[pos]); pos += 4;
        entry.flags = tocData[pos]; pos += 1;

        m_entries.push_back(std::move(entry));
    }

    // データ開始位置 = magic(8) + TOCヘッダ(16) + TOCデータ(tocSize)
    m_dataOffset = 8 + 16 + tocSize;

    GX_LOG_INFO("Archive::Open: Loaded %s (%u entries, encrypted=%s)",
        filePath.c_str(), entryCount, m_encrypted ? "true" : "false");
    return true;
}

void Archive::Close()
{
    m_entries.clear();
    m_filePath.clear();
    m_encrypted = false;
    m_dataOffset = 0;
    m_key = {};
}

bool Archive::Contains(const std::string& path) const
{
    for (const auto& e : m_entries)
    {
        if (e.path == path)
            return true;
    }
    return false;
}

FileData Archive::Read(const std::string& path) const
{
    const ArchiveEntry* found = nullptr;
    for (const auto& e : m_entries)
    {
        if (e.path == path)
        {
            found = &e;
            break;
        }
    }

    if (!found)
        return FileData{};

    std::ifstream file(m_filePath, std::ios::binary);
    if (!file.is_open())
        return FileData{};

    // ファイルデータ位置にシーク
    file.seekg(m_dataOffset + found->offset);

    std::vector<uint8_t> compressed(found->compressedSize);
    file.read(reinterpret_cast<char*>(compressed.data()), found->compressedSize);

    FileData result;

    if (found->flags & k_EntryFlagCompressed)
    {
        // LZ4で伸長する
        result.data.resize(found->originalSize);
        int decompressed = LZ4_decompress_safe(
            reinterpret_cast<const char*>(compressed.data()),
            reinterpret_cast<char*>(result.data.data()),
            static_cast<int>(found->compressedSize),
            static_cast<int>(found->originalSize));

        if (decompressed <= 0)
        {
            GX_LOG_ERROR("Archive::Read: LZ4 decompression failed for: %s", path.c_str());
            result.data.clear();
        }
    }
    else
    {
        // 非圧縮データ
        result.data = std::move(compressed);
    }

    return result;
}

// ============================================================================
// アーカイブ書き込み (ArchiveWriter)
// ============================================================================

void ArchiveWriter::SetPassword(const std::string& password)
{
    m_password = password;
}

void ArchiveWriter::SetCompression(bool enable)
{
    m_compress = enable;
}

void ArchiveWriter::AddFile(const std::string& archivePath, const std::string& diskPath)
{
    std::ifstream file(diskPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        GX_LOG_WARN("ArchiveWriter::AddFile: Cannot open: %s", diskPath.c_str());
        return;
    }

    auto fileSize = file.tellg();
    if (fileSize <= 0) return;

    PendingFile pf;
    pf.archivePath = archivePath;
    pf.data.resize(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(pf.data.data()), fileSize);
    m_files.push_back(std::move(pf));
}

void ArchiveWriter::AddFile(const std::string& archivePath, const void* data, size_t size)
{
    PendingFile pf;
    pf.archivePath = archivePath;
    pf.data.assign(static_cast<const uint8_t*>(data),
                   static_cast<const uint8_t*>(data) + size);
    m_files.push_back(std::move(pf));
}

bool ArchiveWriter::Save(const std::string& outputPath)
{
    bool encrypted = !m_password.empty();
    std::array<uint8_t, 32> key = {};
    if (encrypted)
        key = Crypto::SHA256(m_password.c_str(), m_password.size());

    // ファイルを圧縮しエントリを構築する
    struct FileBlock {
        std::vector<uint8_t> data;
        uint32_t originalSize;
        uint8_t flags;
    };
    std::vector<FileBlock> blocks;
    std::vector<ArchiveEntry> entries;
    blocks.reserve(m_files.size());
    entries.reserve(m_files.size());

    uint64_t currentOffset = 0;
    for (const auto& pf : m_files)
    {
        FileBlock block;
        block.originalSize = static_cast<uint32_t>(pf.data.size());
        block.flags = 0;

        if (m_compress && pf.data.size() > 64) // 64バイト以上のみ圧縮 (小さすぎると効果が薄い)
        {
            int maxCompressed = LZ4_compressBound(static_cast<int>(pf.data.size()));
            std::vector<uint8_t> compressed(maxCompressed);
            int compressedSize = LZ4_compress_default(
                reinterpret_cast<const char*>(pf.data.data()),
                reinterpret_cast<char*>(compressed.data()),
                static_cast<int>(pf.data.size()),
                maxCompressed);

            // 実際に小さくなった場合のみ圧縮版を採用する
            if (compressedSize > 0 && static_cast<size_t>(compressedSize) < pf.data.size())
            {
                compressed.resize(compressedSize);
                block.data = std::move(compressed);
                block.flags = k_EntryFlagCompressed;
            }
            else
            {
                block.data = pf.data;
            }
        }
        else
        {
            block.data = pf.data;
        }

        ArchiveEntry entry;
        entry.path = pf.archivePath;
        entry.offset = currentOffset;
        entry.compressedSize = static_cast<uint32_t>(block.data.size());
        entry.originalSize = block.originalSize;
        entry.flags = block.flags;
        entries.push_back(std::move(entry));

        currentOffset += block.data.size();
        blocks.push_back(std::move(block));
    }

    // TOCを構築する
    std::vector<uint8_t> tocData;
    for (const auto& entry : entries)
    {
        uint16_t pathLen = static_cast<uint16_t>(entry.path.size());
        tocData.insert(tocData.end(),
            reinterpret_cast<const uint8_t*>(&pathLen),
            reinterpret_cast<const uint8_t*>(&pathLen) + 2);
        tocData.insert(tocData.end(), entry.path.begin(), entry.path.end());
        tocData.insert(tocData.end(),
            reinterpret_cast<const uint8_t*>(&entry.offset),
            reinterpret_cast<const uint8_t*>(&entry.offset) + 8);
        tocData.insert(tocData.end(),
            reinterpret_cast<const uint8_t*>(&entry.compressedSize),
            reinterpret_cast<const uint8_t*>(&entry.compressedSize) + 4);
        tocData.insert(tocData.end(),
            reinterpret_cast<const uint8_t*>(&entry.originalSize),
            reinterpret_cast<const uint8_t*>(&entry.originalSize) + 4);
        tocData.push_back(entry.flags);
    }

    // 必要ならTOCを暗号化する
    uint32_t tocSize;
    std::vector<uint8_t> tocFinal;
    if (encrypted)
    {
        uint8_t iv[16];
        Crypto::GenerateRandomBytes(iv, 16);
        auto encryptedToc = Crypto::Encrypt(tocData.data(), tocData.size(),
                                             key.data(), iv);
        // IVを先頭に付与する (復号に必要)
        tocFinal.reserve(16 + encryptedToc.size());
        tocFinal.insert(tocFinal.end(), iv, iv + 16);
        tocFinal.insert(tocFinal.end(), encryptedToc.begin(), encryptedToc.end());
        tocSize = static_cast<uint32_t>(tocFinal.size());
    }
    else
    {
        tocFinal = std::move(tocData);
        tocSize = static_cast<uint32_t>(tocFinal.size());
    }

    // アーカイブを書き込む
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open())
    {
        GX_LOG_ERROR("ArchiveWriter::Save: Cannot create: %s", outputPath.c_str());
        return false;
    }

    // マジック
    out.write(reinterpret_cast<const char*>(k_Magic), 8);

    // TOCヘッダ
    uint32_t entryCount = static_cast<uint32_t>(entries.size());
    uint32_t flags = 0;
    if (encrypted) flags |= k_FlagEncrypted;
    if (m_compress) flags |= k_FlagCompressed;
    uint32_t reserved = 0;

    out.write(reinterpret_cast<const char*>(&entryCount), 4);
    out.write(reinterpret_cast<const char*>(&tocSize), 4);
    out.write(reinterpret_cast<const char*>(&flags), 4);
    out.write(reinterpret_cast<const char*>(&reserved), 4);

    // TOCデータ
    out.write(reinterpret_cast<const char*>(tocFinal.data()), tocFinal.size());

    // ファイル本体データ
    for (const auto& block : blocks)
    {
        out.write(reinterpret_cast<const char*>(block.data.data()), block.data.size());
    }

    GX_LOG_INFO("ArchiveWriter::Save: Created %s (%u files, %llu bytes)",
        outputPath.c_str(), entryCount,
        static_cast<unsigned long long>(8 + 16 + tocFinal.size() + currentOffset));
    return true;
}

} // namespace GX
