/// @file CharacterController.cpp
/// @brief CharacterController の実装
#include "pch_common.h"
#include "Physics/CharacterController.h"
#include "Physics/PhysicsWorld3D.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Core/TempAllocator.h>

JPH_SUPPRESS_WARNINGS

namespace {
    inline JPH::Vec3 ToJolt(const gx::Vector3& v) { return JPH::Vec3(v.x, v.y, v.z); }
    inline JPH::Quat ToJolt(const gx::Quaternion& q) { return JPH::Quat(q.x, q.y, q.z, q.w); }
    inline gx::Vector3 FromJoltV(const JPH::Vec3& v) { return { v.GetX(), v.GetY(), v.GetZ() }; }
    inline gx::Vector3 FromJoltR(const JPH::RVec3& v) { return { static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()) }; }
    inline gx::Quaternion FromJoltQ(const JPH::Quat& q) { return { q.GetX(), q.GetY(), q.GetZ(), q.GetW() }; }
}

namespace gx {

struct CharacterController::Impl
{
    JPH::Ref<JPH::CharacterVirtual> character;
    JPH::PhysicsSystem* physicsSystem = nullptr;
    JPH::TempAllocator* tempAllocator = nullptr;
    CharacterControllerSettings settings;
    Vector3 movementInput;      ///< フレーム毎の移動入力（XZ平面）
    float verticalVelocity = 0.0f; ///< 垂直方向の速度（ジャンプ/落下）
    bool isRunning = false;     ///< 走行モード
    bool initialized = false;
};

CharacterController::CharacterController() : m_impl(std::make_unique<Impl>()) {}
CharacterController::~CharacterController() { Shutdown(); }

bool CharacterController::Initialize(PhysicsWorld3D* world, const CharacterControllerSettings& settings, const Vector3& initialPos)
{
    if (!world) return false;

    m_impl->physicsSystem = static_cast<JPH::PhysicsSystem*>(world->GetInternalPhysicsSystem());
    m_impl->tempAllocator = static_cast<JPH::TempAllocator*>(world->GetInternalTempAllocator());
    if (!m_impl->physicsSystem || !m_impl->tempAllocator) return false;

    m_impl->settings = settings;

    // カプセル形状（足元がY=0になるようオフセット）
    JPH::RefConst<JPH::Shape> capsule = new JPH::CapsuleShape(settings.capsuleHalfHeight, settings.capsuleRadius);
    JPH::RefConst<JPH::Shape> shape = new JPH::RotatedTranslatedShape(
        JPH::Vec3(0, settings.capsuleHalfHeight + settings.capsuleRadius, 0),
        JPH::Quat::sIdentity(),
        capsule
    );

    JPH::CharacterVirtualSettings charSettings;
    charSettings.mShape = shape;
    charSettings.mMaxSlopeAngle = DirectX::XMConvertToRadians(settings.maxSlopeAngle);
    charSettings.mMass = settings.mass;
    charSettings.mMaxStrength = 100.0f;
    charSettings.mPenetrationRecoverySpeed = 1.0f;

    m_impl->character = new JPH::CharacterVirtual(
        &charSettings,
        JPH::RVec3(initialPos.x, initialPos.y, initialPos.z),
        JPH::Quat::sIdentity(),
        0,
        m_impl->physicsSystem
    );

    m_impl->initialized = true;
    return true;
}

void CharacterController::Shutdown()
{
    if (!m_impl->initialized) return;
    m_impl->character = nullptr;
    m_impl->initialized = false;
}

void CharacterController::Update(float deltaTime)
{
    if (!m_impl->initialized) return;

    // 高レベル移動: 移動入力から水平速度を計算
    float speed = m_impl->isRunning ? m_impl->settings.runSpeed : m_impl->settings.walkSpeed;
    Vector3 horizontalVel = m_impl->movementInput * speed;

    // 重力を垂直速度に適用
    bool onGround = (GetGroundState() == CharacterGroundState::OnGround);
    if (onGround)
    {
        // 地面上: 垂直速度をリセット（ジャンプ中を除く）
        if (m_impl->verticalVelocity < 0.0f)
            m_impl->verticalVelocity = 0.0f;
    }
    else
    {
        m_impl->verticalVelocity += m_impl->settings.gravity * deltaTime;
    }

    // 合成速度を設定
    Vector3 finalVel(horizontalVel.x, m_impl->verticalVelocity, horizontalVel.z);
    m_impl->character->SetLinearVelocity(ToJolt(finalVel));

    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
    updateSettings.mStickToFloorStepDown = JPH::Vec3(0, -m_impl->settings.stepDownHeight, 0);
    updateSettings.mWalkStairsStepUp = JPH::Vec3(0, m_impl->settings.stepHeight, 0);

    // 全レイヤーと衝突する簡易フィルター（キャラクターは全オブジェクトと衝突判定）
    struct AllBPFilter : public JPH::BroadPhaseLayerFilter {
        bool ShouldCollide(JPH::BroadPhaseLayer) const override { return true; }
    };
    struct AllObjFilter : public JPH::ObjectLayerFilter {
        bool ShouldCollide(JPH::ObjectLayer) const override { return true; }
    };
    AllBPFilter bpFilter;
    AllObjFilter objFilter;
    JPH::BodyFilter bodyFilter;
    JPH::ShapeFilter shapeFilter;

    m_impl->character->ExtendedUpdate(
        deltaTime,
        JPH::Vec3(0, m_impl->settings.gravity, 0),
        updateSettings,
        bpFilter,
        objFilter,
        bodyFilter,
        shapeFilter,
        *m_impl->tempAllocator
    );

    // 移動入力をリセット（毎フレーム再設定が必要）
    m_impl->movementInput = {};
}

void CharacterController::SetPosition(const Vector3& pos)
{
    if (!m_impl->initialized) return;
    m_impl->character->SetPosition(JPH::RVec3(pos.x, pos.y, pos.z));
}

void CharacterController::SetRotation(const Quaternion& rot)
{
    if (!m_impl->initialized) return;
    m_impl->character->SetRotation(ToJolt(rot));
}

void CharacterController::SetLinearVelocity(const Vector3& vel)
{
    if (!m_impl->initialized) return;
    m_impl->character->SetLinearVelocity(ToJolt(vel));
}

Vector3 CharacterController::GetPosition() const
{
    if (!m_impl->initialized) return {};
    return FromJoltR(m_impl->character->GetPosition());
}

Quaternion CharacterController::GetRotation() const
{
    if (!m_impl->initialized) return {};
    return FromJoltQ(m_impl->character->GetRotation());
}

Vector3 CharacterController::GetLinearVelocity() const
{
    if (!m_impl->initialized) return {};
    return FromJoltV(m_impl->character->GetLinearVelocity());
}

Matrix4x4 CharacterController::GetWorldTransform() const
{
    if (!m_impl->initialized) return {};

    auto pos = m_impl->character->GetPosition();
    auto rot = m_impl->character->GetRotation();

    XMVECTOR q = XMVectorSet(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());
    XMMATRIX rotMat = XMMatrixRotationQuaternion(q);
    XMMATRIX transMat = XMMatrixTranslation(
        static_cast<float>(pos.GetX()),
        static_cast<float>(pos.GetY()),
        static_cast<float>(pos.GetZ())
    );

    Matrix4x4 result;
    XMStoreFloat4x4(XM(&result), XMMatrixMultiply(rotMat, transMat));
    return result;
}

CharacterGroundState CharacterController::GetGroundState() const
{
    if (!m_impl->initialized) return CharacterGroundState::InAir;

    auto joltState = m_impl->character->GetGroundState();
    switch (joltState)
    {
    case JPH::CharacterVirtual::EGroundState::OnGround:       return CharacterGroundState::OnGround;
    case JPH::CharacterVirtual::EGroundState::OnSteepGround:
    {
        // 急斜面上で垂直速度が下向きなら「滑っている」状態
        if (m_impl->verticalVelocity < -0.1f)
            return CharacterGroundState::Sliding;
        return CharacterGroundState::OnSteepGround;
    }
    case JPH::CharacterVirtual::EGroundState::NotSupported:    return CharacterGroundState::NotSupported;
    case JPH::CharacterVirtual::EGroundState::InAir:          return CharacterGroundState::InAir;
    default: return CharacterGroundState::InAir;
    }
}

bool CharacterController::IsOnGround() const
{
    return GetGroundState() == CharacterGroundState::OnGround;
}

void CharacterController::SetMovementInput(const Vector3& input)
{
    if (!m_impl->initialized) return;
    m_impl->movementInput = input;
}

void CharacterController::Jump()
{
    if (!m_impl->initialized) return;
    if (!IsOnGround()) return;
    m_impl->verticalVelocity = m_impl->settings.jumpForce;
}

void CharacterController::SetRunning(bool running)
{
    if (!m_impl->initialized) return;
    m_impl->isRunning = running;
}

Vector3 CharacterController::GetGroundNormal() const
{
    if (!m_impl->initialized) return { 0.0f, 1.0f, 0.0f };

    auto joltState = m_impl->character->GetGroundState();
    if (joltState == JPH::CharacterVirtual::EGroundState::InAir)
        return { 0.0f, 1.0f, 0.0f };

    JPH::Vec3 normal = m_impl->character->GetGroundNormal();
    return FromJoltV(normal);
}

float CharacterController::GetHeight() const
{
    if (!m_impl->initialized) return 0.0f;
    return m_impl->settings.capsuleHalfHeight;
}

float CharacterController::GetRadius() const
{
    if (!m_impl->initialized) return 0.0f;
    return m_impl->settings.capsuleRadius;
}

void CharacterController::SetWalkSpeed(float speed)
{
    if (!m_impl->initialized) return;
    m_impl->settings.walkSpeed = speed;
}

void CharacterController::SetRunSpeed(float speed)
{
    if (!m_impl->initialized) return;
    m_impl->settings.runSpeed = speed;
}

void CharacterController::SetJumpForce(float force)
{
    if (!m_impl->initialized) return;
    m_impl->settings.jumpForce = force;
}

void CharacterController::SetGravity(float g)
{
    if (!m_impl->initialized) return;
    m_impl->settings.gravity = g;
}

} // namespace gx
