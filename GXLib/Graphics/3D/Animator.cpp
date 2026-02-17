/// @file Animator.cpp
/// @brief アニメーターの実装
#include "pch.h"
#include "Graphics/3D/Animator.h"

namespace GX
{

void Animator::SetSkeleton(Skeleton* skeleton)
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

void Animator::EvaluateBindPose()
{
    if (!m_skeleton)
        return;

    EnsurePoseStorage();
    uint32_t jointCount = m_skeleton->GetJointCount();
    if (jointCount == 0)
        return;

    if (!m_bindPose.empty())
        m_localPose = m_bindPose;
    else
        m_localPose.assign(jointCount, IdentityTRS());

    for (uint32_t i = 0; i < jointCount; ++i)
    {
        XMMATRIX S = XMMatrixScaling(m_localPose[i].scale.x, m_localPose[i].scale.y, m_localPose[i].scale.z);
        XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&m_localPose[i].rotation));
        XMMATRIX T = XMMatrixTranslation(m_localPose[i].translation.x, m_localPose[i].translation.y, m_localPose[i].translation.z);
        XMStoreFloat4x4(&m_localTransforms[i], S * R * T);
    }

    m_skeleton->ComputeGlobalTransforms(m_localTransforms.data(), m_globalTransforms.data());
    m_skeleton->ComputeBoneMatrices(m_globalTransforms.data(), m_boneConstants.boneMatrices);
}

void Animator::Play(const AnimationClip* clip, bool loop, float speed)
{
    m_current.clip = clip;
    m_current.time = 0.0f;
    m_current.loop = loop;
    m_current.speed = speed;
    m_next = {};
    m_fading = false;
    m_playing = (clip != nullptr);
    m_paused = false;
}

void Animator::SetBlendStack(BlendStack* stack)
{
    m_blendStack = stack;
    m_mode = stack ? AnimMode::BlendStack : AnimMode::Simple;
}

void Animator::SetStateMachine(AnimatorStateMachine* sm)
{
    m_stateMachine = sm;
    m_mode = sm ? AnimMode::StateMachine : AnimMode::Simple;
}

void Animator::CrossFade(const AnimationClip* clip, float duration, bool loop, float speed)
{
    if (!clip)
        return;

    if (!m_current.clip)
    {
        Play(clip, loop, speed);
        return;
    }

    m_next.clip = clip;
    m_next.time = 0.0f;
    m_next.loop = loop;
    m_next.speed = speed;
    m_fadeDuration = (std::max)(duration, 0.0001f);
    m_fadeTime = 0.0f;
    m_fading = true;
}

void Animator::EnsurePoseStorage()
{
    if (!m_skeleton)
        return;

    uint32_t jointCount = m_skeleton->GetJointCount();
    m_poseA.resize(jointCount);
    m_poseB.resize(jointCount);
    m_localPose.resize(jointCount);
    m_localTransforms.resize(jointCount);
    m_globalTransforms.resize(jointCount);
}

void Animator::AdvanceClip(ClipState& state, float deltaTime)
{
    if (!state.clip) return;

    state.time += deltaTime * state.speed;
    float duration = state.clip->GetDuration();
    if (duration <= 0.0f) return;

    if (state.loop)
    {
        state.time = fmodf(state.time, duration);
        if (state.time < 0.0f)
            state.time += duration;
    }
    else if (state.time >= duration)
    {
        state.time = duration;
    }
}

void Animator::SampleClip(const ClipState& state, std::vector<TransformTRS>& outPose)
{
    if (!m_skeleton) return;

    uint32_t jointCount = m_skeleton->GetJointCount();
    if (!state.clip)
    {
        if (!m_bindPose.empty())
            outPose = m_bindPose;
        else
            outPose.assign(jointCount, IdentityTRS());
        return;
    }

    if (!m_bindPose.empty())
        state.clip->SampleTRS(state.time, jointCount, outPose.data(), m_bindPose.data());
    else
        state.clip->SampleTRS(state.time, jointCount, outPose.data(), nullptr);
}

