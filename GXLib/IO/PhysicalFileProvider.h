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
    /// @param path ファイルパス（ルートディレクトリ相対）
    /// @return 存在すればtrue
    bool Exists(const std::string& path) const override;

    /// @brief ファイルを読み込む
    /// @param path ファイルパス（ルートディレクトリ相対）
    /// @return ファイルデータ（失敗時はIsValid()==false）
    FileData Read(const std::string& path) const override;

    /// @brief ファイルを書き込む
    /// @param path ファイルパス（ルートディレクトリ相対）
    /// @param data 書き込むデータ
    /// @param size データサイズ（バイト）
    /// @return 成功した場合true
    bool Write(const std::string& path, const void* data, size_t size) override;

private:
    std::string m_rootDir;
    std::string ResolvePath(const std::string& path) const;
};

} // namespace GX
