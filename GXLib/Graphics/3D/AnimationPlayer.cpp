/// @file AnimationPlayer.cpp
/// @brief アニメーションプレイヤーの実装
#include "pch.h"
#include "Graphics/3D/AnimationPlayer.h"

namespace GX
{

void AnimationPlayer::Play(const AnimationClip* clip, bool loop)
{
    m_currentClip = clip;
    m_playing = true;
    m_paused  = false;
    m_loop    = loop;
    m_currentTime = 0.0f;
}

void AnimationPlayer::Update(float deltaTime)
{
    if (!m_playing || m_paused || !m_currentClip || !m_skeleton)
        return;

    m_currentTime += deltaTime * m_speed;

    float duration = m_currentClip->GetDuration();
    if (duration <= 0.0f)
        return;

    if (m_loop)
    {
        m_currentTime = fmodf(m_currentTime, duration);
        if (m_currentTime < 0.0f)
            m_currentTime += duration;
    }
    else if (m_currentTime >= duration)
    {
        m_currentTime = duration;
        m_playing = false;
    }

    uint32_t jointCount = m_skeleton->GetJointCount();
    if (jointCount == 0)
        return;

    // アニメーションサンプリング
    std::vector<XMFLOAT4X4> localTransforms(jointCount);

    // デフォルトローカルトランスフォームで初期化
    for (uint32_t i = 0; i < jointCount; ++i)
        localTransforms[i] = m_skeleton->GetJoints()[i].localTransform;

    // アニメーションを上書き
    m_currentClip->Sample(m_currentTime, jointCount, localTransforms.data());

    // グローバルトランスフォーム計算
    std::vector<XMFLOAT4X4> globalTransforms(jointCount);
    m_skeleton->ComputeGlobalTransforms(localTransforms.data(), globalTransforms.data());

    // ボーン行列計算
    uint32_t boneCount = (std::min)(jointCount, BoneConstants::k_MaxBones);
    m_skeleton->ComputeBoneMatrices(globalTransforms.data(), m_boneConstants.boneMatrices);
}

} // namespace GX
