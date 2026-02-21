/// @file IKPanel.cpp
/// @brief IK（インバースキネマティクス）設定パネル実装
///
/// FootIK・LookAtIK・CCDIKSolverの状態表示とパラメータ編集を行う。
/// SceneEntityにIKメンバーがないため、外部から個別にポインタを渡す設計。

#include "IKPanel.h"
#include <imgui.h>
#include "Graphics/3D/FootIK.h"
#include "Graphics/3D/LookAtIK.h"

void IKPanel::Draw(GX::FootIK* footIK, GX::LookAtIK* lookAtIK, bool hasSkeleton)
{
    if (!ImGui::Begin("IK"))
    {
        ImGui::End();
        return;
    }
    DrawContent(footIK, lookAtIK, hasSkeleton);
    ImGui::End();
}

void IKPanel::DrawContent(GX::FootIK* footIK, GX::LookAtIK* lookAtIK, bool hasSkeleton)
{
    if (!hasSkeleton)
    {
        ImGui::TextDisabled("Select a skinned model to configure IK");
        return;
    }

    // --- Foot IK ---
    if (ImGui::CollapsingHeader("Foot IK", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (!footIK)
        {
            ImGui::TextDisabled("FootIK instance not available");
        }
        else
        {
            bool isSetup = footIK->IsSetup();

            bool enabled = footIK->IsEnabled();
            if (ImGui::Checkbox("Enable Foot IK", &enabled))
                footIK->SetEnabled(enabled);

            if (!isSetup)
            {
                ImGui::TextDisabled("Foot IK not set up (call Setup() with bone names)");
            }
            else
            {
                float offset = footIK->GetFootOffset();
                if (ImGui::DragFloat("Foot Offset", &offset, 0.01f, -1.0f, 1.0f))
                    footIK->SetFootOffset(offset);
            }
        }
    }

    // --- LookAt IK ---
    if (ImGui::CollapsingHeader("LookAt IK"))
    {
        if (!lookAtIK)
        {
            ImGui::TextDisabled("LookAtIK instance not available");
        }
        else
        {
            bool isSetup = lookAtIK->IsSetup();

            bool enabled = lookAtIK->IsEnabled();
            if (ImGui::Checkbox("Enable LookAt IK", &enabled))
                lookAtIK->SetEnabled(enabled);

            if (!isSetup)
            {
                ImGui::TextDisabled("LookAt IK not set up (call Setup() with head bone)");
            }
            else
            {
                ImGui::DragFloat3("Look Target", m_lookAtTarget, 0.1f);

                float maxAngle = lookAtIK->GetMaxAngle();
                float maxAngleDeg = maxAngle * 57.2957795f;
                if (ImGui::SliderFloat("Max Angle (deg)", &maxAngleDeg, 0.0f, 180.0f))
                    lookAtIK->SetMaxAngle(maxAngleDeg * 0.0174532925f);
            }
        }
    }

    // --- CCD IK Chain Info ---
    if (ImGui::CollapsingHeader("CCD IK Solver"))
    {
        ImGui::DragFloat3("Target Position", m_targetPos, 0.1f);
        ImGui::DragFloat3("Pole Vector", m_poleVector, 0.1f);
        ImGui::TextDisabled("CCD IK chains are configured programmatically");
    }
}
