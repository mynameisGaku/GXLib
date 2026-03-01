/// @file LensFlare.cpp
/// @brief レンズフレア/グレアエフェクトの実装
///
/// HDRシーンから閾値以上の明部を抽出し、ゴーストイメージ・ハローリング・
/// スターバースト・色収差を合成してレンズフレアを生成する。
/// フルスクリーンパスで処理し、結果をHDRシーンに加算合成する。
#include "pch_graphics.h"
#include "Graphics/PostEffect/LensFlare.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace gx
{

bool LensFlare::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // CPU-only 初期化 (テスト用)
    if (!device)
    {
        m_initialized = true;
        return true;
    }

    if (!m_shader.Initialize())
        return false;

    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    if (!CreatePipelines(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/LensFlare.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    m_initialized = true;
    GX_LOG_INFO("LensFlare initialized (%ux%u)", width, height);
    return true;
}

void LensFlare::Shutdown()
{
    m_pso.Reset();
    m_rootSignature.Reset();
    m_initialized = false;
}

bool LensFlare::CreatePipelines(ID3D12Device* device)
{
    auto vsBlob = m_shader.CompileFromFile(L"Shaders/LensFlare.hlsl", L"FullscreenVS", L"vs_6_0");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/LensFlare.hlsl", L"PSMain",       L"ps_6_0");

    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("LensFlare: Failed to compile shaders");
        return false;
    }

    // ルートシグネチャ: b0(CBV) + t0(SRV: HDR scene) + s0(sampler)
    RootSignatureBuilder rsBuilder;
    rsBuilder.AddCBV(0);               // LensFlareCB
    rsBuilder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);  // tHDRScene
    rsBuilder.AddStaticSampler(0);     // sLinear
    m_rootSignature = rsBuilder.Build(device);
    if (!m_rootSignature)
    {
        GX_LOG_ERROR("LensFlare: Failed to create root signature");
        return false;
    }

    // PSO作成 (アディティブブレンド)
    PipelineStateBuilder psoBuilder;
    psoBuilder.SetRootSignature(m_rootSignature.Get());
    psoBuilder.SetVertexShader(vsBlob.GetBytecode());
    psoBuilder.SetPixelShader(psBlob.GetBytecode());
    psoBuilder.SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
    psoBuilder.SetDepthEnable(false);
    m_pso = psoBuilder.Build(device);
    if (!m_pso)
    {
        GX_LOG_ERROR("LensFlare: Failed to create PSO");
        return false;
    }

    return true;
}

void LensFlare::Execute(ID3D12GraphicsCommandList* cmdList,
                        RenderTarget& hdrInput, RenderTarget& outputRT,
                        uint32_t frameIndex)
{
    if (!m_settings.enabled || !m_initialized || !cmdList)
        return;

    // 定数バッファ更新
    struct LensFlareCB
    {
        float    threshold;
        float    intensity;
        int      ghostCount;
        float    ghostDispersion;
        float    haloRadius;
        float    chromaticShift;
        float    screenSizeX;
        float    screenSizeY;
    } cb = {};

    cb.threshold       = m_settings.threshold;
    cb.intensity       = m_settings.intensity;
    cb.ghostCount      = m_settings.ghostCount;
    cb.ghostDispersion = m_settings.ghostDispersion;
    cb.haloRadius      = m_settings.haloRadius;
    cb.chromaticShift  = m_settings.chromaticShift;
    cb.screenSizeX     = static_cast<float>(m_width);
    cb.screenSizeY     = static_cast<float>(m_height);

    void* p = m_constantBuffer.Map(frameIndex);
    if (p) { memcpy(p, &cb, sizeof(cb)); m_constantBuffer.Unmap(frameIndex); }

    // パイプラインバインド
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(frameIndex));

    // フルスクリーン三角形描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

} // namespace gx
