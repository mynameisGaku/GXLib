/// @file AssetDatabase.cpp
/// @brief アセットデータベース実装
#include "pch_common.h"
#include "Core/AssetDatabase.h"
#include "Core/Logger.h"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace gx
{

// ============================================================================
// シングルトン
// ============================================================================

AssetDatabase& AssetDatabase::Instance()
{
    static AssetDatabase instance;
    return instance;
}

// ============================================================================
// 静的ヘルパー
// ============================================================================

std::string AssetDatabase::ToLower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string AssetDatabase::NormalizePath(const std::string& path)
{
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

AssetType AssetDatabase::ClassifyExtension(const std::string& ext)
{
    std::string lower = ToLower(ext);

    // テクスチャ
    if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" ||
        lower == ".bmp" || lower == ".tga" || lower == ".dds" || lower == ".hdr")
        return AssetType::Texture;

    // モデル
    if (lower == ".fbx" || lower == ".obj" || lower == ".gltf" ||
        lower == ".glb" || lower == ".gxm" || lower == ".gxmdl")
        return AssetType::Model;

    // サウンド
    if (lower == ".wav" || lower == ".mp3")
        return AssetType::Sound;

    // 音楽
    if (lower == ".ogg")
        return AssetType::Music;

    // フォント
    if (lower == ".ttf" || lower == ".otf" || lower == ".spritefont")
        return AssetType::Font;

    // シェーダー
    if (lower == ".hlsl" || lower == ".hlsli" || lower == ".cso")
        return AssetType::Shader;

    // スクリプト
    if (lower == ".lua")
        return AssetType::Script;

    // シーン
    if (lower == ".gxscene")
        return AssetType::Scene;

    // マテリアル
    if (lower == ".gxmat")
        return AssetType::Material;

    // アニメーション
    if (lower == ".gxanim")
        return AssetType::Animation;

    // プレハブ
    if (lower == ".gxprefab")
        return AssetType::Prefab;

    // タイルマップ
    if (lower == ".tmx" || lower == ".tmj")
        return AssetType::Tilemap;

    // コンフィグ
    if (lower == ".json" || lower == ".xml" || lower == ".ini")
        return AssetType::Config;

    return AssetType::Unknown;
}

// ============================================================================
// レガシーAPI
// ============================================================================

bool AssetDatabase::Scan(const std::string& rootDirectory, bool recursive)
{
    m_lastRootDirectory = rootDirectory;
    m_lastRecursive = recursive;
    m_entries.clear();

    std::error_code ec;
    if (!fs::exists(rootDirectory, ec) || !fs::is_directory(rootDirectory, ec))
        return false;

    auto processEntry = [&](const fs::directory_entry& entry)
    {
        if (!entry.is_regular_file(ec)) return;

        AssetEntry asset;
        asset.path = NormalizePath(entry.path().string());
        asset.fullPath = asset.path;
        asset.name = entry.path().filename().string();
        asset.extension = ToLower(entry.path().extension().string());
        asset.type = ClassifyExtension(entry.path().extension().string());
        asset.fileSize = static_cast<uint64_t>(entry.file_size(ec));

        auto ftime = entry.last_write_time(ec);
        auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::clock_cast<std::chrono::system_clock>(ftime));
        asset.lastModified = static_cast<uint64_t>(sctp.time_since_epoch().count());

        // 相対パスを計算
        std::error_code relEc;
        auto relPath = fs::relative(entry.path(), rootDirectory, relEc);
        if (!relEc)
        {
            asset.relativePath = NormalizePath(relPath.string());
        }

        std::string key = NormalizePath(entry.path().string());
        m_entries[key] = std::move(asset);
    };

    if (recursive)
    {
        for (const auto& entry : fs::recursive_directory_iterator(rootDirectory, ec))
            processEntry(entry);
    }
    else
    {
        for (const auto& entry : fs::directory_iterator(rootDirectory, ec))
            processEntry(entry);
    }

    return true;
}

