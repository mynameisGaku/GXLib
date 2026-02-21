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

    /// @brief 位置を設定する（DxLibの MV1SetPosition に相当）
    /// @param x X座標
    /// @param y Y座標
    /// @param z Z座標
    void SetPosition(float x, float y, float z) { m_position = { x, y, z }; m_dirty = true; }

    /// @brief 位置を設定する
    /// @param pos 位置ベクトル
    void SetPosition(const XMFLOAT3& pos) { m_position = pos; m_dirty = true; }

    /// @brief 回転を設定する（DxLibの MV1SetRotationXYZ に相当）
    /// @param pitch X軸回転（ピッチ、ラジアン）
    /// @param yaw Y軸回転（ヨー、ラジアン）
    /// @param roll Z軸回転（ロール、ラジアン）
    void SetRotation(float pitch, float yaw, float roll) { m_rotation = { pitch, yaw, roll }; m_dirty = true; }

    /// @brief 回転を設定する
    /// @param rot 回転ベクトル (pitch, yaw, roll) ラジアン
    void SetRotation(const XMFLOAT3& rot) { m_rotation = rot; m_dirty = true; }

    /// @brief スケールを設定する（DxLibの MV1SetScale に相当）
    /// @param x X方向スケール
    /// @param y Y方向スケール
    /// @param z Z方向スケール
    void SetScale(float x, float y, float z) { m_scale = { x, y, z }; m_dirty = true; }

    /// @brief 均一スケールを設定する
    /// @param uniform 全軸に適用するスケール値
    void SetScale(float uniform) { m_scale = { uniform, uniform, uniform }; m_dirty = true; }

    /// @brief スケールを設定する
    /// @param s スケールベクトル
    void SetScale(const XMFLOAT3& s) { m_scale = s; m_dirty = true; }

    /// @brief 位置を取得する
    /// @return 位置ベクトルへのconst参照
    const XMFLOAT3& GetPosition() const { return m_position; }

    /// @brief 回転を取得する
    /// @return 回転ベクトル (pitch, yaw, roll) への const参照
    const XMFLOAT3& GetRotation() const { return m_rotation; }

    /// @brief スケールを取得する
    /// @return スケールベクトルへのconst参照
    const XMFLOAT3& GetScale() const { return m_scale; }

    /// @brief ワールド行列を取得する（SRT順: Scale -> Rotate -> Translate）
    /// @return ワールド変換行列
    XMMATRIX GetWorldMatrix() const;

    /// @brief ワールド逆転置行列を取得する（法線をワールド空間に正しく変換するために使う）
    /// @return 逆転置行列
    XMMATRIX GetWorldInverseTranspose() const;

private:
    XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f };  // pitch, yaw, roll (radians)
    XMFLOAT3 m_scale    = { 1.0f, 1.0f, 1.0f };
    mutable bool m_dirty = true;
};

} // namespace GX
