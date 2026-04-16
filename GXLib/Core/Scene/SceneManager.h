#pragma once
/// @file SceneManager.h
/// @brief シーン遷移・スタック管理
///
/// タイトル→ゲーム→リザルトのような画面切り替えを、
/// フェードイン/アウト付きで行う。PushScene/PopScene で
/// ポーズ画面のようなスタック型の重ね合わせにも対応。
/// @addtogroup grp_scene/// @{

#include "pch_common.h"
#include "Core/Scene/Scene.h"

namespace gx
{

/// @brief シーン遷移の状態
enum class SceneTransitionState
{
    None,      ///< 遷移なし
    FadeOut,   ///< フェードアウト中
    Loading,   ///< 新シーン構築中
    FadeIn,    ///< フェードイン中
};

/// @brief シーン遷移の設定
struct SceneTransitionDesc
{
    float fadeOutDuration = 0.3f;  ///< フェードアウト時間（秒）
    float fadeInDuration  = 0.3f;  ///< フェードイン時間（秒）
};

/// @brief シーンを動的に生成するファクトリ関数型
using SceneFactory = std::function<std::unique_ptr<Scene>()>;

/// @brief シーンマネージャー
///
/// ChangeScene() で遷移、PushScene()/PopScene() でスタック管理。
/// トランジション中は GetTransitionAlpha() でフェードの進行度を取得できる。
class SceneManager
{
public:
    /// @brief シーンを切り替える（現在のスタックを全クリア→新シーン）
    /// @param factory 新シーンを生成するファクトリ関数
    /// @param desc 遷移設定
    void ChangeScene(SceneFactory factory, const SceneTransitionDesc& desc = {});

    /// @brief シーンをスタックに積む
    /// @param factory 新シーンを生成するファクトリ関数
    /// @param desc 遷移設定
    void PushScene(SceneFactory factory, const SceneTransitionDesc& desc = {});

    /// @brief スタックトップのシーンを取り除く
    /// @param desc 遷移設定
    void PopScene(const SceneTransitionDesc& desc = {});

    /// @brief 毎フレーム更新
    /// @param deltaTime デルタタイム（秒）
    void Update(float deltaTime);

    /// @brief 現在のシーンを取得する
    /// @return スタックトップのシーン（空の場合nullptr）
    Scene* GetCurrentScene() const;

    /// @brief 遷移中か判定する
    /// @return 遷移中なら true
    bool IsTransitioning() const { return m_transitionState != SceneTransitionState::None; }

    /// @brief 遷移の透明度を取得する（0.0=透明、1.0=不透明）
    /// @return 遷移アルファ値（0.0〜1.0）
    float GetTransitionAlpha() const { return m_transitionAlpha; }

    /// @brief 遷移状態を取得する
    /// @return 現在の遷移状態
    SceneTransitionState GetTransitionState() const { return m_transitionState; }

    /// @brief スタックの深さを取得する
    /// @return シーンスタックの段数
    size_t GetStackDepth() const { return m_sceneStack.size(); }

private:
    /// @brief 遷移を開始する（内部用）
    /// @param factory 新シーンのファクトリ関数
    /// @param desc 遷移設定
    /// @param clearStack true の場合スタック全体をクリアして置換
    void BeginTransition(SceneFactory factory, const SceneTransitionDesc& desc, bool clearStack);

    /// @brief 遷移を完了する（内部用）
    void FinishTransition();

    /// @brief 遷移完了時に実行するアクション
    enum class PendingAction
    {
        Change, ///< シーン切り替え（スタッククリア→新シーン）
        Push,   ///< シーンをスタックに積む
        Pop,    ///< スタックトップのシーンを取り除く
    };

    gx::Vector<std::unique_ptr<Scene>> m_sceneStack;              ///< シーンスタック（所有）
    SceneTransitionState m_transitionState = SceneTransitionState::None; ///< 遷移状態
    float m_transitionAlpha   = 0.0f;   ///< 遷移の透明度 (0.0~1.0)
    float m_transitionTimer   = 0.0f;   ///< 遷移タイマー（秒）
    float m_fadeOutDuration   = 0.3f;   ///< フェードアウト時間（秒）
    float m_fadeInDuration    = 0.3f;   ///< フェードイン時間（秒）
    SceneFactory m_pendingFactory;      ///< 遷移完了時に使うファクトリ関数
    PendingAction m_pendingAction = PendingAction::Change; ///< 遷移完了時のアクション
};

} // namespace gx
/// @}
