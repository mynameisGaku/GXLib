#pragma once
/// @file AnimationPlayer.h
/// @brief 単一クリップ再生用のアニメーションプレイヤー（姿勢キャッシュ付き）
/// 初学者向け: 1つのアニメーションだけを再生する、シンプルな再生器です。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Skeleton.h"

namespace GX
{

/// @brief シンプルなアニメーションプレイヤー（1クリップ）
class AnimationPlayer
{
public:
    AnimationPlayer() = default;
    ~AnimationPlayer() = default;

    void SetSkeleton(Skeleton* skeleton);

    void Play(const AnimationClip* clip, bool loop = true);
    void Stop() { m_playing = false; }
    void Pause() { m_paused = true; }
    void Resume() { m_paused = false; }

    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }

    void Update(float deltaTime);

    const BoneConstants& GetBoneConstants() const { return m_boneConstants; }
    const std::vector<XMFLOAT4X4>& GetGlobalTransforms() const { return m_globalTransforms; }
    const std::vector<TransformTRS>& GetLocalPose() const { return m_localPose; }

    bool IsPlaying() const { return m_playing; }
    float GetCurrentTime() const { return m_currentTime; }

private:
    void EnsurePoseStorage();

    Skeleton*            m_skeleton    = nullptr;
    const AnimationClip* m_currentClip = nullptr;
    bool  m_playing = false;
    bool  m_paused  = false;
    bool  m_loop    = true;
    float m_speed   = 1.0f;
    float m_currentTime = 0.0f;

    std::vector<TransformTRS> m_bindPose;
    std::vector<TransformTRS> m_localPose;
    std::vector<XMFLOAT4X4>   m_localTransforms;
    std::vector<XMFLOAT4X4>   m_globalTransforms;
    BoneConstants             m_boneConstants;
};

} // namespace GX
