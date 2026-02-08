#pragma once
/// @file AnimationPlayer.h
/// @brief アニメーション再生制御

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Skeleton.h"

namespace GX
{

/// @brief アニメーション再生プレイヤー
class AnimationPlayer
{
public:
    AnimationPlayer() = default;
    ~AnimationPlayer() = default;

    /// スケルトンを設定
    void SetSkeleton(Skeleton* skeleton) { m_skeleton = skeleton; }

    /// アニメーションクリップを再生
    void Play(const AnimationClip* clip, bool loop = true);

    /// 停止
    void Stop() { m_playing = false; }

    /// 一時停止
    void Pause() { m_paused = true; }

    /// 再開
    void Resume() { m_paused = false; }

    /// 再生速度
    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }

    /// 更新（deltaTime秒進める）
    void Update(float deltaTime);

    /// ボーン行列を取得
    const BoneConstants& GetBoneConstants() const { return m_boneConstants; }

    /// 再生中か
    bool IsPlaying() const { return m_playing; }

    /// 現在のアニメーション時刻
    float GetCurrentTime() const { return m_currentTime; }

private:
    Skeleton*            m_skeleton    = nullptr;
    const AnimationClip* m_currentClip = nullptr;
    bool  m_playing = false;
    bool  m_paused  = false;
    bool  m_loop    = true;
    float m_speed   = 1.0f;
    float m_currentTime = 0.0f;

    BoneConstants m_boneConstants;
};

} // namespace GX
