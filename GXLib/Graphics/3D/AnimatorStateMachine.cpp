/// @file AnimatorStateMachine.cpp
/// @brief アニメーションステートマシンの実装
#include "pch.h"
#include "Graphics/3D/AnimatorStateMachine.h"

namespace GX
{

uint32_t AnimatorStateMachine::AddState(const AnimState& state)
{
    uint32_t index = static_cast<uint32_t>(m_states.size());
    m_states.push_back(state);
    return index;
}

void AnimatorStateMachine::AddTransition(const AnimTransition& transition)
{
    m_transitions.push_back(transition);
}

void AnimatorStateMachine::SetTrigger(const std::string& name)
{
    m_triggers[name] = true;
}

void AnimatorStateMachine::SetFloat(const std::string& name, float value)
{
    m_floats[name] = value;
}

float AnimatorStateMachine::GetFloat(const std::string& name) const
{
    auto it = m_floats.find(name);
    return (it != m_floats.end()) ? it->second : 0.0f;
}

void AnimatorStateMachine::SetCurrentState(uint32_t index)
{
    if (index < static_cast<uint32_t>(m_states.size()))
    {
        m_currentState = index;
        m_stateTime = 0.0f;
        m_transitioning = false;
    }
}

const AnimState* AnimatorStateMachine::GetCurrentState() const
{
    if (m_currentState < static_cast<uint32_t>(m_states.size()))
        return &m_states[m_currentState];
    return nullptr;
}

const AnimState* AnimatorStateMachine::GetState(uint32_t index) const
{
    if (index < static_cast<uint32_t>(m_states.size()))
        return &m_states[index];
    return nullptr;
}

uint32_t AnimatorStateMachine::GetStateCount() const
{
    return static_cast<uint32_t>(m_states.size());
}

float AnimatorStateMachine::GetStateDuration(const AnimState& state) const
{
    if (state.clip)
        return state.clip->GetDuration();
    if (state.blendTree)
        return state.blendTree->GetDuration();
    return 0.0f;
}

void AnimatorStateMachine::SampleState(const AnimState& state, float time,
                                        uint32_t jointCount,
                                        const TransformTRS* bindPose,
                                        TransformTRS* outPose) const
{
    if (state.blendTree)
    {
        // BlendTreeのfloatパラメータを反映
        // 初学者向け: ステートマシンが持つパラメータをブレンドツリーに渡します。
        for (const auto& pair : m_floats)
        {
            // BlendTreeは非constのSetParameterを持つので、一時的にconst_castを使用
            // （ステートマシンがblendTreeのオーナーなのでセマンティクス的に安全）
            const_cast<BlendTree*>(state.blendTree)->SetParameter(pair.second);
        }
        state.blendTree->Evaluate(time, jointCount, bindPose, outPose);
    }
    else if (state.clip)
    {
        state.clip->SampleTRS(time, jointCount, outPose, bindPose);
    }
    else
    {
        for (uint32_t i = 0; i < jointCount; ++i)
            outPose[i] = bindPose ? bindPose[i] : IdentityTRS();
    }
}

void AnimatorStateMachine::CheckTransitions()
{
    if (m_transitioning) return;
    if (m_states.empty()) return;

    const AnimState* current = GetCurrentState();
    if (!current) return;

    float duration = GetStateDuration(*current);

    for (const auto& trans : m_transitions)
    {
        if (trans.fromState != m_currentState) continue;

        bool shouldTransition = false;

        // トリガー条件チェック
        if (!trans.triggerName.empty())
        {
            auto it = m_triggers.find(trans.triggerName);
            if (it != m_triggers.end() && it->second)
            {
                shouldTransition = true;
                it->second = false; // トリガー消費（自動リセット）
            }
        }

        // 再生終了時間条件チェック
        if (trans.hasExitTime && !shouldTransition)
        {
            if (duration > 0.0f)
            {
                float normalizedTime = m_stateTime / duration;
                if (normalizedTime >= trans.exitTimeNorm)
                    shouldTransition = true;
            }
        }

        if (shouldTransition)
        {
            m_transitioning = true;
            m_nextState = trans.toState;
            m_transitionDuration = (std::max)(trans.duration, 0.0001f);
            m_transitionTime = 0.0f;
            m_nextStateTime = 0.0f;
            break;
        }
    }
}

void AnimatorStateMachine::Update(float deltaTime, uint32_t jointCount,
                                   const TransformTRS* bindPose,
                                   TransformTRS* outPose)
{
    if (m_states.empty() || jointCount == 0) return;

    m_poseA.resize(jointCount);
    m_poseB.resize(jointCount);

    if (m_transitioning)
    {
        // 遷移中: 両方のステートを進めてクロスフェード
        // 初学者向け: 前のステートから次のステートへ、徐々に切り替えています。
        const AnimState* curState = GetCurrentState();
        const AnimState* nxtState = GetState(m_nextState);

        if (!curState || !nxtState)
        {
            m_transitioning = false;
            return;
        }

        m_stateTime += deltaTime * curState->speed;
        m_nextStateTime += deltaTime * nxtState->speed;

        // ループ処理
        float curDuration = GetStateDuration(*curState);
        if (curDuration > 0.0f && curState->loop)
        {
            m_stateTime = fmodf(m_stateTime, curDuration);
            if (m_stateTime < 0.0f) m_stateTime += curDuration;
        }

        float nxtDuration = GetStateDuration(*nxtState);
        if (nxtDuration > 0.0f && nxtState->loop)
        {
            m_nextStateTime = fmodf(m_nextStateTime, nxtDuration);
            if (m_nextStateTime < 0.0f) m_nextStateTime += nxtDuration;
        }

        SampleState(*curState, m_stateTime, jointCount, bindPose, m_poseA.data());
        SampleState(*nxtState, m_nextStateTime, jointCount, bindPose, m_poseB.data());

        m_transitionTime += deltaTime;
        float t = (std::min)(m_transitionTime / m_transitionDuration, 1.0f);

        // クロスフェードブレンド
        for (uint32_t i = 0; i < jointCount; ++i)
        {
            outPose[i].translation.x = m_poseA[i].translation.x + (m_poseB[i].translation.x - m_poseA[i].translation.x) * t;
            outPose[i].translation.y = m_poseA[i].translation.y + (m_poseB[i].translation.y - m_poseA[i].translation.y) * t;
            outPose[i].translation.z = m_poseA[i].translation.z + (m_poseB[i].translation.z - m_poseA[i].translation.z) * t;

            XMVECTOR qa = XMLoadFloat4(&m_poseA[i].rotation);
            XMVECTOR qb = XMLoadFloat4(&m_poseB[i].rotation);
            XMStoreFloat4(&outPose[i].rotation, XMQuaternionSlerp(qa, qb, t));

            outPose[i].scale.x = m_poseA[i].scale.x + (m_poseB[i].scale.x - m_poseA[i].scale.x) * t;
            outPose[i].scale.y = m_poseA[i].scale.y + (m_poseB[i].scale.y - m_poseA[i].scale.y) * t;
            outPose[i].scale.z = m_poseA[i].scale.z + (m_poseB[i].scale.z - m_poseA[i].scale.z) * t;
        }

        // 遷移完了チェック
        if (t >= 1.0f)
        {
            m_currentState = m_nextState;
            m_stateTime = m_nextStateTime;
            m_transitioning = false;
        }
    }
    else
    {
        // 通常再生
        const AnimState* curState = GetCurrentState();
        if (!curState) return;

        m_stateTime += deltaTime * curState->speed;
        float curDuration = GetStateDuration(*curState);

        if (curDuration > 0.0f)
        {
            if (curState->loop)
            {
                m_stateTime = fmodf(m_stateTime, curDuration);
                if (m_stateTime < 0.0f) m_stateTime += curDuration;
            }
            else if (m_stateTime >= curDuration)
            {
                m_stateTime = curDuration;
            }
        }

        SampleState(*curState, m_stateTime, jointCount, bindPose, outPose);

        // 遷移条件チェック
        CheckTransitions();
    }
}

} // namespace GX
