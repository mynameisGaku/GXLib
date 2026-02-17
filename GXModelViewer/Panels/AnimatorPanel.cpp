/// @file AnimatorPanel.cpp
/// @brief State machine visualizer using imnodes

#include "AnimatorPanel.h"
#include <imgui.h>
#include <imnodes.h>
#include "Graphics/3D/AnimatorStateMachine.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/BlendTree.h"

// ID scheme for imnodes:
// Node IDs:       state index (0, 1, 2, ...)
// Output pin IDs: stateIndex * 1000 + 0
// Input pin IDs:  stateIndex * 1000 + 1
// Link IDs:       transitionIndex + 10000

static int OutputPinId(int stateIndex) { return stateIndex * 1000; }
static int InputPinId(int stateIndex)  { return stateIndex * 1000 + 1; }
static int LinkId(int transitionIndex) { return transitionIndex + 10000; }

void AnimatorPanel::Draw(GX::AnimatorStateMachine* stateMachine)
{
    if (!ImGui::Begin("Animator State Machine"))
    {
        ImGui::End();
        return;
    }

    if (!stateMachine)
    {
        ImGui::TextDisabled("No StateMachine assigned.");
        ImGui::End();
        return;
    }

    uint32_t stateCount = stateMachine->GetStateCount();
    uint32_t currentState = stateMachine->GetCurrentStateIndex();
    bool transitioning = stateMachine->IsTransitioning();

    // Info header
    const GX::AnimState* curState = stateMachine->GetCurrentState();
    if (curState)
    {
        ImGui::Text("Current State: %s", curState->name.c_str());
        if (transitioning)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(transitioning)");
        }
    }
    ImGui::Separator();

    // --- Node editor ---
    ImNodes::BeginNodeEditor();

    // Place nodes in a grid layout on first frame
    if (!m_initialized && stateCount > 0)
    {
        for (uint32_t i = 0; i < stateCount; ++i)
        {
            float x = static_cast<float>(i % 4) * 250.0f + 50.0f;
            float y = static_cast<float>(i / 4) * 200.0f + 50.0f;
            ImNodes::SetNodeGridSpacePos(static_cast<int>(i), ImVec2(x, y));
        }
        m_initialized = true;
    }

    // Draw state nodes
    for (uint32_t i = 0; i < stateCount; ++i)
    {
        const GX::AnimState* state = stateMachine->GetState(i);
        if (!state)
            continue;

        int nodeId = static_cast<int>(i);

        // Highlight current state
        if (i == currentState)
        {
            ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(40, 140, 40, 255));
            ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(50, 170, 50, 255));
            ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(60, 200, 60, 255));
        }

        ImNodes::BeginNode(nodeId);

        // Title
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(state->name.c_str());
        ImNodes::EndNodeTitleBar();

        // Node body: clip info
        if (state->clip)
        {
            ImGui::Text("Clip: %s", state->clip->GetName().c_str());
            ImGui::Text("Duration: %.2fs", state->clip->GetDuration());
        }
        else if (state->blendTree)
        {
            const char* typeName = (state->blendTree->GetType() == GX::BlendTreeType::Simple1D)
                                       ? "1D" : "2D";
            ImGui::Text("BlendTree (%s)", typeName);
        }
        else
        {
            ImGui::TextDisabled("(no clip)");
        }

        ImGui::Text("Loop: %s  Speed: %.1f", state->loop ? "Yes" : "No", state->speed);

        // Output pin (transitions go from here)
        ImNodes::BeginOutputAttribute(OutputPinId(nodeId));
        ImGui::Text("Out");
        ImNodes::EndOutputAttribute();

        // Input pin (transitions come in here)
        ImNodes::BeginInputAttribute(InputPinId(nodeId));
        ImGui::Text("In");
        ImNodes::EndInputAttribute();

        ImNodes::EndNode();

        if (i == currentState)
        {
            ImNodes::PopColorStyle(); // TitleBarSelected
            ImNodes::PopColorStyle(); // TitleBarHovered
            ImNodes::PopColorStyle(); // TitleBar
        }
    }

    // Draw transition links
    // We iterate all transitions from internal state. Since AnimatorStateMachine
    // doesn't expose its transition list directly, we draw links based on
    // the state connectivity we know about. For the visualization, we build
    // links from the transitions stored internally.
    // NOTE: AnimatorStateMachine stores transitions as a private vector.
    // Since we can't access them, we skip link drawing if the API doesn't expose them.
    // For now, we draw a self-explanatory note.

    // The panel visualizes nodes; links require exposing transitions from the state machine.
    // A future API addition (GetTransitions) would enable this.

    ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
    ImNodes::EndNodeEditor();

    // Check node selection
    int selectedCount = ImNodes::NumSelectedNodes();
    if (selectedCount == 1)
    {
        int selectedId = -1;
        ImNodes::GetSelectedNodes(&selectedId);
        m_selectedNode = selectedId;
    }
    else
    {
        m_selectedNode = -1;
    }

    // --- Selected node details ---
    if (m_selectedNode >= 0 && m_selectedNode < static_cast<int>(stateCount))
    {
        const GX::AnimState* state = stateMachine->GetState(static_cast<uint32_t>(m_selectedNode));
        if (state)
        {
            ImGui::Separator();
            ImGui::Text("Selected: %s", state->name.c_str());
            if (state->clip)
            {
                ImGui::Text("  Clip: %s", state->clip->GetName().c_str());
                ImGui::Text("  Duration: %.2f s", state->clip->GetDuration());
                ImGui::Text("  Channels: %d", static_cast<int>(state->clip->GetChannels().size()));
            }
            ImGui::Text("  Loop: %s", state->loop ? "Yes" : "No");
            ImGui::Text("  Speed: %.2f", state->speed);

            // Allow setting this state as current
            if (ImGui::Button("Set as Current"))
            {
                stateMachine->SetCurrentState(static_cast<uint32_t>(m_selectedNode));
            }
        }
    }

    // --- Parameters ---
    ImGui::Separator();
    ImGui::Text("Parameters:");
    ImGui::TextDisabled("(Triggers/Floats are set via code)");

    // Trigger buttons (user can fire triggers from the panel)
    // Since we can't enumerate triggers, provide a text input
    static char triggerBuf[64] = {};
    ImGui::InputText("Trigger", triggerBuf, sizeof(triggerBuf));
    ImGui::SameLine();
    if (ImGui::Button("Fire") && triggerBuf[0] != '\0')
    {
        stateMachine->SetTrigger(triggerBuf);
    }

    // Float parameter setter
    static char floatNameBuf[64] = {};
    static float floatValue = 0.0f;
    ImGui::InputText("Float Name", floatNameBuf, sizeof(floatNameBuf));
    ImGui::DragFloat("Float Value", &floatValue, 0.01f);
    ImGui::SameLine();
    if (ImGui::Button("Set") && floatNameBuf[0] != '\0')
    {
        stateMachine->SetFloat(floatNameBuf, floatValue);
    }

    ImGui::End();
}
