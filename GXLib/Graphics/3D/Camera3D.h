#pragma once
/// @file Camera3D.h
/// @brief 3Dカメラ（パースペクティブ/正射影、Free/FPS/TPSモード）

#include "pch.h"

namespace GX
{

/// カメラモード
enum class CameraMode
{
    Free,   // 自由移動（エディタ用）
    FPS,    // 一人称視点
    TPS,    // 三人称視点
};

/// @brief 3Dカメラクラス
class Camera3D
{
public:
    Camera3D() = default;
    ~Camera3D() = default;

    // --- 射影設定 ---
    void SetPerspective(float fovY, float aspect, float nearZ, float farZ);
    void SetOrthographic(float width, float height, float nearZ, float farZ);

    // --- カメラモード ---
    void SetMode(CameraMode mode) { m_mode = mode; }
    CameraMode GetMode() const { return m_mode; }

    // --- 位置・方向設定 ---
    void SetPosition(float x, float y, float z) { m_position = { x, y, z }; }
    void SetPosition(const XMFLOAT3& pos) { m_position = pos; }
    void SetTarget(const XMFLOAT3& target) { m_target = target; }

    // --- TPS用設定 ---
    void SetTPSDistance(float distance) { m_tpsDistance = distance; }
    void SetTPSOffset(const XMFLOAT3& offset) { m_tpsOffset = offset; }

    // --- 移動・回転 ---
    void Rotate(float deltaPitch, float deltaYaw);
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);

    // --- ジッター (TAA用) ---
    void SetJitter(float x, float y) { m_jitterOffset = { x, y }; }
    void ClearJitter() { m_jitterOffset = { 0.0f, 0.0f }; }
    XMFLOAT2 GetJitter() const { return m_jitterOffset; }
    XMMATRIX GetJitteredProjectionMatrix() const;

    // --- 行列取得 ---
    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjectionMatrix() const;
    XMMATRIX GetViewProjectionMatrix() const;

    // --- プロパティ取得 ---
    const XMFLOAT3& GetPosition() const { return m_position; }
    XMFLOAT3 GetForward() const;
    XMFLOAT3 GetRight() const;
    XMFLOAT3 GetUp() const;
    float GetPitch() const { return m_pitch; }
    float GetYaw() const { return m_yaw; }
    float GetNearZ() const { return m_nearZ; }
    float GetFarZ() const { return m_farZ; }
    float GetFovY() const { return m_fovY; }
    float GetAspect() const { return m_aspect; }

private:
    void UpdateVectors() const;

    CameraMode m_mode = CameraMode::Free;

    // 位置・方向
    XMFLOAT3 m_position = { 0.0f, 0.0f, -5.0f };
    XMFLOAT3 m_target   = { 0.0f, 0.0f, 0.0f };
    float m_pitch = 0.0f;   // 上下回転（ラジアン）
    float m_yaw   = 0.0f;   // 左右回転（ラジアン）

    // TPS用
    float    m_tpsDistance = 5.0f;
    XMFLOAT3 m_tpsOffset  = { 0.0f, 1.5f, 0.0f };

    // 射影パラメータ
    bool  m_isPerspective = true;
    float m_fovY   = XM_PIDIV4;  // 45度
    float m_aspect = 16.0f / 9.0f;
    float m_nearZ  = 0.1f;
    float m_farZ   = 1000.0f;
    float m_orthoWidth  = 20.0f;
    float m_orthoHeight = 20.0f;

    // ジッターオフセット (NDC空間、TAA用)
    XMFLOAT2 m_jitterOffset = { 0.0f, 0.0f };

    // キャッシュ用方向ベクトル
    mutable XMFLOAT3 m_forward = { 0.0f, 0.0f, 1.0f };
    mutable XMFLOAT3 m_right   = { 1.0f, 0.0f, 0.0f };
    mutable XMFLOAT3 m_up      = { 0.0f, 1.0f, 0.0f };
    mutable bool m_dirtyVectors = true;
};

} // namespace GX
