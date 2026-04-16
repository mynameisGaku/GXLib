/// @file ShaderRegistry.cpp
/// @brief シェーダーモデルPSOレジストリの実装

#include "pch_graphics.h"
#include "Graphics/3D/ShaderRegistry.h"
#include "Graphics/3D/Vertex3D.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace gx
{

/// シェーダーモデル列挙値からHLSLファイルパスを引く対応表
static const wchar_t* GetShaderPath(gxfmt::ShaderModel model)
{
    switch (model)
    {
    case gxfmt::ShaderModel::Standard:  return L"Shaders/PBR.hlsl";
    case gxfmt::ShaderModel::Unlit:     return L"Shaders/Unlit.hlsl";
    case gxfmt::ShaderModel::Toon:      return L"Shaders/Toon.hlsl";
    case gxfmt::ShaderModel::Phong:     return L"Shaders/Phong.hlsl";
    case gxfmt::ShaderModel::Subsurface:return L"Shaders/Subsurface.hlsl";
    case gxfmt::ShaderModel::ClearCoat: return L"Shaders/ClearCoat.hlsl";
    default:                            return L"Shaders/PBR.hlsl";
    }
}

bool ShaderRegistry::Initialize(ID3D12Device* device, ID3D12RootSignature* rootSignature)
{
    m_rootSignature = rootSignature;

    if (!m_shaderCompiler.Initialize())
    {
        GX_LOG_ERROR("ShaderRegistry: Failed to initialize shader compiler");
        return false;
    }

    // 各シェーダーモデルのPSOを生成
    for (uint32_t i = 0; i < k_NumShaderModels; ++i)
    {
        auto model = static_cast<gxfmt::ShaderModel>(i);
        if (!CompileAndCreatePSO(device, model))
        {
            GX_LOG_ERROR("ShaderRegistry: Failed to create PSO for model %s",
                         gxfmt::ShaderModelToString(model));
            return false;
        }
    }

    // ToonアウトラインPSO
    if (!CompileToonOutlinePSO(device))
    {
        GX_LOG_ERROR("ShaderRegistry: Failed to create Toon outline PSO");
        return false;
    }

    GX_LOG_INFO("ShaderRegistry: Initialized %u shader model PSOs + Toon outline",
                k_NumShaderModels);
    return true;
}

bool ShaderRegistry::CompileAndCreatePSO(ID3D12Device* device, gxfmt::ShaderModel model)
{
    const wchar_t* path = GetShaderPath(model);
    uint32_t idx = static_cast<uint32_t>(model);
    if (idx >= k_NumShaderModels)
        return false;

    // Static variant
    auto vsBlob = m_shaderCompiler.CompileFromFile(path, L"VSMain", L"vs_6_0");
    auto psBlob = m_shaderCompiler.CompileFromFile(path, L"PSMain", L"ps_6_0");

    // Skinned variant
    gx::Vector<std::pair<gx::WString, gx::WString>> skinnedDefines = { { L"SKINNED", L"1" } };
    auto vsSkinned = m_shaderCompiler.CompileFromFile(path, L"VSMain", L"vs_6_0", skinnedDefines);
    auto psSkinned = m_shaderCompiler.CompileFromFile(path, L"PSMain", L"ps_6_0", skinnedDefines);

    if (!vsBlob.valid || !psBlob.valid || !vsSkinned.valid || !psSkinned.valid)
    {
        GX_LOG_ERROR("ShaderRegistry: Compile failed for %ls: %s",
                     path, m_shaderCompiler.GetLastError().c_str());
        return false;
    }

    // Static PSO（Vertex3D_PBRレイアウト、3RT: HDR + Normal + Albedo）
    PipelineStateBuilder psoBuilder;
    m_psos[idx].pso = psoBuilder
        .SetRootSignature(m_rootSignature)
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(k_Vertex3DPBRLayout, _countof(k_Vertex3DPBRLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)  // HDR
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 1)  // Normal
        .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 2)     // Albedo (GI)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_BACK)
        .Build(device);

    // Skinned PSO（Vertex3D_Skinnedレイアウト、SKINNEDマクロ定義でボーンスキニング有効化）
    PipelineStateBuilder psoSkinnedBuilder;
    m_psos[idx].psoSkinned = psoSkinnedBuilder
        .SetRootSignature(m_rootSignature)
        .SetVertexShader(vsSkinned.GetBytecode())
        .SetPixelShader(psSkinned.GetBytecode())
        .SetInputLayout(k_Vertex3DSkinnedLayout, _countof(k_Vertex3DSkinnedLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 1)
        .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 2)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_BACK)
        .Build(device);

    return m_psos[idx].pso != nullptr && m_psos[idx].psoSkinned != nullptr;
}

