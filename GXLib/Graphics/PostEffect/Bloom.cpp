/// @file Bloom.cpp
/// @brief Bloomエフェクトの実装
///
/// 処理フロー:
/// 1. Threshold: hdrRT → mipRT[0] (閾値以上のみ抽出)
/// 2. Downsample: mipRT[0] → mipRT[1] → ... → mipRT[4]
/// 3. Blur: 各レベルで H blur(mipRT[i] → blurTemp[i]) + V blur(blurTemp[i] → mipRT[i])
/// 4. Upsample: mipRT[4] → mipRT[3]にadd → ... → mipRT[0]にadd
/// 5. Composite: destRTにhdrRTをコピーし、mipRT[0]をアディティブ合成
#include "pch.h"
#include "Graphics/PostEffect/Bloom.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

bool Bloom::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;

    if (!m_shader.Initialize())
        return false;

    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    if (!CreatePipelines(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/Bloom.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    CreateMipRenderTargets(device, width, height);

    GX_LOG_INFO("Bloom initialized (%d mip levels)", k_MaxMipLevels);
    return true;
}

bool Bloom::CreatePipelines(ID3D12Device* device)
{
    auto vsBlob      = m_shader.CompileFromFile(L"Shaders/Bloom.hlsl", L"FullscreenVS",     L"vs_6_0");
    auto psThreshold = m_shader.CompileFromFile(L"Shaders/Bloom.hlsl", L"PSThreshold",      L"ps_6_0");
    auto psDown      = m_shader.CompileFromFile(L"Shaders/Bloom.hlsl", L"PSDownsample",     L"ps_6_0");
    auto psCopy      = m_shader.CompileFromFile(L"Shaders/Bloom.hlsl", L"PSCopy",           L"ps_6_0");
    auto psBlurH     = m_shader.CompileFromFile(L"Shaders/Bloom.hlsl", L"PSGaussianBlurH",  L"ps_6_0");
    auto psBlurV     = m_shader.CompileFromFile(L"Shaders/Bloom.hlsl", L"PSGaussianBlurV",  L"ps_6_0");
    auto psAdditive  = m_shader.CompileFromFile(L"Shaders/Bloom.hlsl", L"PSAdditive",       L"ps_6_0");

    if (!vsBlob.valid || !psThreshold.valid || !psDown.valid || !psCopy.valid ||
        !psBlurH.valid || !psBlurV.valid || !psAdditive.valid)
    {
        GX_LOG_ERROR("Bloom: Failed to compile shaders");
        return false;
    }

    // ルートシグネチャ: b0 + t0 + s0
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

    const DXGI_FORMAT fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
    auto vs = vsBlob.GetBytecode();

    // 各PSO（通常ブレンド = 不透明描画）
    auto buildPSO = [&](D3D12_SHADER_BYTECODE ps) -> ComPtr<ID3D12PipelineState> {
        PipelineStateBuilder b;
        return b.SetRootSignature(m_rootSignature.Get())
            .SetVertexShader(vs).SetPixelShader(ps)
            .SetRenderTargetFormat(fmt)
            .SetDepthEnable(false).SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
    };

    m_thresholdPSO  = buildPSO(psThreshold.GetBytecode());
    m_downsamplePSO = buildPSO(psDown.GetBytecode());
    m_copyPSO       = buildPSO(psCopy.GetBytecode());
    m_blurHPSO      = buildPSO(psBlurH.GetBytecode());
    m_blurVPSO      = buildPSO(psBlurV.GetBytecode());

    if (!m_thresholdPSO || !m_downsamplePSO || !m_copyPSO || !m_blurHPSO || !m_blurVPSO)
        return false;

    // アディティブブレンドPSO（Bloom結果を既存画像に加算）
    {
        PipelineStateBuilder b;
        m_additivePSO = b.SetRootSignature(m_rootSignature.Get())
            .SetVertexShader(vs).SetPixelShader(psAdditive.GetBytecode())
            .SetRenderTargetFormat(fmt)
            .SetDepthEnable(false).SetCullMode(D3D12_CULL_MODE_NONE)
            .SetAdditiveBlend()
            .Build(device);
        if (!m_additivePSO) return false;
    }

    return true;
}

void Bloom::CreateMipRenderTargets(ID3D12Device* device, uint32_t width, uint32_t height)
{
    const DXGI_FORMAT fmt = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uint32_t w = width, h = height;

    for (uint32_t i = 0; i < k_MaxMipLevels; ++i)
    {
        w = (std::max)(w / 2, 1u);
        h = (std::max)(h / 2, 1u);
        m_mipWidths[i]  = w;
        m_mipHeights[i] = h;
        if (!m_mipRT[i].Create(device, w, h, fmt))
        {
            GX_LOG_ERROR("Bloom: Failed to create mip RT level %u (%ux%u)", i, w, h);
        }
        if (!m_blurTempRT[i].Create(device, w, h, fmt))
        {
            GX_LOG_ERROR("Bloom: Failed to create blur temp RT level %u (%ux%u)", i, w, h);
        }
    }
}

void Bloom::UpdateCB(ID3D12GraphicsCommandList* cmdList, float texelX, float texelY)
{
    BloomConstants bc = {};
    bc.threshold  = m_threshold;
    bc.intensity  = m_intensity;
    bc.texelSizeX = texelX;
    bc.texelSizeY = texelY;

    void* p = m_constantBuffer.Map(0);
    if (p)
    {
        memcpy(p, &bc, sizeof(bc));
        m_constantBuffer.Unmap(0);
    }
}

void Bloom::DrawFullscreen(ID3D12GraphicsCommandList* cmdList,
                            ID3D12PipelineState* pso,
                            RenderTarget& dest, RenderTarget& src,
                            float texelX, float texelY)
{
    dest.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    src.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    auto rtv = dest.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(dest.GetWidth());
    vp.Height = static_cast<float>(dest.GetHeight());
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(dest.GetWidth());
    sc.bottom = static_cast<LONG>(dest.GetHeight());
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(pso);
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { src.GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    UpdateCB(cmdList, texelX, texelY);
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(0));
    cmdList->SetGraphicsRootDescriptorTable(1, src.GetSRVGPUHandle());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

void Bloom::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                     RenderTarget& hdrRT, RenderTarget& destRT)
{
    if (!m_enabled) return;

    // 1. Threshold: hdrRT → mipRT[0]
    {
        float tw = 1.0f / static_cast<float>(hdrRT.GetWidth());
        float th = 1.0f / static_cast<float>(hdrRT.GetHeight());
        DrawFullscreen(cmdList, m_thresholdPSO.Get(), m_mipRT[0], hdrRT, tw, th);
    }

    // 2. Downsample chain
    for (uint32_t i = 1; i < k_MaxMipLevels; ++i)
    {
        float tw = 1.0f / static_cast<float>(m_mipWidths[i - 1]);
        float th = 1.0f / static_cast<float>(m_mipHeights[i - 1]);
        DrawFullscreen(cmdList, m_downsamplePSO.Get(), m_mipRT[i], m_mipRT[i - 1], tw, th);
    }

    // 3. Gaussian blur at each level
    for (uint32_t i = 0; i < k_MaxMipLevels; ++i)
    {
        float tw = 1.0f / static_cast<float>(m_mipWidths[i]);
        float th = 1.0f / static_cast<float>(m_mipHeights[i]);
        // H: mipRT[i] → blurTempRT[i]
        DrawFullscreen(cmdList, m_blurHPSO.Get(), m_blurTempRT[i], m_mipRT[i], tw, th);
        // V: blurTempRT[i] → mipRT[i]
        DrawFullscreen(cmdList, m_blurVPSO.Get(), m_mipRT[i], m_blurTempRT[i], tw, th);
    }

    // 4. Upsample chain: 最小ミップから最大ミップへ逆順にアディティブ合成。
    // 低ミップの広いブラーが上位に伝播し、マルチスケールBloomの柔らかな光条を生む
    for (int i = k_MaxMipLevels - 1; i > 0; --i)
    {
        float tw = 1.0f / static_cast<float>(m_mipWidths[i]);
        float th = 1.0f / static_cast<float>(m_mipHeights[i]);

        // mipRT[i]をmipRT[i-1]にアディティブ描画
        m_mipRT[i].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_mipRT[i - 1].TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto rtv = m_mipRT[i - 1].GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        D3D12_VIEWPORT vp = {};
        vp.Width  = static_cast<float>(m_mipWidths[i - 1]);
        vp.Height = static_cast<float>(m_mipHeights[i - 1]);
        vp.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &vp);

        D3D12_RECT sc = {};
        sc.right  = static_cast<LONG>(m_mipWidths[i - 1]);
        sc.bottom = static_cast<LONG>(m_mipHeights[i - 1]);
        cmdList->RSSetScissorRects(1, &sc);

        cmdList->SetPipelineState(m_additivePSO.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

        ID3D12DescriptorHeap* heaps[] = { m_mipRT[i].GetSRVHeap().GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        // intensityは最終合成時に使うので、ここでは1.0
        BloomConstants bc = {};
        bc.intensity = 1.0f;
        bc.texelSizeX = tw;
        bc.texelSizeY = th;
        void* p = m_constantBuffer.Map(0);
        if (p) { memcpy(p, &bc, sizeof(bc)); m_constantBuffer.Unmap(0); }
        cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(0));
        cmdList->SetGraphicsRootDescriptorTable(1, m_mipRT[i].GetSRVGPUHandle());

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // 5. Final composite: hdrRTをdestRTにコピーし、mipRT[0]をアディティブ合成
    // Step 5a: hdrRT → destRT（コピー — PSCopyで1:1サンプル）
    DrawFullscreen(cmdList, m_copyPSO.Get(), destRT, hdrRT,
                   1.0f / static_cast<float>(hdrRT.GetWidth()),
                   1.0f / static_cast<float>(hdrRT.GetHeight()));

    // Step 5b: mipRT[0] → destRTにアディティブ合成
    {
        m_mipRT[0].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        destRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto rtv = destRT.GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        D3D12_VIEWPORT vp = {};
        vp.Width  = static_cast<float>(destRT.GetWidth());
        vp.Height = static_cast<float>(destRT.GetHeight());
        vp.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &vp);

        D3D12_RECT sc = {};
        sc.right  = static_cast<LONG>(destRT.GetWidth());
        sc.bottom = static_cast<LONG>(destRT.GetHeight());
        cmdList->RSSetScissorRects(1, &sc);

        cmdList->SetPipelineState(m_additivePSO.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

        ID3D12DescriptorHeap* heaps[] = { m_mipRT[0].GetSRVHeap().GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        UpdateCB(cmdList, 1.0f / static_cast<float>(m_mipWidths[0]),
                          1.0f / static_cast<float>(m_mipHeights[0]));
        cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(0));
        cmdList->SetGraphicsRootDescriptorTable(1, m_mipRT[0].GetSRVGPUHandle());

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
}

void Bloom::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    CreateMipRenderTargets(device, width, height);
}

} // namespace GX
