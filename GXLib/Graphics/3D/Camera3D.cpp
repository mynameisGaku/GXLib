/// @file Camera3D.cpp
/// @brief 3Dカメラの実装
#include "pch.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

void Camera3D::SetPerspective(float fovY, float aspect, float nearZ, float farZ)
{
    m_isPerspective = true;
    m_fovY   = fovY;
    m_aspect = aspect;
    m_nearZ  = nearZ;
    m_farZ   = farZ;
}

void Camera3D::SetOrthographic(float width, float height, float nearZ, float farZ)
{
    m_isPerspective = false;
    m_orthoWidth  = width;
    m_orthoHeight = height;
    m_nearZ  = nearZ;
    m_farZ   = farZ;
}

void Camera3D::Rotate(float deltaPitch, float deltaYaw)
{
    m_pitch += deltaPitch;
    m_yaw   += deltaYaw;

    // ピッチ制限（±89度）
    constexpr float maxPitch = XM_PIDIV2 - 0.01f;
    if (m_pitch > maxPitch)  m_pitch = maxPitch;
    if (m_pitch < -maxPitch) m_pitch = -maxPitch;

    m_dirtyVectors = true;
}

void Camera3D::MoveForward(float distance)
{
    UpdateVectors();
    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMVECTOR fwd = XMLoadFloat3(&m_forward);
    pos = XMVectorAdd(pos, XMVectorScale(fwd, distance));
    XMStoreFloat3(&m_position, pos);
}

void Camera3D::MoveRight(float distance)
{
    UpdateVectors();
    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMVECTOR right = XMLoadFloat3(&m_right);
    pos = XMVectorAdd(pos, XMVectorScale(right, distance));
    XMStoreFloat3(&m_position, pos);
}

void Camera3D::MoveUp(float distance)
{
    // ワールドアップ方向に移動（カメラローカルではなく）
    m_position.y += distance;
}

XMMATRIX Camera3D::GetViewMatrix() const
{
    UpdateVectors();

    switch (m_mode)
    {
    case CameraMode::Free:
    case CameraMode::FPS:
    {
        XMVECTOR pos = XMLoadFloat3(&m_position);
        XMVECTOR fwd = XMLoadFloat3(&m_forward);
        XMVECTOR up  = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        return XMMatrixLookToLH(pos, fwd, up);
    }
    case CameraMode::TPS:
    {
        // ターゲットの後方にカメラを配置
        XMVECTOR target = XMLoadFloat3(&m_target);
        XMVECTOR offset = XMLoadFloat3(&m_tpsOffset);
        target = XMVectorAdd(target, offset);

        XMVECTOR fwd = XMLoadFloat3(&m_forward);
        XMVECTOR pos = XMVectorSubtract(target, XMVectorScale(fwd, m_tpsDistance));
        XMStoreFloat3(&const_cast<Camera3D*>(this)->m_position, pos);

        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        return XMMatrixLookAtLH(pos, target, up);
    }
    }

    return XMMatrixIdentity();
}

XMMATRIX Camera3D::GetProjectionMatrix() const
{
    if (m_isPerspective)
        return XMMatrixPerspectiveFovLH(m_fovY, m_aspect, m_nearZ, m_farZ);
    else
        return XMMatrixOrthographicLH(m_orthoWidth, m_orthoHeight, m_nearZ, m_farZ);
}

XMMATRIX Camera3D::GetViewProjectionMatrix() const
{
    return GetViewMatrix() * GetProjectionMatrix();
}

XMFLOAT3 Camera3D::GetForward() const
{
    UpdateVectors();
    return m_forward;
}

XMFLOAT3 Camera3D::GetRight() const
{
    UpdateVectors();
    return m_right;
}

XMFLOAT3 Camera3D::GetUp() const
{
    UpdateVectors();
    return m_up;
}

void Camera3D::UpdateVectors() const
{
    if (!m_dirtyVectors)
        return;

    // pitch/yawから方向ベクトルを計算
    float cosPitch = cosf(m_pitch);
    float sinPitch = sinf(m_pitch);
    float cosYaw   = cosf(m_yaw);
    float sinYaw   = sinf(m_yaw);

    m_forward = {
        cosPitch * sinYaw,
        sinPitch,
        cosPitch * cosYaw
    };

    // forwardを正規化
    XMVECTOR fwd = XMVector3Normalize(XMLoadFloat3(&m_forward));
    XMStoreFloat3(&m_forward, fwd);

    // right = normalize(cross(worldUp, forward))
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, fwd));
    XMStoreFloat3(&m_right, right);

    // up = cross(forward, right)
    XMVECTOR up = XMVector3Cross(fwd, right);
    XMStoreFloat3(&m_up, up);

    m_dirtyVectors = false;
}

} // namespace GX