bool ShaderRegistry::CompileToonOutlinePSO(ID3D12Device* device)
{
    const wchar_t* path = L"Shaders/ToonOutline.hlsl";

    // --- Static variant ---
    auto vsBlob = m_shaderCompiler.CompileFromFile(path, L"VSMain_Outline", L"vs_6_0");
    auto psBlob = m_shaderCompiler.CompileFromFile(path, L"PSMain_Outline", L"ps_6_0");

    // --- Skinned variant ---
    gx::Vector<std::pair<gx::WString, gx::WString>> skinnedDefines = { { L"SKINNED", L"1" } };
    auto vsSkinned = m_shaderCompiler.CompileFromFile(path, L"VSMain_Outline", L"vs_6_0", skinnedDefines);
    auto psSkinned = m_shaderCompiler.CompileFromFile(path, L"PSMain_Outline", L"ps_6_0", skinnedDefines);

    if (!vsBlob.valid || !psBlob.valid || !vsSkinned.valid || !psSkinned.valid)
    {
        GX_LOG_ERROR("ShaderRegistry: Compile failed for ToonOutline: %s",
                     m_shaderCompiler.GetLastError().c_str());
        return false;
    }

    // Outline PSO — static: 前面カリング + スムース法線(slot 1)で頂点膨張 + 深度バイアス
    {
        PipelineStateBuilder b;
        m_toonOutline.pso = b
            .SetRootSignature(m_rootSignature)
            .SetVertexShader(vsBlob.GetBytecode())
            .SetPixelShader(psBlob.GetBytecode())
            .SetInputLayout(k_Vertex3DPBROutlineLayout, _countof(k_Vertex3DPBROutlineLayout))
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 1)
            .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 2)
            .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
            .SetDepthEnable(true)
            .SetCullMode(D3D12_CULL_MODE_FRONT)
            .SetDepthBias(500, 0.0f, 2.0f)
            .Build(device);
    }

    // Outline PSO — skinned
    {
        PipelineStateBuilder b;
        m_toonOutline.psoSkinned = b
            .SetRootSignature(m_rootSignature)
            .SetVertexShader(vsSkinned.GetBytecode())
            .SetPixelShader(psSkinned.GetBytecode())
            .SetInputLayout(k_Vertex3DSkinnedOutlineLayout, _countof(k_Vertex3DSkinnedOutlineLayout))
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 1)
            .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 2)
            .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
            .SetDepthEnable(true)
            .SetCullMode(D3D12_CULL_MODE_FRONT)
            .SetDepthBias(500, 0.0f, 2.0f)
            .Build(device);
    }

    return m_toonOutline.pso != nullptr && m_toonOutline.psoSkinned != nullptr;
}

ID3D12PipelineState* ShaderRegistry::GetPSO(gxfmt::ShaderModel model, bool skinned) const
{
    uint32_t idx = static_cast<uint32_t>(model);

    // 組み込み 6 モデル (0-5)
    if (idx < k_NumShaderModels)
        return skinned ? m_psos[idx].psoSkinned.Get() : m_psos[idx].pso.Get();

    // Layer 2 カスタムシェーダーモデル (6-254) — Custom=255 はMaterial側のshaderHandle経路で処理
    if (idx < 255)
    {
        for (const auto& entry : m_customModels)
        {
            if (entry.id == idx)
                return skinned ? entry.pso.psoSkinned.Get() : entry.pso.pso.Get();
        }
        // 登録されていないIDが指定された
        GX_LOG_ERROR("ShaderRegistry::GetPSO: custom shader model id %u not registered, falling back to Standard", idx);
    }

    // フォールバック: Standard
    return skinned ? m_psos[0].psoSkinned.Get() : m_psos[0].pso.Get();
}

ID3D12PipelineState* ShaderRegistry::GetToonOutlinePSO(bool skinned) const
{
    return skinned ? m_toonOutline.psoSkinned.Get() : m_toonOutline.pso.Get();
}

bool ShaderRegistry::Rebuild(ID3D12Device* device)
{
    for (uint32_t i = 0; i < k_NumShaderModels; ++i)
    {
        if (!CompileAndCreatePSO(device, static_cast<gxfmt::ShaderModel>(i)))
            return false;
    }
    if (!CompileToonOutlinePSO(device))
        return false;

    // Layer 2: 登録済みカスタムシェーダーモデルも再コンパイル
    for (auto& entry : m_customModels)
    {
        if (!CompileCustomShaderModelPSO(device, entry))
        {
            GX_LOG_ERROR("ShaderRegistry::Rebuild: custom model id %u failed", entry.id);
            // カスタムの失敗は全体失敗にしない(組み込みが生きていれば動作可能)
        }
    }

    GX_LOG_INFO("ShaderRegistry: Rebuilt all PSOs (%zu custom)", m_customModels.size());
    return true;
}

// =========================================================================
// Layer 2: カスタムシェーダーモデル登録 (ADR-0017 L2, T1.6/T1.7)
// =========================================================================

