/// @file AnimationPlayer.cpp
/// @brief アニメーションプレイヤーの実装
#include "pch.h"
#include "Graphics/3D/AnimationPlayer.h"

namespace GX
{

void AnimationPlayer::SetSkeleton(Skeleton* skeleton)
{
    m_skeleton = skeleton;
    EnsurePoseStorage();

    if (!m_skeleton)
        return;

    const auto& joints = m_skeleton->GetJoints();
    const uint32_t jointCount = static_cast<uint32_t>(joints.size());
    m_bindPose.resize(jointCount);
    for (uint32_t i = 0; i < jointCount; ++i)
        m_bindPose[i] = DecomposeTRS(joints[i].localTransform);
}

void AnimationPlayer::EnsurePoseStorage()
{
    if (!m_skeleton)
        return;

    uint32_t jointCount = m_skeleton->GetJointCount();
    m_localPose.resize(jointCount);
    m_localTransforms.resize(jointCount);
    m_globalTransforms.resize(jointCount);
}

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

    EnsurePoseStorage();
    uint32_t jointCount = m_skeleton->GetJointCount();
    if (jointCount == 0)
        return;

    // バインドポーズをベースにクリップをサンプル（キーの無い関節はバインドポーズを維持）
    if (!m_bindPose.empty())
        m_currentClip->SampleTRS(m_currentTime, jointCount, m_localPose.data(), m_bindPose.data());
    else
        m_currentClip->SampleTRS(m_currentTime, jointCount, m_localPose.data(), nullptr);

    // ローカル変換行列を作成
    for (uint32_t i = 0; i < jointCount; ++i)
    {
        XMMATRIX S = XMMatrixScaling(m_localPose[i].scale.x, m_localPose[i].scale.y, m_localPose[i].scale.z);
        XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&m_localPose[i].rotation));
        XMMATRIX T = XMMatrixTranslation(m_localPose[i].translation.x, m_localPose[i].translation.y, m_localPose[i].translation.z);
        XMStoreFloat4x4(&m_localTransforms[i], S * R * T);
    }

    // グローバル変換に変換
    m_skeleton->ComputeGlobalTransforms(m_localTransforms.data(), m_globalTransforms.data());

    // ボーン行列を作成（グローバル変換 × 逆バインドポーズ → スキニング用行列）
    uint32_t boneCount = (std::min)(jointCount, BoneConstants::k_MaxBones);
    m_skeleton->ComputeBoneMatrices(m_globalTransforms.data(), m_boneConstants.boneMatrices);
}

} // namespace GX