void Animator::BlendPoses(const std::vector<TransformTRS>& a,
                          const std::vector<TransformTRS>& b,
                          float t,
                          std::vector<TransformTRS>& outPose)
{
    const uint32_t count = static_cast<uint32_t>(outPose.size());
    for (uint32_t i = 0; i < count; ++i)
    {
        TransformTRS out = outPose[i];
        out.translation = {
            a[i].translation.x + (b[i].translation.x - a[i].translation.x) * t,
            a[i].translation.y + (b[i].translation.y - a[i].translation.y) * t,
            a[i].translation.z + (b[i].translation.z - a[i].translation.z) * t
        };

        XMVECTOR qa = XMLoadFloat4(&a[i].rotation);
        XMVECTOR qb = XMLoadFloat4(&b[i].rotation);
        XMFLOAT4 rot;
        XMStoreFloat4(&rot, XMQuaternionSlerp(qa, qb, t));
        out.rotation = rot;

        out.scale = {
            a[i].scale.x + (b[i].scale.x - a[i].scale.x) * t,
            a[i].scale.y + (b[i].scale.y - a[i].scale.y) * t,
            a[i].scale.z + (b[i].scale.z - a[i].scale.z) * t
        };

        outPose[i] = out;
    }
}

void Animator::Update(float deltaTime)
{
    if (!m_skeleton)
        return;

    // BlendStack/StateMachineモードはm_playingに依存しない
    if (m_mode == AnimMode::Simple && (!m_playing || m_paused))
        return;

    EnsurePoseStorage();
    uint32_t jointCount = m_skeleton->GetJointCount();
    if (jointCount == 0)
        return;

    const TransformTRS* bindPose = m_bindPose.empty() ? nullptr : m_bindPose.data();

    switch (m_mode)
    {
    case AnimMode::BlendStack:
        // BlendStackモード: レイヤースタックがポーズを計算
        // 初学者向け: 複数レイヤーを重ねてブレンドしたポーズを取得します。
        if (m_blendStack)
        {
            m_blendStack->Update(deltaTime, jointCount, bindPose, m_localPose.data());
        }
        break;

    case AnimMode::StateMachine:
        // StateMachineモード: ステートマシンがポーズを計算
        // 初学者向け: 現在の状態に応じたアニメーションポーズを取得します。
        if (m_stateMachine)
        {
            m_stateMachine->Update(deltaTime, jointCount, bindPose, m_localPose.data());
        }
        break;

    case AnimMode::Simple:
    default:
        // Simpleモード: 既存のクロスフェードロジック
        if (m_fading && m_next.clip)
        {
            AdvanceClip(m_current, deltaTime);
            AdvanceClip(m_next, deltaTime);

            SampleClip(m_current, m_poseA);
            SampleClip(m_next, m_poseB);

            m_fadeTime += deltaTime;
            float t = (std::min)(m_fadeTime / m_fadeDuration, 1.0f);
            BlendPoses(m_poseA, m_poseB, t, m_localPose);

            if (t >= 1.0f)
            {
                m_current = m_next;
                m_next = {};
                m_fading = false;
            }
        }
        else
        {
            AdvanceClip(m_current, deltaTime);
            SampleClip(m_current, m_localPose);
        }
        break;
    }

    // ローカル変換行列を作成
    for (uint32_t i = 0; i < jointCount; ++i)
    {
        XMMATRIX S = XMMatrixScaling(m_localPose[i].scale.x, m_localPose[i].scale.y, m_localPose[i].scale.z);
        XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&m_localPose[i].rotation));
        XMMATRIX T = XMMatrixTranslation(m_localPose[i].translation.x, m_localPose[i].translation.y, m_localPose[i].translation.z);
        XMStoreFloat4x4(&m_localTransforms[i], S * R * T);
    }

    // グローバル変換とボーン行列を作成
    // 初学者向け: ローカル→グローバルに変換した後、スキニング用の行列にまとめます。
    m_skeleton->ComputeGlobalTransforms(m_localTransforms.data(), m_globalTransforms.data());
    uint32_t boneCount = (std::min)(jointCount, BoneConstants::k_MaxBones);
    m_skeleton->ComputeBoneMatrices(m_globalTransforms.data(), m_boneConstants.boneMatrices);
}

} // namespace GX
