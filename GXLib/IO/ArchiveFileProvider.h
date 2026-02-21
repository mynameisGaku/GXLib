#pragma once
/// @file ArchiveFileProvider.h
/// @brief アーカイブファイルプロバイダー — .gxarc アーカイブからの読み取り専用アクセス
///
/// FileSystem にマウントしてアーカイブ内ファイルをVFS経由で読み込む。
/// 優先度100で物理ファイルより優先される。

#include "IO/FileSystem.h"
#include "IO/Archive.h"

namespace GX {

/// @brief アーカイブファイルプロバイダー (読み取り専用)
///
/// .gxarc アーカイブをバックエンドとするIFileProvider実装。
/// 書き込みは常にfalseを返す (読み取り専用)。
class ArchiveFileProvider : public IFileProvider
{
public:
    /// @brief アーカイブファイルを開く
    /// @param archivePath .gxarc ファイルパス
    /// @param password 復号パスワード (暗号化されていない場合は空文字)
    /// @return 成功した場合true
    bool Open(const std::string& archivePath, const std::string& password = "");

    /// @brief ファイルの存在を確認する
    /// @param path アーカイブ内パス
    /// @return 存在すればtrue
    bool Exists(const std::string& path) const override;

    /// @brief ファイルを読み込む
    /// @param path アーカイブ内パス
    /// @return ファイルデータ（失敗時はIsValid()==false）
    FileData Read(const std::string& path) const override;

    /// @brief 書き込みは非サポート (常にfalse)
    bool Write(const std::string& path, const void* data, size_t size) override { return false; }

    /// @brief プロバイダー優先度 (100: 物理ファイルより優先)
    int Priority() const override { return 100; }

private:
    Archive m_archive;
};

} // namespace GX
