/// @file AutoExposure.cpp
/// @brief 自動露出の実装
///
/// HDR→256x256 log輝度→64→16→4→1 ダウンサンプル→CPUリードバック→露出計算
/// ピクセルシェーダーベース（CSインフラ不要）、2フレームリングバッファでストール回避。
#include "pch.h"
#include "Graphics/PostEffect/AutoExposure.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

// k_Sizes の定義
constexpr uint32_t AutoExposure::k_Sizes[k_NumLevels];

bool AutoExposure::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    // 輝度ダウンサンプルRT
    for (uint32_t i = 0; i < k_NumLevels; ++i)
    {
        if (!m_luminanceRT[i].Create(device, k_Sizes[i], k_Sizes[i], DXGI_FORMAT_R16_FLOAT))
            return false;
    }

    if (!m_shader.Initialize())
        return false;

    if (!m_cb.Initialize(device, 256, 256))
        return false;

    // 共通RS: b0 + t0 + s0 (linear)
    {
        RootSignatureBuilder rsb;
        m_commonRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0)
            .Build(device);
        if (!m_commonRS) return false;
    }

    auto vs = m_shader.CompileFromFile(L"Shaders/AutoExposure.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;
    auto vsBytecode = vs.GetBytecode();

    // PSO1: HDR → log luminance (R16_FLOAT)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/AutoExposure.hlsl", L"PSLogLuminance", L"ps_6_0");
        if (!ps.valid) return false;

        PipelineStateBuilder b;
        m_luminancePSO = b.SetRootSignature(m_commonRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_luminancePSO) return false;
    }

    // PSO2: downsample (R16_FLOAT → R16_FLOAT)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/AutoExposure.hlsl", L"PSDownsample", L"ps_6_0");
        if (!ps.valid) return false;

        PipelineStateBuilder b;
        m_downsamplePSO = b.SetRootSignature(m_commonRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_downsamplePSO) return false;
    }

    // リードバックバッファ (2フレーム分、各1ピクセル=2バイト=R16_FLOAT)
    for (int i = 0; i < 2; ++i)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width = 256; // R16_FLOATの1ピクセルは2バイト、余裕を持って256
        resDesc.Height = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_readbackBuffer[i]));
        if (FAILED(hr)) return false;
    }

    GX_LOG_INFO("AutoExposure initialized");
    return true;
}

