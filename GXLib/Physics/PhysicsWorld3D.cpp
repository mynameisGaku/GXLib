#include "pch.h"
#include "Physics/PhysicsWorld3D.h"
#include "Core/Logger.h"

// Jolt Physics ヘッダー（PIMPLのためここでのみインクルード）
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

JPH_SUPPRESS_WARNINGS

// Joltトレースコールバック（デバッグコンソールへ出力）
static void JoltTraceImpl(const char* inFMT, ...)
{
    va_list args;
    va_start(args, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, args);
    va_end(args);
    OutputDebugStringA("[Jolt] ");
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

#ifdef JPH_ENABLE_ASSERTS
static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "[Jolt Assert] %s:%u: %s (%s)\n", inFile, inLine, inExpression, inMessage ? inMessage : "");
    OutputDebugStringA(buffer);
    return true; // trueでデバッガ中断
}
#endif

namespace {

// 変換ヘルパー
inline JPH::Vec3 ToJolt(const GX::Vector3& v) { return JPH::Vec3(v.x, v.y, v.z); }
inline JPH::Quat ToJolt(const GX::Quaternion& q) { return JPH::Quat(q.x, q.y, q.z, q.w); }
inline GX::Vector3 FromJoltV(const JPH::Vec3& v) { return { v.GetX(), v.GetY(), v.GetZ() }; }
inline GX::Vector3 FromJoltR(const JPH::RVec3& v) { return { static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()) }; }
inline GX::Quaternion FromJoltQ(const JPH::Quat& q) { return { q.GetX(), q.GetY(), q.GetZ(), q.GetW() }; }

// ブロードフェーズ用レイヤー
namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint32_t NUM_LAYERS = 2;
}

// オブジェクトレイヤー
namespace ObjectLayers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr uint32_t NUM_LAYERS = 2;
}

class BPLayerInterface final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterface()
    {
        m_objectToBroadPhase[ObjectLayers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_objectToBroadPhase[ObjectLayers::MOVING] = BroadPhaseLayers::MOVING;
    }

    JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        assert(inLayer < ObjectLayers::NUM_LAYERS);
        return m_objectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:     return "MOVING";
        default: return "UNKNOWN";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer m_objectToBroadPhase[ObjectLayers::NUM_LAYERS];
};

class ObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer inObject, JPH::BroadPhaseLayer inBroadPhaseLayer) const override
    {
        if (inObject == ObjectLayers::NON_MOVING)
            return inBroadPhaseLayer == BroadPhaseLayers::MOVING;
        return true;
    }
};

class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        if (inObject1 == ObjectLayers::NON_MOVING && inObject2 == ObjectLayers::NON_MOVING)
            return false;
        return true;
    }
};

class ContactListenerImpl final : public JPH::ContactListener
{
public:
    GX::PhysicsWorld3D* world = nullptr;

    JPH::ValidateResult OnContactValidate(const JPH::Body&, const JPH::Body&,
        JPH::RVec3Arg, const JPH::CollideShapeResult&) override
    {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold, JPH::ContactSettings&) override
    {
        if (world && world->onContactAdded)
        {
            GX::PhysicsBodyID id1, id2;
            id1.id = inBody1.GetID().GetIndexAndSequenceNumber();
            id2.id = inBody2.GetID().GetIndexAndSequenceNumber();
            GX::Vector3 contactPt = FromJoltR(inManifold.GetWorldSpaceContactPointOn1(0));
            world->onContactAdded(id1, id2, contactPt);
        }
    }

    void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
    {
        if (world && world->onContactRemoved)
        {
            GX::PhysicsBodyID id1, id2;
            id1.id = inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber();
            id2.id = inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber();
            world->onContactRemoved(id1, id2);
        }
    }
};

} // anonymous namespace

