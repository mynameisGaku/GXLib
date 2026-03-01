#pragma once
/// @file RVOSolver.h
/// @brief エージェント同士の衝突回避（RVO：相互速度障害物法）
///
/// 複数のNavAgentが同じ場所に重なりそうなとき、互いを避ける速度を計算する。
/// ヘッダーオンリーで、大量のエージェントが密集する場面でも
/// 自然な回避行動を生み出す軽量な実装。
/// @addtogroup grp_ai/// @{

#include "pch_common.h"

namespace gx { namespace RVO {

/// @brief RVO計算用のエージェント状態
struct AgentState
{
    XMFLOAT3 position = { 0,0,0 };  ///< 現在のワールド座標
    XMFLOAT3 velocity = { 0,0,0 };  ///< 現在の速度
    float radius = 0.5f;             ///< エージェントの衝突半径
    float maxSpeed = 3.5f;           ///< 最大速度
};

/// @brief RVOを使用して回避速度を計算する
/// @param self このエージェントの状態
/// @param desiredVelocity このエージェントが達成したい速度
/// @param others 他の近隣エージェント全て
/// @param timeHorizon 衝突を考慮する未来の時間範囲（秒）
/// @return 衝突を回避する安全速度
inline XMFLOAT3 ComputeAvoidanceVelocity(
    const AgentState& self,
    XMFLOAT3 desiredVelocity,
    const std::vector<AgentState>& others,
    float timeHorizon = 2.0f)
{
    if (others.empty())
        return desiredVelocity;

    // 2D（XZ平面）で処理
    float vx = desiredVelocity.x;
    float vz = desiredVelocity.z;

    for (const auto& other : others)
    {
        // 相対位置（other - self）
        float relX = other.position.x - self.position.x;
        float relZ = other.position.z - self.position.z;
        float distSq = relX * relX + relZ * relZ;
        float combinedRadius = self.radius + other.radius;
        float combinedRadiusSq = combinedRadius * combinedRadius;

        // 遠すぎる場合はスキップ（時間ホライズン内に衝突しない）
        float maxDist = self.maxSpeed * timeHorizon + combinedRadius;
        if (distSq > maxDist * maxDist)
            continue;

        // 重なっている場合はスキップ（押し分けは別処理）
        if (distSq < 0.001f)
            continue;

        float dist = std::sqrt(distSq);

        // 相対速度（self - other、相手の視点で平均化: RVO）
        float relVelX = vx - other.velocity.x;
        float relVelZ = vz - other.velocity.z;

        // 相対位置から相対速度へのベクトル
        float cutoffCenterX = relX / timeHorizon;
        float cutoffCenterZ = relZ / timeHorizon;
        float cutoffRadius = combinedRadius / timeHorizon;

        float wX = relVelX - cutoffCenterX;
        float wZ = relVelZ - cutoffCenterZ;
        float wLenSq = wX * wX + wZ * wZ;
        float wLen = std::sqrt(wLenSq);

        if (wLen < 0.0001f) continue;

        // 相対速度が速度障害物の内側にあるか確認
        if (wLen < cutoffRadius)
        {
            // 外側に押し出す
            float pushX = wX / wLen;
            float pushZ = wZ / wLen;
            float pushDist = (cutoffRadius - wLen) * 0.5f; // 半分の責任

            vx += pushX * pushDist;
            vz += pushZ * pushDist;
        }
    }

    // 最大速度にクランプ
    float speedSq = vx * vx + vz * vz;
    if (speedSq > self.maxSpeed * self.maxSpeed)
    {
        float s = self.maxSpeed / std::sqrt(speedSq);
        vx *= s;
        vz *= s;
    }

    return { vx, desiredVelocity.y, vz };
}

}} // namespace gx::RVO
/// @}
