/// @file AssetBrowser.cpp
/// @brief アセットブラウザパネルの ImGui 実装
#include "pch_graphics.h"
#include "Editor/AssetBrowser.h"
#include "Core/AssetDatabase.h"

#include <imgui.h>
#include <filesystem>

namespace gx
{

// ============================================================================
// ユーティリティ
// ============================================================================
namespace
{

const char* AssetTypeToString(AssetType type)
{
    switch (type)
    {
    case AssetType::Texture:   return "[TEX]";
    case AssetType::Model:     return "[MDL]";
    case AssetType::Sound:     return "[SFX]";
    case AssetType::Music:     return "[MUS]";
    case AssetType::Font:      return "[FNT]";
    case AssetType::Shader:    return "[SHD]";
    case AssetType::Script:    return "[LUA]";
    case AssetType::Scene:     return "[SCN]";
    case AssetType::Material:  return "[MAT]";
    case AssetType::Animation: return "[ANM]";
    case AssetType::Prefab:    return "[PFB]";
    case AssetType::Tilemap:   return "[TMP]";
    case AssetType::Config:    return "[CFG]";
    case AssetType::Unknown:   return "[???]";
    }
    return "[???]";
}

bool MatchesFilter(const gx::String& name, const char* filter)
{
    if (!filter || filter[0] == '\0') return true;

    // 大文字小文字を区別しない部分文字列検索
    gx::String lowerName = name;
    gx::String lowerFilter(filter);
    for (auto& c : lowerName) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    for (auto& c : lowerFilter) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    return lowerName.find(lowerFilter) != gx::String::npos;
}

} // anonymous namespace

// ============================================================================
// アセットタイプアイコン
// ============================================================================

const char* AssetBrowser::GetAssetTypeIcon(const gx::String& extension)
{
    AssetType type = AssetDatabase::DetectType(extension);
    return AssetTypeToString(type);
}

// ============================================================================
// メイン描画
// ============================================================================

void AssetBrowser::Draw()
{
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Asset Browser", &m_visible))
    {
        ImGui::End();
        return;
    }

    auto& db = AssetDatabase::Instance();

    // 上部: 検索バー + パス表示
    ImGui::Text("Root: %s", db.GetRootPath().c_str());
    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
    {
        db.ScanAll();
    }

    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Search", "Search assets...", m_searchBuffer, sizeof(m_searchBuffer)))
    {
        m_searchFilter = m_searchBuffer;
    }

    ImGui::Separator();

    // 左右分割レイアウト
    float panelWidth = ImGui::GetContentRegionAvail().x;
    float leftPanelWidth = panelWidth * 0.3f;

    // 左ペイン: ディレクトリツリー
    ImGui::BeginChild("DirTree", ImVec2(leftPanelWidth, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
    {
        gx::String rootPath = db.GetRootPath();
        if (rootPath.empty())
            rootPath = "Assets";

        DrawDirectoryTree(rootPath);
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // 右ペイン: アセットグリッド
    ImGui::BeginChild("AssetGrid", ImVec2(0, 0), ImGuiChildFlags_Borders);
    {
        // 現在のパス表示
        ImGui::Text("Path: %s", m_currentDir.c_str());
        ImGui::Separator();

        DrawAssetGrid();
    }
    ImGui::EndChild();

    ImGui::End();
}

// ============================================================================
// ディレクトリツリー描画
// ============================================================================

void AssetBrowser::DrawDirectoryTree(const gx::String& path)
{
    namespace fs = std::filesystem;

    fs::path dirPath(path.c_str());
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
    {
        // ルートがまだスキャンされていない場合: ディレクトリ名だけ表示
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (m_currentDir == path)
            flags |= ImGuiTreeNodeFlags_Selected;

        if (ImGui::TreeNodeEx(path.c_str(), flags))
        {
            if (ImGui::IsItemClicked())
                m_currentDir = path;
            ImGui::TreePop();
        }
        return;
    }

    // ディレクトリ名を抽出
    gx::String dirName(dirPath.filename().string().c_str());
    if (dirName.empty())
        dirName = path;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (m_currentDir == path)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool nodeOpen = ImGui::TreeNodeEx(dirName.c_str(), flags);

    if (ImGui::IsItemClicked())
    {
        m_currentDir = path;
    }

    if (nodeOpen)
    {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(dirPath, ec))
        {
            if (entry.is_directory())
            {
                gx::String subPath(entry.path().string().c_str());
                DrawDirectoryTree(subPath);
            }
        }
        ImGui::TreePop();
    }
}

// ============================================================================
// アセットグリッド描画
// ============================================================================

void AssetBrowser::DrawAssetGrid()
{
    auto& db = AssetDatabase::Instance();
    const auto& allAssets = db.GetAllAssets();

    // 現在ディレクトリに一致するアセットを収集
    gx::Vector<const AssetEntry*> visibleAssets;

    for (const auto& [key, entry] : allAssets)
    {
        // 検索フィルタ
        if (!MatchesFilter(entry.name, m_searchBuffer))
            continue;

        // ディレクトリフィルタ: パスが現在のディレクトリを含むか
        // (フラットに全てのアセットを表示)
        bool inCurrentDir = false;
        if (m_currentDir.empty() || entry.path.find(m_currentDir) != gx::String::npos
            || entry.relativePath.find(m_currentDir) != gx::String::npos
            || entry.fullPath.find(m_currentDir) != gx::String::npos)
        {
            inCurrentDir = true;
        }

        if (inCurrentDir)
        {
            visibleAssets.push_back(&entry);
        }
    }

    if (visibleAssets.empty())
    {
        ImGui::TextDisabled("No assets found");
        return;
    }

    // グリッドレイアウト (テーブルベース)
    float cellWidth = 180.0f;
    float availWidth = ImGui::GetContentRegionAvail().x;
    int columns = static_cast<int>(availWidth / cellWidth);
    if (columns < 1) columns = 1;

    if (ImGui::BeginTable("AssetTable", columns))
    {
        int itemIdx = 0;
        for (const auto* asset : visibleAssets)
        {
            if (itemIdx % columns == 0)
                ImGui::TableNextRow();

            ImGui::TableNextColumn();

            // アセットアイテム
            ImGui::PushID(static_cast<int>(itemIdx));

            bool isSelected = (m_selectedPath == asset->path || m_selectedPath == asset->relativePath);
            gx::String displayLabel;
            displayLabel += GetAssetTypeIcon(asset->extension);
            displayLabel += " ";
            displayLabel += asset->name;

            if (ImGui::Selectable(displayLabel.c_str(), isSelected,
                                  ImGuiSelectableFlags_AllowDoubleClick,
                                  ImVec2(cellWidth - 10.0f, 20.0f)))
            {
                m_selectedPath = asset->relativePath.empty() ? asset->path : asset->relativePath;

                // ダブルクリック
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    // 選択確定（外部で GetSelectedAssetPath() を使用）
                }
            }

            // ツールチップ
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Name: %s", asset->name.c_str());
                ImGui::Text("Type: %s", AssetTypeToString(asset->type));
                ImGui::Text("Ext:  %s", asset->extension.c_str());
                if (asset->fileSize > 0)
                {
                    if (asset->fileSize > 1024 * 1024)
                        ImGui::Text("Size: %.1f MB", asset->fileSize / (1024.0 * 1024.0));
                    else if (asset->fileSize > 1024)
                        ImGui::Text("Size: %.1f KB", asset->fileSize / 1024.0);
                    else
                        ImGui::Text("Size: %llu bytes", static_cast<unsigned long long>(asset->fileSize));
                }
                ImGui::EndTooltip();
            }

            // 右クリックコンテキストメニュー
            if (ImGui::BeginPopupContextItem())
            {
                m_selectedPath = asset->relativePath.empty() ? asset->path : asset->relativePath;

                ImGui::Text("%s", asset->name.c_str());
                ImGui::Separator();

                if (ImGui::MenuItem("Select"))
                {
                    // 選択（既に選択済み）
                }

                if (ImGui::MenuItem("Show in Explorer"))
                {
                    // OS のファイルエクスプローラーでパスを開く
                    gx::String cmd = "explorer /select,\"";
                    cmd += asset->fullPath.empty() ? asset->path : asset->fullPath;
                    cmd += "\"";
                    // system呼び出しは省略（プロダクション時に WinAPI で置き換え）
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Copy Path"))
                {
                    const char* path = asset->relativePath.empty()
                        ? asset->path.c_str()
                        : asset->relativePath.c_str();
                    ImGui::SetClipboardText(path);
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();
            itemIdx++;
        }
        ImGui::EndTable();
    }
}

} // namespace gx
