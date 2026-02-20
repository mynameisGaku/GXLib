/// @file AssetBrowserPanel.cpp
/// @brief Project folder browser panel implementation

#include "AssetBrowserPanel.h"
#include "GXModelViewerApp.h"
#include "Core/Logger.h"

#include <imgui.h>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

void AssetBrowserPanel::SetRootPath(const std::string& path)
{
    m_rootPath = path;
    m_currentPath = path;
    m_needsRefresh = true;
    // Copy to editable buffer
    strncpy_s(m_pathBuffer, m_currentPath.c_str(), sizeof(m_pathBuffer) - 1);
}

void AssetBrowserPanel::RefreshEntries()
{
    m_entries.clear();
    m_needsRefresh = false;

    if (m_currentPath.empty()) return;

    std::error_code ec;
    if (!fs::exists(m_currentPath, ec) || !fs::is_directory(m_currentPath, ec))
        return;

    for (const auto& entry : fs::directory_iterator(m_currentPath, ec))
    {
        FileEntry fe;
        fe.name = entry.path().filename().string();
        fe.fullPath = entry.path().string();
        fe.isDirectory = entry.is_directory(ec);

        // Skip hidden files/folders (starting with '.')
        if (!fe.name.empty() && fe.name[0] == '.')
            continue;

        m_entries.push_back(std::move(fe));
    }

    // Sort: directories first, then alphabetically
    std::sort(m_entries.begin(), m_entries.end(),
        [](const FileEntry& a, const FileEntry& b) {
            if (a.isDirectory != b.isDirectory)
                return a.isDirectory > b.isDirectory; // dirs first
            return a.name < b.name;
        });
}

bool AssetBrowserPanel::IsModelFile(const std::string& ext) const
{
    return ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".gxmd";
}

bool AssetBrowserPanel::IsAnimFile(const std::string& ext) const
{
    return ext == ".gxan";
}

void AssetBrowserPanel::Draw(GXModelViewerApp& app)
{
    if (ImGui::Begin("Asset Browser"))
    {
        DrawContent(app);
    }
    ImGui::End();
}

void AssetBrowserPanel::DrawContent(GXModelViewerApp& app)
{
    if (m_needsRefresh)
        RefreshEntries();

    // Path bar
    {
        // Back button
        bool canGoBack = !m_currentPath.empty() && m_currentPath != m_rootPath;
        if (!canGoBack) ImGui::BeginDisabled();
        if (ImGui::Button("<"))
        {
            fs::path parent = fs::path(m_currentPath).parent_path();
            m_currentPath = parent.string();
            strncpy_s(m_pathBuffer, m_currentPath.c_str(), sizeof(m_pathBuffer) - 1);
            m_needsRefresh = true;
        }
        if (!canGoBack) ImGui::EndDisabled();

        ImGui::SameLine();

        // Editable path
        ImGui::SetNextItemWidth(-60.0f);
        if (ImGui::InputText("##path", m_pathBuffer, sizeof(m_pathBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue))
        {
            m_currentPath = m_pathBuffer;
            m_needsRefresh = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
            m_needsRefresh = true;
    }

    ImGui::Separator();

    // File list
    if (ImGui::BeginChild("FileList"))
    {
        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            const auto& entry = m_entries[i];

            // Icon prefix
            const char* icon = "   ";
            if (entry.isDirectory) icon = "[D]";
            else
            {
                std::string ext = fs::path(entry.name).extension().string();
                for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
                if (IsModelFile(ext)) icon = "[M]";
                else if (IsAnimFile(ext)) icon = "[A]";
            }

            char label[512];
            snprintf(label, sizeof(label), "%s %s", icon, entry.name.c_str());

            if (ImGui::Selectable(label, false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (entry.isDirectory)
                    {
                        // Navigate into directory
                        m_currentPath = entry.fullPath;
                        strncpy_s(m_pathBuffer, m_currentPath.c_str(), sizeof(m_pathBuffer) - 1);
                        m_needsRefresh = true;
                    }
                    else
                    {
                        // Import on double-click
                        std::string ext = fs::path(entry.name).extension().string();
                        for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
                        if (IsModelFile(ext))
                            app.ImportModel(entry.fullPath);
                        else if (IsAnimFile(ext))
                            app.ImportAnimation(entry.fullPath);
                    }
                }
            }

            // Drag-drop source for files (not directories)
            if (!entry.isDirectory && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("ASSET_PATH",
                    entry.fullPath.c_str(),
                    entry.fullPath.size() + 1); // include null terminator
                ImGui::Text("%s", entry.name.c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
    ImGui::EndChild();
}
