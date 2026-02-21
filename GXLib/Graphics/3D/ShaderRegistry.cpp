/// @file ShaderRegistry.cpp
/// @brief シェーダーモデルPSOレジストリの実装

#include "pch.h"
#include "Graphics/3D/ShaderRegistry.h"
#include "Graphics/3D/Vertex3D.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
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
    std::vector<std::pair<std::wstring, std::wstring>> skinnedDefines = { { L"SKINNED", L"1" } };
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
    std::vector<std::pair<std::wstring, std::wstring>> skinnedDefines = { { L"SKINNED", L"1" } };
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
    if (idx >= k_NumShaderModels)
        idx = 0; // fallback to Standard

    return skinned ? m_psos[idx].psoSkinned.Get() : m_psos[idx].pso.Get();
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

    GX_LOG_INFO("ShaderRegistry: Rebuilt all PSOs");
    return true;
}

} // namespace GX
