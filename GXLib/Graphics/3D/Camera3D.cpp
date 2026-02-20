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

XMMATRIX Camera3D::GetJitteredProjectionMatrix() const
{
    XMMATRIX proj = GetProjectionMatrix();
    // ジッターが無い場合はそのまま返す
    if (m_jitterOffset.x == 0.0f && m_jitterOffset.y == 0.0f)
        return proj;

    // プロジェクション行列の _31, _32 にジッターオフセットを加算
    // 行列はrow-majorなので、_31/_32は proj.r[2].m128_f32[0/1] に対応
    // 初学者向け: 行列の要素位置を直接いじると、ジッター（微小オフセット）を加えられます。
    XMFLOAT4X4 projF;
    XMStoreFloat4x4(&projF, proj);
    projF._31 += m_jitterOffset.x;
    projF._32 += m_jitterOffset.y;
    return XMLoadFloat4x4(&projF);
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
    // 初学者向け: 前方向と上方向の外積で、横方向（右）を作ります。
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, fwd));
    XMStoreFloat3(&m_right, right);

    // up = cross(forward, right)
    // 初学者向け: 前方向と右方向の外積で、正しい上方向を作ります。
    XMVECTOR up = XMVector3Cross(fwd, right);
    XMStoreFloat3(&m_up, up);

    m_dirtyVectors = false;
}

} // namespace GX
