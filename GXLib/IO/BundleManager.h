#pragma once
/// @file BundleManager.h
/// @brief バンドル依存解決マネージャー
///
/// gxpak バンドルの登録、GUID→バンドル+内部パス解決、依存関係追跡を行う。
/// バンドルマニフェスト（含有アセットGUID/パスリスト + 外部依存GUIDリスト）を
/// メモリ上で管理し、GUID参照によるクロスバンドル解決を可能にする。
/// @addtogroup grp_io
/// @{

#include "pch_common.h"
#include "Core/GUID.h"
#include "IO/FileSystem.h"

namespace gx
{

/// @brief GUID解決結果
struct ResolveResult
{
    gx::String bundlePath;   ///< バンドルファイルパス
    gx::String internalPath; ///< バンドル内のアセットパス
    bool found = false;      ///< 解決に成功したか

    /// @brief 解決が成功したか判定する
    explicit operator bool() const { return found; }
};

/// @brief バンドルマニフェスト情報（バンドルごと）
struct BundleManifest
{
    gx::String pakPath; ///< .gxpak ファイルパス

    /// @brief 含有アセット (GUID → 内部パス)
    gx::HashMap<GUID, gx::String> containedAssets;

    /// @brief 外部依存GUID（このバンドルが参照しているが含有していないアセット）
    gx::Vector<GUID> externalDependencies;
};

/// @brief バンドル登録・GUID解決マネージャー (シングルトン)
class BundleManager
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return BundleManager のシングルトン参照
    static BundleManager& Instance();

    /// @brief バンドルを読み込み、マニフェスト情報を登録する
    /// @param pakPath .gxpak ファイルパス
    /// @return 成功で true
    bool LoadBundle(const gx::String& pakPath);

    /// @brief バンドルをアンロードし、マニフェスト情報を削除する
    /// @param pakPath .gxpak ファイルパス
    void UnloadBundle(const gx::String& pakPath);

    /// @brief マニフェスト情報を直接登録する（テスト用・ファイルなしでの登録）
    /// @param manifest バンドルマニフェスト
    void RegisterBundleInfo(const BundleManifest& manifest);

    /// @brief GUID からバンドル+内部パスを解決する
    /// @param guid 検索するアセットGUID
    /// @return 解決結果
    ResolveResult Resolve(const GUID& guid) const;

    /// @brief バンドルの全依存が満たされているか判定する
    /// @param pakPath .gxpak ファイルパス
    /// @return 全依存が解決済みなら true
    bool AreDependenciesMet(const gx::String& pakPath) const;

    /// @brief バンドルの未解決依存GUIDリストを取得する
    /// @param pakPath .gxpak ファイルパス
    /// @return 未解決のGUIDリスト
    gx::Vector<GUID> GetMissingDependencies(const gx::String& pakPath) const;

    /// @brief GUIDに対応するデータを読み込む
    /// @param guid 読み込むアセットGUID
    /// @return ファイルデータ（失敗時はIsValid()==false）
    FileData ReadByGUID(const GUID& guid) const;

    /// @brief ロード済みバンドル数を取得する
    /// @return バンドル数
    uint32_t GetBundleCount() const { return static_cast<uint32_t>(m_bundles.size()); }

    /// @brief 登録済み全GUIDの数を取得する
    /// @return GUID数
    uint32_t GetTotalAssetCount() const { return static_cast<uint32_t>(m_guidToBundle.size()); }

    /// @brief 全バンドル情報をクリアする
    void Clear();

private:
    BundleManager() = default;
    ~BundleManager() = default;
    BundleManager(const BundleManager&) = delete;
    BundleManager& operator=(const BundleManager&) = delete;

    /// @brief GUID → バンドルパスの逆引きマップ
    gx::HashMap<GUID, gx::String> m_guidToBundle;

    /// @brief パス → マニフェストのマップ
    gx::HashMap<gx::String, BundleManifest> m_bundles;
};

} // namespace gx
/// @}
