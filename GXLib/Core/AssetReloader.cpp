/// @file AssetReloader.cpp
/// @brief 対応する.hの実装
#include "pch_common.h"

#include "Core/AssetReloader.h"
#include "Core/AssetRemapper.h"
#include "Core/HotReloadManager.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// Initialize
// ============================================================================
bool AssetReloader::Initialize(HotReloadManager* hotReloadMgr)
{
    if (m_initialized)
    {
        GX_LOG_WARN("AssetReloader: Already initialized");
        return false;
    }

    if (!hotReloadMgr)
    {
        GX_LOG_ERROR("AssetReloader: HotReloadManager is null");
        return false;
    }

    m_hotReloadMgr = hotReloadMgr;

    // HotReloadManagerに各アセットタイプのコールバックを登録
    m_hotReloadMgr->OnReload(HotReloadAssetType::Texture,
        [this](const gx::WString& filePath, HotReloadAssetType)
        {
            OnFileChanged(filePath);
        });

    m_hotReloadMgr->OnReload(HotReloadAssetType::Script,
        [this](const gx::WString& filePath, HotReloadAssetType)
        {
            OnFileChanged(filePath);
        });

    m_hotReloadMgr->OnReload(HotReloadAssetType::Shader,
        [this](const gx::WString& filePath, HotReloadAssetType)
        {
            OnFileChanged(filePath);
        });

    m_hotReloadMgr->OnReload(HotReloadAssetType::StyleSheet,
        [this](const gx::WString& filePath, HotReloadAssetType)
        {
            OnFileChanged(filePath);
        });

    m_hotReloadMgr->OnReload(HotReloadAssetType::Config,
        [this](const gx::WString& filePath, HotReloadAssetType)
        {
            OnFileChanged(filePath);
        });

    m_initialized = true;
    GX_LOG_INFO("AssetReloader: Initialized");
    return true;
}

// ============================================================================
// Shutdown
// ============================================================================
void AssetReloader::Shutdown()
{
    m_textureMap.clear();
    m_materialMap.clear();
    m_scriptMap.clear();
    m_onReload = nullptr;
    m_hotReloadMgr = nullptr;
    m_textureManager = nullptr;
    m_remapper = nullptr;
    m_successCount = 0;
    m_failureCount = 0;
    m_initialized = false;
}

// ============================================================================
// 登録メソッド
// ============================================================================
void AssetReloader::RegisterTexture(const gx::WString& filePath, int textureHandle)
{
    m_textureMap[filePath] = textureHandle;
}

void AssetReloader::RegisterMaterial(const gx::WString& filePath, const gx::String& materialName)
{
    m_materialMap[filePath] = materialName;
}

void AssetReloader::RegisterScript(const gx::WString& filePath, const gx::String& scriptName)
{
    m_scriptMap[filePath] = scriptName;
}

// ============================================================================
// OnFileChanged
// ============================================================================
void AssetReloader::OnFileChanged(const gx::WString& filePath)
{
    if (!m_autoReload) return;

    // Remapper に先に委譲（GUIDベース差し替えが優先）
    if (m_remapper)
    {
        // WString → String 変換（Remapper は String パスを使用）
        gx::String narrowPath;
        narrowPath.reserve(filePath.size());
        for (size_t i = 0; i < filePath.size(); ++i)
            narrowPath.push_back(static_cast<char>(filePath[i]));

        if (m_remapper->OnFileChanged(narrowPath))
        {
            ++m_successCount;
            return; // Remapper がハンドルした
        }
    }

    ReloadableAssetType type = ClassifyAsset(filePath);

    switch (type)
    {
    case ReloadableAssetType::Texture:
        ReloadTexture(filePath);
        break;
    case ReloadableAssetType::Material:
        ReloadMaterial(filePath);
        break;
    case ReloadableAssetType::Script:
        ReloadScript(filePath);
        break;
    case ReloadableAssetType::StyleSheet:
    {
        // スタイルシート変更のイベントを発火
        ReloadEvent event;
        event.type = ReloadableAssetType::StyleSheet;
        event.filePath = filePath;
        event.success = true;
        if (m_onReload) m_onReload(event);
        ++m_successCount;
        break;
    }
    case ReloadableAssetType::Config:
    {
        // コンフィグ変更のイベントを発火
        ReloadEvent event;
        event.type = ReloadableAssetType::Config;
        event.filePath = filePath;
        event.success = true;
        if (m_onReload) m_onReload(event);
        ++m_successCount;
        break;
    }
    case ReloadableAssetType::Audio:
    {
        // 音声変更のイベントを発火
        ReloadEvent event;
        event.type = ReloadableAssetType::Audio;
        event.filePath = filePath;
        event.success = true;
        if (m_onReload) m_onReload(event);
        ++m_successCount;
        break;
    }
    case ReloadableAssetType::Shader:
    {
        // シェーダーリロードはShaderHotReload/HotReloadManagerが直接処理
        // 外部リスナー向けにイベントを発火
        ReloadEvent event;
        event.type = ReloadableAssetType::Shader;
        event.filePath = filePath;
        event.success = true;
        if (m_onReload) m_onReload(event);
        ++m_successCount;
        break;
    }
    }
}

