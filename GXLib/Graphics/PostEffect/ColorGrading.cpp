/// @file ColorGrading.cpp
/// @brief Color Grading エフェクトの実装
///
/// シーンテクスチャに対してピクセルシェーダーで色調補正を一括適用する。
/// 処理順序: 露出 → 色温度/ティント → コントラスト → 彩度 → 明度 → ガンマ
#include "pch_graphics.h"
#include "Graphics/PostEffect/ColorGrading.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace gx
{

bool ColorGrading::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    if (!device)
    {
        GX_LOG_INFO("ColorGrading initialized (CPU-only, no device)");
        return true;
    }

    if (!m_shader.Initialize())
        return false;

    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    // ルートシグネチャ: [0]=CBV(b0) + [1]=DescTable(t0 SRV) + s0(linear clamp)
    {
        RootSignatureBuilder rsb;
        m_rootSignature = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_rootSignature) return false;
    }

    if (!CreatePipelines(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/ColorGrading.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("ColorGrading initialized (%dx%d)", width, height);
    return true;
}

bool ColorGrading::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/ColorGrading.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/ColorGrading.hlsl", L"PSColorGrading", L"ps_6_0");
    if (!ps.valid) return false;

    PipelineStateBuilder b;
    m_pso = b.SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vs.GetBytecode())
        .SetPixelShader(ps.GetBytecode())
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .Build(device);
    if (!m_pso) return false;

    return true;
}

void ColorGrading::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                             RenderTarget& sceneRT, RenderTarget& destRT)
{
    if (!m_enabled || !m_device) return;

    // リソース遷移
    sceneRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // RTV設定
    auto rtv = destRT.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width    = static_cast<float>(destRT.GetWidth());
    vp.Height   = static_cast<float>(destRT.GetHeight());
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(destRT.GetWidth());
    sc.bottom = static_cast<LONG>(destRT.GetHeight());
    cmdList->RSSetScissorRects(1, &sc);

    // PSO + RS
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // SRVヒープバインド
    ID3D12DescriptorHeap* heaps[] = { sceneRT.GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 定数バッファ更新
    ColorGradingConstants constants = {};
    constants.contrast    = m_contrast;
    constants.saturation  = m_saturation;
    constants.brightness  = m_brightness;
    constants.temperature = m_temperature;
    constants.tint        = m_tint;
    constants.gamma       = m_gamma;
    constants.exposure    = m_exposure;
    constants._pad        = 0.0f;

    void* p = m_constantBuffer.Map(0);
    if (p)
    {
        memcpy(p, &constants, sizeof(constants));
        m_constantBuffer.Unmap(0);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(0));
    cmdList->SetGraphicsRootDescriptorTable(1, sceneRT.GetSRVGPUHandle());

    // フルスクリーンドロー (頂点バッファなし、VS内でUV生成)
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

void ColorGrading::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    if (!device || width == 0 || height == 0)
    {
        GX_LOG_ERROR("ColorGrading::OnResize: invalid parameters (device=%p, %ux%u)",
                     device, width, height);
        return;
    }
    m_width  = width;
    m_height = height;
}

} // namespace gx
