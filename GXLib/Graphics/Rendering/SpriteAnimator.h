#pragma once
/// @file SpriteAnimator.h
/// @brief スプライトアニメーション管理（名前付きクリップ、遷移、クロスフェード、イベント）
///
/// Animation2D がハンドル配列ベースの簡易フレーム送りであるのに対し、
/// SpriteAnimator は名前付きクリップ・自動遷移・クロスフェード・イベントコールバック
/// など、キャラクターアニメーションに必要な高度な制御を提供する。
///
/// 使い方:
///   SpriteAnimator animator;
///   SpriteAnimClip idle;
///   idle.name = "Idle"; idle.startFrame = 0; idle.endFrame = 3; idle.fps = 8.0f;
///   animator.AddClip(idle);
///   animator.Play("Idle");
///   animator.Update(deltaTime);
///   uint32_t frame = animator.GetCurrentFrame();

#include "pch_graphics.h"

namespace gx
{
/// @addtogroup grp_gfx_2d
/// @{

/// @brief スプライトアニメーションイベント
///
/// 特定フレームに到達したときにコールバックを発火する。
/// 足音やエフェクト発生などのタイミング制御に使用する。
struct SpriteAnimEvent
{
    uint32_t frame = 0;                     ///< 発火フレーム番号
    std::string name;                       ///< イベント名（デバッグ/識別用）
    std::function<void()> callback;         ///< コールバック関数
};

/// @brief スプライトアニメーションクリップ
///
/// 1つのアニメーション動作（歩行、攻撃など）に対応するフレーム範囲と再生設定。
struct SpriteAnimClip
{
    std::string name;                       ///< クリップ名（一意識別子）
    uint32_t startFrame = 0;                ///< 開始フレーム番号
    uint32_t endFrame = 0;                  ///< 終了フレーム番号（inclusive）
    float fps = 10.0f;                      ///< 再生フレームレート
    bool loop = true;                       ///< ループ再生するか
    std::vector<SpriteAnimEvent> events;    ///< フレームイベント配列
    float speed = 1.0f;                     ///< 再生速度倍率
};

/// @brief スプライトアニメーション遷移ルール
///
/// あるクリップから別のクリップへの自動遷移を定義する。
/// exitTime に達すると自動的に遷移が開始される。
struct SpriteAnimTransition
{
    std::string fromClip;                   ///< 遷移元クリップ名
    std::string toClip;                     ///< 遷移先クリップ名
    float exitTime = 1.0f;                  ///< 遷移開始の正規化時間（0-1、1=最後まで再生）
    float blendDuration = 0.0f;             ///< クロスフェード時間（秒、0=即切替）
};

/// @brief スプライトアニメーター
///
/// 複数のアニメーションクリップを管理し、再生・遷移・クロスフェード・
/// イベント発火を統合的に制御するクラス。
class SpriteAnimator
{
public:
    SpriteAnimator() = default;
    ~SpriteAnimator() = default;

    /// @brief クリップを追加する
    /// @param clip 追加するクリップ
    void AddClip(const SpriteAnimClip& clip);

    /// @brief クリップを削除する
    /// @param name 削除するクリップ名
    void RemoveClip(const std::string& name);

    /// @brief クリップを取得する
    /// @param name クリップ名
    /// @return クリップへのポインタ。見つからなければ nullptr
    const SpriteAnimClip* GetClip(const std::string& name) const;

    /// @brief 遷移ルールを追加する
    /// @param transition 遷移ルール
    void AddTransition(const SpriteAnimTransition& transition);

    /// @brief 指定クリップの再生を開始する
    /// @param clipName 再生するクリップ名
    void Play(const std::string& clipName);

    /// @brief 再生速度を設定する
    /// @param speed 再生速度倍率（1.0がデフォルト）
    void SetSpeed(float speed) { m_speed = speed; }

    /// @brief 再生速度を取得する
    /// @return 再生速度倍率
    float GetSpeed() const { return m_speed; }

    /// @brief 再生方向を設定する
    /// @param dir 1=順方向、-1=逆方向
    void SetDirection(int dir) { m_direction = (dir >= 0) ? 1 : -1; }

    /// @brief 再生方向を取得する
    /// @return 1=順方向、-1=逆方向
    int GetDirection() const { return m_direction; }

    /// @brief アニメーションを更新する（毎フレーム呼ぶ）
    /// @param deltaTime 前フレームからの経過時間（秒）
    void Update(float deltaTime);

    /// @brief 現在のフレーム番号を取得する
    /// @return 現在のフレーム番号
    uint32_t GetCurrentFrame() const { return m_currentFrame; }

    /// @brief 現在再生中のクリップ名を取得する
    /// @return クリップ名への参照
    const std::string& GetCurrentClipName() const { return m_currentClipName; }

    /// @brief クロスフェードのブレンド進捗を取得する
    /// @return 0-1（0=遷移開始、1=遷移完了）
    float GetBlendAlpha() const { return m_blendAlpha; }

    /// @brief クロスフェード中の前クリップのフレーム番号を取得する
    /// @return 前クリップのフレーム番号
    uint32_t GetPreviousFrame() const { return m_previousFrame; }

    /// @brief 再生中かどうかを取得する
    /// @return 再生中なら true
    bool IsPlaying() const { return m_playing; }

    /// @brief 遷移中（クロスフェード中）かどうかを取得する
    /// @return 遷移中なら true
    bool IsTransitioning() const { return m_transitioning; }

private:
    /// @brief クリップのフレーム番号を計算する
    /// @param clip クリップ
    /// @param timer フレームタイマー値
    /// @return 計算されたフレーム番号
    uint32_t ComputeFrame(const SpriteAnimClip& clip, float timer) const;

    /// @brief 正規化されたクリップ進捗を計算する
    /// @param clip クリップ
    /// @param timer フレームタイマー値
    /// @return 0-1の正規化進捗
    float ComputeNormalizedTime(const SpriteAnimClip& clip, float timer) const;

    std::unordered_map<std::string, SpriteAnimClip> m_clips;    ///< クリップマップ
    std::vector<SpriteAnimTransition> m_transitions;             ///< 遷移ルール配列

    std::string m_currentClipName;      ///< 現在再生中のクリップ名
    std::string m_previousClipName;     ///< クロスフェード中の前クリップ名

    float m_frameTimer = 0.0f;          ///< 現在クリップのフレームタイマー
    float m_prevFrameTimer = 0.0f;      ///< 前クリップのフレームタイマー
    float m_blendTimer = 0.0f;          ///< クロスフェードタイマー
    float m_blendDuration = 0.0f;       ///< クロスフェード時間
    float m_blendAlpha = 0.0f;          ///< クロスフェード進捗（0-1）

    uint32_t m_currentFrame = 0;        ///< 現在のフレーム番号
    uint32_t m_previousFrame = 0;       ///< 前クリップのフレーム番号

    float m_speed = 1.0f;               ///< 再生速度倍率
    int   m_direction = 1;              ///< 再生方向（1=順, -1=逆）

    bool m_playing = false;             ///< 再生中フラグ
    bool m_transitioning = false;       ///< 遷移中フラグ
};

/// @}
} // namespace gx
