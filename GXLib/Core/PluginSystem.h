#pragma once
/// @file PluginSystem.h
/// @brief プラグインシステム — 動的拡張機能の登録・管理・ライフサイクル
///
/// IPlugin インターフェースを実装したプラグインを RegisterPlugin() で登録し、
/// InitializeAll() / UpdateAll() / ShutdownAll() で一括管理する。
/// 優先度（GetPriority）に基づいたソート順で初期化・更新が行われる。
/// シングルトンではなく通常のクラス。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <functional>

namespace gx
{

/// @brief プラグイン情報
struct PluginInfo
{
    std::string name;        ///< プラグイン名
    std::string version;     ///< バージョン文字列
    std::string author;      ///< 作者名
    std::string description; ///< 説明
};

/// @brief プラグインインターフェース
class IPlugin
{
public:
    virtual ~IPlugin() = default;

    /// @brief プラグイン情報を返す
    virtual PluginInfo GetInfo() const = 0;

    /// @brief 初期化処理（成功時 true を返す）
    virtual bool Initialize() = 0;

    /// @brief シャットダウン処理
    virtual void Shutdown() = 0;

    /// @brief フレーム更新
    virtual void Update(float deltaTime) { (void)deltaTime; }

    /// @brief 実行優先度（低い値が先に実行される）
    virtual int GetPriority() const { return 0; }
};

/// @brief プラグイン管理システム
class PluginSystem
{
public:
    /// @brief システムを初期化する
    void Initialize();

    /// @brief システムをシャットダウンする（全プラグインを解放する）
    void Shutdown();

    /// @brief プラグインを登録する
    /// @return 登録成功時 true（同名プラグインが既にある場合は false）
    bool RegisterPlugin(std::unique_ptr<IPlugin> plugin);

    /// @brief プラグインの登録を解除する
    /// @return 解除成功時 true
    bool UnregisterPlugin(const std::string& name);

    /// @brief 名前でプラグインを取得する
    /// @return プラグインポインタ（見つからない場合は nullptr）
    IPlugin* GetPlugin(const std::string& name) const;

    /// @brief 指定名のプラグインが存在するかどうか
    bool HasPlugin(const std::string& name) const;

    /// @brief 全プラグインを優先度順に初期化する
    void InitializeAll();

    /// @brief 全プラグインをシャットダウンする
    void ShutdownAll();

    /// @brief 全有効プラグインを優先度順に更新する
    void UpdateAll(float deltaTime);

    /// @brief 登録プラグインの情報一覧を返す
    std::vector<PluginInfo> GetLoadedPlugins() const;

    /// @brief 登録プラグイン数を返す
    size_t GetPluginCount() const;

    /// @brief システムが初期化済みかどうか
    bool IsInitialized() const;

    /// @brief プラグインの有効/無効を切り替える
    void SetPluginEnabled(const std::string& name, bool enabled);

    /// @brief プラグインが有効かどうか
    bool IsPluginEnabled(const std::string& name) const;

    /// @brief プラグイン登録時コールバック
    std::function<void(const std::string& name)> OnPluginLoaded;

    /// @brief プラグイン解除時コールバック
    std::function<void(const std::string& name)> OnPluginUnloaded;

private:
    /// @brief プラグイン管理エントリ
    struct PluginEntry
    {
        std::unique_ptr<IPlugin> plugin;
        bool enabled     = true;  ///< 有効フラグ
        bool initialized = false; ///< 初期化済みフラグ
    };

    std::vector<PluginEntry> m_plugins;      ///< 登録プラグイン一覧
    bool m_initialized = false;              ///< システム初期化済みフラグ

    /// @brief プラグインを優先度順にソートする
    void SortByPriority();
};

} // namespace gx
/// @}
