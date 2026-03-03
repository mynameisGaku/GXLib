/// @file ProceduralAnimation.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Graphics/3D/ProceduralAnimation.h"
#include "Math/MathUtil.h"

using namespace DirectX;

namespace gx {

// ============================================================================
// 定数
// ============================================================================
static constexpr float k_TwoPi = MathUtil::TAU;

// ============================================================================
// AddLeg
// ============================================================================
int ProceduralAnimator::AddLeg(const LegConfig& config)
{
    LegData data;
    data.config = config;
    data.state  = {};
    data.state.footWorldPosition  = config.footOffset;
    data.state.targetFootPosition = config.footOffset;
    data.liftoffPos = config.footOffset;
    data.landingPos = config.footOffset;
    m_legs.push_back(std::move(data));
    return static_cast<int>(m_legs.size()) - 1;
}

// ============================================================================
// GetLegState
// ============================================================================
LegState ProceduralAnimator::GetLegState(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_legs.size()))
        return m_legs[static_cast<size_t>(index)].state;
    return {};
}

// ============================================================================
// GetFootTarget
// ============================================================================
Vector3 ProceduralAnimator::GetFootTarget(int legIndex) const
{
    if (legIndex >= 0 && legIndex < static_cast<int>(m_legs.size()))
        return m_legs[static_cast<size_t>(legIndex)].state.targetFootPosition;
    return { 0, 0, 0 };
}

// ============================================================================
// Reset
// ============================================================================
void ProceduralAnimator::Reset()
{
    m_globalPhase = 0.0f;
    m_bodyOffset  = { 0, 0, 0 };
    m_bodyTilt    = { 0, 0, 0 };
    m_bobSpring   = SpringDamper();
    m_swaySpring  = SpringDamper();

    for (auto& leg : m_legs)
    {
        leg.state = {};
        leg.state.footWorldPosition  = leg.config.footOffset;
        leg.state.targetFootPosition = leg.config.footOffset;
        leg.liftoffPos = leg.config.footOffset;
        leg.landingPos = leg.config.footOffset;
    }
}

