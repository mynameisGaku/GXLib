#pragma once
/// @file RigidBody3D.h
/// @brief シンプルな3D剛体ラッパー

#include "Physics/PhysicsWorld3D.h"

namespace GX
{

/// @brief PhysicsWorld3D用の軽量ボディラッパー
class RigidBody3D
{
public:
    RigidBody3D() = default;

    bool Create(PhysicsWorld3D* world, PhysicsShape* shape, const PhysicsBodySettings& settings)
    {
        if (!world || !shape)
            return false;
        m_world = world;
        m_id = m_world->AddBody(shape, settings);
        return m_id.IsValid();
    }

    void Destroy()
    {
        if (m_world && m_id.IsValid())
            m_world->RemoveBody(m_id);
        m_id = {};
        m_world = nullptr;
    }

    bool IsValid() const { return m_world && m_id.IsValid(); }

    PhysicsBodyID GetID() const { return m_id; }
    PhysicsWorld3D* GetWorld() const { return m_world; }

    void SetPosition(const Vector3& pos) { if (IsValid()) m_world->SetPosition(m_id, pos); }
    void SetRotation(const Quaternion& rot) { if (IsValid()) m_world->SetRotation(m_id, rot); }
    void SetLinearVelocity(const Vector3& vel) { if (IsValid()) m_world->SetLinearVelocity(m_id, vel); }
    void SetAngularVelocity(const Vector3& vel) { if (IsValid()) m_world->SetAngularVelocity(m_id, vel); }
    void ApplyForce(const Vector3& force) { if (IsValid()) m_world->ApplyForce(m_id, force); }
    void ApplyImpulse(const Vector3& impulse) { if (IsValid()) m_world->ApplyImpulse(m_id, impulse); }
    void ApplyTorque(const Vector3& torque) { if (IsValid()) m_world->ApplyTorque(m_id, torque); }
    void SetMotionType(MotionType3D type) { if (IsValid()) m_world->SetMotionType(m_id, type); }

    Vector3 GetPosition() const { return IsValid() ? m_world->GetPosition(m_id) : Vector3{}; }
    Quaternion GetRotation() const { return IsValid() ? m_world->GetRotation(m_id) : Quaternion{}; }
    Vector3 GetLinearVelocity() const { return IsValid() ? m_world->GetLinearVelocity(m_id) : Vector3{}; }
    Matrix4x4 GetWorldTransform() const { return IsValid() ? m_world->GetWorldTransform(m_id) : Matrix4x4{}; }
    bool IsActive() const { return IsValid() && m_world->IsActive(m_id); }

    bool SetShape(PhysicsShape* shape, bool updateMassProperties = true, bool activate = true)
    {
        if (!IsValid() || !shape)
            return false;
        return m_world->SetBodyShape(m_id, shape, updateMassProperties, activate);
    }

private:
    PhysicsWorld3D* m_world = nullptr;
    PhysicsBodyID   m_id;
};

} // namespace GX
