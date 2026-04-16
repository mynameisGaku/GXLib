/// @file SpringBone.cpp
/// @brief スプリングボーンシミュレーションの実装
#include "pch_graphics.h"
#include "Graphics/3D/SpringBone.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "Math/MathUtil.h"
#include "Math/MathConvert.h"

namespace gx
{

// ---------------------------------------------------------------------------
// ヘルパー関数
// ---------------------------------------------------------------------------

/// @brief 点を球コライダーから押し出す
static Vector3 ResolveSphereCollision(const Vector3& pos, float boneRadius,
                                       const SpringCollider& collider)
{
    Vector3 diff = pos - collider.position;
    float dist = diff.Length();
    float minDist = collider.radius + boneRadius;
    if (dist < minDist && dist > 1e-6f)
    {
        Vector3 normal = diff * (1.0f / dist);
        return collider.position + normal * minDist;
    }
    return pos;
}

/// @brief 線分上の最近接点を計算する
static Vector3 ClosestPointOnSegment(const Vector3& point,
                                      const Vector3& segA, const Vector3& segB)
{
    Vector3 ab = segB - segA;
    float abLenSq = ab.LengthSquared();
    if (abLenSq < 1e-10f)
        return segA;

    float t = (point - segA).Dot(ab) / abLenSq;
    t = MathUtil::Clamp(t, 0.0f, 1.0f);
    return segA + ab * t;
}

/// @brief 点をカプセルコライダーから押し出す
static Vector3 ResolveCapsuleCollision(const Vector3& pos, float boneRadius,
                                        const SpringCollider& collider)
{
    Vector3 closest = ClosestPointOnSegment(pos, collider.position, collider.endPosition);
    Vector3 diff = pos - closest;
    float dist = diff.Length();
    float minDist = collider.radius + boneRadius;
    if (dist < minDist && dist > 1e-6f)
    {
        Vector3 normal = diff * (1.0f / dist);
        return closest + normal * minDist;
    }
    return pos;
}

/// @brief グローバル変換行列からワールド位置を取得する
static Vector3 GetWorldPosition(const Matrix4x4& globalTransform)
{
    return Vector3(globalTransform._41, globalTransform._42, globalTransform._43);
}

// ---------------------------------------------------------------------------
// SpringBone
// ---------------------------------------------------------------------------

void SpringBone::AddBone(const SpringBoneConfig& config)
{
    BoneState state;
    state.config = config;
    state.currentPosition = Vector3::Zero();
    state.previousPosition = Vector3::Zero();
    state.velocity = Vector3::Zero();
    state.initialRotation = Quaternion::Identity();
    state.initialized = false;
    m_bones.push_back(state);
}

void SpringBone::AddChain(const gx::Vector<int>& boneIndices, float stiffness, float damping)
{
    for (int idx : boneIndices)
    {
        SpringBoneConfig config;
        config.boneIndex = idx;
        config.stiffness = stiffness;
        config.damping = damping;
        AddBone(config);
    }
}

void SpringBone::AddCollider(const SpringCollider& collider)
{
    m_colliders.push_back(collider);
}

void SpringBone::RemoveBone(int boneIndex)
{
    m_bones.erase(
        std::remove_if(m_bones.begin(), m_bones.end(),
                        [boneIndex](const BoneState& bs) {
                            return bs.config.boneIndex == boneIndex;
                        }),
        m_bones.end());
}

void SpringBone::RemoveAllBones()
{
    m_bones.clear();
}

void SpringBone::ClearColliders()
{
    m_colliders.clear();
}

void SpringBone::SetGravity(float gravity)
{
    for (auto& bone : m_bones)
        bone.config.gravity = gravity;
}

void SpringBone::SetWindForce(const Vector3& wind)
{
    m_windForce = wind;
}

void SpringBone::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool SpringBone::IsEnabled() const
{
    return m_enabled;
}

size_t SpringBone::GetBoneCount() const
{
    return m_bones.size();
}

size_t SpringBone::GetColliderCount() const
{
    return m_colliders.size();
}

void SpringBone::Reset()
{
    for (auto& bone : m_bones)
    {
        bone.initialized = false;
        bone.velocity = Vector3::Zero();
    }
}

void SpringBone::Update(float deltaTime, const Skeleton& skeleton,
                          gx::Vector<TransformTRS>& localTransforms,
                          const gx::Vector<Matrix4x4>& globalTransforms)
{
    if (!m_enabled || deltaTime <= 0.0f)
        return;

    const auto& joints = skeleton.GetJoints();
    const uint32_t jointCount = skeleton.GetJointCount();

    // dt を安全な範囲にクランプ
    float dt = MathUtil::Clamp(deltaTime, 0.0f, 1.0f / 30.0f);

    for (auto& bone : m_bones)
    {
        int boneIdx = bone.config.boneIndex;
        if (boneIdx < 0 || boneIdx >= static_cast<int>(jointCount))
            continue;
        if (boneIdx >= static_cast<int>(globalTransforms.size()))
            continue;

        // ワールド位置をグローバル変換から取得
        Vector3 worldPos = GetWorldPosition(globalTransforms[boneIdx]);

        // 初回初期化
        if (!bone.initialized)
        {
            bone.currentPosition = worldPos;
            bone.previousPosition = worldPos;
            bone.velocity = Vector3::Zero();
            bone.initialRotation.x = localTransforms[boneIdx].rotation.x;
            bone.initialRotation.y = localTransforms[boneIdx].rotation.y;
            bone.initialRotation.z = localTransforms[boneIdx].rotation.z;
            bone.initialRotation.w = localTransforms[boneIdx].rotation.w;
            bone.initialized = true;
            continue;
        }

        const auto& cfg = bone.config;

        // 親ボーンのワールド位置を取得（レスト位置の参照点として使用）
        Vector3 parentWorldPos = Vector3::Zero();
        int parentIdx = joints[boneIdx].parentIndex;
        if (parentIdx >= 0 && parentIdx < static_cast<int>(globalTransforms.size()))
        {
            parentWorldPos = GetWorldPosition(globalTransforms[parentIdx]);
        }

        // --- Verlet積分 ---
        // 重力
        Vector3 gravityForce = cfg.gravityDirection.Normalized() * cfg.gravity * (-1.0f);
        // 質量が0以下にならないよう保護
        float mass = (std::max)(cfg.mass, 0.01f);

        // ばね力: レスト位置（アニメーション上のワールド位置）に引き戻す
        Vector3 displacement = bone.currentPosition - worldPos;
        Vector3 springForce = displacement * (-cfg.stiffness);

        // 減衰力
        Vector3 dampingForce = bone.velocity * (-cfg.damping);

        // 合力
        Vector3 acceleration = (springForce + dampingForce + gravityForce + m_windForce) / mass;

        // Verlet積分: newPos = cur + (cur - prev) * (1 - drag) + accel * dt^2
        float drag = MathUtil::Clamp(cfg.dragForce, 0.0f, 1.0f);
        Vector3 inertia = (bone.currentPosition - bone.previousPosition) * (1.0f - drag);
        Vector3 newPos = bone.currentPosition + inertia + acceleration * (dt * dt);

        // 速度を更新
        bone.velocity = (newPos - bone.currentPosition) * (1.0f / dt);

        bone.previousPosition = bone.currentPosition;
        bone.currentPosition = newPos;

        // --- 角度制限 ---
        if (cfg.maxAngle > 0.0f && parentIdx >= 0)
        {
            // 親ボーンからの方向を制限
            Vector3 toChild = bone.currentPosition - parentWorldPos;
            Vector3 restDir = worldPos - parentWorldPos;
            float restLen = restDir.Length();
            float curLen = toChild.Length();

            if (restLen > 1e-6f && curLen > 1e-6f)
            {
                Vector3 restNorm = restDir * (1.0f / restLen);
                Vector3 curNorm = toChild * (1.0f / curLen);

                float cosAngle = MathUtil::Clamp(restNorm.Dot(curNorm), -1.0f, 1.0f);
                float angle = MathUtil::RadiansToDegrees(acosf(cosAngle));

                if (angle > cfg.maxAngle)
                {
                    // 回転軸
                    Vector3 axis = restNorm.Cross(curNorm);
                    float axisLen = axis.Length();
                    if (axisLen > 1e-6f)
                    {
                        axis = axis * (1.0f / axisLen);
                        float clampedRad = MathUtil::DegreesToRadians(cfg.maxAngle);
                        Quaternion rot = Quaternion::FromAxisAngle(axis, clampedRad);
                        Vector3 clampedDir = rot.RotateVector(restNorm);
                        bone.currentPosition = parentWorldPos + clampedDir * curLen;
                    }
                }
            }
        }

        // --- コライダー衝突応答 ---
        for (const auto& collider : m_colliders)
        {
            if (collider.type == SpringCollider::Sphere)
            {
                bone.currentPosition = ResolveSphereCollision(
                    bone.currentPosition, cfg.radius, collider);
            }
            else if (collider.type == SpringCollider::Capsule)
            {
                bone.currentPosition = ResolveCapsuleCollision(
                    bone.currentPosition, cfg.radius, collider);
            }
        }

        // --- ローカル変換を更新 ---
        // 親空間での目標方向を計算し、回転を調整する
        if (parentIdx >= 0 && parentIdx < static_cast<int>(globalTransforms.size()))
        {
            // 元のレスト方向（親空間）
            Vector3 restLocal(localTransforms[boneIdx].translation.x,
                              localTransforms[boneIdx].translation.y,
                              localTransforms[boneIdx].translation.z);
            float restLocalLen = restLocal.Length();

            if (restLocalLen > 1e-6f)
            {
                // 親の逆グローバル変換を使ってワールド→親ローカルに変換
                XMMATRIX parentGlobal = XMLoadFloat4x4(XM(&globalTransforms[parentIdx]));
                XMMATRIX parentGlobalInv = XMMatrixInverse(nullptr, parentGlobal);

                // シミュレーション後の位置を親ローカル空間に変換
                XMVECTOR simWorldPos = XMVectorSet(bone.currentPosition.x,
                                                    bone.currentPosition.y,
                                                    bone.currentPosition.z, 1.0f);
                XMVECTOR simLocalPos = XMVector3TransformCoord(simWorldPos, parentGlobalInv);
                Vector3 simLocalVec;
                XMStoreFloat3(XM(&simLocalVec), simLocalPos);
                float simLocalLen = simLocalVec.Length();

                if (simLocalLen > 1e-6f)
                {
                    // レスト方向からシミュレーション方向への回転を計算
                    Vector3 restNorm = restLocal * (1.0f / restLocalLen);
                    Vector3 simNorm = simLocalVec * (1.0f / simLocalLen);

                    float dot = MathUtil::Clamp(restNorm.Dot(simNorm), -1.0f, 1.0f);
                    if (dot < 0.9999f)
                    {
                        Vector3 axis = restNorm.Cross(simNorm);
                        float axisLen = axis.Length();
                        if (axisLen > 1e-6f)
                        {
                            axis = axis * (1.0f / axisLen);
                            float angle = acosf(dot);
                            Quaternion deltaRot = Quaternion::FromAxisAngle(axis, angle);

                            // 初期回転にdelta回転を適用
                            Quaternion newRot = bone.initialRotation * deltaRot;
                            newRot.Normalize();

                            localTransforms[boneIdx].rotation.x = newRot.x;
                            localTransforms[boneIdx].rotation.y = newRot.y;
                            localTransforms[boneIdx].rotation.z = newRot.z;
                            localTransforms[boneIdx].rotation.w = newRot.w;
                        }
                    }
                }
            }
        }
    }
}

} // namespace gx