namespace GX {

struct PhysicsWorld3D::Impl {
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem;
    std::unique_ptr<JPH::PhysicsSystem> physicsSystem;
    BPLayerInterface bpLayerInterface;
    ObjectVsBroadPhaseFilter objectVsBPFilter;
    ObjectLayerPairFilter objectPairFilter;
    ContactListenerImpl contactListener;
    std::vector<PhysicsShape*> ownedShapes;
    bool initialized = false;
};

PhysicsWorld3D::PhysicsWorld3D() : m_impl(std::make_unique<Impl>()) {}

PhysicsWorld3D::~PhysicsWorld3D()
{
    Shutdown();
}

bool PhysicsWorld3D::Initialize(uint32_t maxBodies)
{
    if (m_impl->initialized) return true;

    JPH::RegisterDefaultAllocator();
    JPH::Trace = JoltTraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailedImpl;)
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    try
    {
        m_impl->tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(32 * 1024 * 1024); // 32MB
        m_impl->jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
            std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1)
        );
    }
    catch (const std::exception& e)
    {
        GX_LOG_ERROR("PhysicsWorld3D: failed to allocate Jolt subsystems: %s", e.what());
        m_impl->tempAllocator.reset();
        m_impl->jobSystem.reset();
        return false;
    }

    const uint32_t maxBodyPairs = std::min(maxBodies * 2, 65536u);
    const uint32_t maxContactConstraints = std::min(maxBodies * 2, 10240u);

    m_impl->physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    m_impl->physicsSystem->Init(
        maxBodies,             // 最大ボディ数
        0,                     // ボディミューテックス数（0=既定）
        maxBodyPairs,          // 最大ボディペア数
        maxContactConstraints, // 最大接触制約数
        m_impl->bpLayerInterface,
        m_impl->objectVsBPFilter,
        m_impl->objectPairFilter
    );

    m_impl->contactListener.world = this;
    m_impl->physicsSystem->SetContactListener(&m_impl->contactListener);

    m_impl->initialized = true;
    return true;
}

void PhysicsWorld3D::Shutdown()
{
    if (!m_impl->initialized) return;

    for (auto* shape : m_impl->ownedShapes)
    {
        if (shape)
        {
            if (shape->internal)
            {
                auto* ref = static_cast<JPH::ShapeRefC*>(shape->internal);
                delete ref;
            }
            delete shape;
        }
    }
    m_impl->ownedShapes.clear();

    m_impl->physicsSystem.reset();
    m_impl->jobSystem.reset();
    m_impl->tempAllocator.reset();

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    m_impl->initialized = false;
}

void PhysicsWorld3D::Step(float deltaTime)
{
    if (!m_impl->initialized) return;

    int collisionSteps = 1;
    m_impl->physicsSystem->Update(deltaTime, collisionSteps,
        m_impl->tempAllocator.get(), m_impl->jobSystem.get());
}

void PhysicsWorld3D::SetGravity(const Vector3& gravity)
{
    if (m_impl->physicsSystem)
        m_impl->physicsSystem->SetGravity(ToJolt(gravity));
}

Vector3 PhysicsWorld3D::GetGravity() const
{
    if (m_impl->physicsSystem)
        return FromJoltV(m_impl->physicsSystem->GetGravity());
    return { 0, -9.81f, 0 };
}

PhysicsShape* PhysicsWorld3D::CreateBoxShape(const Vector3& halfExtents)
{
    if (halfExtents.x <= 0.0f || halfExtents.y <= 0.0f || halfExtents.z <= 0.0f)
    {
        GX_LOG_ERROR("PhysicsWorld3D: Invalid box half extents");
        return nullptr;
    }
    auto* shape = new PhysicsShape();
    auto jphShape = new JPH::ShapeRefC(new JPH::BoxShape(ToJolt(halfExtents)));
    shape->internal = jphShape;
    m_impl->ownedShapes.push_back(shape);
    return shape;
}

