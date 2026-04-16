#pragma once
/// @file CharacterController.h
/// @brief プレイヤーや NPC の移動を制御するキャラクターコントローラー
///
/// Jolt Physics の CharacterVirtual をラップし、階段の乗り越えや
/// 急斜面での滑りを自動処理する。SetMovementInput() で方向入力、
/// Jump() でジャンプを指定するだけで物理と整合した移動が得られる。
/// @addtogroup grp_physics/// @{

#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "Math/Matrix4x4.h"
#include <memory>
#include <cstdint>

namespace gx {

class PhysicsWorld3D;

/// @brief キャラクターコントローラーの設定
struct CharacterControllerSettings {
    float capsuleHalfHeight = 0.9f;   ///< カプセル半高（球を除く円柱部分）
    float capsuleRadius = 0.3f;       ///< カプセル半径
    float mass = 70.0f;               ///< 質量 (kg)
    float maxSlopeAngle = 50.0f;      ///< 歩行可能な最大斜面角度（度）
    float stepHeight = 0.4f;          ///< 階段乗り越え高さ
    float stepDownHeight = 0.5f;      ///< 階段降り時の張り付き高さ
    float walkSpeed = 4.0f;           ///< 歩行速度 (m/s)
    float runSpeed = 8.0f;            ///< 走行速度 (m/s)
    float jumpForce = 5.0f;           ///< ジャンプ初速 (m/s)
    float gravity = -9.81f;           ///< 重力加速度 (m/s^2, 負値=下向き)
    float skinWidth = 0.02f;          ///< スキン幅 (衝突マージン)
    uint16_t layer = 1;               ///< 衝突レイヤー
};

/// @brief キャラクターの地面状態
enum class CharacterGroundState {
    OnGround,       ///< 地面上
    OnSteepGround,  ///< 急斜面上
    NotSupported,   ///< サポートなし
    InAir,          ///< 空中
    Sliding         ///< 急斜面で滑っている
};

/// @brief CharacterGroundState の短縮エイリアス
using GroundState = CharacterGroundState;

/// @brief 物理ベースのキャラクターコントローラー
class CharacterController
{
public:
    CharacterController();
    ~CharacterController();

    /// @brief 初期化する
    /// @param world 物理ワールド
    /// @param settings コントローラー設定
    /// @param initialPos 初期位置
    /// @return 初期化に成功した場合true
    bool Initialize(PhysicsWorld3D* world, const CharacterControllerSettings& settings, const Vector3& initialPos);

    /// @brief 終了処理
    void Shutdown();

    /// @brief フレーム更新（物理ステップ後に呼ぶ）
    /// @param deltaTime 経過時間（秒）
    void Update(float deltaTime);

    /// @brief 位置を設定する
    void SetPosition(const Vector3& pos);

    /// @brief 回転を設定する
    void SetRotation(const Quaternion& rot);

    /// @brief 線速度を設定する
    void SetLinearVelocity(const Vector3& vel);

    /// @brief 位置を取得する
    Vector3 GetPosition() const;

    /// @brief 回転を取得する
    Quaternion GetRotation() const;

    /// @brief 線速度を取得する
    Vector3 GetLinearVelocity() const;

    /// @brief ワールド変換行列を取得する
    Matrix4x4 GetWorldTransform() const;

    /// @brief 地面状態を取得する
    CharacterGroundState GetGroundState() const;

    /// @brief 地面上にいるかどうか判定する
    bool IsOnGround() const;

    // ----- 高レベル移動API -----

    /// @brief 移動入力を設定する（XZ平面、次のUpdateで適用）
    /// @param input 入力ベクトル（通常は正規化済み、長さ0〜1）
    void SetMovementInput(const Vector3& input);

    /// @brief ジャンプする（地面にいる場合のみ有効）
    void Jump();

    /// @brief 走行モードを切り替える
    /// @param running trueで走行、falseで歩行
    void SetRunning(bool running);

    /// @brief 地面の法線ベクトルを取得する
    /// @return 地面の法線（空中の場合は上方向 {0,1,0}）
    Vector3 GetGroundNormal() const;

    /// @brief カプセルの高さ（半高）を取得する
    float GetHeight() const;

    /// @brief カプセルの半径を取得する
    float GetRadius() const;

    /// @brief 歩行速度を設定する
    void SetWalkSpeed(float speed);

    /// @brief 走行速度を設定する
    void SetRunSpeed(float speed);

    /// @brief ジャンプ力を設定する
    void SetJumpForce(float force);

    /// @brief 重力加速度を設定する
    void SetGravity(float g);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace gx
/// @}
