#pragma once
/// @file PakFileProvider.h
/// @brief GXPAK バンドルファイルプロバイダー — .gxpak アーカイブからの読み取り専用アクセス
///
/// FileSystem にマウントして GXPAK バンドル内ファイルをVFS経由で読み込む。
/// 優先度100で物理ファイルより優先される。

#include "IO/FileSystem.h"
#include <pak_loader.h>

namespace GX {

/// @brief GXPAK バンドルファイルプロバイダー (読み取り専用)
///
/// .gxpak バンドルをバックエンドとするIFileProvider実装。
/// 書き込みは常にfalseを返す (読み取り専用)。
class PakFileProvider : public IFileProvider
{
public:
    /// @brief GXPAK バンドルファイルを開く
    /// @param pakPath .gxpak ファイルパス
    /// @return 成功した場合true
    bool Open(const std::string& pakPath);

    /// @brief ファイルの存在を確認する
    /// @param path バンドル内パス
    /// @return 存在すればtrue
    bool Exists(const std::string& path) const override;

    /// @brief ファイルを読み込む（LZ4圧縮エントリは自動伸長される）
    /// @param path バンドル内パス
    /// @return ファイルデータ（失敗時はIsValid()==false）
    FileData Read(const std::string& path) const override;

    /// @brief 書き込みは非サポート (常にfalse)
    bool Write(const std::string& path, const void* data, size_t size) override { return false; }

    /// @brief プロバイダー優先度 (100: 物理ファイルより優先)
    int Priority() const override { return 100; }

private:
    gxloader::PakLoader m_loader;
};

} // namespace GX
