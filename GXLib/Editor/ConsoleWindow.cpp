/// @file ConsoleWindow.cpp
/// @brief コンソールウィンドウパネルの ImGui 実装
#include "pch_graphics.h"
#include "Editor/ConsoleWindow.h"
#include "Core/GameConsole.h"

#include <imgui.h>

namespace gx
{

// ============================================================================
// メイン描画
// ============================================================================

void ConsoleWindow::Draw(GameConsole* console)
{
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(520, 300), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Console", &m_visible))
    {
        ImGui::End();
        return;
    }

    if (!console)
    {
        ImGui::TextDisabled("GameConsole not available");
        ImGui::End();
        return;
    }

    // ================================================================
    // ツールバー: フィルタボタン + クリア + 自動スクロール
    // ================================================================

    // Info ボタン
    if (m_showInfo)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
    }
    if (ImGui::SmallButton("Info"))
        m_showInfo = !m_showInfo;
    if (m_showInfo)
        ImGui::PopStyleColor();

    ImGui::SameLine();

    // Warn ボタン
    if (m_showWarn)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.1f, 1.0f));
    }
    if (ImGui::SmallButton("Warn"))
        m_showWarn = !m_showWarn;
    if (m_showWarn)
        ImGui::PopStyleColor();

    ImGui::SameLine();

    // Error ボタン
    if (m_showError)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
    }
    if (ImGui::SmallButton("Error"))
        m_showError = !m_showError;
    if (m_showError)
        ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    if (ImGui::SmallButton("Clear"))
    {
        console->ClearHistory();
    }

    ImGui::SameLine();

    ImGui::Checkbox("Auto-scroll", &m_autoScroll);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputTextWithHint("##Filter", "Filter...", m_filterBuffer, sizeof(m_filterBuffer));

    ImGui::Separator();

    // ================================================================
    // ログ表示エリア
    // ================================================================
    float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("LogArea", ImVec2(0, -footerHeight), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);

    const auto& history = console->GetHistory();

    for (const auto& entry : history)
    {
        // タイプフィルタ
        bool show = false;
        ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);

        switch (entry.type)
        {
        case ConsoleEntry::Info:
        case ConsoleEntry::Output:
            show = m_showInfo;
            color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
            break;
        case ConsoleEntry::Input:
            show = m_showInfo;
            color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
            break;
        case ConsoleEntry::Warning:
            show = m_showWarn;
            color = ImVec4(1.0f, 0.9f, 0.3f, 1.0f);
            break;
        case ConsoleEntry::Error:
            show = m_showError;
            color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            break;
        }

        if (!show) continue;

        // テキストフィルタ
        if (m_filterBuffer[0] != '\0')
        {
            gx::String lowerText = entry.text;
            gx::String lowerFilter(m_filterBuffer);
            for (auto& c : lowerText) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            for (auto& c : lowerFilter) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (lowerText.find(lowerFilter) == gx::String::npos)
                continue;
        }

        // 種別プレフィックス
        const char* prefix = "";
        switch (entry.type)
        {
        case ConsoleEntry::Input:   prefix = "> "; break;
        case ConsoleEntry::Output:  prefix = "  "; break;
        case ConsoleEntry::Error:   prefix = "[ERR] "; break;
        case ConsoleEntry::Warning: prefix = "[WRN] "; break;
        case ConsoleEntry::Info:    prefix = "[INF] "; break;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted((gx::String(prefix) + entry.text).c_str());
        ImGui::PopStyleColor();
    }

    // 自動スクロール
    if (m_scrollToBottom || (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
    {
        ImGui::SetScrollHereY(1.0f);
    }
    m_scrollToBottom = false;

    ImGui::EndChild();

    // ================================================================
    // コマンド入力行
    // ================================================================
    ImGui::Separator();

    bool reclaimFocus = false;
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;

    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##Input", m_inputBuffer, sizeof(m_inputBuffer), inputFlags))
    {
        if (m_inputBuffer[0] != '\0')
        {
            gx::String input(m_inputBuffer);
            console->Execute(input);
            m_inputBuffer[0] = '\0';
            m_scrollToBottom = true;
        }
        reclaimFocus = true;
    }

    // 入力フィールドにフォーカスを戻す
    ImGui::SetItemDefaultFocus();
    if (reclaimFocus)
    {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

} // namespace gx
