/// @file Transform3D.cpp
/// @brief 3Dトランスフォームの実装
#include "pch_common.h"
#include "Math/Transform3D.h"
#include "Math/MathConvert.h"

namespace gx
{

Matrix4x4 Transform3D::GetWorldMatrix() const
{
    // S * R * T の順で合成。DirectXMathは行ベクトル規約なので左から右へ適用される
    XMMATRIX S = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
    XMMATRIX T = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    return FromXMMATRIX(S * R * T);
}

Matrix4x4 Transform3D::GetWorldInverseTranspose() const
{
    // 法線変換には逆転置行列が必要（不均一スケールでも法線が正しく変換される）
    XMMATRIX world = ToXMMATRIX(GetWorldMatrix());
    XMMATRIX worldInv = XMMatrixInverse(nullptr, world);
    return FromXMMATRIX(XMMatrixTranspose(worldInv));
}

} // namespace gx
