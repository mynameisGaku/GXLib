#pragma once
/// @file Transform3D.h
/// @brief 3Dトランスフォーム（位置・回転・スケール → ワールド行列）

#include "pch_common.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"

namespace gx
{
/// @addtogroup grp_math
/// @{

/// @brief 3Dオブジェクトの位置・回転・スケールを管理するクラス
class Transform3D
{
public:
    Transform3D() = default;
    ~Transform3D() = default;

    /// @brief 位置を設定する（DxLibの MV1SetPosition に相当）
    void SetPosition(float x, float y, float z) { m_position = { x, y, z }; m_dirty = true; }

    /// @brief 位置を設定する
    void SetPosition(const Vector3& pos) { m_position = pos; m_dirty = true; }

    /// @brief 回転を設定する（DxLibの MV1SetRotationXYZ に相当）
    void SetRotation(float pitch, float yaw, float roll) { m_rotation = { pitch, yaw, roll }; m_dirty = true; }

    /// @brief 回転を設定する
    void SetRotation(const Vector3& rot) { m_rotation = rot; m_dirty = true; }

    /// @brief スケールを設定する（DxLibの MV1SetScale に相当）
    void SetScale(float x, float y, float z) { m_scale = { x, y, z }; m_dirty = true; }

    /// @brief 均一スケールを設定する
    void SetScale(float uniform) { m_scale = { uniform, uniform, uniform }; m_dirty = true; }

    /// @brief スケールを設定する
    void SetScale(const Vector3& s) { m_scale = s; m_dirty = true; }

    /// @brief 位置を取得する
    const Vector3& GetPosition() const { return m_position; }

    /// @brief 回転を取得する
    const Vector3& GetRotation() const { return m_rotation; }

    /// @brief スケールを取得する
    const Vector3& GetScale() const { return m_scale; }

    /// @brief ワールド行列を取得する（SRT順: Scale -> Rotate -> Translate）
    Matrix4x4 GetWorldMatrix() const;

    /// @brief ワールド逆転置行列を取得する（法線をワールド空間に正しく変換するために使う）
    Matrix4x4 GetWorldInverseTranspose() const;

private:
    Vector3 m_position = { 0.0f, 0.0f, 0.0f };
    Vector3 m_rotation = { 0.0f, 0.0f, 0.0f };  // pitch, yaw, roll (radians)
    Vector3 m_scale    = { 1.0f, 1.0f, 1.0f };
    mutable bool m_dirty = true;
};

/// @}
} // namespace gx