bool ShaderRegistry::RegisterCustomShaderModel(uint32_t customId, const CustomShaderModelDesc& desc)
{
    if (customId < k_NumShaderModels || customId >= 255)
    {
        GX_LOG_ERROR("RegisterCustomShaderModel: id %u out of allowed range [6, 254] (0-5 reserved for builtins, 255 reserved for Custom)", customId);
        return false;
    }
    if (desc.vsPath.empty() || desc.psPath.empty())
    {
        GX_LOG_ERROR("RegisterCustomShaderModel: vsPath or psPath is empty (id=%u)", customId);
        return false;
    }
    if (!m_rootSignature)
    {
        GX_LOG_ERROR("RegisterCustomShaderModel: ShaderRegistry not initialized (id=%u)", customId);
        return false;
    }

    // 既存エントリを探して置換、なければ追加
    CustomShaderModelEntry* target = nullptr;
    for (auto& e : m_customModels)
    {
        if (e.id == customId) { target = &e; break; }
    }
    if (!target)
    {
        m_customModels.push_back(CustomShaderModelEntry{});
        target = &m_customModels.back();
    }
    target->id   = customId;
    target->desc = desc;

    // コンパイル
    ID3D12Device* device = nullptr;
    m_rootSignature->GetDevice(IID_PPV_ARGS(&device));
    if (!device)
    {
        GX_LOG_ERROR("RegisterCustomShaderModel: failed to obtain D3D12 device (id=%u)", customId);
        return false;
    }

    bool ok = CompileCustomShaderModelPSO(device, *target);
    device->Release();

    if (ok)
        GX_LOG_INFO("ShaderRegistry: registered custom shader model id=%u (vs='%ls' ps='%ls')",
                    customId, desc.vsPath.c_str(), desc.psPath.c_str());
    return ok;
}

bool ShaderRegistry::UnregisterCustomShaderModel(uint32_t customId)
{
    for (auto it = m_customModels.begin(); it != m_customModels.end(); ++it)
    {
        if (it->id == customId)
        {
            m_customModels.erase(it);
            return true;
        }
    }
    return false;
}

// char (ASCII) 文字列を wchar_t 文字列に1対1で変換するヘルパー
// シェーダーの #define 名はASCIIのみの想定なのでこれで十分
static gx::WString AsciiToWString(const gx::String& s)
{
    gx::WString result;
    result.reserve(s.size());
    for (char c : s) result.push_back(static_cast<wchar_t>(c));
    return result;
}

bool ShaderRegistry::CompileCustomShaderModelPSO(ID3D12Device* device, CustomShaderModelEntry& entry)
{
    const auto& desc = entry.desc;

    // ユーザー定義の #define を変換
    gx::Vector<std::pair<gx::WString, gx::WString>> defines;
    for (const auto& d : desc.defines)
    {
        // "NAME=VALUE" 形式をパース(= が無ければ値 "1")
        auto eq = d.find('=');
        gx::WString name, val;
        if (eq == gx::String::npos)
        {
            name = AsciiToWString(d);
            val  = L"1";
        }
        else
        {
            name = AsciiToWString(d.substr(0, eq));
            val  = AsciiToWString(d.substr(eq + 1));
        }
        defines.push_back({ name, val });
    }

    gx::WString vsEntryW = AsciiToWString(desc.vsEntry);
    gx::WString psEntryW = AsciiToWString(desc.psEntry);

    // Static variant
    auto vsBlob = m_shaderCompiler.CompileFromFile(desc.vsPath.c_str(), vsEntryW.c_str(), L"vs_6_0", defines);
    auto psBlob = m_shaderCompiler.CompileFromFile(desc.psPath.c_str(), psEntryW.c_str(), L"ps_6_0", defines);
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("ShaderRegistry: custom model id=%u compile failed: %s",
                     entry.id, m_shaderCompiler.GetLastError().c_str());
        return false;
    }

    PipelineStateBuilder b;
    entry.pso.pso = b
        .SetRootSignature(m_rootSignature)
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(k_Vertex3DPBRLayout, _countof(k_Vertex3DPBRLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 1)
        .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 2)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_BACK)
        .Build(device);

    // Skinned variant (opt-in)
    if (desc.supportsSkinning)
    {
        auto skinDefs = defines;
        skinDefs.push_back({ L"SKINNED", L"1" });
        auto vsSk = m_shaderCompiler.CompileFromFile(desc.vsPath.c_str(), vsEntryW.c_str(), L"vs_6_0", skinDefs);
        auto psSk = m_shaderCompiler.CompileFromFile(desc.psPath.c_str(), psEntryW.c_str(), L"ps_6_0", skinDefs);
        if (vsSk.valid && psSk.valid)
        {
            PipelineStateBuilder bs;
            entry.pso.psoSkinned = bs
                .SetRootSignature(m_rootSignature)
                .SetVertexShader(vsSk.GetBytecode())
                .SetPixelShader(psSk.GetBytecode())
                .SetInputLayout(k_Vertex3DSkinnedLayout, _countof(k_Vertex3DSkinnedLayout))
                .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)
                .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 1)
                .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, 2)
                .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
                .SetDepthEnable(true)
                .SetCullMode(D3D12_CULL_MODE_BACK)
                .Build(device);
        }
        else
        {
            GX_LOG_ERROR("ShaderRegistry: custom model id=%u skinned compile failed: %s",
                         entry.id, m_shaderCompiler.GetLastError().c_str());
        }
    }

    return entry.pso.pso != nullptr;
}

} // namespace gx
