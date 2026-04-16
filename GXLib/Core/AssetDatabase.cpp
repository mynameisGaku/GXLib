/// @file AssetDatabase.cpp
/// @brief アセットデータベース実装
#include "pch_common.h"
#include "Core/AssetDatabase.h"
#include "Core/AssetMeta.h"
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

gx::String AssetDatabase::ToLower(const gx::String& s)
{
    gx::String result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

gx::String AssetDatabase::NormalizePath(const gx::String& path)
{
    gx::String result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

AssetType AssetDatabase::ClassifyExtension(const gx::String& ext)
{
    gx::String lower = ToLower(ext);

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

bool AssetDatabase::Scan(const gx::String& rootDirectory, bool recursive)
{
    m_lastRootDirectory = rootDirectory;
    m_lastRecursive = recursive;
    m_entries.clear();

    std::error_code ec;
    if (!fs::exists(fs::path(rootDirectory.c_str()), ec) || !fs::is_directory(fs::path(rootDirectory.c_str()), ec))
        return false;

    auto processEntry = [&](const fs::directory_entry& entry)
    {
        if (!entry.is_regular_file(ec)) return;

        AssetEntry asset;
        asset.path = NormalizePath(gx::String(entry.path().string().c_str()));
        asset.fullPath = asset.path;
        asset.name = gx::String(entry.path().filename().string().c_str());
        asset.extension = ToLower(gx::String(entry.path().extension().string().c_str()));
        asset.type = ClassifyExtension(gx::String(entry.path().extension().string().c_str()));
        asset.fileSize = static_cast<uint64_t>(entry.file_size(ec));

        auto ftime = entry.last_write_time(ec);
        auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::clock_cast<std::chrono::system_clock>(ftime));
        asset.lastModified = static_cast<uint64_t>(sctp.time_since_epoch().count());

        // 相対パスを計算
        std::error_code relEc;
        auto relPath = fs::relative(entry.path(), fs::path(rootDirectory.c_str()), relEc);
        if (!relEc)
        {
            asset.relativePath = NormalizePath(gx::String(relPath.string().c_str()));
        }

        gx::String key = NormalizePath(gx::String(entry.path().string().c_str()));
        m_entries[key] = std::move(asset);
    };

    if (recursive)
    {
        for (const auto& entry : fs::recursive_directory_iterator(fs::path(rootDirectory.c_str()), ec))
            processEntry(entry);
    }
    else
    {
        for (const auto& entry : fs::directory_iterator(fs::path(rootDirectory.c_str()), ec))
            processEntry(entry);
    }

    return true;
}

const AssetEntry* AssetDatabase::Find(const gx::String& path) const
{
    gx::String key = NormalizePath(path);
    auto it = m_entries.find(key);
    if (it != m_entries.end()) return &it->second;
    return nullptr;
}

gx::Vector<const AssetEntry*> AssetDatabase::Search(const gx::String& pattern) const
{
    gx::String lowerPattern = ToLower(pattern);
    gx::Vector<const AssetEntry*> results;

    for (const auto& [key, entry] : m_entries)
    {
        gx::String lowerPath = ToLower(entry.path);
        if (lowerPath.find(lowerPattern) != gx::String::npos)
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
    m_guidToPath.clear();
}

// ============================================================================
// 新API (Phase 7)
// ============================================================================

void AssetDatabase::Initialize(const gx::String& rootPath)
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
    if (!fs::exists(fs::path(m_lastRootDirectory.c_str()), ec) || !fs::is_directory(fs::path(m_lastRootDirectory.c_str()), ec))
    {
        Logger::Warn("AssetDatabase::ScanAll: root path '%s' does not exist or is not a directory",
                     m_lastRootDirectory.c_str());
        return;
    }

    ScanDirectory(fs::path(m_lastRootDirectory.c_str()));
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

        // .meta ファイルはスキップ（アセットとは別扱い）
        gx::String ext = ToLower(gx::String(entry.path().extension().string().c_str()));
        if (ext == ".meta")
            continue;

        AssetEntry asset;
        asset.path = NormalizePath(gx::String(entry.path().string().c_str()));
        asset.fullPath = asset.path;
        asset.name = gx::String(entry.path().filename().string().c_str());
        asset.extension = ToLower(gx::String(entry.path().extension().string().c_str()));
        asset.type = ClassifyExtension(gx::String(entry.path().extension().string().c_str()));
        asset.fileSize = static_cast<uint64_t>(entry.file_size(ec));

        auto ftime = entry.last_write_time(ec);
        auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::clock_cast<std::chrono::system_clock>(ftime));
        asset.lastModified = static_cast<uint64_t>(sctp.time_since_epoch().count());

        // 相対パスを計算
        std::error_code relEc;
        auto relPath = fs::relative(entry.path(), fs::path(m_lastRootDirectory.c_str()), relEc);
        if (!relEc)
        {
            asset.relativePath = NormalizePath(gx::String(relPath.string().c_str()));
        }

        // .meta ファイルからGUIDを読み込み、なければ自動生成
        gx::String metaPath = AssetMeta::GetMetaPath(asset.fullPath);
        AssetMetaData metaData;
        if (AssetMeta::Load(metaPath, metaData))
        {
            asset.guid = metaData.guid;
            // インポート設定をマージ
            for (const auto& [k, v] : metaData.importSettings)
                asset.importSettings.Set(k, v);
        }
        else
        {
            // .meta がない場合は新規生成して書き出し
            metaData.guid = GUID::Generate();
            asset.guid = metaData.guid;
            AssetMeta::Save(metaPath, metaData);
        }

        // GUID逆引きマップに登録
        if (asset.guid.IsValid())
        {
            gx::String relPathStr = asset.relativePath.empty() ? asset.path : asset.relativePath;
            m_guidToPath[asset.guid] = relPathStr;
        }

        // キーは正規化された完全パス（レガシー互換のため）
        gx::String key = asset.path;
        m_entries[key] = std::move(asset);
    }
}

