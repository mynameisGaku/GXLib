/// @file BlendTreeEditor.cpp
/// @brief ブレンドツリーエディタパネル実装
///
/// 1D: パラメータスライダー + ImDrawListで数直線・ノード閾値マーカー・現在値三角形を描画。
/// 2D: X/Yスライダー + 散布図キャンバス（ドラッグ操作対応）でノード位置と現在パラメータを表示。
/// 下部にノード一覧テーブル（クリップ名/閾値/位置）も表示する。

#include "BlendTreeEditor.h"
#include <imgui.h>
#include "Graphics/3D/BlendTree.h"
#include "Graphics/3D/AnimationClip.h"

#include <algorithm>
#include <cmath>

void BlendTreeEditor::Draw(GX::BlendTree* blendTree)
{
    if (!ImGui::Begin("Blend Tree"))
    {
        ImGui::End();
        return;
    }

    if (!blendTree)
    {
        ImGui::TextDisabled("No BlendTree assigned.");
        ImGui::End();
        return;
    }

    GX::BlendTreeType type = blendTree->GetType();
    const auto& nodes = blendTree->GetNodes();

    const char* typeName = (type == GX::BlendTreeType::Simple1D) ? "Simple 1D" : "Simple Directional 2D";
    ImGui::Text("Type: %s", typeName);
    ImGui::Text("Nodes: %d", static_cast<int>(nodes.size()));
    ImGui::Separator();

    if (nodes.empty())
    {
        ImGui::TextDisabled("No nodes in blend tree.");
        ImGui::End();
        return;
    }

    // ============================================================
    // 1D Blend Tree
    // ============================================================
    if (type == GX::BlendTreeType::Simple1D)
    {
        // Parameter slider
        float param = blendTree->GetParameter();

        // Determine range from node thresholds
        float minThreshold = nodes[0].threshold;
        float maxThreshold = nodes[0].threshold;
        for (const auto& node : nodes)
        {
            minThreshold = (std::min)(minThreshold, node.threshold);
            maxThreshold = (std::max)(maxThreshold, node.threshold);
        }

        // Add some padding
        float rangePad = (maxThreshold - minThreshold) * 0.1f;
        if (rangePad < 0.01f) rangePad = 1.0f;
        float sliderMin = minThreshold - rangePad;
        float sliderMax = maxThreshold + rangePad;

        if (ImGui::SliderFloat("Parameter", &param, sliderMin, sliderMax, "%.2f"))
        {
            blendTree->SetParameter(param);
        }

        ImGui::Separator();

        // --- Number line visualization ---
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        float canvasWidth = ImGui::GetContentRegionAvail().x;
        float canvasHeight = 60.0f;

        ImGui::InvisibleButton("##1DCanvas", ImVec2(canvasWidth, canvasHeight));
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Background
        drawList->AddRectFilled(
            canvasPos,
            ImVec2(canvasPos.x + canvasWidth, canvasPos.y + canvasHeight),
            IM_COL32(30, 30, 30, 255));

        // Number line
        float lineY = canvasPos.y + canvasHeight * 0.5f;
        float margin = 20.0f;
        float lineLeft = canvasPos.x + margin;
        float lineRight = canvasPos.x + canvasWidth - margin;
        drawList->AddLine(
            ImVec2(lineLeft, lineY),
            ImVec2(lineRight, lineY),
            IM_COL32(120, 120, 120, 255), 2.0f);

        float range = sliderMax - sliderMin;
        if (range < 0.001f) range = 1.0f;

        // Draw node threshold markers
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            float t = (nodes[i].threshold - sliderMin) / range;
            float x = lineLeft + t * (lineRight - lineLeft);

            // Tick mark
            drawList->AddLine(
                ImVec2(x, lineY - 10.0f),
                ImVec2(x, lineY + 10.0f),
                IM_COL32(180, 180, 180, 255), 2.0f);

            // Label
            char label[64];
            if (nodes[i].clip)
                snprintf(label, sizeof(label), "%.1f\n%s", nodes[i].threshold, nodes[i].clip->GetName().c_str());
            else
                snprintf(label, sizeof(label), "%.1f", nodes[i].threshold);
            drawList->AddText(ImVec2(x - 15.0f, lineY + 12.0f), IM_COL32(200, 200, 200, 255), label);
        }

        // Current parameter indicator (triangle)
        {
            float t = (param - sliderMin) / range;
            float x = lineLeft + t * (lineRight - lineLeft);
            drawList->AddTriangleFilled(
                ImVec2(x, lineY - 14.0f),
                ImVec2(x - 6.0f, lineY - 24.0f),
                ImVec2(x + 6.0f, lineY - 24.0f),
                IM_COL32(50, 200, 50, 255));
        }
    }
    // ============================================================
    // 2D Blend Tree
    // ============================================================
    else
    {
        // We cannot read the 2D parameter back from BlendTree (no Get for 2D),
        // so we maintain a local copy for the UI.
        static float param2D[2] = {0.0f, 0.0f};

        bool changed = false;
        changed |= ImGui::DragFloat("Param X", &param2D[0], 0.01f, -2.0f, 2.0f, "%.2f");
        changed |= ImGui::DragFloat("Param Y", &param2D[1], 0.01f, -2.0f, 2.0f, "%.2f");
        if (changed)
        {
            blendTree->SetParameter2D(param2D[0], param2D[1]);
        }

        ImGui::Separator();

        // --- 2D scatter visualization ---
        float canvasSize = (std::min)(ImGui::GetContentRegionAvail().x, 300.0f);
        if (canvasSize < 100.0f) canvasSize = 100.0f;
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton("##2DCanvas", ImVec2(canvasSize, canvasSize));
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Background
        drawList->AddRectFilled(
            canvasPos,
            ImVec2(canvasPos.x + canvasSize, canvasPos.y + canvasSize),
            IM_COL32(30, 30, 30, 255));

        // Grid lines
        float cx = canvasPos.x + canvasSize * 0.5f;
        float cy = canvasPos.y + canvasSize * 0.5f;
        drawList->AddLine(
            ImVec2(canvasPos.x, cy),
            ImVec2(canvasPos.x + canvasSize, cy),
            IM_COL32(60, 60, 60, 255));
        drawList->AddLine(
            ImVec2(cx, canvasPos.y),
            ImVec2(cx, canvasPos.y + canvasSize),
            IM_COL32(60, 60, 60, 255));

        // Determine world-space range from node positions
        float worldRange = 2.0f; // default +-1
        for (const auto& node : nodes)
        {
            worldRange = (std::max)(worldRange, std::abs(node.position[0]) * 2.0f + 0.5f);
            worldRange = (std::max)(worldRange, std::abs(node.position[1]) * 2.0f + 0.5f);
        }

        float halfRange = worldRange * 0.5f;
        float margin = 10.0f;
        float drawSize = canvasSize - margin * 2.0f;

        auto worldToCanvas = [&](float wx, float wy) -> ImVec2 {
            float nx = (wx + halfRange) / worldRange; // 0..1
            float ny = (wy + halfRange) / worldRange; // 0..1
            return ImVec2(
                canvasPos.x + margin + nx * drawSize,
                canvasPos.y + margin + (1.0f - ny) * drawSize // flip Y
            );
        };

        // Draw node dots
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            ImVec2 p = worldToCanvas(nodes[i].position[0], nodes[i].position[1]);
            drawList->AddCircleFilled(p, 6.0f, IM_COL32(100, 150, 255, 255));

            // Label
            char label[64];
            if (nodes[i].clip)
                snprintf(label, sizeof(label), "%s", nodes[i].clip->GetName().c_str());
            else
                snprintf(label, sizeof(label), "[%zu]", i);
            drawList->AddText(ImVec2(p.x + 8.0f, p.y - 6.0f), IM_COL32(200, 200, 200, 255), label);
        }

        // Draw current parameter position
        {
            ImVec2 p = worldToCanvas(param2D[0], param2D[1]);
            drawList->AddCircleFilled(p, 8.0f, IM_COL32(50, 200, 50, 200));
            drawList->AddCircle(p, 8.0f, IM_COL32(255, 255, 255, 200), 0, 2.0f);
        }

        // Allow dragging in the canvas to set parameter
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 mouse = ImGui::GetMousePos();
            float nx = (mouse.x - canvasPos.x - margin) / drawSize;
            float ny = 1.0f - (mouse.y - canvasPos.y - margin) / drawSize;
            param2D[0] = nx * worldRange - halfRange;
            param2D[1] = ny * worldRange - halfRange;
            blendTree->SetParameter2D(param2D[0], param2D[1]);
        }
    }

    // ============================================================
    // Node list table
    // ============================================================
    ImGui::Separator();
    ImGui::Text("Nodes:");

    if (ImGui::BeginTable("BlendNodes", (type == GX::BlendTreeType::Simple1D) ? 3 : 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Clip");
        if (type == GX::BlendTreeType::Simple1D)
        {
            ImGui::TableSetupColumn("Threshold", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        }
        else
        {
            ImGui::TableSetupColumn("Position X", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Position Y", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        }
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", static_cast<int>(i));

            ImGui::TableSetColumnIndex(1);
            if (nodes[i].clip)
                ImGui::TextUnformatted(nodes[i].clip->GetName().c_str());
            else
                ImGui::TextDisabled("(none)");

            if (type == GX::BlendTreeType::Simple1D)
            {
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.2f", nodes[i].threshold);
            }
            else
            {
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.2f", nodes[i].position[0]);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.2f", nodes[i].position[1]);
            }
        }
        ImGui::EndTable();
    }

    ImGui::End();
}
