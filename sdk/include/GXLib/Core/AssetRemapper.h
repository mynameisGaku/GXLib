#pragma once
/// @file AssetRemapper.h
/// @brief ホットリロードRemapper — GUID→スロット対応、差し替え実行
///
/// FileWatcher 検知 → HotReloadManager(デバウンス) → AssetRemapper::OnFileChanged(path)
///   → path から GUID 逆引き → reloadFunc(path, slotIndex) → 同一スロットに差し替え
/// @addtogroup grp_core
/// @{

#include "pch_common.h"
#include "Core/GUID.h"

namespace gx
{

/// @brief リロード関数型
/// @param path リロード対象のファイルパス
/// @param slotIndex リソースのスロットインデックス
/// @return リロード成功で true
using ReloadFunc = std::function<bool(const gx::String& path, uint32_t slotIndex)>;

/// @brief Remapper エントリ（内部管理用）
struct RemapEntry
{
    GUID guid;              ///< アセットGUID
    gx::String path;       ///< ファイルパス
    uint32_t slotIndex = 0; ///< リソーススロット
    ReloadFunc reloadFunc;  ///< リロード関数
};

/// @brief リロード統計情報
struct RemapStats
{
    uint32_t totalReloads = 0;   ///< リロード試行回数
    uint32_t successCount = 0;   ///< リロード成功回数
    uint32_t failureCount = 0;   ///< リロード失敗回数
};

/// @brief ホットリロード Remapper (シングルトン)
///
/// GUID ベースでリソースを登録し、ファイル変更時に同一スロットで差し替える。
class AssetRemapper
{
public:
    /// @brief シングルトンインスタンスを取得する
    /// @return AssetRemapper のシングルトン参照
    static AssetRemapper& Instance();

    /// @brief リソースを登録する
    /// @param guid アセットGUID
    /// @param path ファイルパス
    /// @param slotIndex リソーススロットインデックス
    /// @param reloadFunc リロード実行関数
    void Register(const GUID& guid, const gx::String& path,
                  uint32_t slotIndex, ReloadFunc reloadFunc);

    /// @brief 登録を解除する
    /// @param guid 解除するアセットのGUID
    void Unregister(const GUID& guid);

    /// @brief ファイル変更通知を処理する
    ///
    /// パスからGUID逆引きし、登録済みならreloadFuncを呼び出す。
    /// @param path 変更されたファイルパス
    /// @return リロードが実行された場合 true
    bool OnFileChanged(const gx::String& path);

    /// @brief GUIDに対応するスロットインデックスを取得する
    /// @param guid アセットGUID
    /// @return スロットインデックス（未登録の場合は UINT32_MAX）
    uint32_t GetSlot(const GUID& guid) const;

    /// @brief GUIDに対応するパスを取得する
    /// @param guid アセットGUID
    /// @return ファイルパス（未登録の場合は空文字列）
    gx::String GetPath(const GUID& guid) const;

    /// @brief 登録数を取得する
    /// @return 登録済みエントリ数
    uint32_t GetRegisteredCount() const { return static_cast<uint32_t>(m_entries.size()); }

    /// @brief リロード統計を取得する
    /// @return 統計情報
    const RemapStats& GetStats() const { return m_stats; }

    /// @brief 統計をリセットする
    void ResetStats() { m_stats = RemapStats{}; }

    /// @brief 全登録をクリアする
    void Clear();

private:
    AssetRemapper() = default;
    ~AssetRemapper() = default;
    AssetRemapper(const AssetRemapper&) = delete;
    AssetRemapper& operator=(const AssetRemapper&) = delete;

    gx::HashMap<GUID, RemapEntry> m_entries;        ///< GUID → エントリ
    gx::HashMap<gx::String, GUID> m_pathToGuid;     ///< パス → GUID 逆引き
    RemapStats m_stats;                               ///< リロード統計
};

} // namespace gx
/// @}
