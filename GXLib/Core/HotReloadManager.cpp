/// @file HotReloadManager.cpp
/// @brief 対応する.hの実装
#include "pch_common.h"
#include "Core/HotReloadManager.h"
#include "IO/FileWatcher.h"
#include "Core/Logger.h"

namespace gx {

HotReloadManager::HotReloadManager() = default;

HotReloadManager::~HotReloadManager()
{
    Shutdown();
}

bool HotReloadManager::Initialize(FileWatcher* watcher)
{
    if (m_initialized)
    {
        GX_LOG_WARN("HotReloadManager: Already initialized");
        return false;
    }

    if (!watcher)
    {
        GX_LOG_ERROR("HotReloadManager: FileWatcher is null");
        return false;
    }

    m_watcher = watcher;
    m_initialized = true;
    GX_LOG_INFO("HotReloadManager: Initialized (debounce=%.2fs)", m_debounceTime);
    return true;
}

void HotReloadManager::Shutdown()
{
    if (!m_initialized)
        return;

    m_pendingChanges.clear();
    m_callbacks.clear();
    m_watcher = nullptr;
    m_watchedDirCount = 0;
    m_initialized = false;
}

void HotReloadManager::WatchAssetDirectory(const gx::WString& dir, HotReloadAssetType type)
{
    if (!m_initialized || !m_watcher)
    {
        GX_LOG_ERROR("HotReloadManager: Not initialized");
        return;
    }

    // wstring → string 変換（FileWatcher は gx::String ベース）
    std::wstring wdir(dir.c_str());
    gx::String dirNarrow(std::string(wdir.begin(), wdir.end()));

    // FileWatcher にコールバックを登録
    m_watcher->Watch(dirNarrow,
        [this](const gx::String& path)
        {
            // gx::String → gx::WString 変換
            std::string spath(path.c_str());
            gx::WString wpath(std::wstring(spath.begin(), spath.end()));

            // ファイル拡張子から実際のアセットタイプを判定
            HotReloadAssetType actualType = ClassifyFile(wpath);

            // 同じファイルが既にペンディング中ならタイマーをリセット（デバウンス）
            for (auto& pending : m_pendingChanges)
            {
                if (pending.filePath == wpath)
                {
                    pending.timer = m_debounceTime;
                    return;
                }
            }

            // 新しいペンディングエントリを追加
            PendingChange change;
            change.filePath = std::move(wpath);
            change.type = actualType;
            change.timer = m_debounceTime;
            m_pendingChanges.push_back(std::move(change));
        });

    ++m_watchedDirCount;
}

void HotReloadManager::OnReload(HotReloadAssetType type, HotReloadCallback callback)
{
    m_callbacks[static_cast<int>(type)] = std::move(callback);
}

void HotReloadManager::SetDebounceTime(float seconds)
{
    m_debounceTime = (std::max)(0.0f, seconds);
}

void HotReloadManager::Update(float deltaTime)
{
    if (!m_initialized || m_pendingChanges.empty())
        return;

    // タイマーを減算し、期限切れのものを発火
    for (auto it = m_pendingChanges.begin(); it != m_pendingChanges.end(); )
    {
        it->timer -= deltaTime;

        if (it->timer <= 0.0f)
        {
            // コールバックを発火
            auto cbIt = m_callbacks.find(static_cast<int>(it->type));
            if (cbIt != m_callbacks.end() && cbIt->second)
            {
                cbIt->second(it->filePath, it->type);
            }
            it = m_pendingChanges.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

uint32_t HotReloadManager::GetWatchedDirectoryCount() const
{
    return m_watchedDirCount;
}

HotReloadAssetType HotReloadManager::ClassifyFile(const gx::WString& path) const
{
    // 拡張子を抽出（小文字に変換して比較）
    gx::WString ext;
    auto dotPos = path.rfind(L'.');
    if (dotPos != gx::WString::npos)
    {
        ext = path.substr(dotPos);
        // 小文字に変換
        for (auto& ch : ext)
            ch = static_cast<wchar_t>(towlower(ch));
    }

    // シェーダー
    if (ext == L".hlsl" || ext == L".hlsli")
        return HotReloadAssetType::Shader;

    // テクスチャ
    if (ext == L".png" || ext == L".dds" || ext == L".jpg" || ext == L".jpeg" || ext == L".tga")
        return HotReloadAssetType::Texture;

    // スクリプト
    if (ext == L".lua")
        return HotReloadAssetType::Script;

    // スタイルシート
    if (ext == L".css")
        return HotReloadAssetType::StyleSheet;

    // コンフィグ
    if (ext == L".json" || ext == L".xml" || ext == L".ini")
        return HotReloadAssetType::Config;

    // デフォルト: Config として扱う
    return HotReloadAssetType::Config;
}

} // namespace gx
