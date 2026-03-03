/// @file BundleManager.cpp
/// @brief バンドル依存解決マネージャー実装
#include "pch_common.h"
#include "IO/BundleManager.h"
#include "IO/PakFileProvider.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// シングルトン
// ============================================================================

BundleManager& BundleManager::Instance()
{
    static BundleManager instance;
    return instance;
}

// ============================================================================
// LoadBundle — gxpak を開いてマニフェスト情報を構築する
// ============================================================================

bool BundleManager::LoadBundle(const gx::String& pakPath)
{
    // 既にロード済みなら何もしない
    if (m_bundles.count(pakPath))
        return true;

    // PakFileProvider でバンドルを開く
    auto provider = std::make_shared<PakFileProvider>();
    if (!provider->Open(pakPath))
    {
        GX_LOG_ERROR("BundleManager: Failed to open bundle '%s'", pakPath.c_str());
        return false;
    }

    // マニフェストを空で作成（LoadBundle は TOC からパスのみ取得）
    // GUID情報は RegisterBundleInfo() で外部から供給する設計
    BundleManifest manifest;
    manifest.pakPath = pakPath;

    m_bundles[pakPath] = std::move(manifest);
    return true;
}

// ============================================================================
// UnloadBundle
// ============================================================================

void BundleManager::UnloadBundle(const gx::String& pakPath)
{
    auto it = m_bundles.find(pakPath);
    if (it == m_bundles.end())
        return;

    // GUID逆引きマップからこのバンドルのエントリを削除
    const auto& manifest = it->second;
    for (const auto& [guid, _] : manifest.containedAssets)
    {
        m_guidToBundle.erase(guid);
    }

    m_bundles.erase(it);
}

// ============================================================================
// RegisterBundleInfo — テスト用・ファイルなしでの登録
// ============================================================================

void BundleManager::RegisterBundleInfo(const BundleManifest& manifest)
{
    m_bundles[manifest.pakPath] = manifest;

    // GUID逆引きマップに登録
    for (const auto& [guid, path] : manifest.containedAssets)
    {
        m_guidToBundle[guid] = manifest.pakPath;
    }
}

// ============================================================================
// Resolve — GUID → バンドル + 内部パス
// ============================================================================

ResolveResult BundleManager::Resolve(const GUID& guid) const
{
    ResolveResult result;

    auto bundleIt = m_guidToBundle.find(guid);
    if (bundleIt == m_guidToBundle.end())
        return result;

    const gx::String& bundlePath = bundleIt->second;
    auto manifestIt = m_bundles.find(bundlePath);
    if (manifestIt == m_bundles.end())
        return result;

    auto assetIt = manifestIt->second.containedAssets.find(guid);
    if (assetIt == manifestIt->second.containedAssets.end())
        return result;

    result.bundlePath = bundlePath;
    result.internalPath = assetIt->second;
    result.found = true;
    return result;
}

// ============================================================================
// AreDependenciesMet
// ============================================================================

bool BundleManager::AreDependenciesMet(const gx::String& pakPath) const
{
    auto it = m_bundles.find(pakPath);
    if (it == m_bundles.end())
        return false;

    for (const auto& depGuid : it->second.externalDependencies)
    {
        if (m_guidToBundle.find(depGuid) == m_guidToBundle.end())
            return false;
    }

    return true;
}

// ============================================================================
// GetMissingDependencies
// ============================================================================

gx::Vector<GUID> BundleManager::GetMissingDependencies(const gx::String& pakPath) const
{
    gx::Vector<GUID> missing;

    auto it = m_bundles.find(pakPath);
    if (it == m_bundles.end())
        return missing;

    for (const auto& depGuid : it->second.externalDependencies)
    {
        if (m_guidToBundle.find(depGuid) == m_guidToBundle.end())
            missing.push_back(depGuid);
    }

    return missing;
}

// ============================================================================
// ReadByGUID
// ============================================================================

FileData BundleManager::ReadByGUID(const GUID& guid) const
{
    ResolveResult resolved = Resolve(guid);
    if (!resolved)
        return FileData{};

    // バンドルを開いてエントリを読み込む
    PakFileProvider provider;
    if (!provider.Open(resolved.bundlePath))
        return FileData{};

    return provider.Read(resolved.internalPath);
}

// ============================================================================
// Clear
// ============================================================================

void BundleManager::Clear()
{
    m_guidToBundle.clear();
    m_bundles.clear();
}

} // namespace gx
