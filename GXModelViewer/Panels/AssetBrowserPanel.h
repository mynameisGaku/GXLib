#pragma once
/// @file AssetBrowserPanel.h
/// @brief Project folder browser panel with drag-drop support

#include <string>
#include <vector>

class GXModelViewerApp;

/// @brief File/folder browser panel for importing assets
class AssetBrowserPanel
{
public:
    void Draw(GXModelViewerApp& app);
    void DrawContent(GXModelViewerApp& app);
    void SetRootPath(const std::string& path);

private:
    struct FileEntry
    {
        std::string name;
        std::string fullPath;
        bool isDirectory;
    };

    void RefreshEntries();
    bool IsModelFile(const std::string& ext) const;
    bool IsAnimFile(const std::string& ext) const;

    std::string              m_rootPath;
    std::string              m_currentPath;
    std::vector<FileEntry>   m_entries;
    bool                     m_needsRefresh = true;
    char                     m_pathBuffer[512] = {};
};
