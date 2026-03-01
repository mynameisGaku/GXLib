#pragma once
/// @file SaveVersion.h
/// @brief セーブフォーマットバージョニング（ヘッダー管理、マイグレーション、チェックサム検証）
///
/// セーブデータにヘッダーを付与し、フォーマットバージョンの追跡と
/// 古いバージョンからのマイグレーションパスを提供する。
/// CRC32によるチェックサム検証でデータ整合性を保証する。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <functional>

namespace gx
{

/// @brief セーブフォーマットヘッダー
struct SaveFormatHeader
{
    uint32_t magic        = 0;   ///< マジックナンバー (0x47585356 = "GXSV")
    uint32_t version      = 0;   ///< フォーマットバージョン
    uint32_t flags        = 0;   ///< フラグビットフィールド
    uint32_t checksum     = 0;   ///< データペイロードのCRC32
    uint64_t timestamp    = 0;   ///< タイムスタンプ（Unix time）
    uint32_t metadataSize = 0;   ///< メタデータサイズ（バイト）
    uint32_t dataSize     = 0;   ///< データサイズ（バイト）
};

/// @brief マイグレーションステップ
struct MigrationStep
{
    uint32_t    fromVersion;   ///< 移行元バージョン
    uint32_t    toVersion;     ///< 移行先バージョン
    std::string description;   ///< 説明
    std::function<bool(std::vector<uint8_t>&)> migrationFn; ///< マイグレーション関数
};

/// @brief セーブバージョンマネージャー
///
/// セーブデータのヘッダー管理、バージョン追跡、マイグレーション適用、
/// CRC32チェックサム検証を提供する。
class SaveVersionManager
{
public:
    /// @brief 現在のフォーマットバージョン
    static constexpr uint32_t k_DefaultVersion = 1;

    /// @brief マジックナンバー ("GXSV")
    static constexpr uint32_t k_Magic = 0x47585356;

    /// @brief ヘッダーサイズ（バイト）
    static constexpr size_t k_HeaderSize = sizeof(SaveFormatHeader);

    SaveVersionManager() = default;
    ~SaveVersionManager() = default;

    /// @brief マイグレーションを登録する
    /// @param from 移行元バージョン
    /// @param to 移行先バージョン
    /// @param description 説明
    /// @param fn マイグレーション関数
    void RegisterMigration(uint32_t from, uint32_t to, const std::string& description,
                           std::function<bool(std::vector<uint8_t>&)> fn);

    /// @brief 登録済みマイグレーション数を取得する
    /// @return マイグレーション数
    uint32_t GetMigrationCount() const { return static_cast<uint32_t>(m_migrations.size()); }

    /// @brief データにヘッダーを書き込む（先頭にプリペンド）
    /// @param data データバッファ（先頭にヘッダーが追加される）
    /// @param version バージョン番号（0でcurrentVersion）
    /// @param flags フラグ
    void WriteHeader(std::vector<uint8_t>& data, uint32_t version = 0, uint32_t flags = 0);

    /// @brief データからヘッダーを読み取る
    /// @param data データバッファ
    /// @param outHeader 読み取ったヘッダーの出力先
    /// @return 成功でtrue
    bool ReadHeader(const std::vector<uint8_t>& data, SaveFormatHeader& outHeader) const;

    /// @brief ヘッダーの妥当性を検証する
    /// @param header ヘッダー
    /// @return 有効でtrue
    bool ValidateHeader(const SaveFormatHeader& header) const;

    /// @brief マイグレーションが必要か判定する
    /// @param header ヘッダー
    /// @return マイグレーションが必要ならtrue
    bool NeedsMigration(const SaveFormatHeader& header) const;

    /// @brief マイグレーションを適用する
    /// @param data データバッファ（ヘッダー含む）
    /// @return 成功でtrue
    bool Migrate(std::vector<uint8_t>& data);

    /// @brief 指定バージョンからのマイグレーションが可能か判定する
    /// @param fromVersion 移行元バージョン
    /// @return 可能でtrue
    bool CanMigrate(uint32_t fromVersion) const;

    /// @brief CRC32チェックサムを計算する
    /// @param data データポインタ
    /// @param size データサイズ
    /// @return CRC32値
    static uint32_t ComputeChecksum(const uint8_t* data, size_t size);

    /// @brief ヘッダーのチェックサムを検証する
    /// @param header ヘッダー
    /// @param data データバッファ（ヘッダー後のペイロード）
    /// @return チェックサムが一致すればtrue
    bool VerifyChecksum(const SaveFormatHeader& header, const std::vector<uint8_t>& data) const;

    /// @brief 現在のバージョンを設定する（テスト用）
    /// @param version バージョン番号
    void SetCurrentVersion(uint32_t version) { m_currentVersion = version; }

    /// @brief 現在のバージョンを取得する
    /// @return 現在のバージョン
    uint32_t GetCurrentVersion() const { return m_currentVersion; }

    /// @brief 全マイグレーションをクリアする
    void Clear();

private:
    uint32_t m_currentVersion = k_DefaultVersion; ///< 現在のフォーマットバージョン
    std::vector<MigrationStep> m_migrations;      ///< 登録済みマイグレーションステップ

    /// @brief CRC32テーブルを取得する
    static const uint32_t* GetCRC32Table();
};

} // namespace gx
/// @}