PhysicsShape* PhysicsWorld3D::CreateSphereShape(float radius)
{
    if (radius <= 0.0f)
    {
        GX_LOG_ERROR("PhysicsWorld3D: Invalid sphere radius");
        return nullptr;
    }
    auto* shape = new PhysicsShape();
    auto jphShape = new JPH::ShapeRefC(new JPH::SphereShape(radius));
    shape->internal = jphShape;
    m_impl->ownedShapes.push_back(shape);
    return shape;
}

PhysicsShape* PhysicsWorld3D::CreateCapsuleShape(float halfHeight, float radius)
{
    if (halfHeight <= 0.0f || radius <= 0.0f)
    {
        GX_LOG_ERROR("PhysicsWorld3D: Invalid capsule dimensions");
        return nullptr;
    }
    auto* shape = new PhysicsShape();
    auto jphShape = new JPH::ShapeRefC(new JPH::CapsuleShape(halfHeight, radius));
    shape->internal = jphShape;
    m_impl->ownedShapes.push_back(shape);
    return shape;
}

PhysicsShape* PhysicsWorld3D::CreateMeshShape(const Vector3* vertices, uint32_t vertexCount,
                                                const uint32_t* indices, uint32_t indexCount)
{
    JPH::TriangleList triangles;
    for (uint32_t i = 0; i + 2 < indexCount; i += 3)
    {
        const Vector3& v0 = vertices[indices[i]];
        const Vector3& v1 = vertices[indices[i + 1]];
        const Vector3& v2 = vertices[indices[i + 2]];
        triangles.push_back(JPH::Triangle(
            JPH::Float3(v0.x, v0.y, v0.z),
            JPH::Float3(v1.x, v1.y, v1.z),
            JPH::Float3(v2.x, v2.y, v2.z)
        ));
    }

    auto* shape = new PhysicsShape();
    JPH::MeshShapeSettings settings(triangles);
    auto result = settings.Create();
    if (result.IsValid())
    {
        auto jphShape = new JPH::ShapeRefC(result.Get());
        shape->internal = jphShape;
    }
    m_impl->ownedShapes.push_back(shape);
    return shape;
}

PhysicsShape* PhysicsWorld3D::CreateConvexHullShape(const Vector3* vertices, uint32_t vertexCount,
                                                     float maxConvexRadius)
{
    if (!vertices || vertexCount == 0)
        return nullptr;

    JPH::Array<JPH::Vec3> points;
    points.reserve(vertexCount);
    for (uint32_t i = 0; i < vertexCount; ++i)
        points.push_back(ToJolt(vertices[i]));

    auto* shape = new PhysicsShape();
    float radius = (maxConvexRadius > 0.0f) ? maxConvexRadius : JPH::cDefaultConvexRadius;
    JPH::ConvexHullShapeSettings settings(points, radius);
    auto result = settings.Create();
    if (result.IsValid())
    {
        auto jphShape = new JPH::ShapeRefC(result.Get());
        shape->internal = jphShape;
    }
    m_impl->ownedShapes.push_back(shape);
    return shape;
}

PhysicsBodyID PhysicsWorld3D::AddBody(PhysicsShape* shape, const PhysicsBodySettings& settings)
{
    PhysicsBodyID result;
    if (!m_impl->initialized || !shape || !shape->internal) return result;

    auto* shapeRef = static_cast<JPH::ShapeRefC*>(shape->internal);

    JPH::EMotionType motionType;
    JPH::ObjectLayer layer;
    switch (settings.motionType)
    {
    case MotionType3D::Static:    motionType = JPH::EMotionType::Static;    layer = ObjectLayers::NON_MOVING; break;
    case MotionType3D::Kinematic: motionType = JPH::EMotionType::Kinematic; layer = ObjectLayers::MOVING; break;
    default:                      motionType = JPH::EMotionType::Dynamic;   layer = ObjectLayers::MOVING; break;
    }

    JPH::BodyCreationSettings bodySettings(
        *shapeRef,
        JPH::RVec3(settings.position.x, settings.position.y, settings.position.z),
        ToJolt(settings.rotation),
        motionType,
        layer
    );
    bodySettings.mFriction = settings.friction;
    bodySettings.mRestitution = settings.restitution;
    bodySettings.mLinearDamping = settings.linearDamping;
    bodySettings.mAngularDamping = settings.angularDamping;
    if (settings.motionType == MotionType3D::Dynamic && settings.mass > 0.0f)
    {
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        bodySettings.mMassPropertiesOverride.mMass = settings.mass;
    }

    JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();
    JPH::BodyID bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    if (bodyID.IsInvalid()) return result;
    result.id = bodyID.GetIndexAndSequenceNumber();
    return result;
}

