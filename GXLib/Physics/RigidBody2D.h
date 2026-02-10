#pragma once
/// @file RigidBody2D.h
/// @brief 2D剛体 — 位置・速度・質量・コライダー形状を保持
#include "Math/Vector2.h"

namespace GX {

/// @brief 2Dボディタイプ
enum class BodyType2D {
    Static,     ///< 静的 (移動しない、質量無限大)
    Dynamic,    ///< 動的 (力・衝突で移動する)
    Kinematic   ///< キネマティック (コードで直接移動、他の動的ボディを押す)
};

/// @brief 2Dコライダーの形状タイプ
enum class ShapeType2D {
    Circle, ///< 円形コライダー
    AABB    ///< 軸整列バウンディングボックスコライダー
};

/// @brief 2Dコライダー形状定義
struct ColliderShape2D {
    ShapeType2D type = ShapeType2D::Circle; ///< 形状タイプ
    float radius = 0.5f;                     ///< 円形の半径 (type == Circle 時)
    Vector2 halfExtents = { 0.5f, 0.5f };   ///< AABBの半サイズ (type == AABB 時)
};

/// @brief 2D剛体クラス
///
/// 2D物理シミュレーション用の剛体。位置・速度・質量・コライダー形状などを保持する。
/// PhysicsWorld2D::AddBody() で作成し、RemoveBody() で削除する。
class RigidBody2D
{
public:
    RigidBody2D() = default;

    Vector2 position;               ///< ワールド座標での位置
    float rotation = 0.0f;          ///< 回転角度 (ラジアン)

    Vector2 velocity;               ///< 線速度 (px/秒)
    float angularVelocity = 0.0f;   ///< 角速度 (rad/秒)

    float mass = 1.0f;              ///< 質量 (kg)
    float restitution = 0.3f;       ///< 反発係数 (0=完全非弾性, 1=完全弾性)
    float friction = 0.2f;          ///< 摩擦係数
    float linearDamping = 0.01f;    ///< 線速度の減衰率
    float angularDamping = 0.01f;   ///< 角速度の減衰率
    bool fixedRotation = false;     ///< trueの場合回転しない

    BodyType2D bodyType = BodyType2D::Dynamic; ///< ボディタイプ
    ColliderShape2D shape;                      ///< コライダー形状

    bool isTrigger = false;         ///< trueの場合トリガー (衝突応答なし、コールバックのみ)
    void* userData = nullptr;       ///< ユーザー任意データポインタ
    uint32_t layer = 0xFFFFFFFF;    ///< 衝突レイヤービットマスク

    /// @brief 力を加える (次のStep()で適用)
    /// @param force 加える力ベクトル
    void ApplyForce(const Vector2& force) { m_forceAccum += force; }

    /// @brief 衝撃を加える (即座に速度変化)
    /// @param impulse 加える衝撃ベクトル
    void ApplyImpulse(const Vector2& impulse) { if (InverseMass() > 0.0f) velocity += impulse * InverseMass(); }

    /// @brief トルクを加える (次のStep()で適用)
    /// @param torque 加えるトルク値
    void ApplyTorque(float torque) { m_torqueAccum += torque; }

    /// @brief 逆質量を取得する (Static/Kinematicは0を返す)
    /// @return 逆質量 (1/mass)、非Dynamicの場合は0
    float InverseMass() const { return (bodyType == BodyType2D::Dynamic && mass > 0.0f) ? 1.0f / mass : 0.0f; }

    Vector2 m_forceAccum;           ///< 蓄積された力 (Stepで消費される)
    float m_torqueAccum = 0.0f;     ///< 蓄積されたトルク (Stepで消費される)
};

} // namespace GX
