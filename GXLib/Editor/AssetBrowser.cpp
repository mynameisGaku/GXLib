/// @file AssetBrowser.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"

#include "Editor/AssetBrowser.h"
#include "Core/Logger.h"

namespace gx
{

namespace
{
    /// @brief パス区切り文字をスラッシュに正規化し、末尾のスラッシュを除去する
    std::string NormalizePath(const std::string& path)
    {
        std::string result = path;
        for (auto& ch : result)
        {
            if (ch == '\\')
                ch = '/';
        }
        // 末尾のスラッシュを除去（ルートの場合を除く。例: "C:/"）
        while (result.size() > 1 && result.back() == '/')
        {
            // コロンの後のスラッシュは保持（ドライブルート。例: "C:/"）
            if (result.size() >= 3 && result[result.size() - 2] == ':')
                break;
            result.pop_back();
        }
        return result;
    }

    /// @brief childPathがparentPath配下にあるかチェックする（正規化後のプレフィックス比較）
    bool IsSubPath(const std::string& parentPath, const std::string& childPath)
    {
        std::string parent = NormalizePath(parentPath);
        std::string child  = NormalizePath(childPath);
        if (child.size() < parent.size())
            return false;
        // Windows上では大文字小文字を区別しない比較
        for (size_t i = 0; i < parent.size(); ++i)
        {
            char a = static_cast<char>(std::tolower(static_cast<unsigned char>(parent[i])));
            char b = static_cast<char>(std::tolower(static_cast<unsigned char>(child[i])));
            if (a != b)
                return false;
        }
        // 子パスは親パスと完全一致するか、次の文字がセパレータである必要がある
        if (child.size() == parent.size())
            return true;
        char next = child[parent.size()];
        return next == '/' || next == '\\';
    }

    /// @brief ファイル拡張子を小文字で取得する（ドットを含む）
    std::string GetLowerExtension(const std::filesystem::path& p)
    {
        std::string ext = p.extension().string();
        for (auto& ch : ext)
        {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return ext;
    }
} // anonymous namespace

// ============================================================================
// SetRootPath
// ============================================================================

void AssetBrowser::SetRootPath(const std::string& rootPath)
{
    m_rootPath    = NormalizePath(rootPath);
    m_currentPath = m_rootPath;
    ScanDirectory();
}

// ============================================================================
// NavigateTo
// ============================================================================

bool AssetBrowser::NavigateTo(const std::string& path)
{
    std::string normalized = NormalizePath(path);

    // パスがルート配下にあることを検証
    if (!IsSubPath(m_rootPath, normalized))
    {
        Logger::Warn("AssetBrowser: Cannot navigate outside root '%s'", m_rootPath.c_str());
        return false;
    }

    // 有効なディレクトリであることを確認
    std::error_code ec;
    if (!std::filesystem::is_directory(normalized, ec))
    {
        Logger::Warn("AssetBrowser: '%s' is not a directory", normalized.c_str());
        return false;
    }

    m_currentPath = normalized;
    ScanDirectory();
    return true;
}

// ============================================================================
// NavigateUp
// ============================================================================

bool AssetBrowser::NavigateUp()
{
    if (m_currentPath == m_rootPath)
        return false;

    std::filesystem::path current(m_currentPath);
    std::filesystem::path parent = current.parent_path();
    std::string parentStr = NormalizePath(parent.string());

    // ルートより上には移動しない
    if (!IsSubPath(m_rootPath, parentStr))
        return false;

    m_currentPath = parentStr;
    ScanDirectory();
    return true;
}

// ============================================================================
// Refresh / SetFilter
// ============================================================================

void AssetBrowser::Refresh()
{
    ScanDirectory();
}

void AssetBrowser::SetFilter(const std::string& extension)
{
    m_filter = extension;
    // 一貫したマッチングのためフィルタを小文字に変換
    for (auto& ch : m_filter)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    ScanDirectory();
}

// ============================================================================
// HandleDoubleClick
// ============================================================================

void AssetBrowser::HandleDoubleClick(int index)
{
    if (index < 0 || index >= static_cast<int>(m_entries.size()))
        return;

    const auto& entry = m_entries[index];

    if (entry.isDirectory)
    {
        NavigateTo(entry.path);
    }
    else
    {
        if (m_onAssetOpened)
        {
            m_onAssetOpened(entry.path);
        }
    }
}

// ============================================================================
// ScanDirectory
// ============================================================================

void AssetBrowser::ScanDirectory()
{
    m_entries.clear();

    if (m_currentPath.empty())
        return;

    std::error_code ec;
    if (!std::filesystem::exists(m_currentPath, ec))
    {
        Logger::Warn("AssetBrowser: directory '%s' does not exist", m_currentPath.c_str());
        return;
    }

    // ディレクトリ優先・ファイル後のソート用一時ベクタ
    std::vector<AssetBrowserEntry> dirs;
    std::vector<AssetBrowserEntry> files;

    for (const auto& dirEntry : std::filesystem::directory_iterator(m_currentPath, ec))
    {
        if (ec)
        {
            Logger::Warn("AssetBrowser: error iterating '%s'", m_currentPath.c_str());
            break;
        }

        AssetBrowserEntry entry;
        entry.name = dirEntry.path().filename().string();
        entry.path = NormalizePath(dirEntry.path().string());

        if (dirEntry.is_directory(ec))
        {
            entry.isDirectory = true;
            entry.fileSize    = 0;
            dirs.push_back(std::move(entry));
        }
        else if (dirEntry.is_regular_file(ec))
        {
            entry.isDirectory = false;
            entry.extension   = GetLowerExtension(dirEntry.path());
            entry.fileSize    = static_cast<uint64_t>(dirEntry.file_size(ec));

            // 拡張子フィルタを適用
            if (!m_filter.empty() && entry.extension != m_filter)
            {
                continue;
            }

            files.push_back(std::move(entry));
        }
    }

    // ディレクトリをアルファベット順にソート（大文字小文字を区別しない）
    auto cmpName = [](const AssetBrowserEntry& a, const AssetBrowserEntry& b)
    {
        std::string nameA = a.name;
        std::string nameB = b.name;
        for (auto& ch : nameA) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        for (auto& ch : nameB) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return nameA < nameB;
    };

    std::sort(dirs.begin(), dirs.end(), cmpName);
    std::sort(files.begin(), files.end(), cmpName);

    // ディレクトリを先に、次にファイルを格納
    m_entries.reserve(dirs.size() + files.size());
    for (auto& d : dirs)
        m_entries.push_back(std::move(d));
    for (auto& f : files)
        m_entries.push_back(std::move(f));
}

} // namespace gx
