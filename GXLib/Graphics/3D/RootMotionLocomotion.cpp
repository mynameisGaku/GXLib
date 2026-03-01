/// @file RootMotionLocomotion.cpp
/// @brief ルートモーション移動コントローラーの実装
#include "pch_graphics.h"
#include "Graphics/3D/RootMotionLocomotion.h"
#include "Graphics/3D/Animator.h"

namespace gx
{

void RootMotionLocomotion::SetConfig(const RootMotionConfig& config)
{
    m_config = config;
}

const RootMotionConfig& RootMotionLocomotion::GetConfig() const
{
    return m_config;
}

void RootMotionLocomotion::SetAnimator(Animator* animator)
{
    m_animator = animator;
}

Animator* RootMotionLocomotion::GetAnimator() const
{
    return m_animator;
}

void RootMotionLocomotion::Update(float deltaTime)
{
    // Animatorが未設定の場合はゼロに
    if (!m_animator)
    {
        m_positionDelta = Vector3::Zero();
        m_rotationDelta = Quaternion::Identity();
        m_speed = 0.0f;
        return;
    }

    // Animatorからルートモーション差分を取得
    XMFLOAT3 rawDelta = m_animator->GetRootMotionDelta();
    XMFLOAT4 rawRotDelta = m_animator->GetRootMotionRotationDelta();

    // 位置差分の加工
    if (m_config.applyPosition)
    {
        m_positionDelta = Vector3(rawDelta.x, rawDelta.y, rawDelta.z) * m_config.positionScale;

        // 垂直ロック
        if (m_config.lockVertical)
        {
            m_positionDelta.y = 0.0f;
        }
    }
    else
    {
        m_positionDelta = Vector3::Zero();
    }

    // 回転差分の加工
    if (m_config.applyRotation)
    {
        Quaternion rawQuat(rawRotDelta.x, rawRotDelta.y, rawRotDelta.z, rawRotDelta.w);

        if (m_config.rotationScale != 1.0f)
        {
            // 回転スケーリング: identity と rawQuat の間を rotationScale で Slerp
            m_rotationDelta = Quaternion::Slerp(Quaternion::Identity(), rawQuat,
                                                 m_config.rotationScale);
        }
        else
        {
            m_rotationDelta = rawQuat;
        }
    }
    else
    {
        m_rotationDelta = Quaternion::Identity();
    }

    // 累積位置の更新
    m_accumulatedPosition += m_positionDelta;

    // 地面スナッピング
    if (m_config.useGroundSnapping && m_groundFunc)
    {
        float groundY = m_groundFunc(m_accumulatedPosition.x, m_accumulatedPosition.z);
        m_accumulatedPosition.y = groundY;
    }

    // 速度計算
    if (deltaTime > 1e-6f)
    {
        m_speed = m_positionDelta.Length() / deltaTime;
    }
    else
    {
        m_speed = 0.0f;
    }
}

Vector3 RootMotionLocomotion::GetPositionDelta() const
{
    return m_positionDelta;
}

Quaternion RootMotionLocomotion::GetRotationDelta() const
{
    return m_rotationDelta;
}

Vector3 RootMotionLocomotion::GetAccumulatedPosition() const
{
    return m_accumulatedPosition;
}

float RootMotionLocomotion::GetSpeed() const
{
    return m_speed;
}

void RootMotionLocomotion::SetGroundHeightFunction(GroundHeightFunc func)
{
    m_groundFunc = std::move(func);
}

void RootMotionLocomotion::Reset()
{
    m_positionDelta = Vector3::Zero();
    m_rotationDelta = Quaternion::Identity();
    m_accumulatedPosition = Vector3::Zero();
    m_speed = 0.0f;
}

} // namespace gx
