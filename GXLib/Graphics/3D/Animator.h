#pragma once
/// @file Animator.h
/// @brief アニメーションのブレンド制御器
/// 初学者向け: 複数のアニメを混ぜたり切り替えたりする役割です。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Skeleton.h"

namespace GX
{

/// @brief シンプルなクロスフェード対応アニメーター
/// 初学者向け: 1つのアニメから別のアニメへ、時間をかけて滑らかに切り替えます。
class Animator
{
public:
    Animator() = default;
    ~Animator() = default;

    void SetSkeleton(Skeleton* skeleton);

    void Play(const AnimationClip* clip, bool loop = true, float speed = 1.0f);
    void CrossFade(const AnimationClip* clip, float duration, bool loop = true, float speed = 1.0f);

    void Stop() { m_playing = false; }
    void Pause() { m_paused = true; }
    void Resume() { m_paused = false; }

    /// @brief バインドポーズをボーン行列に評価する（再生前の初期化に便利）
    /// 初学者向け: まだアニメを再生していない時に、正しい「立ち姿」を作ります。
    void EvaluateBindPose();

    void Update(float deltaTime);

    const BoneConstants& GetBoneConstants() const { return m_boneConstants; }
    const std::vector<XMFLOAT4X4>& GetGlobalTransforms() const { return m_globalTransforms; }
    const std::vector<TransformTRS>& GetLocalPose() const { return m_localPose; }

    bool IsPlaying() const { return m_playing; }

private:
    struct ClipState
    {
        const AnimationClip* clip = nullptr;
        float time = 0.0f;
        float speed = 1.0f;
        bool  loop = true;
    };

    void EnsurePoseStorage();
    void AdvanceClip(ClipState& state, float deltaTime);
    void SampleClip(const ClipState& state, std::vector<TransformTRS>& outPose);
    void BlendPoses(const std::vector<TransformTRS>& a,
                    const std::vector<TransformTRS>& b,
                    float t,
                    std::vector<TransformTRS>& outPose);

    Skeleton* m_skeleton = nullptr;

    ClipState m_current;
    ClipState m_next;
    bool      m_playing = false;
    bool      m_paused  = false;

    float m_fadeDuration = 0.0f;
    float m_fadeTime = 0.0f;
    bool  m_fading = false;

    std::vector<TransformTRS> m_bindPose;
    std::vector<TransformTRS> m_poseA;
    std::vector<TransformTRS> m_poseB;
    std::vector<TransformTRS> m_localPose;
    std::vector<XMFLOAT4X4>   m_localTransforms;
    std::vector<XMFLOAT4X4>   m_globalTransforms;
    BoneConstants             m_boneConstants;
};

} // namespace GX
