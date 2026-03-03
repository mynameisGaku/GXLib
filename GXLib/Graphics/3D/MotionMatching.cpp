/// @file MotionMatching.cpp
/// @brief モーションマッチングシステムの実装
#include "pch_graphics.h"
#include "Graphics/3D/MotionMatching.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/Skeleton.h"
#include "Math/MathUtil.h"

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
}

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

MotionPose MotionDatabase::FindBestMatch(const MotionFeature& query,
                                           const MotionMatchingConfig& config,
                                           float& outCost) const
{
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