const AssetEntry* AssetDatabase::Find(const std::string& path) const
{
    std::string key = NormalizePath(path);
    auto it = m_entries.find(key);
    if (it != m_entries.end()) return &it->second;
    return nullptr;
}

std::vector<const AssetEntry*> AssetDatabase::Search(const std::string& pattern) const
{
    std::string lowerPattern = ToLower(pattern);
    std::vector<const AssetEntry*> results;

    for (const auto& [key, entry] : m_entries)
    {
        std::string lowerPath = ToLower(entry.path);
        if (lowerPath.find(lowerPattern) != std::string::npos)
            results.push_back(&entry);
    }

    return results;
}

bool AssetDatabase::Refresh()
{
    if (m_lastRootDirectory.empty()) return false;
    return Scan(m_lastRootDirectory, m_lastRecursive);
}

void AssetDatabase::Clear()
{
    m_entries.clear();
}

// ============================================================================
// 新API (Phase 7)
// ============================================================================

void AssetDatabase::Initialize(const std::string& rootPath)
{
    m_lastRootDirectory = rootPath;
    m_lastRecursive = true;
    ScanAll();
}

void AssetDatabase::ScanAll()
{
    if (m_lastRootDirectory.empty())
    {
        Logger::Warn("AssetDatabase::ScanAll: no root path set");
        return;
    }

    m_entries.clear();

    std::error_code ec;
    if (!fs::exists(m_lastRootDirectory, ec) || !fs::is_directory(m_lastRootDirectory, ec))
    {
        Logger::Warn("AssetDatabase::ScanAll: root path '%s' does not exist or is not a directory",
                     m_lastRootDirectory.c_str());
        return;
    }

    ScanDirectory(fs::path(m_lastRootDirectory));
}

void AssetDatabase::ScanDirectory(const std::filesystem::path& dir)
{
    std::error_code ec;

    for (const auto& entry : fs::recursive_directory_iterator(dir, ec))
    {
        if (ec)
        {
            Logger::Warn("AssetDatabase::ScanDirectory: iteration error in '%s'",
                         dir.string().c_str());
            break;
        }

        if (!entry.is_regular_file(ec))
            continue;

        AssetEntry asset;
        asset.path = NormalizePath(entry.path().string());
        asset.fullPath = asset.path;
        asset.name = entry.path().filename().string();
        asset.extension = ToLower(entry.path().extension().string());
        asset.type = ClassifyExtension(entry.path().extension().string());
        asset.fileSize = static_cast<uint64_t>(entry.file_size(ec));

        auto ftime = entry.last_write_time(ec);
        auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::clock_cast<std::chrono::system_clock>(ftime));
        asset.lastModified = static_cast<uint64_t>(sctp.time_since_epoch().count());

        // 相対パスを計算
        std::error_code relEc;
        auto relPath = fs::relative(entry.path(), m_lastRootDirectory, relEc);
        if (!relEc)
        {
            asset.relativePath = NormalizePath(relPath.string());
        }

        // キーは正規化された完全パス（レガシー互換のため）
        std::string key = asset.path;
        m_entries[key] = std::move(asset);
    }
}

int AssetDatabase::DetectChanges()
{
    int changeCount = 0;
    std::error_code ec;

    for (auto& [key, asset] : m_entries)
    {
        fs::path filePath(asset.fullPath);

        if (!fs::exists(filePath, ec))
        {
            if (!asset.dirty)
            {
                asset.dirty = true;
                ++changeCount;
            }
            continue;
        }

        auto ftime = fs::last_write_time(filePath, ec);
        if (ec) continue;

        auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::clock_cast<std::chrono::system_clock>(ftime));
        uint64_t currentModified = static_cast<uint64_t>(sctp.time_since_epoch().count());

        if (currentModified != asset.lastModified)
        {
            asset.dirty = true;
            asset.lastModified = currentModified;
            asset.fileSize = static_cast<uint64_t>(fs::file_size(filePath, ec));
            ++changeCount;
        }
    }

    return changeCount;
}

