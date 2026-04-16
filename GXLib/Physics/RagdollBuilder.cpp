/// @file RagdollBuilder.cpp
/// @brief RagdollBuilder の実装
#include "pch_common.h"
#include "Physics/RagdollBuilder.h"
#include "Core/Logger.h"

namespace gx {

RagdollBuilder::RagdollBuilder() = default;
RagdollBuilder::~RagdollBuilder() = default;

bool RagdollBuilder::Build(PhysicsWorld3D* world, RagdollPreset preset, const Vector3& position, float scale)
{
    if (!world || m_built) return false;

    switch (preset)
    {
    case RagdollPreset::Humanoid:
        BuildHumanoid(world, position, scale);
        break;
    case RagdollPreset::Custom:
        GX_LOG_WARN("RagdollBuilder: Custom preset requires BuildCustom()");
        return false;
    }

    m_built = !m_bones.empty();
    return m_built;
}

bool RagdollBuilder::BuildCustom(PhysicsWorld3D* world, const gx::Vector<RagdollBone>& bones,
                                  const gx::Vector<std::pair<int,int>>& connections, const Vector3& position)
{
    if (!world || m_built || bones.empty()) return false;

    m_bones = bones;

    // 各ボーンのカプセルボディを作成
    for (size_t i = 0; i < m_bones.size(); ++i)
    {
        auto& bone = m_bones[i];
        Vector3 bonePos = position + bone.offset;

        PhysicsShape* shape = world->CreateCapsuleShape(bone.length, bone.radius);
        if (!shape)
        {
            GX_LOG_ERROR("RagdollBuilder: Failed to create capsule shape for bone '%s'", bone.name.c_str());
            continue;
        }

        PhysicsBodySettings bodySettings;
        bodySettings.position = bonePos;
        bodySettings.motionType = MotionType3D::Dynamic;
        bodySettings.mass = bone.mass;
        bodySettings.friction = 0.5f;
        bodySettings.restitution = 0.1f;
        bodySettings.linearDamping = 0.1f;
        bodySettings.angularDamping = 0.3f;
        bodySettings.layer = 1;

        bone.bodyID = world->AddBody(shape, bodySettings);
    }

    // コンストレイントを作成
    for (auto& [idxA, idxB] : connections)
    {
        if (idxA < 0 || idxA >= static_cast<int>(m_bones.size()) ||
            idxB < 0 || idxB >= static_cast<int>(m_bones.size()))
            continue;

        if (!m_bones[idxA].bodyID.IsValid() || !m_bones[idxB].bodyID.IsValid())
            continue;

        RagdollJoint joint;
        joint.boneA = idxA;
        joint.boneB = idxB;

        ConeConstraintSettings coneSettings;
        coneSettings.point1 = {};
        coneSettings.twistAxis1 = { 0.0f, 1.0f, 0.0f };
        coneSettings.point2 = {};
        coneSettings.twistAxis2 = { 0.0f, 1.0f, 0.0f };
        coneSettings.halfConeAngle = DirectX::XMConvertToRadians(joint.coneAngle);

        joint.constraintID = world->CreateConeConstraint(
            m_bones[idxA].bodyID, m_bones[idxB].bodyID, coneSettings);
        m_joints.push_back(joint);
    }

    m_built = true;
    return true;
}

void RagdollBuilder::Destroy(PhysicsWorld3D* world)
{
    if (!world || !m_built) return;

    // コンストレイントを先に破棄
    for (auto& joint : m_joints)
    {
        if (joint.constraintID.IsValid())
            world->DestroyConstraint(joint.constraintID);
    }
    m_joints.clear();

    // ボディを破棄
    for (auto& bone : m_bones)
    {
        if (bone.bodyID.IsValid())
            world->RemoveBody(bone.bodyID);
    }
    m_bones.clear();

    m_built = false;
}

void RagdollBuilder::SetEnabled(PhysicsWorld3D* world, bool enabled)
{
    if (!world || !m_built) return;

    for (auto& bone : m_bones)
    {
        if (!bone.bodyID.IsValid()) continue;

        if (enabled)
        {
            // Dynamicに戻してアクティブ化（位置セットでActivateされる）
            world->SetMotionType(bone.bodyID, MotionType3D::Dynamic);
        }
        else
        {
            // Staticにして凍結
            world->SetMotionType(bone.bodyID, MotionType3D::Static);
        }
    }
}

void RagdollBuilder::ApplyImpulse(PhysicsWorld3D* world, int boneIndex, const Vector3& impulse)
{
    if (!world || !m_built) return;
    if (boneIndex < 0 || boneIndex >= static_cast<int>(m_bones.size())) return;
    if (!m_bones[boneIndex].bodyID.IsValid()) return;

    world->ApplyImpulse(m_bones[boneIndex].bodyID, impulse);
}

gx::Vector<Matrix4x4> RagdollBuilder::GetBoneTransforms(PhysicsWorld3D* world) const
{
    gx::Vector<Matrix4x4> transforms;
    if (!world || !m_built) return transforms;

    transforms.reserve(m_bones.size());
    for (auto& bone : m_bones)
    {
        if (bone.bodyID.IsValid())
        {
            transforms.push_back(world->GetWorldTransform(bone.bodyID));
        }
        else
        {
            transforms.push_back(Matrix4x4{});
        }
    }
    return transforms;
}

void RagdollBuilder::BuildHumanoid(PhysicsWorld3D* world, const Vector3& position, float scale)
{
    // 人型ラグドール: 15ボーン
    // hips, spine, chest, head,
    // left_upper_arm, left_forearm, left_hand,
    // right_upper_arm, right_forearm, right_hand,
    // left_thigh, left_shin, left_foot,
    // right_thigh, right_shin

    struct BoneDef {
        const char* name;
        int parent;
        Vector3 offset;  // 親からの相対オフセット (スケール前)
        float length;     // カプセル半高
        float radius;     // カプセル半径
        float mass;
        float coneAngle;  // 親との制約コーン角 (度)
    };

    // Y-up, 身長約1.75m の人型（ヒップ高さ ≈ 0.9m）
    const BoneDef defs[] = {
        // name             parent  offset(from parent)           halfH   radius  mass   cone
        { "hips",           -1,     { 0.0f, 0.90f, 0.0f },       0.10f,  0.10f,  8.0f,  0.0f },
        { "spine",           0,     { 0.0f, 0.15f, 0.0f },       0.12f,  0.09f,  7.0f,  20.0f },
        { "chest",           1,     { 0.0f, 0.20f, 0.0f },       0.12f,  0.10f,  8.0f,  15.0f },
        { "head",            2,     { 0.0f, 0.25f, 0.0f },       0.10f,  0.09f,  5.0f,  25.0f },
        { "left_upper_arm",  2,     { 0.20f, 0.15f, 0.0f },      0.13f,  0.04f,  3.0f,  45.0f },
        { "left_forearm",    4,     { 0.28f, 0.0f,  0.0f },      0.12f,  0.03f,  2.0f,  30.0f },
        { "left_hand",       5,     { 0.25f, 0.0f,  0.0f },      0.06f,  0.03f,  1.0f,  20.0f },
        { "right_upper_arm", 2,     { -0.20f, 0.15f, 0.0f },     0.13f,  0.04f,  3.0f,  45.0f },
        { "right_forearm",   7,     { -0.28f, 0.0f,  0.0f },     0.12f,  0.03f,  2.0f,  30.0f },
        { "right_hand",      8,     { -0.25f, 0.0f,  0.0f },     0.06f,  0.03f,  1.0f,  20.0f },
        { "left_thigh",      0,     { 0.10f, -0.05f, 0.0f },     0.20f,  0.06f,  5.0f,  35.0f },
        { "left_shin",      10,     { 0.0f, -0.42f, 0.0f },      0.18f,  0.05f,  3.0f,  25.0f },
        { "left_foot",      11,     { 0.0f, -0.40f, 0.05f },     0.06f,  0.04f,  1.0f,  15.0f },
        { "right_thigh",     0,     { -0.10f, -0.05f, 0.0f },    0.20f,  0.06f,  5.0f,  35.0f },
        { "right_shin",     13,     { 0.0f, -0.42f, 0.0f },      0.18f,  0.05f,  3.0f,  25.0f },
    };

    constexpr int numBones = static_cast<int>(std::size(defs));
    m_bones.resize(numBones);

    // 各ボーンの絶対位置を計算してボディを作成
    gx::Vector<Vector3> absolutePositions(numBones);

    for (int i = 0; i < numBones; ++i)
    {
        const auto& def = defs[i];
        auto& bone = m_bones[i];

        bone.name = def.name;
        bone.parentIndex = def.parent;
        bone.offset = def.offset * scale;
        bone.length = def.length * scale;
        bone.radius = def.radius * scale;
        bone.mass = def.mass;

        // 絶対位置を計算
        if (def.parent >= 0)
            absolutePositions[i] = absolutePositions[def.parent] + bone.offset;
        else
            absolutePositions[i] = position + bone.offset;

        // カプセルシェイプを作成
        PhysicsShape* shape = world->CreateCapsuleShape(bone.length, bone.radius);
        if (!shape)
        {
            GX_LOG_ERROR("RagdollBuilder: Failed to create shape for bone '%s'", bone.name.c_str());
            continue;
        }

        PhysicsBodySettings bodySettings;
        bodySettings.position = absolutePositions[i];
        bodySettings.motionType = MotionType3D::Dynamic;
        bodySettings.mass = bone.mass;
        bodySettings.friction = 0.5f;
        bodySettings.restitution = 0.1f;
        bodySettings.linearDamping = 0.1f;
        bodySettings.angularDamping = 0.3f;
        bodySettings.layer = 1;

        bone.bodyID = world->AddBody(shape, bodySettings);
    }

    // 隣接ボーン間のコーンコンストレイントを作成
    for (int i = 0; i < numBones; ++i)
    {
        int parentIdx = defs[i].parent;
        if (parentIdx < 0) continue;
        if (!m_bones[parentIdx].bodyID.IsValid() || !m_bones[i].bodyID.IsValid()) continue;

        float coneAngleRad = DirectX::XMConvertToRadians(defs[i].coneAngle);
        if (coneAngleRad <= 0.0f) continue;

        RagdollJoint joint;
        joint.boneA = parentIdx;
        joint.boneB = i;
        joint.coneAngle = defs[i].coneAngle;

        // ツイスト軸: 親ボーンから子ボーンへの方向
        Vector3 dir = m_bones[i].offset;
        float dirLen = dir.Length();
        Vector3 twistAxis = (dirLen > 0.001f) ? dir / dirLen : Vector3(0.0f, 1.0f, 0.0f);

        ConeConstraintSettings coneSettings;
        coneSettings.point1 = {};       // 親ボディのローカル原点
        coneSettings.twistAxis1 = twistAxis;
        coneSettings.point2 = {};       // 子ボディのローカル原点
        coneSettings.twistAxis2 = twistAxis;
        coneSettings.halfConeAngle = coneAngleRad;

        joint.constraintID = world->CreateConeConstraint(
            m_bones[parentIdx].bodyID, m_bones[i].bodyID, coneSettings);
        m_joints.push_back(joint);
    }
}

} // namespace gx
