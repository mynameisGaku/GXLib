#pragma once
/// @file FileSystem.h
/// @brief Virtual File System — プロバイダーベースのファイルアクセス抽象化
///
/// マウントポイントにプロバイダーを登録し、パス解決を行うシングルトンVFS。
/// 物理ファイル、暗号化アーカイブなど複数のソースを統一的にアクセスできる。

namespace gx {
/// @addtogroup grp_io
/// @{

/// @brief ファイルデータコンテナ (プロバイダーから返される)
struct FileData
{
    gx::Vector<uint8_t> data;  ///< ファイルの生バイトデータ

    /// @brief データが有効かどうか判定する
    /// @return データが空でなければtrue
    bool IsValid() const { return !data.empty(); }

    /// @brief データへのポインタを取得する
    /// @return バイトデータの先頭ポインタ
    const uint8_t* Data() const { return data.data(); }

    /// @brief データサイズを取得する
    /// @return バイト数
    size_t Size() const { return data.size(); }

    /// @brief データを文字列として取得する
    /// @return UTF-8文字列
    gx::String AsString() const { return gx::String(reinterpret_cast<const char*>(data.data()), data.size()); }
};

/// @brief ファイルプロバイダーの抽象インターフェース
///
/// FileSystem にマウントされ、ファイルの読み書きを提供する。
/// PhysicalFileProvider (ディスク) や ArchiveFileProvider (アーカイブ) などの実装がある。
class IFileProvider
{
public:
    virtual ~IFileProvider() = default;

    /// @brief 指定パスにファイルが存在するか判定する
    /// @param path ファイルパス (マウントポイント相対)
    /// @return 存在すればtrue
    virtual bool Exists(const gx::String& path) const = 0;

    /// @brief ファイルを読み込む
    /// @param path ファイルパス (マウントポイント相対)
    /// @return ファイルデータ (失敗時はIsValid()==false)
    virtual FileData Read(const gx::String& path) const = 0;

    /// @brief ファイルを書き込む
    /// @param path ファイルパス (マウントポイント相対)
    /// @param data 書き込むデータ
    /// @param size データサイズ (バイト)
    /// @return 成功した場合true
    virtual bool Write(const gx::String& path, const void* data, size_t size) = 0;

    /// @brief プロバイダーの優先度を取得する (高い値が優先)
    /// @return 優先度 (デフォルト: 0)
    virtual int Priority() const { return 0; }
};

/// @brief シングルトン仮想ファイルシステム
///
/// マウントポイントにプロバイダーを登録し、優先度順にファイルアクセスを解決する。
/// 同じパスに複数のプロバイダーがマッチする場合、Priority() が高いものが優先される。
class FileSystem
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return FileSystemの参照
    static FileSystem& Instance();

    /// @brief マウントポイントにプロバイダーを登録する
    /// @param mountPoint マウントポイント (例: "assets", "data")
    /// @param provider ファイルプロバイダー
    void Mount(const gx::String& mountPoint, std::shared_ptr<IFileProvider> provider);

    /// @brief マウントポイントの登録を解除する
    /// @param mountPoint 解除するマウントポイント
    void Unmount(const gx::String& mountPoint);

    /// @brief 指定パスにファイルが存在するか判定する
    /// @param path ファイルパス
    /// @return 存在すればtrue
    bool Exists(const gx::String& path) const;

    /// @brief ファイルを読み込む
    /// @param path ファイルパス
    /// @return ファイルデータ (失敗時はIsValid()==false)
    FileData ReadFile(const gx::String& path) const;

    /// @brief ファイルを書き込む
    /// @param path ファイルパス
    /// @param data 書き込むデータ
    /// @param size データサイズ (バイト)
    /// @return 成功した場合true
    bool WriteFile(const gx::String& path, const void* data, size_t size);

    /// @brief 全マウントポイントを解除する
    void Clear();

private:
    FileSystem() = default;

    struct MountEntry {
        gx::String mountPoint;
        std::shared_ptr<IFileProvider> provider;
    };
    gx::Vector<MountEntry> m_mounts;

    static gx::String NormalizePath(const gx::String& path);
};

/// @}
} // namespace gx