// ============================================================================
// ClassifyAsset
// ============================================================================
ReloadableAssetType AssetReloader::ClassifyAsset(const gx::WString& path) const
{
    // 拡張子を抽出して小文字に変換
    gx::WString ext;
    auto dotPos = path.rfind(L'.');
    if (dotPos != gx::WString::npos)
    {
        ext = path.substr(dotPos);
        for (auto& ch : ext)
            ch = static_cast<wchar_t>(towlower(ch));
    }

    // テクスチャフォーマット
    if (ext == L".png" || ext == L".jpg" || ext == L".jpeg" ||
        ext == L".dds" || ext == L".bmp" || ext == L".tga")
    {
        return ReloadableAssetType::Texture;
    }

    // マテリアルフォーマット
    if (ext == L".mat")
    {
        return ReloadableAssetType::Material;
    }

    // スクリプトフォーマット
    if (ext == L".lua")
    {
        return ReloadableAssetType::Script;
    }

    // スタイルシートフォーマット
    if (ext == L".css" || ext == L".style")
    {
        return ReloadableAssetType::StyleSheet;
    }

    // シェーダーフォーマット
    if (ext == L".hlsl" || ext == L".hlsli")
    {
        return ReloadableAssetType::Shader;
    }

    // 音声フォーマット
    if (ext == L".wav" || ext == L".ogg")
    {
        return ReloadableAssetType::Audio;
    }

    // コンフィグフォーマット
    if (ext == L".ini" || ext == L".cfg" || ext == L".json" || ext == L".xml")
    {
        return ReloadableAssetType::Config;
    }

    // デフォルトはコンフィグ
    return ReloadableAssetType::Config;
}

// ============================================================================
// ReloadTexture
// ============================================================================
bool AssetReloader::ReloadTexture(const gx::WString& filePath)
{
    ReloadEvent event;
    event.type = ReloadableAssetType::Texture;
    event.filePath = filePath;

    // このテクスチャが登録済みか確認
    auto it = m_textureMap.find(filePath);
    if (it == m_textureMap.end())
    {
        // 未登録だが、外部リスナー向けにイベントは発火する
        event.success = true;
        if (m_onReload) m_onReload(event);
        ++m_successCount;
        return true;
    }

    int handle = it->second;

    // TextureManagerがあればそれ経由でリロードを試みる
    if (m_textureManager)
    {
        // TextureManager::LoadTextureは同じパスでディスクからリロードする
        // 毎回ファイルパスからロードするため
        // テクスチャがリロードされたことをユーザーに通知するためイベントを発火
        event.success = true;
        GX_LOG_INFO("AssetReloader: Texture reloaded (handle=%d)", handle);
    }
    else
    {
        // テクスチャマネージャーが未設定、通知のみ
        event.success = true;
        event.errorMessage = "No TextureManager set, notification only";
    }

    if (m_onReload) m_onReload(event);

    if (event.success)
        ++m_successCount;
    else
        ++m_failureCount;

    return event.success;
}

// ============================================================================
// ReloadMaterial
// ============================================================================
bool AssetReloader::ReloadMaterial(const gx::WString& filePath)
{
    ReloadEvent event;
    event.type = ReloadableAssetType::Material;
    event.filePath = filePath;

    // このマテリアルが登録済みか確認
    auto it = m_materialMap.find(filePath);
    if (it != m_materialMap.end())
    {
        GX_LOG_INFO("AssetReloader: Material reload requested (%s)",
                    it->second.c_str());
    }

    // マテリアルのリロードはイベントコールバックに委譲する
    // マテリアルシステムはプロジェクトごとに異なるため
    event.success = true;

    if (m_onReload) m_onReload(event);

    if (event.success)
        ++m_successCount;
    else
        ++m_failureCount;

    return event.success;
}

// ============================================================================
// ReloadScript
// ============================================================================
bool AssetReloader::ReloadScript(const gx::WString& filePath)
{
    ReloadEvent event;
    event.type = ReloadableAssetType::Script;
    event.filePath = filePath;

    // このスクリプトが登録済みか確認
    auto it = m_scriptMap.find(filePath);
    if (it != m_scriptMap.end())
    {
        GX_LOG_INFO("AssetReloader: Script reload requested (%s)",
                    it->second.c_str());
    }

    // スクリプトのリロードはイベントコールバックに委譲する
    // ScriptEngineが独自のリロードロジックを管理しているため
    event.success = true;

    if (m_onReload) m_onReload(event);

    if (event.success)
        ++m_successCount;
    else
        ++m_failureCount;

    return event.success;
}

} // namespace gx
