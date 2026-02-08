/// @file Transform3D.cpp
/// @brief 3Dトランスフォームの実装
#include "pch.h"
#include "Graphics/3D/Transform3D.h"

namespace GX
{

XMMATRIX Transform3D::GetWorldMatrix() const
{
    XMMATRIX S = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
    XMMATRIX T = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    return S * R * T;
}

XMMATRIX Transform3D::GetWorldInverseTranspose() const
{
    XMMATRIX world = GetWorldMatrix();
    XMMATRIX worldInv = XMMatrixInverse(nullptr, world);
    return XMMatrixTranspose(worldInv);
}

} // namespace GX
