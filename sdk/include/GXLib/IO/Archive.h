#pragma once
/// @file Archive.h
/// @brief カスタムアーカイブ形式 (.gxarc) — AES-256暗号化 + LZ4圧縮
///
/// ゲーム用アセットを1つの .gxarc ファイルにパッキングし、
/// オプションでAES-256-CBC暗号化とLZ4圧縮をサポートする。
/// ArchiveWriter でパック、Archive で読み込み、ArchiveFileProvider でVFSマウント可能。

#include "IO/FileSystem.h"

namespace gx {
/// @addtogroup grp_io
/// @{

/// @brief アーカイブ内のファイルエントリ情報
struct ArchiveEntry {
    gx::String path;           ///< アーカイブ内パス
    uint64_t offset;            ///< データ領域内のオフセット
    uint32_t compressedSize;    ///< 圧縮後のサイズ (非圧縮時はoriginalSizeと同じ)
    uint32_t originalSize;      ///< 元のサイズ
    uint8_t flags;              ///< フラグ (bit0: 圧縮済み)
};

/// @brief アーカイブリーダー
///
/// .gxarc ファイルを開き、格納されたファイルを読み込む。
/// パスワード付きアーカイブの復号もサポートする。
class Archive
{
public:
    /// @brief アーカイブファイルを開く
    /// @param filePath .gxarc ファイルパス
    /// @param password 復号パスワード (暗号化されていない場合は空文字)
    /// @return 成功した場合true
    bool Open(const gx::String& filePath, const gx::String& password = "");

    /// @brief アーカイブを閉じる
    void Close();

    /// @brief 指定パスのファイルがアーカイブ内に存在するか判定する
    /// @param path アーカイブ内パス
    /// @return 存在すればtrue
    bool Contains(const gx::String& path) const;

    /// @brief アーカイブからファイルを読み込む
    /// @param path アーカイブ内パス
    /// @return ファイルデータ (失敗時はIsValid()==false)
    FileData Read(const gx::String& path) const;

    /// @brief 全エントリ一覧を取得する
    /// @return エントリの配列への参照
    const gx::Vector<ArchiveEntry>& GetEntries() const { return m_entries; }

private:
    gx::String m_filePath;
    gx::Vector<ArchiveEntry> m_entries;
    gx::Array<uint8_t, 32> m_key{};
    bool m_encrypted = false;
    uint64_t m_dataOffset = 0;
};

/// @brief アーカイブライター (パックツール)
///
/// 複数ファイルを .gxarc 形式にパッキングする。
/// オプションでAES-256暗号化とLZ4圧縮を適用できる。
class ArchiveWriter
{
public:
    /// @brief 暗号化パスワードを設定する (空文字で暗号化無効)
    /// @param password パスワード
    void SetPassword(const gx::String& password);

    /// @brief LZ4圧縮の有効/無効を設定する
    /// @param enable trueで圧縮有効 (デフォルト: true)
    void SetCompression(bool enable);

    /// @brief ディスク上のファイルをアーカイブに追加する
    /// @param archivePath アーカイブ内でのパス
    /// @param diskPath ディスク上のファイルパス
    void AddFile(const gx::String& archivePath, const gx::String& diskPath);

    /// @brief メモリ上のデータをアーカイブに追加する
    /// @param archivePath アーカイブ内でのパス
    /// @param data データポインタ
    /// @param size データサイズ (バイト)
    void AddFile(const gx::String& archivePath, const void* data, size_t size);

    /// @brief アーカイブを保存する
    /// @param outputPath 出力ファイルパス
    /// @return 成功した場合true
    bool Save(const gx::String& outputPath);

private:
    struct PendingFile {
        gx::String archivePath;
        gx::Vector<uint8_t> data;
    };
    gx::Vector<PendingFile> m_files;
    gx::String m_password;
    bool m_compress = true;
};

/// @}
} // namespace gx
