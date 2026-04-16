/// @file MotionMatching.cpp
/// @brief モーションマッチングシステムの実装
///
/// KD-Tree 高速化:
///   Build() でポーズ特徴量を事前計算した後、17 次元の KD-Tree を構築する。
///   FindBestMatch() は KD-Tree が構築済みなら O(log n) 近傍探索を使い、
///   未構築時は従来の O(n) 線形探索にフォールバックする。
#include "pch_graphics.h"
#include "Graphics/3D/MotionMatching.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/Skeleton.h"
#include "Math/MathUtil.h"
#include <algorithm>

namespace gx
{

// ===========================================================================
// MotionDatabase
// ===========================================================================

void MotionDatabase::AddClip(const AnimationClip* clip)
{
    if (clip)
        m_clips.push_back(clip);
}

size_t MotionDatabase::GetClipCount() const
{
    return m_clips.size();
}

void MotionDatabase::Build(const Skeleton& skeleton, const MotionMatchingConfig& config)
{
    m_poses.clear();
    m_kdNodes.clear();
    m_kdRoot  = -1;
    m_kdBuilt = false;

    const uint32_t jointCount = skeleton.GetJointCount();
    if (jointCount == 0)
        return;

    // サンプリング間隔
    float sampleInterval = 1.0f / (std::max)(config.sampleRate, 1.0f);

    // バインドポーズを取得（SampleTRSのベースとして使用）
    gx::Vector<TransformTRS> basePose(jointCount);
    const auto& joints = skeleton.GetJoints();
    for (uint32_t j = 0; j < jointCount; ++j)
        basePose[j] = DecomposeTRS(joints[j].localTransform);

    // ローカル→グローバル変換用のバッファ
    gx::Vector<TransformTRS> currentPose(jointCount);
    gx::Vector<TransformTRS> futurePose(jointCount);
    gx::Vector<Matrix4x4>  localMats(jointCount);
    gx::Vector<Matrix4x4>  globalMats(jointCount);

    for (size_t clipIdx = 0; clipIdx < m_clips.size(); ++clipIdx)
    {
        const AnimationClip* clip = m_clips[clipIdx];
        if (!clip)
            continue;

        float duration = clip->GetDuration();
        if (duration <= 0.0f)
            continue;

        // クリップを等間隔でサンプリング
        for (float t = 0.0f; t < duration; t += sampleInterval)
        {
            // 現在フレームをサンプリング
            clip->SampleTRS(t, jointCount, currentPose.data(), basePose.data());

            // ローカル行列を構築
            for (uint32_t j = 0; j < jointCount; ++j)
                localMats[j] = ComposeTRS(currentPose[j]);

            // グローバル変換を計算
            skeleton.ComputeGlobalTransforms(localMats.data(), globalMats.data());

            // 特徴量を構築
            MotionFeature feature{};

            // ルートの速度: (次フレーム位置 - 現フレーム位置) / dt
            float nextT = (std::min)(t + sampleInterval, duration);
            if (nextT > t)
            {
                gx::Vector<TransformTRS> nextPose(jointCount);
                clip->SampleTRS(nextT, jointCount, nextPose.data(), basePose.data());

                float dt = nextT - t;
                feature.velocity = Vector3(
                    (nextPose[0].translation.x - currentPose[0].translation.x) / dt,
                    (nextPose[0].translation.y - currentPose[0].translation.y) / dt,
                    (nextPose[0].translation.z - currentPose[0].translation.z) / dt);
            }

            // 将来位置の予測
            float futureT = (std::min)(t + config.futureTime, duration);
            clip->SampleTRS(futureT, jointCount, futurePose.data(), basePose.data());
            feature.futurePosition = Vector3(
                futurePose[0].translation.x,
                futurePose[0].translation.y,
                futurePose[0].translation.z);

            // 向き: ルートボーンの正面方向（グローバル行列のZ軸）
            feature.facingDirection = Vector3(
                globalMats[0]._31, globalMats[0]._32, globalMats[0]._33);
            float facingLen = feature.facingDirection.Length();
            if (facingLen > 1e-6f)
                feature.facingDirection = feature.facingDirection * (1.0f / facingLen);

            // 足ボーンの位置（設定されている場合）
            if (config.leftFootBone >= 0 && config.leftFootBone < static_cast<int>(jointCount))
            {
                feature.leftFootPosition = Vector3(
                    globalMats[config.leftFootBone]._41,
                    globalMats[config.leftFootBone]._42,
                    globalMats[config.leftFootBone]._43);
            }

            if (config.rightFootBone >= 0 && config.rightFootBone < static_cast<int>(jointCount))
            {
                feature.rightFootPosition = Vector3(
                    globalMats[config.rightFootBone]._41,
                    globalMats[config.rightFootBone]._42,
                    globalMats[config.rightFootBone]._43);
            }

            // 足の速度（簡易: 次フレームとの位置差分の大きさ）
            if (nextT > t)
            {
                gx::Vector<TransformTRS> nextPoseForFoot(jointCount);
                clip->SampleTRS(nextT, jointCount, nextPoseForFoot.data(), basePose.data());

                gx::Vector<Matrix4x4> nextLocalMats(jointCount);
                for (uint32_t j = 0; j < jointCount; ++j)
                    nextLocalMats[j] = ComposeTRS(nextPoseForFoot[j]);

                gx::Vector<Matrix4x4> nextGlobalMats(jointCount);
                skeleton.ComputeGlobalTransforms(nextLocalMats.data(), nextGlobalMats.data());

                float dt = nextT - t;

                if (config.leftFootBone >= 0 && config.leftFootBone < static_cast<int>(jointCount))
                {
                    Vector3 nextLeft(nextGlobalMats[config.leftFootBone]._41,
                                     nextGlobalMats[config.leftFootBone]._42,
                                     nextGlobalMats[config.leftFootBone]._43);
                    feature.leftFootVelocity = (nextLeft - feature.leftFootPosition).Length() / dt;
                }

                if (config.rightFootBone >= 0 && config.rightFootBone < static_cast<int>(jointCount))
                {
                    Vector3 nextRight(nextGlobalMats[config.rightFootBone]._41,
                                      nextGlobalMats[config.rightFootBone]._42,
                                      nextGlobalMats[config.rightFootBone]._43);
                    feature.rightFootVelocity = (nextRight - feature.rightFootPosition).Length() / dt;
                }
            }

            MotionPose pose;
            pose.clipIndex = static_cast<int>(clipIdx);
            pose.time = t;
            pose.feature = feature;
            m_poses.push_back(pose);
        }
    }

    // KD-Tree を構築する
    if (!m_poses.empty())
    {
        BuildKDTree();
    }
}

// ===========================================================================
// Cost function
// ===========================================================================

float MotionDatabase::ComputeCost(const MotionFeature& a, const MotionFeature& b,
                                    const MotionMatchingConfig& config)
{
    float cost = 0.0f;

    // 位置コスト: 将来位置の差の二乗
    if (config.positionWeight > 0.0f)
    {
        Vector3 posDiff = a.futurePosition - b.futurePosition;
        cost += config.positionWeight * posDiff.LengthSquared();
    }

    // 速度コスト
    if (config.velocityWeight > 0.0f)
    {
        Vector3 velDiff = a.velocity - b.velocity;
        cost += config.velocityWeight * velDiff.LengthSquared();
    }

    // 足位置コスト
    if (config.footWeight > 0.0f)
    {
        Vector3 leftDiff = a.leftFootPosition - b.leftFootPosition;
        Vector3 rightDiff = a.rightFootPosition - b.rightFootPosition;
        cost += config.footWeight * (leftDiff.LengthSquared() + rightDiff.LengthSquared());
    }

    // 向きコスト: 向きベクトルの角度差の二乗
    if (config.facingWeight > 0.0f)
    {
        // 向きベクトル間の角度を計算
        float dot = MathUtil::Clamp(a.facingDirection.Dot(b.facingDirection), -1.0f, 1.0f);
        float angle = acosf(dot);
        cost += config.facingWeight * (angle * angle);
    }

    return cost;
}

// ===========================================================================
// FindBestMatch — KD-Tree or linear fallback
// ===========================================================================

MotionPose MotionDatabase::FindBestMatch(const MotionFeature& query,
                                           const MotionMatchingConfig& config,
                                           float& outCost) const
{
    if (m_poses.empty())
    {
        outCost = FLT_MAX;
        return MotionPose{};
    }

    // ----- KD-Tree accelerated path -----
    if (m_kdBuilt && m_kdRoot >= 0)
    {
        float queryFlat[k_FeatureDims];
        FlattenFeature(query, queryFlat);

        int   bestIdx  = -1;
        float bestDist = FLT_MAX;
        KDSearch(m_kdRoot, queryFlat, bestIdx, bestDist);

        if (bestIdx >= 0 && bestIdx < static_cast<int>(m_poses.size()))
        {
            // Recompute the full weighted cost using ComputeCost for accuracy,
            // since the KD-Tree search uses unweighted L2 distance.
            outCost = ComputeCost(query, m_poses[bestIdx].feature, config);
            return m_poses[bestIdx];
        }
    }

    // ----- Linear fallback -----
    MotionPose bestPose;
    float bestCost = FLT_MAX;

    for (const auto& pose : m_poses)
    {
        float cost = ComputeCost(query, pose.feature, config);
        if (cost < bestCost)
        {
            bestCost = cost;
            bestPose = pose;
        }
    }

    outCost = bestCost;
    return bestPose;
}

// ===========================================================================
// KD-Tree construction
// ===========================================================================

void MotionDatabase::FlattenFeature(const MotionFeature& f, float out[k_FeatureDims])
{
    // 0-2:   velocity
    out[0]  = f.velocity.x;
    out[1]  = f.velocity.y;
    out[2]  = f.velocity.z;
    // 3-5:   futurePosition
    out[3]  = f.futurePosition.x;
    out[4]  = f.futurePosition.y;
    out[5]  = f.futurePosition.z;
    // 6-8:   facingDirection
    out[6]  = f.facingDirection.x;
    out[7]  = f.facingDirection.y;
    out[8]  = f.facingDirection.z;
    // 9-11:  leftFootPosition
    out[9]  = f.leftFootPosition.x;
    out[10] = f.leftFootPosition.y;
    out[11] = f.leftFootPosition.z;
    // 12-14: rightFootPosition
    out[12] = f.rightFootPosition.x;
    out[13] = f.rightFootPosition.y;
    out[14] = f.rightFootPosition.z;
    // 15:    leftFootVelocity
    out[15] = f.leftFootVelocity;
    // 16:    rightFootVelocity
    out[16] = f.rightFootVelocity;
}

float MotionDatabase::FeatureValue(int poseIndex, int axis) const
{
    const MotionFeature& f = m_poses[poseIndex].feature;
    switch (axis)
    {
    case 0:  return f.velocity.x;
    case 1:  return f.velocity.y;
    case 2:  return f.velocity.z;
    case 3:  return f.futurePosition.x;
    case 4:  return f.futurePosition.y;
    case 5:  return f.futurePosition.z;
    case 6:  return f.facingDirection.x;
    case 7:  return f.facingDirection.y;
    case 8:  return f.facingDirection.z;
    case 9:  return f.leftFootPosition.x;
    case 10: return f.leftFootPosition.y;
    case 11: return f.leftFootPosition.z;
    case 12: return f.rightFootPosition.x;
    case 13: return f.rightFootPosition.y;
    case 14: return f.rightFootPosition.z;
    case 15: return f.leftFootVelocity;
    case 16: return f.rightFootVelocity;
    default: return 0.0f;
    }
}

void MotionDatabase::BuildKDTree()
{
    const int n = static_cast<int>(m_poses.size());
    if (n == 0)
        return;

    m_kdNodes.clear();
    // Reserve a reasonable upper bound (2*n - 1 nodes for a balanced tree)
    m_kdNodes.reserve(static_cast<size_t>(n) * 2);

    gx::Vector<int> indices(n);
    for (int i = 0; i < n; ++i)
        indices[i] = i;

    m_kdRoot  = BuildKDTreeRecursive(indices, 0, n, 0);
    m_kdBuilt = true;
}

int MotionDatabase::BuildKDTreeRecursive(gx::Vector<int>& indices, int begin, int end, int depth)
{
    int count = end - begin;
    if (count <= 0)
        return -1;

    int nodeIdx = static_cast<int>(m_kdNodes.size());
    m_kdNodes.push_back(KDNode{});

    // Leaf node: single element
    if (count == 1)
    {
        m_kdNodes[nodeIdx].poseIndex  = indices[begin];
        m_kdNodes[nodeIdx].splitAxis  = 0;
        m_kdNodes[nodeIdx].splitValue = 0.0f;
        m_kdNodes[nodeIdx].left       = -1;
        m_kdNodes[nodeIdx].right      = -1;
        return nodeIdx;
    }

    // Choose split axis by cycling through dimensions
    int axis = depth % k_FeatureDims;

    // Sort the range by feature value on the chosen axis
    // Use std::nth_element for O(n) median selection
    int mid = begin + count / 2;
    std::nth_element(
        indices.begin() + begin,
        indices.begin() + mid,
        indices.begin() + end,
        [this, axis](int a, int b)
        {
            return FeatureValue(a, axis) < FeatureValue(b, axis);
        });

    float splitVal = FeatureValue(indices[mid], axis);

    // Store the median pose in this node (for nearest-neighbor backtracking)
    m_kdNodes[nodeIdx].poseIndex  = indices[mid];
    m_kdNodes[nodeIdx].splitAxis  = axis;
    m_kdNodes[nodeIdx].splitValue = splitVal;

    // Recurse left: [begin, mid)
    m_kdNodes[nodeIdx].left  = BuildKDTreeRecursive(indices, begin, mid, depth + 1);

    // Recurse right: [mid+1, end)
    m_kdNodes[nodeIdx].right = BuildKDTreeRecursive(indices, mid + 1, end, depth + 1);

    return nodeIdx;
}

// ===========================================================================
// KD-Tree search
// ===========================================================================

float MotionDatabase::FeatureDistSq(const float a[k_FeatureDims], const float b[k_FeatureDims])
{
    float sum = 0.0f;
    for (int i = 0; i < k_FeatureDims; ++i)
    {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

void MotionDatabase::KDSearch(int nodeIdx, const float query[k_FeatureDims],
                               int& bestIdx, float& bestDist) const
{
    if (nodeIdx < 0 || nodeIdx >= static_cast<int>(m_kdNodes.size()))
        return;

    const KDNode& node = m_kdNodes[nodeIdx];

    // Compute distance from this node's pose to the query
    if (node.poseIndex >= 0)
    {
        float nodeFeature[k_FeatureDims];
        FlattenFeature(m_poses[node.poseIndex].feature, nodeFeature);

        float dist = FeatureDistSq(query, nodeFeature);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestIdx  = node.poseIndex;
        }
    }

    // Leaf check: no children
    if (node.left < 0 && node.right < 0)
        return;

    int axis = node.splitAxis;
    float diff = query[axis] - node.splitValue;

    // Determine which side of the split plane to search first
    int nearChild  = (diff <= 0.0f) ? node.left  : node.right;
    int farChild   = (diff <= 0.0f) ? node.right : node.left;

    // Search the near subtree first
    KDSearch(nearChild, query, bestIdx, bestDist);

    // Check if the far subtree could contain a closer point.
    // The split plane distance squared must be less than current best.
    float planeDist = diff * diff;
    if (planeDist < bestDist)
    {
        KDSearch(farChild, query, bestIdx, bestDist);
    }
}

// ===========================================================================
// Pose queries
// ===========================================================================

size_t MotionDatabase::GetTotalPoseCount() const
{
    return m_poses.size();
}

const MotionPose& MotionDatabase::GetPose(size_t index) const
{
    return m_poses[index];
}

const AnimationClip* MotionDatabase::GetClip(size_t index) const
{
    if (index < m_clips.size())
        return m_clips[index];
    return nullptr;
}

// ===========================================================================
// MotionMatcher
// ===========================================================================

void MotionMatcher::SetDatabase(const MotionDatabase* database)
{
    m_database = database;
}

const MotionDatabase* MotionMatcher::GetDatabase() const
{
    return m_database;
}

void MotionMatcher::SetConfig(const MotionMatchingConfig& config)
{
    m_config = config;
}

const MotionMatchingConfig& MotionMatcher::GetConfig() const
{
    return m_config;
}

void MotionMatcher::SetAnimator(Animator* animator)
{
    m_animator = animator;
}

void MotionMatcher::SetDesiredVelocity(const Vector3& velocity)
{
    m_desiredVelocity = velocity;
}

Vector3 MotionMatcher::GetDesiredVelocity() const
{
    return m_desiredVelocity;
}

void MotionMatcher::SetDesiredFacing(const Vector3& direction)
{
    m_desiredFacing = direction;
}

Vector3 MotionMatcher::GetDesiredFacing() const
{
    return m_desiredFacing;
}

void MotionMatcher::Update(float deltaTime)
{
    if (!m_database || m_database->GetTotalPoseCount() == 0)
        return;

    m_timeSinceSwitch += deltaTime;
    m_currentTime += deltaTime;

    // 最小切り替え間隔を満たしていない場合はスキップ
    if (m_timeSinceSwitch < m_config.minSwitchInterval)
        return;

    // クエリ特徴量を構築
    MotionFeature query{};
    query.velocity = m_desiredVelocity;
    query.facingDirection = m_desiredFacing;

    // 将来位置を希望速度から予測
    query.futurePosition = m_desiredVelocity * m_config.futureTime;

    // 最良マッチを検索
    float cost = 0.0f;
    MotionPose bestPose = m_database->FindBestMatch(query, m_config, cost);

    // マッチが見つかった場合、クリップを切り替え
    if (bestPose.clipIndex >= 0)
    {
        bool shouldSwitch = (m_currentClipIndex != bestPose.clipIndex) ||
                             (fabsf(m_currentTime - bestPose.time) > 0.1f);

        if (shouldSwitch)
        {
            m_currentClipIndex = bestPose.clipIndex;
            m_currentTime = bestPose.time;
            m_timeSinceSwitch = 0.0f;

            // Animatorに反映
            if (m_animator)
            {
                const AnimationClip* clip = m_database->GetClip(bestPose.clipIndex);
                if (clip)
                {
                    m_animator->Play(clip, true);
                    m_animator->SetCurrentTime(bestPose.time);
                }
            }
        }
    }

    m_lastCost = cost;
}

float MotionMatcher::GetLastMatchCost() const
{
    return m_lastCost;
}

int MotionMatcher::GetCurrentClipIndex() const
{
    return m_currentClipIndex;
}

float MotionMatcher::GetCurrentTime() const
{
    return m_currentTime;
}

void MotionMatcher::Reset()
{
    m_timeSinceSwitch = 0.0f;
    m_lastCost = 0.0f;
    m_currentClipIndex = -1;
    m_currentTime = 0.0f;
    m_desiredVelocity = Vector3::Zero();
    m_desiredFacing = Vector3(0, 0, 1);
}

} // namespace gx
