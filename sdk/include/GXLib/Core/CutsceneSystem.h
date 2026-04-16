#pragma once
/// @file CutsceneSystem.h
/// @brief カットシーンシステム — 時刻ベースで移動・フェード・台詞を順番に再生する
///
/// CutsceneAction を時系列に並べてカットシーンを定義し、
/// Play/Pause/Stop で制御する。ゲームのストーリーシーンや
/// ボス登場演出など、決まった段取りの演出に使う。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include "Math/Vector3.h"
#include <functional>
#include <vector>
#include <string>

namespace gx
{

/// @brief カットシーンアクションの種類
enum class CutsceneActionType
{
    MoveTo,         ///< 指定時間でターゲットを位置に移動
    RotateTo,       ///< 指定時間でターゲットを回転
    FadeTo,         ///< 指定時間で画面/オブジェクトのアルファをフェード
    Wait,           ///< 指定時間待機
    PlaySound,      ///< 効果音を再生
    ShowDialogue,   ///< 台詞を表示
    Custom,         ///< カスタムコールバックを実行
};

/// @brief カットシーンタイムラインの1つのアクション
struct CutsceneAction
{
    CutsceneActionType type = CutsceneActionType::Wait; ///< アクション種別
    float startTime = 0.0f;    ///< アクション開始時刻（カットシーン開始からの秒数）
    float duration = 1.0f;     ///< アクションの所要時間

    // 移動/回転
    Vector3 targetPosition = { 0, 0, 0 }; ///< 移動先の位置
    Vector3 targetRotation = { 0, 0, 0 };  ///< 回転先のオイラー角（度）

    // フェード
    float targetAlpha = 0.0f;  ///< フェード先のアルファ値（0.0=透明、1.0=不透明）

    // 効果音再生
    gx::String soundName;     ///< 再生する効果音の名前

    // 台詞表示
    gx::String dialogueText;  ///< 台詞テキスト
    gx::String speakerName;   ///< 話者名

    // カスタムコールバック
    std::function<void()> callback; ///< カスタムアクション実行時に呼ばれるコールバック

    // ランタイム状態
    bool started = false;      ///< このアクションが開始されたか
    bool completed = false;    ///< このアクションが完了したか
};

/// @brief カットシーンの再生状態
enum class CutsceneState
{
    Stopped,  ///< 停止中
    Playing,  ///< 再生中
    Paused,   ///< 一時停止中
};

/// @brief カットシーンイベントコールバック
struct CutsceneCallbacks
{
    std::function<void(const Vector3& pos, float progress)> onMoveTo;           ///< 移動アクション時のコールバック
    std::function<void(const Vector3& rot, float progress)> onRotateTo;         ///< 回転アクション時のコールバック
    std::function<void(float alpha)> onFadeTo;                                    ///< フェードアクション時のコールバック
    std::function<void(const gx::String& sound)> onPlaySound;                    ///< 効果音再生時のコールバック
    std::function<void(const gx::String& speaker, const gx::String& text)> onShowDialogue; ///< 台詞表示時のコールバック
};

/// @brief タイムラインベースのカットシーンシステム
class CutsceneSystem
{
public:
    CutsceneSystem() = default;
    ~CutsceneSystem() = default;

    /// @brief カットシーンタイムラインにアクションを追加する
    /// @param action 追加するアクション
    void AddAction(const CutsceneAction& action);

    /// @brief 全アクションをクリアする
    void Clear();

    /// @brief 先頭から再生を開始する
    void Play();

    /// @brief 再生を一時停止する
    void Pause();

    /// @brief 一時停止から再開する
    void Resume();

    /// @brief 停止してリセットする
    void Stop();

    /// @brief カットシーンを更新する（毎フレーム呼び出す）
    /// @param deltaTime 前フレームからの経過秒数
    void Update(float deltaTime);

    /// @brief 現在の再生状態を取得する
    /// @return 再生状態
    CutsceneState GetState() const { return m_state; }

    /// @brief 現在の再生時刻を取得する
    /// @return カットシーン開始からの経過秒数
    float GetCurrentTime() const { return m_currentTime; }

    /// @brief 総再生時間を取得する（最後のアクションの終了時刻）
    /// @return 総再生時間（秒）
    float GetTotalDuration() const;

    /// @brief 進捗を取得する（0.0 - 1.0）
    /// @return 進捗率
    float GetProgress() const;

    /// @brief カットシーンが終了したか確認する
    /// @return 全アクション完了済みならtrue
    bool IsFinished() const;

    /// @brief イベントコールバックを設定する
    /// @param cb コールバック群
    void SetCallbacks(const CutsceneCallbacks& cb) { m_callbacks = cb; }

    /// @brief アクション数を取得する
    /// @return タイムライン上のアクション数
    int GetActionCount() const { return static_cast<int>(m_actions.size()); }

    /// @brief カットシーン終了時のコールバックを設定する
    /// @param cb 終了時に呼ばれるコールバック
    void SetOnFinished(std::function<void()> cb) { m_onFinished = std::move(cb); }

private:
    gx::Vector<CutsceneAction> m_actions;           ///< タイムラインに登録されたアクション群
    CutsceneState m_state = CutsceneState::Stopped;  ///< 現在の再生状態
    float m_currentTime = 0.0f;                       ///< 現在の再生時刻（秒）
    CutsceneCallbacks m_callbacks;                    ///< イベントコールバック
    std::function<void()> m_onFinished;               ///< 終了時コールバック
    bool m_finishedFired = false;                     ///< 終了コールバック発火済みフラグ
};

} // namespace gx
/// @}
