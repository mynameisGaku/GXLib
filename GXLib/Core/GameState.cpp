/// @file GameState.cpp
/// @brief ゲームステートマシンの実装
#include "pch_common.h"
#include "Core/GameState.h"

namespace gx
{

const gx::String GameStateMachine::s_emptyName;

uint32_t GameStateMachine::RegisterState(const GameState& state)
{
    uint32_t id = static_cast<uint32_t>(m_states.size());
    m_states.push_back(state);
    return id;
}

uint32_t GameStateMachine::FindState(const gx::String& name) const
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_states.size()); ++i)
    {
        if (m_states[i].name == name) return i;
    }
    return UINT32_MAX;
}

void GameStateMachine::ChangeState(uint32_t stateId)
{
    if (stateId >= static_cast<uint32_t>(m_states.size())) return;

    // 全ステートをポップ
    while (!m_stack.empty())
    {
        uint32_t top = m_stack.back();
        if (m_states[top].onExit) m_states[top].onExit();
        m_stack.pop_back();
    }

    // 新しいステートをプッシュ
    m_stack.push_back(stateId);
    if (m_states[stateId].onEnter) m_states[stateId].onEnter();
}

void GameStateMachine::PushState(uint32_t stateId)
{
    if (stateId >= static_cast<uint32_t>(m_states.size())) return;
    m_stack.push_back(stateId);
    if (m_states[stateId].onEnter) m_states[stateId].onEnter();
}

void GameStateMachine::PopState()
{
    if (m_stack.empty()) return;
    uint32_t top = m_stack.back();
    if (m_states[top].onExit) m_states[top].onExit();
    m_stack.pop_back();
}

void GameStateMachine::Update(float deltaTime)
{
    if (m_stack.empty()) return;
    uint32_t top = m_stack.back();
    if (m_states[top].onUpdate) m_states[top].onUpdate(deltaTime);
}

void GameStateMachine::Draw()
{
    if (m_stack.empty()) return;
    uint32_t top = m_stack.back();
    if (m_states[top].onDraw) m_states[top].onDraw();
}

const gx::String& GameStateMachine::GetCurrentStateName() const
{
    if (m_stack.empty()) return s_emptyName;
    return m_states[m_stack.back()].name;
}

} // namespace gx
