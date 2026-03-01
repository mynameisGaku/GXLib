/// @file SaveVersion.cpp
/// @brief セーブフォーマットバージョニングの実装
#include "pch_common.h"
#include "Core/SaveVersion.h"
#include "Core/Logger.h"

#include <chrono>
#include <cstring>
#include <algorithm>

namespace gx
{

// ============================================================================
// CRC32テーブル
// ============================================================================

const uint32_t* SaveVersionManager::GetCRC32Table()
{
    static uint32_t table[256] = {};
    static bool initialized = false;

    if (!initialized)
    {
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j)
            {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320u;
                else
                    crc >>= 1;
            }
            table[i] = crc;
        }
        initialized = true;
    }

    return table;
}

// ============================================================================
// チェックサム
// ============================================================================

uint32_t SaveVersionManager::ComputeChecksum(const uint8_t* data, size_t size)
{
    const uint32_t* table = GetCRC32Table();
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < size; ++i)
    {
        uint8_t index = static_cast<uint8_t>((crc ^ data[i]) & 0xFF);
        crc = (crc >> 8) ^ table[index];
    }

    return crc ^ 0xFFFFFFFFu;
}

bool SaveVersionManager::VerifyChecksum(const SaveFormatHeader& header, const std::vector<uint8_t>& data) const
{
    // data はヘッダー後のペイロード
    if (data.empty())
        return header.checksum == ComputeChecksum(nullptr, 0);

    uint32_t computed = ComputeChecksum(data.data(), data.size());
    return computed == header.checksum;
}

// ============================================================================
// マイグレーション登録
// ============================================================================

void SaveVersionManager::RegisterMigration(uint32_t from, uint32_t to, const std::string& description,
                                            std::function<bool(std::vector<uint8_t>&)> fn)
{
    MigrationStep step;
    step.fromVersion  = from;
    step.toVersion    = to;
    step.description  = description;
    step.migrationFn  = std::move(fn);
    m_migrations.push_back(std::move(step));

    GX_LOG_INFO("SaveVersionManager: Registered migration v%u -> v%u: %s",
                from, to, description.c_str());
}

// ============================================================================
// ヘッダー書き込み / 読み取り
// ============================================================================

void SaveVersionManager::WriteHeader(std::vector<uint8_t>& data, uint32_t version, uint32_t flags)
{
    if (version == 0)
        version = m_currentVersion;

    // タイムスタンプ取得
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    uint64_t timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(epoch).count());

    // ペイロードのチェックサム計算
    uint32_t checksum = 0;
    if (!data.empty())
        checksum = ComputeChecksum(data.data(), data.size());

    // ヘッダー構築
    SaveFormatHeader header;
    header.magic        = k_Magic;
    header.version      = version;
    header.flags        = flags;
    header.checksum     = checksum;
    header.timestamp    = timestamp;
    header.metadataSize = 0;
    header.dataSize     = static_cast<uint32_t>(data.size());

    // ヘッダーをデータの先頭にプリペンド
    std::vector<uint8_t> headerBytes(k_HeaderSize);
    std::memcpy(headerBytes.data(), &header, k_HeaderSize);

    data.insert(data.begin(), headerBytes.begin(), headerBytes.end());
}

bool SaveVersionManager::ReadHeader(const std::vector<uint8_t>& data, SaveFormatHeader& outHeader) const
{
    if (data.size() < k_HeaderSize)
        return false;

    std::memcpy(&outHeader, data.data(), k_HeaderSize);
    return true;
}

// ============================================================================
// ヘッダー検証
// ============================================================================

bool SaveVersionManager::ValidateHeader(const SaveFormatHeader& header) const
{
    // マジックナンバーチェック
    if (header.magic != k_Magic)
    {
        GX_LOG_ERROR("SaveVersionManager: Invalid magic number 0x%08X (expected 0x%08X)",
                     header.magic, k_Magic);
        return false;
    }

    // バージョン範囲チェック（0は無効、currentVersion以下であること）
    if (header.version == 0 || header.version > m_currentVersion)
    {
        GX_LOG_ERROR("SaveVersionManager: Invalid version %u (current: %u)",
                     header.version, m_currentVersion);
        return false;
    }

    return true;
}

// ============================================================================
// マイグレーション
// ============================================================================

bool SaveVersionManager::NeedsMigration(const SaveFormatHeader& header) const
{
    return header.version < m_currentVersion;
}

bool SaveVersionManager::CanMigrate(uint32_t fromVersion) const
{
    if (fromVersion >= m_currentVersion)
        return true; // マイグレーション不要

    // fromVersion -> currentVersion までのパスが存在するか確認
    uint32_t currentVer = fromVersion;

    while (currentVer < m_currentVersion)
    {
        bool found = false;
        for (const auto& step : m_migrations)
        {
            if (step.fromVersion == currentVer)
            {
                currentVer = step.toVersion;
                found = true;
                break;
            }
        }

        if (!found)
            return false; // パスが途切れている
    }

    return true;
}

bool SaveVersionManager::Migrate(std::vector<uint8_t>& data)
{
    // ヘッダー読み取り
    SaveFormatHeader header;
    if (!ReadHeader(data, header))
    {
        GX_LOG_ERROR("SaveVersionManager: Failed to read header for migration");
        return false;
    }

    if (!ValidateHeader(header))
        return false;

    if (!NeedsMigration(header))
        return true; // マイグレーション不要

    // ペイロード部分を抽出
    std::vector<uint8_t> payload(data.begin() + k_HeaderSize, data.end());

    uint32_t currentVer = header.version;

    while (currentVer < m_currentVersion)
    {
        bool found = false;
        for (const auto& step : m_migrations)
        {
            if (step.fromVersion == currentVer)
            {
                GX_LOG_INFO("SaveVersionManager: Migrating v%u -> v%u: %s",
                            step.fromVersion, step.toVersion, step.description.c_str());

                if (!step.migrationFn(payload))
                {
                    GX_LOG_ERROR("SaveVersionManager: Migration v%u -> v%u failed",
                                 step.fromVersion, step.toVersion);
                    return false;
                }

                currentVer = step.toVersion;
                found = true;
                break;
            }
        }

        if (!found)
        {
            GX_LOG_ERROR("SaveVersionManager: No migration path from v%u to v%u",
                         currentVer, m_currentVersion);
            return false;
        }
    }

    // 新しいヘッダーでデータを再構築
    data = payload;
    WriteHeader(data, m_currentVersion, header.flags);

    return true;
}

// ============================================================================
// クリア
// ============================================================================

void SaveVersionManager::Clear()
{
    m_migrations.clear();
}

} // namespace gx