void PhysicsWorld3D::RemoveBody(PhysicsBodyID id)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();
    JPH::BodyID bodyID(id.id);
    bodyInterface.RemoveBody(bodyID);
    bodyInterface.DestroyBody(bodyID);
}

void PhysicsWorld3D::DestroyShape(PhysicsShape* shape)
{
    if (!shape) return;
    auto it = std::find(m_impl->ownedShapes.begin(), m_impl->ownedShapes.end(), shape);
    if (it != m_impl->ownedShapes.end())
    {
        if (shape->internal)
        {
            auto* ref = static_cast<JPH::ShapeRefC*>(shape->internal);
            delete ref;
        }
        delete shape;
        m_impl->ownedShapes.erase(it);
    }
}

void PhysicsWorld3D::SetPosition(PhysicsBodyID id, const Vector3& pos)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    m_impl->physicsSystem->GetBodyInterface().SetPosition(
        JPH::BodyID(id.id), JPH::RVec3(pos.x, pos.y, pos.z), JPH::EActivation::Activate);
}

void PhysicsWorld3D::SetRotation(PhysicsBodyID id, const Quaternion& rot)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    m_impl->physicsSystem->GetBodyInterface().SetRotation(
        JPH::BodyID(id.id), ToJolt(rot), JPH::EActivation::Activate);
}

void PhysicsWorld3D::SetLinearVelocity(PhysicsBodyID id, const Vector3& vel)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    m_impl->physicsSystem->GetBodyInterface().SetLinearVelocity(JPH::BodyID(id.id), ToJolt(vel));
}

void PhysicsWorld3D::SetAngularVelocity(PhysicsBodyID id, const Vector3& vel)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    m_impl->physicsSystem->GetBodyInterface().SetAngularVelocity(JPH::BodyID(id.id), ToJolt(vel));
}

void PhysicsWorld3D::ApplyForce(PhysicsBodyID id, const Vector3& force)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    m_impl->physicsSystem->GetBodyInterface().AddForce(JPH::BodyID(id.id), ToJolt(force));
}

void PhysicsWorld3D::ApplyImpulse(PhysicsBodyID id, const Vector3& impulse)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    m_impl->physicsSystem->GetBodyInterface().AddImpulse(JPH::BodyID(id.id), ToJolt(impulse));
}

void PhysicsWorld3D::ApplyTorque(PhysicsBodyID id, const Vector3& torque)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    m_impl->physicsSystem->GetBodyInterface().AddTorque(JPH::BodyID(id.id), ToJolt(torque));
}

void PhysicsWorld3D::SetMotionType(PhysicsBodyID id, MotionType3D type)
{
    if (!m_impl->initialized || !id.IsValid()) return;
    JPH::EMotionType mt;
    switch (type)
    {
    case MotionType3D::Static:    mt = JPH::EMotionType::Static; break;
    case MotionType3D::Kinematic: mt = JPH::EMotionType::Kinematic; break;
    default:                      mt = JPH::EMotionType::Dynamic; break;
    }
    m_impl->physicsSystem->GetBodyInterface().SetMotionType(
        JPH::BodyID(id.id), mt, JPH::EActivation::Activate);
}

