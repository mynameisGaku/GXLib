/// @file MeshCollider.cpp
/// @brief メッシュコライダーヘルパーの実装
#include "pch.h"
#include "Physics/MeshCollider.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/AnimationPlayer.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/Vertex3D.h"
#include <unordered_map>

namespace GX
{

namespace
{

struct QuantKey
{
    int32_t x;
    int32_t y;
    int32_t z;

    bool operator==(const QuantKey& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct QuantKeyHash
{
    size_t operator()(const QuantKey& k) const
    {
        size_t h = static_cast<size_t>(k.x) * 73856093u;
        h ^= static_cast<size_t>(k.y) * 19349663u;
        h ^= static_cast<size_t>(k.z) * 83492791u;
        return h;
    }
};

static void DeduplicateVertices(std::vector<Vector3>& vertices, float weld)
{
    if (vertices.empty() || weld <= 0.0f)
        return;

    std::unordered_map<QuantKey, uint32_t, QuantKeyHash> map;
    std::vector<Vector3> unique;
    unique.reserve(vertices.size());

    const float inv = 1.0f / weld;
    for (const auto& v : vertices)
    {
        QuantKey key{
            static_cast<int32_t>(std::round(v.x * inv)),
            static_cast<int32_t>(std::round(v.y * inv)),
            static_cast<int32_t>(std::round(v.z * inv))
        };

        if (map.find(key) == map.end())
        {
            map[key] = static_cast<uint32_t>(unique.size());
            unique.push_back(v);
        }
    }

    vertices.swap(unique);
}

static void ReducePoints(std::vector<Vector3>& vertices, uint32_t maxPoints)
{
    if (maxPoints == 0 || maxPoints > 256)
        maxPoints = 256;
    if (vertices.size() <= maxPoints)
        return;

    std::vector<Vector3> reduced;
    reduced.reserve(maxPoints);

    size_t step = vertices.size() / maxPoints;
    if (step == 0) step = 1;

    for (size_t i = 0; i < vertices.size() && reduced.size() < maxPoints; i += step)
        reduced.push_back(vertices[i]);

    vertices.swap(reduced);
}

static bool CollectModelVertices(const Model& model,
                                 std::vector<Vector3>& outVertices,
                                 std::vector<uint32_t>& outIndices)
{
    const MeshCPUData* cpu = model.GetCPUData();
    if (!cpu)
        return false;

    outIndices = cpu->indices;

    if (model.IsSkinned())
    {
        outVertices.resize(cpu->skinnedVertices.size());
        for (size_t i = 0; i < cpu->skinnedVertices.size(); ++i)
        {
            const auto& v = cpu->skinnedVertices[i].position;
            outVertices[i] = { v.x, v.y, v.z };
        }
    }
    else
    {
        outVertices.resize(cpu->staticVertices.size());
        for (size_t i = 0; i < cpu->staticVertices.size(); ++i)
        {
            const auto& v = cpu->staticVertices[i].position;
            outVertices[i] = { v.x, v.y, v.z };
        }
    }

    if (outIndices.empty())
    {
        outIndices.resize(outVertices.size());
        for (uint32_t i = 0; i < static_cast<uint32_t>(outVertices.size()); ++i)
            outIndices[i] = i;
    }
    return !outVertices.empty();
}

static bool BuildBoneMatrices(const Model& model,
                              const std::vector<XMFLOAT4X4>& globalTransforms,
                              std::vector<XMMATRIX>& outBones)
{
    const Skeleton* skeleton = model.GetSkeleton();
    if (!skeleton)
        return false;

    uint32_t jointCount = skeleton->GetJointCount();
    if (jointCount == 0)
        return false;

    std::vector<XMFLOAT4X4> globals = globalTransforms;
    if (globals.size() != jointCount)
    {
        globals.resize(jointCount);
        const auto& joints = skeleton->GetJoints();
        std::vector<XMFLOAT4X4> locals(jointCount);
        for (uint32_t i = 0; i < jointCount; ++i)
            locals[i] = joints[i].localTransform;
        skeleton->ComputeGlobalTransforms(locals.data(), globals.data());
    }

    outBones.resize(jointCount);
    const auto& joints = skeleton->GetJoints();
    for (uint32_t i = 0; i < jointCount; ++i)
    {
        XMMATRIX invBind = XMLoadFloat4x4(&joints[i].inverseBindMatrix);
        XMMATRIX global  = XMLoadFloat4x4(&globals[i]);
        outBones[i] = invBind * global;
    }
    return true;
}

static bool BakeSkinnedVertices(const Model& model,
                                const std::vector<XMFLOAT4X4>& globalTransforms,
                                std::vector<Vector3>& outVertices,
                                std::vector<uint32_t>& outIndices)
{
    const MeshCPUData* cpu = model.GetCPUData();
    if (!cpu || cpu->skinnedVertices.empty())
        return false;

    std::vector<XMMATRIX> bones;
    if (!BuildBoneMatrices(model, globalTransforms, bones))
        return false;

    outIndices = cpu->indices;
    outVertices.resize(cpu->skinnedVertices.size());

    for (size_t i = 0; i < cpu->skinnedVertices.size(); ++i)
    {
        const auto& vtx = cpu->skinnedVertices[i];
        XMVECTOR pos = XMVectorSet(vtx.position.x, vtx.position.y, vtx.position.z, 1.0f);

        XMVECTOR skinned = XMVectorZero();
        const uint32_t joints[4] = { vtx.joints.x, vtx.joints.y, vtx.joints.z, vtx.joints.w };
        const float weights[4] = { vtx.weights.x, vtx.weights.y, vtx.weights.z, vtx.weights.w };

        for (int k = 0; k < 4; ++k)
        {
            if (weights[k] <= 0.0f)
                continue;
            if (joints[k] >= bones.size())
                continue;
            XMVECTOR p = XMVector3TransformCoord(pos, bones[joints[k]]);
            skinned = XMVectorAdd(skinned, XMVectorScale(p, weights[k]));
        }

        XMFLOAT3 out;
        XMStoreFloat3(&out, skinned);
        outVertices[i] = { out.x, out.y, out.z };
    }

    if (outIndices.empty())
    {
        outIndices.resize(outVertices.size());
        for (uint32_t i = 0; i < static_cast<uint32_t>(outVertices.size()); ++i)
            outIndices[i] = i;
    }
    return !outVertices.empty();
}

static PhysicsShape* CreateShapeFromData(PhysicsWorld3D& world,
                                         std::vector<Vector3> vertices,
                                         const std::vector<uint32_t>& indices,
                                         const MeshColliderDesc& desc)
{
    if (vertices.empty())
        return nullptr;

    if (desc.type == MeshColliderType::Convex)
    {
        if (desc.optimize)
            DeduplicateVertices(vertices, desc.weldTolerance);
        ReducePoints(vertices, desc.maxConvexVertices);
        return world.CreateConvexHullShape(vertices.data(),
                                           static_cast<uint32_t>(vertices.size()),
                                           desc.maxConvexRadius);
    }

    if (indices.empty())
        return nullptr;

    return world.CreateMeshShape(vertices.data(),
                                 static_cast<uint32_t>(vertices.size()),
                                 indices.data(),
                                 static_cast<uint32_t>(indices.size()));
}

} // namespace

bool MeshCollider::BuildFromModel(PhysicsWorld3D& world, const Model& model, const MeshColliderDesc& desc)
{
    std::vector<Vector3> vertices;
    std::vector<uint32_t> indices;
    if (!CollectModelVertices(model, vertices, indices))
        return false;

    PhysicsShape* newShape = CreateShapeFromData(world, std::move(vertices), indices, desc);
    if (!newShape || !newShape->internal)
    {
        if (newShape)
            world.DestroyShape(newShape);
        return false;
    }

    if (m_shape)
        world.DestroyShape(m_shape);
    m_shape = newShape;
    return true;
}

bool MeshCollider::BuildFromSkinnedModel(PhysicsWorld3D& world, const Model& model,
                                         const Animator& animator, const MeshColliderDesc& desc)
{
    std::vector<Vector3> vertices;
    std::vector<uint32_t> indices;
    if (!BakeSkinnedVertices(model, animator.GetGlobalTransforms(), vertices, indices))
        return false;

    PhysicsShape* newShape = CreateShapeFromData(world, std::move(vertices), indices, desc);
    if (!newShape || !newShape->internal)
    {
        if (newShape)
            world.DestroyShape(newShape);
        return false;
    }

    if (m_shape)
        world.DestroyShape(m_shape);
    m_shape = newShape;
    return true;
}

bool MeshCollider::BuildFromSkinnedModel(PhysicsWorld3D& world, const Model& model,
                                         const AnimationPlayer& player, const MeshColliderDesc& desc)
{
    std::vector<Vector3> vertices;
    std::vector<uint32_t> indices;
    if (!BakeSkinnedVertices(model, player.GetGlobalTransforms(), vertices, indices))
        return false;

    PhysicsShape* newShape = CreateShapeFromData(world, std::move(vertices), indices, desc);
    if (!newShape || !newShape->internal)
    {
        if (newShape)
            world.DestroyShape(newShape);
        return false;
    }

    if (m_shape)
        world.DestroyShape(m_shape);
    m_shape = newShape;
    return true;
}

bool MeshCollider::UpdateFromSkinnedModel(PhysicsWorld3D& world, PhysicsBodyID body,
                                          const Model& model, const Animator& animator,
                                          const MeshColliderDesc& desc, bool activate)
{
    std::vector<Vector3> vertices;
    std::vector<uint32_t> indices;
    if (!BakeSkinnedVertices(model, animator.GetGlobalTransforms(), vertices, indices))
        return false;

    PhysicsShape* newShape = CreateShapeFromData(world, std::move(vertices), indices, desc);
    if (!newShape || !newShape->internal)
    {
        if (newShape)
            world.DestroyShape(newShape);
        return false;
    }

    if (!world.SetBodyShape(body, newShape, true, activate))
    {
        world.DestroyShape(newShape);
        return false;
    }

    if (m_shape)
        world.DestroyShape(m_shape);
    m_shape = newShape;
    return true;
}

bool MeshCollider::UpdateFromSkinnedModel(PhysicsWorld3D& world, PhysicsBodyID body,
                                          const Model& model, const AnimationPlayer& player,
                                          const MeshColliderDesc& desc, bool activate)
{
    std::vector<Vector3> vertices;
    std::vector<uint32_t> indices;
    if (!BakeSkinnedVertices(model, player.GetGlobalTransforms(), vertices, indices))
        return false;

    PhysicsShape* newShape = CreateShapeFromData(world, std::move(vertices), indices, desc);
    if (!newShape || !newShape->internal)
    {
        if (newShape)
            world.DestroyShape(newShape);
        return false;
    }

    if (!world.SetBodyShape(body, newShape, true, activate))
    {
        world.DestroyShape(newShape);
        return false;
    }

    if (m_shape)
        world.DestroyShape(m_shape);
    m_shape = newShape;
    return true;
}

void MeshCollider::Release(PhysicsWorld3D& world)
{
    if (m_shape)
    {
        world.DestroyShape(m_shape);
        m_shape = nullptr;
    }
}

} // namespace GX
