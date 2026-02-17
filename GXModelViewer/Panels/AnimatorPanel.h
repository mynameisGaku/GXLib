#pragma once
/// @file AnimatorPanel.h
/// @brief State machine visualizer using imnodes

namespace GX { class AnimatorStateMachine; }

/// @brief ImGui panel that visualizes an AnimatorStateMachine using imnodes
class AnimatorPanel
{
public:
    /// Draw the animator state machine panel.
    /// @param stateMachine The state machine to visualize (may be nullptr).
    void Draw(GX::AnimatorStateMachine* stateMachine);

private:
    bool m_initialized = false;  ///< Whether node positions have been set
    int  m_selectedNode = -1;    ///< Currently selected node index
};
