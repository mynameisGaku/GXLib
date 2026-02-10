/// @file ShaderLibrary.cpp
/// @brief シェーダーライブラリ実装
#include "pch.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace GX
{

ShaderLibrary& ShaderLibrary::Instance()
{
    static ShaderLibrary instance;
    return instance;
}

bool ShaderLibrary::Initialize(ID3D12Device* device)
{
    m_device = device;
    if (!m_compiler.Initialize())
    {
        GX_LOG_ERROR("ShaderLibrary: Failed to initialize shader compiler");
        return false;
    }
    GX_LOG_INFO("ShaderLibrary: Initialized");
    return true;
}

ShaderBlob ShaderLibrary::GetShader(const std::wstring& filePath,
                                     const std::wstring& entryPoint,
                                     const std::wstring& target)
{
    return GetShaderVariant(filePath, entryPoint, target, {});
}

ShaderBlob ShaderLibrary::GetShaderVariant(const std::wstring& filePath,
                                            const std::wstring& entryPoint,
                                            const std::wstring& target,
                                            const std::vector<std::pair<std::wstring, std::wstring>>& defines)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    ShaderKey key{ filePath, entryPoint, target, defines };

    // キャッシュヒット確認
    auto it = m_cache.find(key);
    if (it != m_cache.end() && it->second.valid)
    {
        return it->second;
    }

    // コンパイル
    ShaderBlob blob;
    if (defines.empty())
    {
        blob = m_compiler.CompileFromFile(filePath, entryPoint, target);
    }
    else
    {
        blob = m_compiler.CompileFromFile(filePath, entryPoint, target, defines);
    }

    if (!blob.valid)
    {
        m_lastError = m_compiler.GetLastError();
        GX_LOG_ERROR("ShaderLibrary: Compilation failed for %ls [%ls]",
                     entryPoint.c_str(), target.c_str());
        return blob;
    }

    // キャッシュに格納
    m_cache[key] = blob;
    return blob;
}

PSOCallbackID ShaderLibrary::RegisterPSORebuilder(const std::wstring& shaderPath,
                                                    PSORebuilder callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    PSOCallbackID id = m_nextCallbackID++;
    m_rebuilders.push_back({ id, NormalizePath(shaderPath), std::move(callback) });
    GX_LOG_INFO("ShaderLibrary: Registered PSO rebuilder (ID=%u) for %ls", id, shaderPath.c_str());
    return id;
}

void ShaderLibrary::UnregisterPSORebuilder(PSOCallbackID id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rebuilders.erase(
        std::remove_if(m_rebuilders.begin(), m_rebuilders.end(),
            [id](const RebuilderEntry& e) { return e.id == id; }),
        m_rebuilders.end());
}

bool ShaderLibrary::InvalidateFile(const std::wstring& filePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::wstring normalizedPath = NormalizePath(filePath);

    // .hlsli の場合は全キャッシュクリア（include依存追跡は省略）
    bool isInclude = false;
    if (normalizedPath.size() > 6)
    {
        std::wstring ext = normalizedPath.substr(normalizedPath.size() - 6);
        for (auto& c : ext) c = towlower(c);
        isInclude = (ext == L".hlsli");
    }

    if (isInclude)
    {
        GX_LOG_INFO("ShaderLibrary: .hlsli changed — clearing ALL cache");
        m_cache.clear();
    }
    else
    {
        // 該当ファイルのエントリのみ削除
        for (auto it = m_cache.begin(); it != m_cache.end(); )
        {
            if (NormalizePath(it->first.filePath) == normalizedPath)
                it = m_cache.erase(it);
            else
                ++it;
        }
    }

    // 登録済みPSORebuilderを呼び出し
    bool allSuccess = true;
    m_lastError.clear();

    for (auto& entry : m_rebuilders)
    {
        // .hlsli変更時は全rebuilder呼び出し、.hlsl変更時は該当ファイルのみ
        bool shouldRebuild = isInclude || (entry.shaderPath == normalizedPath);
        if (!shouldRebuild) continue;

        GX_LOG_INFO("ShaderLibrary: Rebuilding PSO (ID=%u) ...", entry.id);
        if (!entry.callback(m_device))
        {
            m_lastError = m_compiler.GetLastError();
            GX_LOG_ERROR("ShaderLibrary: PSO rebuild failed (ID=%u)", entry.id);
            allSuccess = false;
            // 最初の失敗のみ記録して続行（他のPSOも試みる）
        }
    }

    return allSuccess;
}

void ShaderLibrary::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_rebuilders.clear();
    m_device = nullptr;
    GX_LOG_INFO("ShaderLibrary: Shutdown");
}

std::wstring ShaderLibrary::NormalizePath(const std::wstring& path)
{
    std::wstring result = path;
    // バックスラッシュ → スラッシュ
    for (auto& c : result)
    {
        if (c == L'\\') c = L'/';
    }
    // 小文字化
    for (auto& c : result)
    {
        c = towlower(c);
    }
    return result;
}

} // namespace GX
