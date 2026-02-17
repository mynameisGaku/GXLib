/// @file LogPanel.cpp
/// @brief ImGui panel for scrollable log output with level filtering

#include "LogPanel.h"
#include <imgui.h>
#include <cstdio>

void LogPanel::AddLog(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    AddLogV(LogLevel::Info, fmt, args);
    va_end(args);
}

void LogPanel::AddLogLevel(LogLevel level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    AddLogV(level, fmt, args);
    va_end(args);
}

void LogPanel::AddLogV(LogLevel level, const char* fmt, va_list args)
{
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);

    LogEntry entry;
    entry.level   = level;
    entry.message = buf;
    m_entries.push_back(entry);
}

void LogPanel::Clear()
{
    m_entries.clear();
}

void LogPanel::Draw()
{
    if (!ImGui::Begin("Log"))
    {
        ImGui::End();
        return;
    }

    // --- Toolbar ---
    if (ImGui::Button("Clear"))
        Clear();

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);

    ImGui::SameLine();
    ImGui::Separator();

    // Level filter toggle buttons
    ImGui::SameLine();
    if (m_showInfo)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Info"))
        m_showInfo = !m_showInfo;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (m_showWarning)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.6f, 0.1f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Warning"))
        m_showWarning = !m_showWarning;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (m_showError)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Error"))
        m_showError = !m_showError;
    ImGui::PopStyleColor();

    ImGui::Separator();

    // --- Scrollable Log Area ---
    if (ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    {
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            const LogEntry& entry = m_entries[i];

            // Filter by level
            bool show = false;
            ImVec4 color;
            const char* prefix;
            switch (entry.level)
            {
            case LogLevel::Info:
                show   = m_showInfo;
                color  = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                prefix = "[INFO]";
                break;
            case LogLevel::Warning:
                show   = m_showWarning;
                color  = ImVec4(1.0f, 0.9f, 0.3f, 1.0f);
                prefix = "[WARN]";
                break;
            case LogLevel::Error:
                show   = m_showError;
                color  = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                prefix = "[ERR ]";
                break;
            }

            if (!show)
                continue;

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(prefix);
            ImGui::SameLine();
            ImGui::TextUnformatted(entry.message.c_str());
            ImGui::PopStyleColor();
        }

        // Auto-scroll
        if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}
