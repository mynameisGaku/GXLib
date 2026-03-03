/// @file TimelineEditor.cpp
/// @brief タイムラインエディタパネルの ImGui 実装
#include "pch_graphics.h"
#include "Editor/TimelineEditor.h"
#include "Core/Timeline.h"

#include <imgui.h>

namespace gx
{

// ============================================================================
// 定数
// ============================================================================
namespace
{
    constexpr float k_TrackHeight       = 24.0f;  // トラックの高さ
    constexpr float k_TrackLabelWidth   = 120.0f; // トラックラベルの幅
    constexpr float k_TimeRulerHeight   = 20.0f;  // タイムルーラーの高さ
    constexpr float k_KeyframeRadius    = 5.0f;   // キーフレームマーカーの半径
    constexpr float k_PixelsPerSecond   = 100.0f; // ベースのピクセル/秒比
    constexpr float k_MinZoom           = 0.1f;
    constexpr float k_MaxZoom           = 10.0f;
    constexpr float k_PlayheadWidth     = 2.0f;   // プレイヘッドの幅
} // anonymous namespace

// ============================================================================
// メイン描画
// ============================================================================

void TimelineEditor::Draw(Timeline* timeline)
{
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(700, 250), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Timeline", &m_visible))
    {
        ImGui::End();
        return;
    }

    if (!timeline)
    {
        ImGui::TextDisabled("No timeline loaded");
        ImGui::End();
        return;
    }

    // ツールバー
    DrawToolbar(timeline);

    ImGui::Separator();

    // トラック数が0の場合
    if (timeline->GetTrackCount() == 0)
    {
        ImGui::TextDisabled("No tracks");
        ImGui::End();
        return;
    }

    // 左右分割: トラックラベル + タイムラインエリア
    float contentHeight = ImGui::GetContentRegionAvail().y;

    // 左ペイン: トラックリスト
    ImGui::BeginChild("TrackList", ImVec2(k_TrackLabelWidth, contentHeight), ImGuiChildFlags_Borders);
    {
        // タイムルーラーの高さ分の空白
        ImGui::Dummy(ImVec2(0, k_TimeRulerHeight));
        ImGui::Separator();

        DrawTrackList(timeline);
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 右ペイン: タイムラインエリア
    ImGui::BeginChild("TimelineArea", ImVec2(0, contentHeight),
                      ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    {
        DrawTimelineArea(timeline);
    }
    ImGui::EndChild();

    ImGui::End();
}

// ============================================================================
// ツールバー（再生/停止/時刻表示）
// ============================================================================

void TimelineEditor::DrawToolbar(Timeline* timeline)
{
    bool isPlaying = timeline->IsPlaying();

    // 再生/一時停止ボタン
    if (isPlaying)
    {
        if (ImGui::Button("||"))
        {
            timeline->Pause();
        }
    }
    else
    {
        if (ImGui::Button(">"))
        {
            timeline->Play();
        }
    }

    ImGui::SameLine();

    // 停止ボタン
    if (ImGui::Button("[]"))
    {
        timeline->Stop();
        m_currentTime = 0.0f;
    }

    ImGui::SameLine();

    // 先頭へ
    if (ImGui::Button("|<"))
    {
        timeline->Seek(0.0f);
        m_currentTime = 0.0f;
    }

    ImGui::SameLine();

    // 末尾へ
    if (ImGui::Button(">|"))
    {
        float dur = timeline->GetDuration();
        timeline->Seek(dur);
        m_currentTime = dur;
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // 現在時刻表示/編集
    m_currentTime = timeline->GetCurrentTime();
    float duration = timeline->GetDuration();

    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::SliderFloat("Time", &m_currentTime, 0.0f, duration > 0.0f ? duration : 1.0f, "%.2f s"))
    {
        timeline->Seek(m_currentTime);
    }

    ImGui::SameLine();
    ImGui::Text("/ %.2f s", duration);

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // ズームスライダー
    ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderFloat("Zoom", &m_zoom, k_MinZoom, k_MaxZoom, "%.1fx");

    ImGui::SameLine();

    // トラック数表示
    ImGui::Text("Tracks: %u", static_cast<uint32_t>(timeline->GetTrackCount()));
}

// ============================================================================
// トラックリスト描画
// ============================================================================

void TimelineEditor::DrawTrackList(Timeline* timeline)
{
    size_t trackCount = timeline->GetTrackCount();

    for (size_t i = 0; i < trackCount; ++i)
    {
        ImGui::PushID(static_cast<int>(i));

        bool isSelected = (m_selectedTrack == static_cast<int>(i));

        char label[64];
        snprintf(label, sizeof(label), "Track %u", static_cast<uint32_t>(i));

        if (ImGui::Selectable(label, isSelected, 0, ImVec2(0, k_TrackHeight)))
        {
            m_selectedTrack = static_cast<int>(i);
            m_selectedKey = -1;
        }

        ImGui::PopID();
    }
}

// ============================================================================
// タイムラインエリア描画
// ============================================================================

void TimelineEditor::DrawTimelineArea(Timeline* timeline)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    float pps = k_PixelsPerSecond * m_zoom; // ピクセル/秒
    float duration = timeline->GetDuration();
    if (duration <= 0.0f) duration = 5.0f; // 最低表示幅
    float totalWidth = duration * pps + 100.0f;

    // ズーム操作（マウスホイール）
    if (ImGui::IsWindowHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            m_zoom += wheel * 0.1f;
            if (m_zoom < k_MinZoom) m_zoom = k_MinZoom;
            if (m_zoom > k_MaxZoom) m_zoom = k_MaxZoom;
        }
    }

    // スクロール可能な内容サイズを確保
    ImGui::Dummy(ImVec2(totalWidth, 0));

    // ================================================================
    // タイムルーラー描画
    // ================================================================
    {
        ImVec2 rulerMin = canvasPos;
        ImVec2 rulerMax = ImVec2(canvasPos.x + totalWidth, canvasPos.y + k_TimeRulerHeight);

        // ルーラー背景
        drawList->AddRectFilled(rulerMin, rulerMax, IM_COL32(40, 40, 40, 255));

        // 時間目盛り
        float step = 1.0f;
        if (m_zoom < 0.3f) step = 5.0f;
        else if (m_zoom < 0.7f) step = 2.0f;
        else if (m_zoom > 3.0f) step = 0.5f;
        else if (m_zoom > 6.0f) step = 0.25f;

        for (float t = 0.0f; t <= duration + step; t += step)
        {
            float x = canvasPos.x + t * pps - m_scrollX;
            if (x < canvasPos.x || x > canvasPos.x + canvasSize.x)
                continue;

            drawList->AddLine(ImVec2(x, rulerMin.y), ImVec2(x, rulerMax.y), IM_COL32(100, 100, 100, 255));

            char timeBuf[16];
            snprintf(timeBuf, sizeof(timeBuf), "%.1fs", t);
            drawList->AddText(ImVec2(x + 2.0f, rulerMin.y + 2.0f), IM_COL32(200, 200, 200, 255), timeBuf);
        }
    }

    // ================================================================
    // トラック背景 + キーフレーム描画
    // ================================================================
    size_t trackCount = timeline->GetTrackCount();
    float trackStartY = canvasPos.y + k_TimeRulerHeight;

    for (size_t i = 0; i < trackCount; ++i)
    {
        float trackY = trackStartY + static_cast<float>(i) * (k_TrackHeight + 2.0f);

        // トラック背景
        ImU32 bgColor = (i % 2 == 0) ? IM_COL32(45, 45, 50, 255) : IM_COL32(50, 50, 55, 255);
        if (m_selectedTrack == static_cast<int>(i))
            bgColor = IM_COL32(60, 60, 80, 255);

        drawList->AddRectFilled(
            ImVec2(canvasPos.x, trackY),
            ImVec2(canvasPos.x + totalWidth, trackY + k_TrackHeight),
            bgColor);

        // トラック境界線
        drawList->AddLine(
            ImVec2(canvasPos.x, trackY + k_TrackHeight),
            ImVec2(canvasPos.x + totalWidth, trackY + k_TrackHeight),
            IM_COL32(60, 60, 60, 255));
    }

    // ================================================================
    // プレイヘッド描画
    // ================================================================
    {
        m_currentTime = timeline->GetCurrentTime();
        float playheadX = canvasPos.x + m_currentTime * pps - m_scrollX;

        if (playheadX >= canvasPos.x && playheadX <= canvasPos.x + canvasSize.x)
        {
            float bottomY = trackStartY + static_cast<float>(trackCount) * (k_TrackHeight + 2.0f);

            // プレイヘッドライン
            drawList->AddLine(
                ImVec2(playheadX, canvasPos.y),
                ImVec2(playheadX, bottomY),
                IM_COL32(255, 80, 80, 255), k_PlayheadWidth);

            // プレイヘッドヘッド（三角形）
            drawList->AddTriangleFilled(
                ImVec2(playheadX - 6.0f, canvasPos.y),
                ImVec2(playheadX + 6.0f, canvasPos.y),
                ImVec2(playheadX, canvasPos.y + 8.0f),
                IM_COL32(255, 80, 80, 255));
        }
    }

    // ================================================================
    // タイムライン上のクリックでシーク
    // ================================================================
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        if (mousePos.y >= canvasPos.y && mousePos.y <= canvasPos.y + k_TimeRulerHeight)
        {
            float clickTime = (mousePos.x - canvasPos.x + m_scrollX) / pps;
            if (clickTime < 0.0f) clickTime = 0.0f;
            if (clickTime > timeline->GetDuration()) clickTime = timeline->GetDuration();
            timeline->Seek(clickTime);
            m_currentTime = clickTime;
        }
    }

    // スクロール位置の更新
    m_scrollX = ImGui::GetScrollX();
}

} // namespace gx