const AssetEntry* AssetDatabase::FindAsset(const std::string& relativePath) const
{
    std::string normalizedRel = NormalizePath(relativePath);

    // 相対パスで検索
    for (const auto& [key, entry] : m_entries)
    {
        if (entry.relativePath == normalizedRel)
            return &entry;
    }

    // フォールバック: 完全パスで検索
    auto it = m_entries.find(normalizedRel);
    if (it != m_entries.end())
        return &it->second;

    return nullptr;
}

std::vector<const AssetEntry*> AssetDatabase::FindByType(AssetType type) const
{
    std::vector<const AssetEntry*> results;
    for (const auto& [key, entry] : m_entries)
    {
        if (entry.type == type)
            results.push_back(&entry);
    }
    return results;
}

std::vector<const AssetEntry*> AssetDatabase::FindByName(const std::string& pattern) const
{
    std::string lowerPattern = ToLower(pattern);
    std::vector<const AssetEntry*> results;

    for (const auto& [key, entry] : m_entries)
    {
        std::string lowerName = ToLower(entry.name);
        if (lowerName.find(lowerPattern) != std::string::npos)
            results.push_back(&entry);
    }

    return results;
}

// static
AssetType AssetDatabase::DetectType(const std::string& extension)
{
    return ClassifyExtension(extension);
}

ImportSettings* AssetDatabase::GetImportSettings(const std::string& relativePath)
{
    std::string normalizedRel = NormalizePath(relativePath);

    // 相対パスで検索
    for (auto& [key, entry] : m_entries)
    {
        if (entry.relativePath == normalizedRel)
            return &entry.importSettings;
    }

    // フォールバック: 完全パスで検索
    auto it = m_entries.find(normalizedRel);
    if (it != m_entries.end())
        return &it->second.importSettings;

    return nullptr;
}

void AssetDatabase::SetImportSetting(const std::string& relativePath,
                                      const std::string& key, const std::string& value)
{
    ImportSettings* settings = GetImportSettings(relativePath);
    if (settings)
    {
        settings->Set(key, value);
    }
    else
    {
        Logger::Warn("AssetDatabase::SetImportSetting: asset '%s' not found",
                     relativePath.c_str());
    }
}

bool AssetDatabase::SaveMetadata(const std::string& filePath) const
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        Logger::Error("AssetDatabase::SaveMetadata: failed to open '%s' for writing",
                      filePath.c_str());
        return false;
    }

    // フォーマット: 設定ごとに1行 — "relativePath:key=value"
    for (const auto& [entryKey, entry] : m_entries)
    {
        const std::string& assetPath = entry.relativePath.empty() ? entry.path : entry.relativePath;

        for (const auto& [settingKey, settingValue] : entry.importSettings.values)
        {
            file << assetPath << ":" << settingKey << "=" << settingValue << "\n";
        }
    }

    return true;
}

bool AssetDatabase::LoadMetadata(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        Logger::Warn("AssetDatabase::LoadMetadata: failed to open '%s' for reading",
                     filePath.c_str());
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        // "path:key=value" をパース
        auto colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string assetPath = line.substr(0, colonPos);
        std::string remainder = line.substr(colonPos + 1);

        auto equalsPos = remainder.find('=');
        if (equalsPos == std::string::npos)
            continue;

        std::string key = remainder.substr(0, equalsPos);
        std::string value = remainder.substr(equalsPos + 1);

        // アセットを検索して設定を適用
        ImportSettings* settings = GetImportSettings(assetPath);
        if (settings)
        {
            settings->Set(key, value);
        }
    }

    return true;
}

void AssetDatabase::Shutdown()
{
    m_entries.clear();
    m_lastRootDirectory.clear();
    m_lastRecursive = true;
}

} // namespace gx
