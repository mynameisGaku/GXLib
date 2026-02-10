#pragma once
/// @file PhysicalFileProvider.h
/// @brief ディスクファイルプロバイダー — 物理ファイルシステムからの読み書き

#include "IO/FileSystem.h"

namespace GX {

/// @brief ディスクファイルプロバイダー
///
/// 指定されたルートディレクトリ以下のファイルに対してアクセスを提供する。
/// FileSystem にマウントして使用する。
class PhysicalFileProvider : public IFileProvider
{
public:
    /// @brief ルートディレクトリを指定して作成する
    /// @param rootDir ファイルアクセスのルートディレクトリ
    explicit PhysicalFileProvider(const std::string& rootDir);

    /// @brief ファイルの存在を確認する
    bool Exists(const std::string& path) const override;

    /// @brief ファイルを読み込む
    FileData Read(const std::string& path) const override;

    /// @brief ファイルを書き込む
    bool Write(const std::string& path, const void* data, size_t size) override;

private:
    std::string m_rootDir;
    std::string ResolvePath(const std::string& path) const;
};

} // namespace GX
