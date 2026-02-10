#pragma once
/// @file ShaderHotReload.h
/// @brief シェーダーホットリロード — ファイル変更検知→再コンパイル→PSO再生成
///
/// Shadersディレクトリを監視し、.hlsl/.hlsliファイルの変更を検知。
/// デバウンス後にShaderLibraryのキャッシュを無効化し、
/// 登録済みPSORebuilderを呼び出してPSOを再生成します。
/// コンパイルエラー時はエラーメッセージを保持し、オーバーレイ表示に使用します。

#include "pch.h"
#include "IO/FileWatcher.h"
#include "Graphics/Device/CommandQueue.h"

namespace GX
{

/// @brief シェーダーホットリロード管理（シングルトン）
class ShaderHotReload
{
public:
    static ShaderHotReload& Instance();

    /// 初期化（Shadersディレクトリの監視開始）
    bool Initialize(ID3D12Device* device, CommandQueue* cmdQueue);

    /// 毎フレーム呼び出し（メインスレッド）
    void Update(float deltaTime);

    /// エラー状態
    bool HasError() const { return !m_errorMessage.empty(); }
    const std::string& GetErrorMessage() const { return m_errorMessage; }

    /// 終了処理
    void Shutdown();

private:
    ShaderHotReload() = default;
    ~ShaderHotReload() = default;
    ShaderHotReload(const ShaderHotReload&) = delete;
    ShaderHotReload& operator=(const ShaderHotReload&) = delete;

    /// ファイル変更コールバック（ワーカースレッドから呼ばれる）
    void OnShaderFileChanged(const std::string& path);

    /// 変更されたファイルがシェーダーファイルか判定
    static bool IsShaderFile(const std::string& path);

    FileWatcher m_watcher;
    ID3D12Device* m_device = nullptr;
    CommandQueue* m_cmdQueue = nullptr;

    std::vector<std::wstring> m_pendingChanges;
    std::mutex m_pendingMutex;

    float m_debounceTimer = 0.0f;
    bool  m_hasPending = false;

    std::string m_errorMessage;

    static constexpr float k_DebounceDelay = 0.3f;
};

} // namespace GX
