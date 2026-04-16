/// @file Camera3D.cpp
/// @brief 3Dカメラの実装
#include "pch_graphics.h"
#include "Graphics/3D/Camera3D.h"
#include "Math/MathConvert.h"

namespace gx
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

void Camera3D::LookAt(const Vector3& target)
{
    float dx = target.x - m_position.x;
    float dy = target.y - m_position.y;
    float dz = target.z - m_position.z;
    float dist = std::sqrt(dx * dx + dz * dz);
    SetPitch(-std::atan2(dy, dist));
    SetYaw(std::atan2(dx, dz));
}

void Camera3D::SetPitch(float pitch)
{
    constexpr float maxPitch = XM_PIDIV2 - 0.01f;
    m_pitch = (std::max)(-maxPitch, (std::min)(pitch, maxPitch));
    m_dirtyVectors = true;
}

void Camera3D::SetYaw(float yaw)
{
    m_yaw = yaw;
    m_dirtyVectors = true;
}

void Camera3D::Rotate(float deltaPitch, float deltaYaw)
{
    m_pitch += deltaPitch;
    m_yaw   += deltaYaw;

    constexpr float maxPitch = XM_PIDIV2 - 0.01f;
    if (m_pitch > maxPitch)  m_pitch = maxPitch;
    if (m_pitch < -maxPitch) m_pitch = -maxPitch;

    m_dirtyVectors = true;
}

void Camera3D::MoveForward(float distance)
{
    UpdateVectors();
    XMVECTOR pos = XMLoadFloat3(XM(&m_position));
    XMVECTOR fwd = XMLoadFloat3(XM(&m_forward));
    pos = XMVectorAdd(pos, XMVectorScale(fwd, distance));
    XMStoreFloat3(XM(&m_position), pos);
}

void Camera3D::MoveRight(float distance)
{
    UpdateVectors();
    XMVECTOR pos = XMLoadFloat3(XM(&m_position));
    XMVECTOR right = XMLoadFloat3(XM(&m_right));
    pos = XMVectorAdd(pos, XMVectorScale(right, distance));
    XMStoreFloat3(XM(&m_position), pos);
}

void Camera3D::MoveUp(float distance)
{
    m_position.y += distance;
}

Matrix4x4 Camera3D::GetViewMatrix() const
{
    UpdateVectors();

    switch (m_mode)
    {
    case CameraMode::Free:
    case CameraMode::FPS:
    {
        XMVECTOR pos = XMLoadFloat3(XM(&m_position));
        XMVECTOR fwd = XMLoadFloat3(XM(&m_forward));
        XMVECTOR up  = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        return FromXMMATRIX(XMMatrixLookToLH(pos, fwd, up));
    }
    case CameraMode::TPS:
    {
        XMVECTOR target = XMLoadFloat3(XM(&m_target));
        XMVECTOR offset = XMLoadFloat3(XM(&m_tpsOffset));
        target = XMVectorAdd(target, offset);

        XMVECTOR fwd = XMLoadFloat3(XM(&m_forward));
        XMVECTOR pos = XMVectorSubtract(target, XMVectorScale(fwd, m_tpsDistance));
        XMStoreFloat3(XM(&const_cast<Camera3D*>(this)->m_position), pos);

        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        return FromXMMATRIX(XMMatrixLookAtLH(pos, target, up));
    }
    }

    return Matrix4x4{};
}

Matrix4x4 Camera3D::GetProjectionMatrix() const
{
    if (m_isPerspective)
        return FromXMMATRIX(XMMatrixPerspectiveFovLH(m_fovY, m_aspect, m_nearZ, m_farZ));
    else
        return FromXMMATRIX(XMMatrixOrthographicLH(m_orthoWidth, m_orthoHeight, m_nearZ, m_farZ));
}

Matrix4x4 Camera3D::GetJitteredProjectionMatrix() const
{
    Matrix4x4 proj = GetProjectionMatrix();
    if (m_jitterOffset.x == 0.0f && m_jitterOffset.y == 0.0f)
        return proj;

    proj._31 += m_jitterOffset.x;
    proj._32 += m_jitterOffset.y;
    return proj;
}

Matrix4x4 Camera3D::GetViewProjectionMatrix() const
{
    return GetViewMatrix() * GetProjectionMatrix();
}

Vector3 Camera3D::GetForward() const
{
    UpdateVectors();
    return m_forward;
}

Vector3 Camera3D::GetRight() const
{
    UpdateVectors();
    return m_right;
}

Vector3 Camera3D::GetUp() const
{
    UpdateVectors();
    return m_up;
}

void Camera3D::UpdateVectors() const
{
    if (!m_dirtyVectors)
        return;

    float cosPitch = cosf(m_pitch);
    float sinPitch = sinf(m_pitch);
    float cosYaw   = cosf(m_yaw);
    float sinYaw   = sinf(m_yaw);

    m_forward = {
        cosPitch * sinYaw,
        sinPitch,
        cosPitch * cosYaw
    };

    XMVECTOR fwd = XMVector3Normalize(XMLoadFloat3(XM(&m_forward)));
    XMStoreFloat3(XM(&m_forward), fwd);

    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, fwd));
    XMStoreFloat3(XM(&m_right), right);

    XMVECTOR up = XMVector3Cross(fwd, right);
    XMStoreFloat3(XM(&m_up), up);

    m_dirtyVectors = false;
}

} // namespace gx
