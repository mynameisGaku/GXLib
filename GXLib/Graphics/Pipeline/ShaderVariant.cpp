/// @file ShaderVariant.cpp
/// @brief シェーダーバリアント管理システムの実装
#include "pch_graphics.h"
#include "Graphics/Pipeline/ShaderVariant.h"
#include "Core/Logger.h"

namespace gx
{

// ---------------------------------------------------------------------------
// ShaderVariantKey
// ---------------------------------------------------------------------------

void ShaderVariantKey::SetKeyword(const gx::String& name, bool enabled)
{
    if (enabled)
    {
        m_keywords[name] = "1";
    }
    else
    {
        m_keywords.erase(name);
    }
}

void ShaderVariantKey::SetKeyword(const gx::String& name, const gx::String& value)
{
    m_keywords[name] = value;
}

void ShaderVariantKey::ClearKeyword(const gx::String& name)
{
    m_keywords.erase(name);
}

bool ShaderVariantKey::HasKeyword(const gx::String& name) const
{
    return m_keywords.find(name) != m_keywords.end();
}

gx::String ShaderVariantKey::GetKeywordValue(const gx::String& name) const
{
    auto it = m_keywords.find(name);
    if (it != m_keywords.end())
        return it->second;
    return gx::String();
}

uint64_t ShaderVariantKey::ComputeHash() const
{
    // キーワードをソートして安定したハッシュを生成する
    gx::Vector<std::pair<gx::String, gx::String>> sorted;
    sorted.reserve(m_keywords.size());
    for (const auto& [name, value] : m_keywords)
    {
        sorted.emplace_back(name, value);
    }
    std::sort(sorted.begin(), sorted.end(),
        [](const std::pair<gx::String, gx::String>& a,
           const std::pair<gx::String, gx::String>& b) {
            return a.first < b.first;
        });

    // FNV-1a 64-bit ハッシュ
    constexpr uint64_t fnvOffset = 14695981039346656037ULL;
    constexpr uint64_t fnvPrime = 1099511628211ULL;

    uint64_t hash = fnvOffset;
    for (const auto& [name, value] : sorted)
    {
        // "NAME=VALUE" を連結してハッシュ
        for (char c : name)
        {
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
            hash *= fnvPrime;
        }
        // 区切り文字
        hash ^= static_cast<uint64_t>('=');
        hash *= fnvPrime;
        for (char c : value)
        {
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
            hash *= fnvPrime;
        }
        // エントリ区切り
        hash ^= static_cast<uint64_t>(';');
        hash *= fnvPrime;
    }

    return hash;
}

gx::Vector<gx::String> ShaderVariantKey::ToDefines() const
{
    gx::Vector<gx::String> defines;
    defines.reserve(m_keywords.size());

    for (const auto& [name, value] : m_keywords)
    {
        if (value == "1")
        {
            // boolean キーワード: 名前のみ
            defines.push_back(name);
        }
        else
        {
            // 値付きキーワード: NAME=VALUE
            defines.push_back(name + "=" + value);
        }
    }

    return defines;
}

bool ShaderVariantKey::operator==(const ShaderVariantKey& other) const
{
    if (m_keywords.size() != other.m_keywords.size())
        return false;

    for (const auto& [name, value] : m_keywords)
    {
        auto it = other.m_keywords.find(name);
        if (it == other.m_keywords.end() || it->second != value)
            return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// ShaderVariantCollection
// ---------------------------------------------------------------------------

void ShaderVariantCollection::Initialize(const gx::String& shaderPath,
                                          const gx::String& vsEntry,
                                          const gx::String& psEntry)
{
    m_shaderPath = shaderPath;
    m_vsEntry = vsEntry;
    m_psEntry = psEntry;
    m_keywords.clear();
    m_variants.clear();
}

void ShaderVariantCollection::AddKeyword(const ShaderKeyword& keyword)
{
    m_keywords.push_back(keyword);
}

void ShaderVariantCollection::EnumerateMultiCompileVariants(gx::Vector<ShaderVariantKey>& outKeys) const
{
    // multi_compileキーワードのみ抽出
    gx::Vector<const ShaderKeyword*> mcKeywords;
    for (const auto& kw : m_keywords)
    {
        if (kw.isMultiCompile)
            mcKeywords.push_back(&kw);
    }

    if (mcKeywords.empty())
    {
        // multi_compileキーワードなし: 空のキーのみ
        outKeys.push_back(ShaderVariantKey{});
        return;
    }

    // 各キーワードの取り得る状態を列挙
    // 値なし(on/off): 2状態
    // 値あり(multi-compile): values.size() 状態
    gx::Vector<gx::Vector<std::pair<gx::String, gx::String>>> allStates;
    for (const auto* kw : mcKeywords)
    {
        gx::Vector<std::pair<gx::String, gx::String>> states;
        if (kw->values.empty())
        {
            // on/offの2状態
            states.emplace_back(kw->name, gx::String());     // off (not defined)
            states.emplace_back(kw->name, gx::String("1"));  // on
        }
        else
        {
            // multi-compile: 各値
            for (const auto& val : kw->values)
            {
                states.emplace_back(kw->name, val);
            }
        }
        allStates.push_back(std::move(states));
    }

    // 全組み合わせを列挙（直積）
    gx::Vector<gx::Vector<uint32_t>> indices;
    {
        gx::Vector<uint32_t> initial(allStates.size(), 0);
        indices.push_back(initial);
    }

    // 各次元のサイズに基づいてインデックスの直積を生成
    gx::Vector<gx::Vector<uint32_t>> result;
    result.push_back(gx::Vector<uint32_t>(allStates.size(), 0));

    for (size_t dim = 0; dim < allStates.size(); ++dim)
    {
        gx::Vector<gx::Vector<uint32_t>> newResult;
        uint32_t stateCount = static_cast<uint32_t>(allStates[dim].size());
        for (const auto& existing : result)
        {
            for (uint32_t s = 0; s < stateCount; ++s)
            {
                auto copy = existing;
                copy[dim] = s;
                newResult.push_back(std::move(copy));
            }
        }
        result = std::move(newResult);
    }

    // 各インデックス組み合わせからShaderVariantKeyを構築
    for (const auto& combo : result)
    {
        ShaderVariantKey key;
        for (size_t dim = 0; dim < allStates.size(); ++dim)
        {
            const auto& [name, value] = allStates[dim][combo[dim]];
            if (!value.empty())
            {
                key.SetKeyword(name, value);
            }
            // value が空 = off 状態 → キーワードを設定しない
        }
        outKeys.push_back(std::move(key));
    }
}

uint32_t ShaderVariantCollection::GetTotalMultiCompileVariantCount() const
{
    uint32_t total = 1;
    for (const auto& kw : m_keywords)
    {
        if (kw.isMultiCompile)
        {
            if (kw.values.empty())
            {
                total *= 2; // on/off
            }
            else
            {
                total *= static_cast<uint32_t>(kw.values.size());
            }
        }
    }
    return total;
}

int ShaderVariantCollection::PrecompileAll(ID3D12Device* device,
                                            ID3D12RootSignature* rootSignature,
                                            DXGI_FORMAT rtFormat)
{
    if (!device || !rootSignature)
        return 0;

    gx::Vector<ShaderVariantKey> keys;
    EnumerateMultiCompileVariants(keys);

    int successCount = 0;
    for (const auto& key : keys)
    {
        if (CompileVariant(key, device, rootSignature, rtFormat))
            ++successCount;
    }

    GX_LOG_INFO("ShaderVariantCollection: Precompiled %d/%zu variants for %s",
                successCount, keys.size(), m_shaderPath.c_str());

    return successCount;
}

ID3D12PipelineState* ShaderVariantCollection::GetVariant(const ShaderVariantKey& key,
                                                          ID3D12Device* device,
                                                          ID3D12RootSignature* rootSignature,
                                                          DXGI_FORMAT rtFormat)
{
    uint64_t hash = key.ComputeHash();

    // キャッシュ検索
    auto it = m_variants.find(hash);
    if (it != m_variants.end())
    {
        return it->second.pso.Get();
    }

    // キャッシュミス — オンデマンドコンパイル
    if (!device || !rootSignature)
        return nullptr;

    if (CompileVariant(key, device, rootSignature, rtFormat))
    {
        auto it2 = m_variants.find(hash);
        if (it2 != m_variants.end())
            return it2->second.pso.Get();
    }

    return nullptr;
}

bool ShaderVariantCollection::CompileVariant(const ShaderVariantKey& key,
                                              ID3D12Device* device,
                                              ID3D12RootSignature* rootSignature,
                                              DXGI_FORMAT rtFormat)
{
    if (!device || !rootSignature)
        return false;

    // DXCコンパイラ初期化（初回のみ）
    if (!m_shader.Initialize())
    {
        GX_LOG_ERROR("ShaderVariantCollection: Failed to initialize shader compiler");
        return false;
    }

    // defines リストを構築（narrow -> wide変換）
    gx::Vector<gx::String> narrowDefines = key.ToDefines();
    gx::Vector<std::pair<gx::WString, gx::WString>> wideDefines;
    wideDefines.reserve(narrowDefines.size());

    for (const auto& def : narrowDefines)
    {
        // "NAME=VALUE" or "NAME" を分解
        size_t eqPos = def.find('=');
        if (eqPos != gx::String::npos)
        {
            gx::String name = def.substr(0, eqPos);
            gx::String value = def.substr(eqPos + 1);
            std::wstring wName(name.begin(), name.end());
            std::wstring wValue(value.begin(), value.end());
            wideDefines.emplace_back(gx::WString(wName.c_str()), gx::WString(wValue.c_str()));
        }
        else
        {
            std::wstring wName(def.begin(), def.end());
            wideDefines.emplace_back(gx::WString(wName.c_str()), gx::WString());
        }
    }

    // シェーダーパスをWStringに変換
    std::wstring wShaderPath(m_shaderPath.begin(), m_shaderPath.end());
    gx::WString wPath(wShaderPath.c_str());

    // VS コンパイル
    std::wstring wVsEntry(m_vsEntry.begin(), m_vsEntry.end());
    ShaderBlob vsBlob;
    if (wideDefines.empty())
    {
        vsBlob = m_shader.CompileFromFile(wPath, gx::WString(wVsEntry.c_str()), L"vs_6_0");
    }
    else
    {
        vsBlob = m_shader.CompileFromFile(wPath, gx::WString(wVsEntry.c_str()), L"vs_6_0", wideDefines);
    }

    if (!vsBlob.valid)
    {
        GX_LOG_ERROR("ShaderVariantCollection: VS compilation failed for variant");
        return false;
    }

    // PS コンパイル
    std::wstring wPsEntry(m_psEntry.begin(), m_psEntry.end());
    ShaderBlob psBlob;
    if (wideDefines.empty())
    {
        psBlob = m_shader.CompileFromFile(wPath, gx::WString(wPsEntry.c_str()), L"ps_6_0");
    }
    else
    {
        psBlob = m_shader.CompileFromFile(wPath, gx::WString(wPsEntry.c_str()), L"ps_6_0", wideDefines);
    }

    if (!psBlob.valid)
    {
        GX_LOG_ERROR("ShaderVariantCollection: PS compilation failed for variant");
        return false;
    }

    // PSO 作成
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = vsBlob.GetBytecode();
    psoDesc.PS = psBlob.GetBytecode();

    // ラスタライザ ステート
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // ブレンド ステート
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // デプスステンシル
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = rtFormat;
    psoDesc.SampleDesc.Count = 1;

    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("ShaderVariantCollection: CreateGraphicsPipelineState failed (HRESULT: 0x%08X)", hr);
        return false;
    }

    // キャッシュに格納
    uint64_t hash = key.ComputeHash();
    CompiledVariant variant;
    variant.pso = std::move(pso);
    variant.key = key;
    m_variants[hash] = std::move(variant);

    return true;
}

gx::String ShaderVariantCollection::GetVariantDebugString(const ShaderVariantKey& key) const
{
    gx::Vector<gx::String> defines = key.ToDefines();

    if (defines.empty())
        return gx::String("<no defines>");

    gx::String result;
    for (size_t i = 0; i < defines.size(); ++i)
    {
        if (i > 0) result += " ";
        result += "-D " + defines[i];
    }
    return result;
}

} // namespace gx