int AssetDatabase::DetectChanges()
{
    int changeCount = 0;
    std::error_code ec;

    for (auto& [key, asset] : m_entries)
    {
        fs::path filePath(asset.fullPath.c_str());

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

const AssetEntry* AssetDatabase::FindAsset(const gx::String& relativePath) const
{
    gx::String normalizedRel = NormalizePath(relativePath);

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

gx::Vector<const AssetEntry*> AssetDatabase::FindByType(AssetType type) const
{
    gx::Vector<const AssetEntry*> results;
    for (const auto& [key, entry] : m_entries)
    {
        if (entry.type == type)
            results.push_back(&entry);
    }
    return results;
}

gx::Vector<const AssetEntry*> AssetDatabase::FindByName(const gx::String& pattern) const
{
    gx::String lowerPattern = ToLower(pattern);
    gx::Vector<const AssetEntry*> results;

    for (const auto& [key, entry] : m_entries)
    {
        gx::String lowerName = ToLower(entry.name);
        if (lowerName.find(lowerPattern) != gx::String::npos)
            results.push_back(&entry);
    }

    return results;
}

// static
AssetType AssetDatabase::DetectType(const gx::String& extension)
{
    return ClassifyExtension(extension);
}

ImportSettings* AssetDatabase::GetImportSettings(const gx::String& relativePath)
{
    gx::String normalizedRel = NormalizePath(relativePath);

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

void AssetDatabase::SetImportSetting(const gx::String& relativePath,
                                      const gx::String& key, const gx::String& value)
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

bool AssetDatabase::SaveMetadata(const gx::String& filePath) const
{
    std::ofstream file(filePath.c_str());
    if (!file.is_open())
    {
        Logger::Error("AssetDatabase::SaveMetadata: failed to open '%s' for writing",
                      filePath.c_str());
        return false;
    }

    // フォーマット: 設定ごとに1行 — "relativePath:key=value"
    for (const auto& [entryKey, entry] : m_entries)
    {
        const gx::String& assetPath = entry.relativePath.empty() ? entry.path : entry.relativePath;

        for (const auto& [settingKey, settingValue] : entry.importSettings.values)
        {
            file << assetPath << ":" << settingKey << "=" << settingValue << "\n";
        }
    }

    return true;
}

bool AssetDatabase::LoadMetadata(const gx::String& filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
    {
        Logger::Warn("AssetDatabase::LoadMetadata: failed to open '%s' for reading",
                     filePath.c_str());
        return false;
    }

    gx::String line;
    while (gx::container::getline(file, line))
    {
        if (line.empty())
            continue;

        // "path:key=value" をパース
        auto colonPos = line.find(':');
        if (colonPos == gx::String::npos)
            continue;

        gx::String assetPath = line.substr(0, colonPos);
        gx::String remainder = line.substr(colonPos + 1);

        auto equalsPos = remainder.find('=');
        if (equalsPos == gx::String::npos)
            continue;

        gx::String key = remainder.substr(0, equalsPos);
        gx::String value = remainder.substr(equalsPos + 1);

        // アセットを検索して設定を適用
        ImportSettings* settings = GetImportSettings(assetPath);
        if (settings)
        {
            settings->Set(key, value);
        }
    }

    return true;
}

// ============================================================================
// GUID 関連
// ============================================================================

const AssetEntry* AssetDatabase::FindByGUID(const GUID& guid) const
{
    auto it = m_guidToPath.find(guid);
    if (it == m_guidToPath.end())
        return nullptr;

    // 相対パスからエントリを検索
    return FindAsset(it->second);
}

GUID AssetDatabase::GetOrCreateGUID(const gx::String& relativePath)
{
    gx::String normalizedRel = NormalizePath(relativePath);

    // 既にGUIDが割り当て済みか確認
    for (auto& [key, entry] : m_entries)
    {
        if (entry.relativePath == normalizedRel)
        {
            if (entry.guid.IsValid())
                return entry.guid;

            // GUIDが未設定の場合は生成
            entry.guid = GUID::Generate();
            m_guidToPath[entry.guid] = normalizedRel;

            // .meta ファイルにも書き出し
            if (!m_lastRootDirectory.empty())
            {
                gx::String fullPath = entry.fullPath;
                gx::String metaPath = AssetMeta::GetMetaPath(fullPath);
                AssetMetaData metaData;
                metaData.guid = entry.guid;
                AssetMeta::Save(metaPath, metaData);
            }

            return entry.guid;
        }
    }

    return GUID{};
}

bool AssetDatabase::OnAssetMoved(const gx::String& oldRelativePath, const gx::String& newRelativePath)
{
    gx::String oldNorm = NormalizePath(oldRelativePath);
    gx::String newNorm = NormalizePath(newRelativePath);

    // 旧パスのエントリを探す
    gx::String oldKey;
    GUID movedGuid;
    AssetEntry movedEntry;

    for (auto& [key, entry] : m_entries)
    {
        if (entry.relativePath == oldNorm)
        {
            oldKey = key;
            movedGuid = entry.guid;
            movedEntry = entry;
            break;
        }
    }

    if (oldKey.empty())
        return false;

    // 旧エントリを削除
    m_entries.erase(oldKey);

    // 新パスで再登録（GUIDは維持）
    movedEntry.relativePath = newNorm;
    movedEntry.name = gx::String(
        std::filesystem::path(newNorm.c_str()).filename().string().c_str());

    // fullPathを更新
    if (!m_lastRootDirectory.empty())
    {
        std::filesystem::path newFullPath =
            std::filesystem::path(m_lastRootDirectory.c_str()) / newNorm.c_str();
        movedEntry.path = NormalizePath(gx::String(newFullPath.string().c_str()));
        movedEntry.fullPath = movedEntry.path;
    }

    // GUID逆引きマップ更新
    if (movedGuid.IsValid())
    {
        m_guidToPath[movedGuid] = newNorm;
    }

    gx::String newKey = movedEntry.path;
    m_entries[newKey] = std::move(movedEntry);

    return true;
}

void AssetDatabase::Shutdown()
{
    m_entries.clear();
    m_guidToPath.clear();
    m_lastRootDirectory.clear();
    m_lastRecursive = true;
}

} // namespace gx