float AutoExposure::ComputeExposure(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                      RenderTarget& hdrScene, float deltaTime)
{
    // === Pass 1: HDR → 256x256 log luminance ===
    {
        hdrScene.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_luminanceRT[0].TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto rtv = m_luminanceRT[0].GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        D3D12_VIEWPORT vp = {};
        vp.Width = static_cast<float>(k_Sizes[0]);
        vp.Height = static_cast<float>(k_Sizes[0]);
        vp.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &vp);

        D3D12_RECT sc = {};
        sc.right = static_cast<LONG>(k_Sizes[0]);
        sc.bottom = static_cast<LONG>(k_Sizes[0]);
        cmdList->RSSetScissorRects(1, &sc);

        cmdList->SetPipelineState(m_luminancePSO.Get());
        cmdList->SetGraphicsRootSignature(m_commonRS.Get());

        ID3D12DescriptorHeap* heaps[] = { hdrScene.GetSRVHeap().GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        // ダミーCB（使わないが RS に b0 がある）
        float dummy[4] = { 0, 0, 0, 0 };
        void* p = m_cb.Map(frameIndex);
        if (p) { memcpy(p, dummy, sizeof(dummy)); m_cb.Unmap(frameIndex); }
        cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
        cmdList->SetGraphicsRootDescriptorTable(1, hdrScene.GetSRVGPUHandle());

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // === Pass 2-5: Downsample 256→64→16→4→1 ===
    for (uint32_t i = 1; i < k_NumLevels; ++i)
    {
        m_luminanceRT[i - 1].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_luminanceRT[i].TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto rtv = m_luminanceRT[i].GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        D3D12_VIEWPORT vp = {};
        vp.Width = static_cast<float>(k_Sizes[i]);
        vp.Height = static_cast<float>(k_Sizes[i]);
        vp.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &vp);

        D3D12_RECT sc = {};
        sc.right = static_cast<LONG>(k_Sizes[i]);
        sc.bottom = static_cast<LONG>(k_Sizes[i]);
        cmdList->RSSetScissorRects(1, &sc);

        cmdList->SetPipelineState(m_downsamplePSO.Get());
        cmdList->SetGraphicsRootSignature(m_commonRS.Get());

        ID3D12DescriptorHeap* heaps[] = { m_luminanceRT[i - 1].GetSRVHeap().GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        float dummy[4] = { 0, 0, 0, 0 };
        void* p = m_cb.Map(frameIndex);
        if (p) { memcpy(p, dummy, sizeof(dummy)); m_cb.Unmap(frameIndex); }
        cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
        cmdList->SetGraphicsRootDescriptorTable(1, m_luminanceRT[i - 1].GetSRVGPUHandle());

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // === 1x1 RT → リードバックバッファにコピー ===
    {
        uint32_t rbIdx = m_readbackFrameCount % 2;
        m_luminanceRT[k_NumLevels - 1].TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

        D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
        srcLoc.pResource = m_luminanceRT[k_NumLevels - 1].GetResource();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource = m_readbackBuffer[rbIdx].Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLoc.PlacedFootprint.Offset = 0;
        dstLoc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R16_FLOAT;
        dstLoc.PlacedFootprint.Footprint.Width = 1;
        dstLoc.PlacedFootprint.Footprint.Height = 1;
        dstLoc.PlacedFootprint.Footprint.Depth = 1;
        dstLoc.PlacedFootprint.Footprint.RowPitch = 256; // 最低256バイトアライメント

        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
    }

    // === 前フレームのリードバック値を読取り (2フレーム遅延) ===
    if (m_readbackFrameCount >= 2)
    {
        uint32_t readIdx = (m_readbackFrameCount) % 2; // 反対側のバッファを読む
        void* mapped = nullptr;
        D3D12_RANGE readRange = { 0, 2 }; // R16_FLOAT = 2 bytes
        if (SUCCEEDED(m_readbackBuffer[readIdx]->Map(0, &readRange, &mapped)))
        {
            // R16_FLOAT → float 変換
            uint16_t halfVal = *reinterpret_cast<const uint16_t*>(mapped);
            D3D12_RANGE writeRange = { 0, 0 };
            m_readbackBuffer[readIdx]->Unmap(0, &writeRange);

            // half → float 変換 (IEEE 754 half precision)
            uint32_t sign = (halfVal >> 15) & 0x1;
            uint32_t exp  = (halfVal >> 10) & 0x1F;
            uint32_t mant = halfVal & 0x3FF;

            float avgLogLum;
            if (exp == 0)
            {
                // subnormal
                avgLogLum = (sign ? -1.0f : 1.0f) * (mant / 1024.0f) * powf(2.0f, -14.0f);
            }
            else if (exp == 31)
            {
                avgLogLum = 0.0f; // inf/nan → treat as 0
            }
            else
            {
                avgLogLum = (sign ? -1.0f : 1.0f) * (1.0f + mant / 1024.0f) * powf(2.0f, static_cast<float>(exp) - 15.0f);
            }

            // 有効値のみ更新
            if (std::isfinite(avgLogLum))
                m_lastAvgLuminance = avgLogLum;
        }
    }

    m_readbackFrameCount++;

    // === 露出計算 ===
    // avgLogLum は log(luminance) の平均値
    float avgLum = expf(m_lastAvgLuminance);
    avgLum = (std::max)(avgLum, 0.001f); // ゼロ除算防止

    float targetExposure = m_keyValue / avgLum;
    targetExposure = (std::max)(m_minExposure, (std::min)(m_maxExposure, targetExposure));

    // 時間的適応 (exponential smoothing)
    float adaptRate = 1.0f - expf(-m_adaptationSpeed * deltaTime);
    m_currentExposure = m_currentExposure + (targetExposure - m_currentExposure) * adaptRate;
    m_currentExposure = (std::max)(m_minExposure, (std::min)(m_maxExposure, m_currentExposure));

    return m_currentExposure;
}

void AutoExposure::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    // ダウンサンプルチェーンは固定サイズなのでリサイズ不要
    (void)device;
    (void)width;
    (void)height;
}

} // namespace GX
