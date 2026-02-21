/// @file ShaderHotReload.cpp
/// @brief シェーダーホットリロード実装
#include "pch.h"
#include "Graphics/Pipeline/ShaderHotReload.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace GX
{

ShaderHotReload& ShaderHotReload::Instance()
{
    static ShaderHotReload instance;
    return instance;
}

bool ShaderHotReload::Initialize(ID3D12Device* device, CommandQueue* cmdQueue)
{
    m_device = device;
    m_cmdQueue = cmdQueue;

    // Shadersディレクトリを監視
    m_watcher.Watch("Shaders", [this](const std::string& path) {
        OnShaderFileChanged(path);
    });

    GX_LOG_INFO("ShaderHotReload: Initialized — watching Shaders/ directory");
    return true;
}

void ShaderHotReload::OnShaderFileChanged(const std::string& path)
{
    if (!IsShaderFile(path)) return;

    // FileWatcherはUTF-8のstd::stringで通知するので、ShaderLibrary用にwstringに変換
    int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), len);

    {
        // ワーカースレッドから呼ばれるのでロック必須
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        // 同じファイルが短時間に何度も通知されることがあるので重複排除
        bool found = false;
        for (auto& existing : m_pendingChanges)
        {
            if (existing == wpath) { found = true; break; }
        }
        if (!found)
            m_pendingChanges.push_back(std::move(wpath));
    }
}

bool ShaderHotReload::IsShaderFile(const std::string& path)
{
    if (path.size() < 5) return false;
    std::string ext = path.substr(path.size() - 5);
    for (auto& c : ext) c = static_cast<char>(tolower(c));
    if (ext == ".hlsl") return true;

    if (path.size() >= 6)
    {
        ext = path.substr(path.size() - 6);
        for (auto& c : ext) c = static_cast<char>(tolower(c));
        if (ext == ".hlsli") return true;
    }
    return false;
}

void ShaderHotReload::Update(float deltaTime)
{
    // FileWatcherの変更通知をメインスレッド側で処理
    m_watcher.Update();

    // 新しい変更があればデバウンスタイマーをリセット
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        if (!m_pendingChanges.empty())
        {
            m_hasPending = true;
            m_debounceTimer = k_DebounceDelay;
        }
    }

    if (!m_hasPending) return;

    // デバウンス待機中 — エディタの連続保存が落ち着くまで待つ
    m_debounceTimer -= deltaTime;
    if (m_debounceTimer > 0.0f) return;

    // デバウンス完了。変更リストを取り出してリロード実行
    std::vector<std::wstring> changes;
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        changes.swap(m_pendingChanges);
    }
    m_hasPending = false;

    if (changes.empty()) return;

    GX_LOG_INFO("ShaderHotReload: Reloading %zu shader(s)...", changes.size());

    // GPUがまだ旧PSOで描画中かもしれないので、完了を待ってから置き換える
    if (m_cmdQueue)
        m_cmdQueue->Flush();

    // ShaderLibraryのキャッシュ無効化 → 登録済みPSORebuilderの実行
    bool allSuccess = true;
    for (auto& path : changes)
    {
        GX_LOG_INFO("ShaderHotReload: Invalidating %ls", path.c_str());
        if (!ShaderLibrary::Instance().InvalidateFile(path))
        {
            allSuccess = false;
        }
    }

    if (allSuccess)
    {
        m_errorMessage.clear();
        GX_LOG_INFO("ShaderHotReload: All shaders reloaded successfully");
    }
    else
    {
        // エラーメッセージを保持しておき、オーバーレイ等で表示する
        m_errorMessage = ShaderLibrary::Instance().GetLastError();
        GX_LOG_ERROR("ShaderHotReload: Some shaders failed to reload");
    }
}

void ShaderHotReload::Shutdown()
{
    m_watcher.Stop();
    m_device = nullptr;
    m_cmdQueue = nullptr;
    GX_LOG_INFO("ShaderHotReload: Shutdown");
}

} // namespace GX
