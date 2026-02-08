#pragma once
/// @file Transform3D.h
/// @brief 3Dトランスフォーム（位置・回転・スケール → ワールド行列）

#include "pch.h"

namespace GX
{

/// @brief 3Dオブジェクトの位置・回転・スケールを管理するクラス
class Transform3D
{
public:
    Transform3D() = default;
    ~Transform3D() = default;

    void SetPosition(float x, float y, float z) { m_position = { x, y, z }; m_dirty = true; }
    void SetPosition(const XMFLOAT3& pos) { m_position = pos; m_dirty = true; }
    void SetRotation(float pitch, float yaw, float roll) { m_rotation = { pitch, yaw, roll }; m_dirty = true; }
    void SetRotation(const XMFLOAT3& rot) { m_rotation = rot; m_dirty = true; }
    void SetScale(float x, float y, float z) { m_scale = { x, y, z }; m_dirty = true; }
    void SetScale(float uniform) { m_scale = { uniform, uniform, uniform }; m_dirty = true; }
    void SetScale(const XMFLOAT3& s) { m_scale = s; m_dirty = true; }

    const XMFLOAT3& GetPosition() const { return m_position; }
    const XMFLOAT3& GetRotation() const { return m_rotation; }
    const XMFLOAT3& GetScale() const { return m_scale; }

    /// ワールド行列を取得（SRT順: Scale → Rotate → Translate）
    XMMATRIX GetWorldMatrix() const;

    /// ワールド逆転置行列を取得（法線変換用）
    XMMATRIX GetWorldInverseTranspose() const;

private:
    XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f };  // pitch, yaw, roll (radians)
    XMFLOAT3 m_scale    = { 1.0f, 1.0f, 1.0f };
    mutable bool m_dirty = true;
};

} // namespace GX
