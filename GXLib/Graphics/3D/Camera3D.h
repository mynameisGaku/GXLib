#pragma once
/// @file Camera3D.h
/// @brief 3Dカメラ（パースペクティブ/正射影、Free/FPS/TPSモード）

#include "pch.h"

namespace GX
{

/// @brief カメラの操作モード
enum class CameraMode
{
    Free,   ///< 自由移動モード（エディタ用）
    FPS,    ///< 一人称視点モード
    TPS,    ///< 三人称視点モード
};

/// @brief 3Dカメラクラス
class Camera3D
{
public:
    Camera3D() = default;
    ~Camera3D() = default;

    // --- 射影設定 ---

    /// @brief パースペクティブ（透視投影）を設定する
    /// @param fovY 垂直方向の視野角（ラジアン）
    /// @param aspect アスペクト比（幅/高さ）
    /// @param nearZ ニアクリップ面の距離
    /// @param farZ ファークリップ面の距離
    void SetPerspective(float fovY, float aspect, float nearZ, float farZ);

    /// @brief 正射影を設定する
    /// @param width 正射影の幅
    /// @param height 正射影の高さ
    /// @param nearZ ニアクリップ面の距離
    /// @param farZ ファークリップ面の距離
    void SetOrthographic(float width, float height, float nearZ, float farZ);

    // --- カメラモード ---

    /// @brief カメラモードを設定する
    /// @param mode カメラモード（Free/FPS/TPS）
    void SetMode(CameraMode mode) { m_mode = mode; }

    /// @brief 現在のカメラモードを取得する
    /// @return 現在のカメラモード
    CameraMode GetMode() const { return m_mode; }

    // --- 位置・方向設定 ---

    /// @brief カメラ位置を設定する
    /// @param x X座標
    /// @param y Y座標
    /// @param z Z座標
    void SetPosition(float x, float y, float z) { m_position = { x, y, z }; }

    /// @brief カメラ位置を設定する
    /// @param pos 位置ベクトル
    void SetPosition(const XMFLOAT3& pos) { m_position = pos; }

    /// @brief カメラの注視点を設定する
    /// @param target 注視点の位置
    void SetTarget(const XMFLOAT3& target) { m_target = target; }

    /// @brief 指定ターゲットを注視するようにピッチ・ヨーを設定する
    /// @param target 注視点のワールド座標
    void LookAt(const XMFLOAT3& target);

    /// @brief ピッチ角を直接設定する（オービットカメラ用）
    /// @param pitch ピッチ角（ラジアン、±89度にクランプ）
    void SetPitch(float pitch);

    /// @brief ヨー角を直接設定する（オービットカメラ用）
    /// @param yaw ヨー角（ラジアン）
    void SetYaw(float yaw);

    // --- TPS用設定 ---

    /// @brief TPSモードでのカメラとターゲット間の距離を設定する
    /// @param distance ターゲットからの距離
    void SetTPSDistance(float distance) { m_tpsDistance = distance; }

    /// @brief TPSモードでのカメラオフセットを設定する
    /// @param offset ターゲットからのオフセット
    void SetTPSOffset(const XMFLOAT3& offset) { m_tpsOffset = offset; }

    // --- 移動・回転 ---

    /// @brief カメラを回転する
    /// @param deltaPitch ピッチ（上下）の回転量（ラジアン）
    /// @param deltaYaw ヨー（左右）の回転量（ラジアン）
    void Rotate(float deltaPitch, float deltaYaw);

    /// @brief カメラを前方向に移動する
    /// @param distance 移動距離（負で後退）
    void MoveForward(float distance);

    /// @brief カメラを右方向に移動する
    /// @param distance 移動距離（負で左移動）
    void MoveRight(float distance);

    /// @brief カメラを上方向に移動する
    /// @param distance 移動距離（負で下移動）
    void MoveUp(float distance);

    // --- ジッター (TAA用) ---

    /// @brief TAAジッターオフセットを設定する（NDC空間）
    /// @param x X方向のジッターオフセット
    /// @param y Y方向のジッターオフセット
    void SetJitter(float x, float y) { m_jitterOffset = { x, y }; }

    /// @brief TAAジッターをクリアする
    void ClearJitter() { m_jitterOffset = { 0.0f, 0.0f }; }

    /// @brief 現在のジッターオフセットを取得する
    /// @return ジッターオフセット（NDC空間）
    XMFLOAT2 GetJitter() const { return m_jitterOffset; }

    /// @brief ジッター適用済みの射影行列を取得する（TAA用）
    /// @return ジッター適用済み射影行列
    XMMATRIX GetJitteredProjectionMatrix() const;

    // --- 行列取得 ---

    /// @brief ビュー行列を取得する
    /// @return ビュー変換行列
    XMMATRIX GetViewMatrix() const;

    /// @brief 射影行列を取得する
    /// @return 射影変換行列
    XMMATRIX GetProjectionMatrix() const;

    /// @brief ビュー射影行列を取得する
    /// @return ビュー行列と射影行列の積
    XMMATRIX GetViewProjectionMatrix() const;

    // --- プロパティ取得 ---

    /// @brief カメラの位置を取得する
    /// @return 位置ベクトルへのconst参照
    const XMFLOAT3& GetPosition() const { return m_position; }

    /// @brief カメラの前方向ベクトルを取得する
    /// @return 正規化された前方向ベクトル
    XMFLOAT3 GetForward() const;

    /// @brief カメラの右方向ベクトルを取得する
    /// @return 正規化された右方向ベクトル
    XMFLOAT3 GetRight() const;

    /// @brief カメラの上方向ベクトルを取得する
    /// @return 正規化された上方向ベクトル
    XMFLOAT3 GetUp() const;

    /// @brief ピッチ角（上下回転）を取得する
    /// @return ピッチ角（ラジアン）
    float GetPitch() const { return m_pitch; }

    /// @brief ヨー角（左右回転）を取得する
    /// @return ヨー角（ラジアン）
    float GetYaw() const { return m_yaw; }

    /// @brief ニアクリップ面の距離を取得する
    /// @return ニアクリップ距離
    float GetNearZ() const { return m_nearZ; }

    /// @brief ファークリップ面の距離を取得する
    /// @return ファークリップ距離
    float GetFarZ() const { return m_farZ; }

    /// @brief 垂直方向の視野角を取得する
    /// @return 視野角（ラジアン）
    float GetFovY() const { return m_fovY; }

    /// @brief アスペクト比を取得する
    /// @return アスペクト比（幅/高さ）
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
