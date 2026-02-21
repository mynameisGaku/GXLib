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

    // ファイルパス + エントリポイント + ターゲット + defines の組み合わせでキャッシュを引く
    ShaderKey key{ filePath, entryPoint, target, defines };

    auto it = m_cache.find(key);
    if (it != m_cache.end() && it->second.valid)
    {
        return it->second;
    }

    // キャッシュミス — 実際にコンパイルを実行
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

    // include依存グラフを更新する（このファイルがどの.hlsliをインクルードしているか）
    ScanIncludes(filePath);

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

    // .hlsliはヘッダファイルなのでどのシェーダーが依存しているか分からない。
    // include依存グラフの追跡は省略し、安全策として全キャッシュをクリアする。
    bool isInclude = false;
    if (normalizedPath.size() > 6)
    {
        std::wstring ext = normalizedPath.substr(normalizedPath.size() - 6);
        for (auto& c : ext) c = towlower(c);
        isInclude = (ext == L".hlsli");
    }

    // 依存する.hlslファイルのリストを構築する
    // .hlsli → 依存グラフから該当する.hlslを特定、.hlsl → そのファイルのみ
    std::vector<std::wstring> affectedHlsl;

    if (isInclude)
    {
        // .hlsliファイル名部分だけ抽出して正規化（依存グラフのキーはファイル名のみ）
        std::wstring includeFileName = normalizedPath;
        auto slashPos = includeFileName.rfind(L'/');
        if (slashPos != std::wstring::npos)
            includeFileName = includeFileName.substr(slashPos + 1);

        auto depIt = m_includeDeps.find(includeFileName);
        if (depIt != m_includeDeps.end())
        {
            affectedHlsl = depIt->second;
            GX_LOG_INFO("ShaderLibrary: .hlsli changed (%ls) — invalidating %zu dependent shader(s)",
                        includeFileName.c_str(), affectedHlsl.size());

            // 依存する.hlslのキャッシュエントリのみ削除
            for (const auto& hlslPath : affectedHlsl)
            {
                for (auto it = m_cache.begin(); it != m_cache.end(); )
                {
                    if (NormalizePath(it->first.filePath) == hlslPath)
                        it = m_cache.erase(it);
                    else
                        ++it;
                }
            }
        }
        else
        {
            // 依存情報なし — 安全策として全キャッシュをクリア
            GX_LOG_INFO("ShaderLibrary: No dependency info for %ls — clearing ALL cache",
                        includeFileName.c_str());
            m_cache.clear();
        }
    }
    else
    {
        // 該当ファイルのエントリのみ削除
        affectedHlsl.push_back(normalizedPath);
        for (auto it = m_cache.begin(); it != m_cache.end(); )
        {
            if (NormalizePath(it->first.filePath) == normalizedPath)
                it = m_cache.erase(it);
            else
                ++it;
        }
    }

    // 登録済みPSO再構築コールバックを呼び出す。
    // .hlsli変更時は依存グラフに基づいて該当コールバックのみ呼ぶ（依存情報なしの場合は全て）。
    // 一部が失敗しても残りは試行する（画面が壊れるよりは部分的にでも更新した方がよい）。
    bool allSuccess = true;
    m_lastError.clear();

    for (auto& entry : m_rebuilders)
    {
        bool shouldRebuild = false;

        if (isInclude)
        {
            // .hlsliファイル名を抽出
            std::wstring includeFileName = normalizedPath;
            auto slashPos = includeFileName.rfind(L'/');
            if (slashPos != std::wstring::npos)
                includeFileName = includeFileName.substr(slashPos + 1);

            auto depIt = m_includeDeps.find(includeFileName);
            if (depIt != m_includeDeps.end())
            {
                // 依存グラフに基づいて対象のPSOのみ再構築
                for (const auto& dep : depIt->second)
                {
                    if (entry.shaderPath == dep)
                    {
                        shouldRebuild = true;
                        break;
                    }
                }
            }
            else
            {
                // 依存情報なし — 安全策として全コールバックを実行
                shouldRebuild = true;
            }
        }
        else
        {
            shouldRebuild = (entry.shaderPath == normalizedPath);
        }

        if (!shouldRebuild) continue;

        GX_LOG_INFO("ShaderLibrary: Rebuilding PSO (ID=%u) ...", entry.id);
        if (!entry.callback(m_device))
        {
            m_lastError = m_compiler.GetLastError();
            GX_LOG_ERROR("ShaderLibrary: PSO rebuild failed (ID=%u)", entry.id);
            allSuccess = false;
        }
    }

    return allSuccess;
}

void ShaderLibrary::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_rebuilders.clear();
    m_includeDeps.clear();
    m_device = nullptr;
    GX_LOG_INFO("ShaderLibrary: Shutdown");
}

void ShaderLibrary::ScanIncludes(const std::wstring& hlslPath)
{
    // HLSLファイルを開いて #include "..." を探す
    std::ifstream file(hlslPath);
    if (!file.is_open())
        return;

    std::wstring normalizedHlsl = NormalizePath(hlslPath);

    std::string line;
    while (std::getline(file, line))
    {
        // #include "filename.hlsli" パターンを検索
        // 先頭の空白を許容する
        size_t pos = 0;
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
            ++pos;

        if (line.compare(pos, 8, "#include") != 0)
            continue;

        pos += 8;
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t'))
            ++pos;

        if (pos >= line.size() || line[pos] != '"')
            continue; // <...> システムインクルードはスキップ

        ++pos; // '"' をスキップ
        size_t endQuote = line.find('"', pos);
        if (endQuote == std::string::npos)
            continue;

        std::string includeName = line.substr(pos, endQuote - pos);

        // パス部分を除去してファイル名だけにする
        size_t lastSlash = includeName.rfind('/');
        if (lastSlash != std::string::npos)
            includeName = includeName.substr(lastSlash + 1);
        size_t lastBackSlash = includeName.rfind('\\');
        if (lastBackSlash != std::string::npos)
            includeName = includeName.substr(lastBackSlash + 1);

        // narrow → wide 変換して正規化
        std::wstring wIncludeName(includeName.begin(), includeName.end());
        for (auto& c : wIncludeName)
            c = towlower(c);

        // 依存グラフに追加（重複チェック）
        auto& deps = m_includeDeps[wIncludeName];
        bool alreadyExists = false;
        for (const auto& existing : deps)
        {
            if (existing == normalizedHlsl)
            {
                alreadyExists = true;
                break;
            }
        }
        if (!alreadyExists)
            deps.push_back(normalizedHlsl);
    }
}

std::wstring ShaderLibrary::NormalizePath(const std::wstring& path)
{
    // パス比較を安定させるため、区切り文字と大文字小文字を統一する
    std::wstring result = path;
    for (auto& c : result)
    {
        if (c == L'\\') c = L'/';
    }
    for (auto& c : result)
    {
        c = towlower(c);
    }
    return result;
}

} // namespace GX