// ============================================================================
// Update
// ============================================================================
void ProceduralAnimator::Update(float deltaTime)
{
    if (!m_enabled) return;
    if (deltaTime <= 0.0f) return;

    const int legCount = static_cast<int>(m_legs.size());
    float cycleTime = m_gait.cycleTime;
    if (cycleTime <= 0.0f) cycleTime = 1.0f;

    // ------- 1. グローバル位相を進める -------
    float phaseAdvance = deltaTime / cycleTime;
    // 移動速度でスケーリング: 速度が0なら位相は進めない（静止）
    float speedFactor = (m_movementSpeed > 0.01f) ? 1.0f : 0.0f;
    m_globalPhase += phaseAdvance * speedFactor;
    // [0, 1)にラップ
    m_globalPhase = std::fmod(m_globalPhase, 1.0f);
    if (m_globalPhase < 0.0f) m_globalPhase += 1.0f;

    // ------- 2. 各脚の位相と状態を更新 -------
    for (int i = 0; i < legCount; ++i)
    {
        auto& leg = m_legs[static_cast<size_t>(i)];

        // 脚の位相 = グローバル位相 + オフセット
        float phaseOffset = 0.0f;
        if (i < static_cast<int>(m_gait.legPhaseOffsets.size()))
            phaseOffset = m_gait.legPhaseOffsets[static_cast<size_t>(i)];

        float legPhase = std::fmod(m_globalPhase + phaseOffset, 1.0f);
        if (legPhase < 0.0f) legPhase += 1.0f;

        float prevPhase = leg.state.phase;
        leg.state.phase = legPhase;

        // Stance/Swing判定
        bool wasGrounded = leg.state.isGrounded;
        bool isStance = (legPhase < m_gait.dutyFactor);
        leg.state.isGrounded = isStance;

        if (isStance)
        {
            // --- スタンスフェーズ: 足は地面に置いたまま ---
            leg.state.stepProgress = 0.0f;

            // スタンス中の足位置: 身体の移動に合わせて後方へスライド
            float stanceProgress = legPhase / m_gait.dutyFactor;
            float slideBack = -leg.config.stepLength * 0.5f * (2.0f * stanceProgress - 1.0f);
            leg.state.targetFootPosition.x = leg.config.footOffset.x + m_movementDir.x * slideBack;
            leg.state.targetFootPosition.y = m_groundHeight + leg.config.footOffset.y;
            leg.state.targetFootPosition.z = leg.config.footOffset.z + m_movementDir.z * slideBack;
            leg.state.footWorldPosition = leg.state.targetFootPosition;

            // スイング開始時のリフトオフ位置を記録
            if (stanceProgress > 0.95f)
                leg.liftoffPos = leg.state.footWorldPosition;
        }
        else
        {
            // --- スイングフェーズ: 足を持ち上げて次のステップへ ---
            float swingDuration = 1.0f - m_gait.dutyFactor;
            float swingPhase = (legPhase - m_gait.dutyFactor) / swingDuration;
            swingPhase = MathUtil::Clamp(swingPhase, 0.0f, 1.0f);
            leg.state.stepProgress = swingPhase;

            // スイング開始時にリフトオフ/着地位置を設定
            if (!wasGrounded == false && wasGrounded) // just entered swing
            {
                leg.liftoffPos = leg.state.footWorldPosition;
                leg.landingPos = leg.config.footOffset;
                leg.landingPos.x += m_movementDir.x * leg.config.stepLength * 0.5f;
                leg.landingPos.z += m_movementDir.z * leg.config.stepLength * 0.5f;
                leg.landingPos.y  = m_groundHeight + leg.config.footOffset.y;
            }

            // 着地位置を更新（移動方向に基づく）
            leg.landingPos.x = leg.config.footOffset.x + m_movementDir.x * leg.config.stepLength * 0.5f;
            leg.landingPos.z = leg.config.footOffset.z + m_movementDir.z * leg.config.stepLength * 0.5f;
            leg.landingPos.y = m_groundHeight + leg.config.footOffset.y;

            // キュービックベジェアーク: liftoff → 高い中間点 → landing
            // t=swingPhase, 制御点: P0=liftoff, P1=mid_high, P2=landing
            float t = swingPhase;
            float oneMinusT = 1.0f - t;

            // 二次ベジェ: P(t) = (1-t)^2 * P0 + 2(1-t)*t * P1 + t^2 * P2
            Vector3 midPoint;
            midPoint.x = (leg.liftoffPos.x + leg.landingPos.x) * 0.5f;
            midPoint.y = (std::max)(leg.liftoffPos.y, leg.landingPos.y) + leg.config.stepHeight;
            midPoint.z = (leg.liftoffPos.z + leg.landingPos.z) * 0.5f;

            float w0 = oneMinusT * oneMinusT;
            float w1 = 2.0f * oneMinusT * t;
            float w2 = t * t;

            leg.state.targetFootPosition.x = w0 * leg.liftoffPos.x + w1 * midPoint.x + w2 * leg.landingPos.x;
            leg.state.targetFootPosition.y = w0 * leg.liftoffPos.y + w1 * midPoint.y + w2 * leg.landingPos.y;
            leg.state.targetFootPosition.z = w0 * leg.liftoffPos.z + w1 * midPoint.z + w2 * leg.landingPos.z;

            leg.state.footWorldPosition = leg.state.targetFootPosition;
        }
    }

    // ------- 3. ボディボブ (上下振動) -------
    float bobFreq = m_motionConfig.bodyBobFrequency;
    float bobAmp  = m_motionConfig.bodyBobAmplitude;
    float bobTarget = bobAmp * std::sin(m_globalPhase * k_TwoPi * bobFreq) * speedFactor;
    float bob = m_bobSpring.Update(bobTarget, deltaTime);

    // ------- 4. ボディスウェイ (左右揺れ) -------
    float swayAmp = m_motionConfig.bodySwayAmplitude;
    float swayTarget = swayAmp * std::cos(m_globalPhase * MathUtil::PI * bobFreq) * speedFactor;
    float sway = m_swaySpring.Update(swayTarget, deltaTime);

    m_bodyOffset.x = sway;
    m_bodyOffset.y = bob;
    m_bodyOffset.z = 0.0f;

    // ------- 5. ボディティルト (地形適応) -------
    // 左右の足の高さの差からロールを推定
    float avgLeftY  = 0.0f;
    float avgRightY = 0.0f;
    int leftCount   = 0;
    int rightCount  = 0;
    for (int i = 0; i < legCount; ++i)
    {
        float footY = m_legs[static_cast<size_t>(i)].state.footWorldPosition.y;
        if (m_legs[static_cast<size_t>(i)].config.footOffset.x < 0.0f)
        {
            avgLeftY += footY; ++leftCount;
        }
        else
        {
            avgRightY += footY; ++rightCount;
        }
    }
    if (leftCount > 0) avgLeftY /= static_cast<float>(leftCount);
    if (rightCount > 0) avgRightY /= static_cast<float>(rightCount);

    float rollAngle = (avgRightY - avgLeftY) * m_motionConfig.bodyTiltFactor;
    m_bodyTilt.x = 0.0f; // pitch
    m_bodyTilt.y = 0.0f; // yaw
    m_bodyTilt.z = rollAngle; // roll
}

} // namespace gx