bool PhysicsWorld3D::SetBodyShape(PhysicsBodyID id, PhysicsShape* shape,
                                  bool updateMassProperties, bool activate)
{
    if (!m_impl->initialized || !id.IsValid() || !shape || !shape->internal)
        return false;

    auto* shapeRef = static_cast<JPH::ShapeRefC*>(shape->internal);
    JPH::BodyInterface& bodyInterface = m_impl->physicsSystem->GetBodyInterface();
    bodyInterface.SetShape(
        JPH::BodyID(id.id),
        shapeRef->GetPtr(),
        updateMassProperties,
        activate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    return true;
}

Vector3 PhysicsWorld3D::GetPosition(PhysicsBodyID id) const
{
    if (!m_impl->initialized || !id.IsValid()) return {};
    return FromJoltR(m_impl->physicsSystem->GetBodyInterface().GetPosition(JPH::BodyID(id.id)));
}

Quaternion PhysicsWorld3D::GetRotation(PhysicsBodyID id) const
{
    if (!m_impl->initialized || !id.IsValid()) return {};
    return FromJoltQ(m_impl->physicsSystem->GetBodyInterface().GetRotation(JPH::BodyID(id.id)));
}

Vector3 PhysicsWorld3D::GetLinearVelocity(PhysicsBodyID id) const
{
    if (!m_impl->initialized || !id.IsValid()) return {};
    return FromJoltV(m_impl->physicsSystem->GetBodyInterface().GetLinearVelocity(JPH::BodyID(id.id)));
}

Matrix4x4 PhysicsWorld3D::GetWorldTransform(PhysicsBodyID id) const
{
    if (!m_impl->initialized || !id.IsValid()) return {};

    JPH::BodyInterface& bi = m_impl->physicsSystem->GetBodyInterface();
    JPH::RVec3 pos = bi.GetPosition(JPH::BodyID(id.id));
    JPH::Quat rot = bi.GetRotation(JPH::BodyID(id.id));

    XMVECTOR q = XMVectorSet(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());
    XMMATRIX rotMat = XMMatrixRotationQuaternion(q);
    XMMATRIX transMat = XMMatrixTranslation(
        static_cast<float>(pos.GetX()),
        static_cast<float>(pos.GetY()),
        static_cast<float>(pos.GetZ())
    );

    return Matrix4x4::FromXMMATRIX(XMMatrixMultiply(rotMat, transMat));
}

bool PhysicsWorld3D::IsActive(PhysicsBodyID id) const
{
    if (!m_impl->initialized || !id.IsValid()) return false;
    return m_impl->physicsSystem->GetBodyInterface().IsActive(JPH::BodyID(id.id));
}

PhysicsWorld3D::RaycastResult PhysicsWorld3D::Raycast(const Vector3& origin, const Vector3& direction, float maxDistance)
{
    RaycastResult result;
    if (!m_impl->initialized) return result;

    JPH::RRayCast ray(
        JPH::RVec3(origin.x, origin.y, origin.z),
        JPH::Vec3(direction.x * maxDistance, direction.y * maxDistance, direction.z * maxDistance)
    );

    JPH::RayCastResult hit;
    if (m_impl->physicsSystem->GetNarrowPhaseQuery().CastRay(ray, hit))
    {
        result.hit = true;
        result.fraction = hit.mFraction;
        result.bodyID.id = hit.mBodyID.GetIndexAndSequenceNumber();

        JPH::RVec3 hitPoint = ray.GetPointOnRay(hit.mFraction);
        result.point = FromJoltR(hitPoint);

        // ヒット点の法線を取得
        JPH::BodyLockRead lock(m_impl->physicsSystem->GetBodyLockInterface(), hit.mBodyID);
        if (lock.Succeeded())
        {
            JPH::Vec3 normal = lock.GetBody().GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, hitPoint);
            result.normal = FromJoltV(normal);
        }
    }

    return result;
}

} // namespace GX
