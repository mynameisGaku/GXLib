#pragma once
/// @file RigidBody3D.h
/// @brief シンプルな3D剛体ラッパー

#include "Physics/PhysicsWorld3D.h"

namespace GX
{

/// @brief PhysicsWorld3D用の軽量ボディラッパー
///
/// PhysicsWorld3Dへの操作をオブジェクト指向的にまとめたクラス。
/// Create()で物理ボディを作成し、Destroy()で削除する。
/// 内部的にはPhysicsBodyIDを通じてPhysicsWorld3Dに委譲する。
class RigidBody3D
{
public:
    RigidBody3D() = default;

    /// @brief 物理ボディを作成してワールドに追加する
    /// @param world 物理ワールド
    /// @param shape コライダーシェイプ
    /// @param settings ボディ設定
    /// @return 作成に成功した場合true
    bool Create(PhysicsWorld3D* world, PhysicsShape* shape, const PhysicsBodySettings& settings)
    {
        if (!world || !shape)
            return false;
        m_world = world;
        m_id = m_world->AddBody(shape, settings);
        return m_id.IsValid();
    }

    /// @brief 物理ボディを削除してワールドから除去する
    void Destroy()
    {
        if (m_world && m_id.IsValid())
            m_world->RemoveBody(m_id);
        m_id = {};
        m_world = nullptr;
    }

    /// @brief ボディが有効かどうか判定する
    /// @return 有効ならtrue
    bool IsValid() const { return m_world && m_id.IsValid(); }

    /// @brief 物理ボディIDを取得する
    /// @return ボディID
    PhysicsBodyID GetID() const { return m_id; }

    /// @brief 所属する物理ワールドを取得する
    /// @return 物理ワールドへのポインタ
    PhysicsWorld3D* GetWorld() const { return m_world; }

    /// @brief 位置を設定する
    /// @param pos 新しい位置
    void SetPosition(const Vector3& pos) { if (IsValid()) m_world->SetPosition(m_id, pos); }

    /// @brief 回転を設定する
    /// @param rot 新しい回転クォータニオン
    void SetRotation(const Quaternion& rot) { if (IsValid()) m_world->SetRotation(m_id, rot); }

    /// @brief 線速度を設定する
    /// @param vel 線速度ベクトル
    void SetLinearVelocity(const Vector3& vel) { if (IsValid()) m_world->SetLinearVelocity(m_id, vel); }

    /// @brief 角速度を設定する
    /// @param vel 角速度ベクトル
    void SetAngularVelocity(const Vector3& vel) { if (IsValid()) m_world->SetAngularVelocity(m_id, vel); }

    /// @brief 力を加える（次のStep()で適用）
    /// @param force 力ベクトル
    void ApplyForce(const Vector3& force) { if (IsValid()) m_world->ApplyForce(m_id, force); }

    /// @brief 衝撃を加える（即座に速度変化）
    /// @param impulse 衝撃ベクトル
    void ApplyImpulse(const Vector3& impulse) { if (IsValid()) m_world->ApplyImpulse(m_id, impulse); }

    /// @brief トルクを加える（次のStep()で適用）
    /// @param torque トルクベクトル
    void ApplyTorque(const Vector3& torque) { if (IsValid()) m_world->ApplyTorque(m_id, torque); }

    /// @brief モーションタイプを変更する
    /// @param type 新しいモーションタイプ
    void SetMotionType(MotionType3D type) { if (IsValid()) m_world->SetMotionType(m_id, type); }

    /// @brief 現在の位置を取得する
    /// @return 位置ベクトル
    Vector3 GetPosition() const { return IsValid() ? m_world->GetPosition(m_id) : Vector3{}; }

    /// @brief 現在の回転を取得する
    /// @return 回転クォータニオン
    Quaternion GetRotation() const { return IsValid() ? m_world->GetRotation(m_id) : Quaternion{}; }

    /// @brief 現在の線速度を取得する
    /// @return 線速度ベクトル
    Vector3 GetLinearVelocity() const { return IsValid() ? m_world->GetLinearVelocity(m_id) : Vector3{}; }

    /// @brief ワールド変換行列を取得する（描画に直接使用可能）
    /// @return ワールド変換行列
    Matrix4x4 GetWorldTransform() const { return IsValid() ? m_world->GetWorldTransform(m_id) : Matrix4x4{}; }

    /// @brief ボディがアクティブ（スリープしていない）かどうか判定する
    /// @return アクティブならtrue
    bool IsActive() const { return IsValid() && m_world->IsActive(m_id); }

    /// @brief コライダーシェイプを差し替える
    /// @param shape 新しいシェイプ
    /// @param updateMassProperties trueで質量/慣性を再計算
    /// @param activate trueでボディを起動状態にする
    /// @return 成功時true
    bool SetShape(PhysicsShape* shape, bool updateMassProperties = true, bool activate = true)
    {
        if (!IsValid() || !shape)
            return false;
        return m_world->SetBodyShape(m_id, shape, updateMassProperties, activate);
    }

private:
    PhysicsWorld3D* m_world = nullptr;  ///< 所属する物理ワールド
    PhysicsBodyID   m_id;               ///< 物理ボディID
};

} // namespace GX
