/// @file SSGI.cpp
/// @brief Screen-Space Global Illumination の実装
///
/// GBuffer情報（深度・法線・カラー）を使ってスクリーン空間で間接照明を計算する。
/// 各ピクセルからランダム方向にレイマーチングし、ヒットしたサーフェスのカラーを
/// バウンスライトとして加算する。テンポラルリプロジェクションで前フレームの結果と
/// ブレンドし、ノイズを大幅に低減する。
#include "pch_graphics.h"
#include "Graphics/PostEffect/SSGI.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"

#include <algorithm>

namespace gx
{

// ---------------------------------------------------------------------------
// パラメータ設定（クランプ付き）
// ---------------------------------------------------------------------------

void SSGI::SetIntensity(float intensity)
{
    m_intensity = std::clamp(intensity, 0.0f, 2.0f);
}

void SSGI::SetSampleCount(int count)
{
    m_sampleCount = std::clamp(count, 4, 128);
}

void SSGI::SetMaxDistance(float dist)
{
    m_maxDistance = std::clamp(dist, 1.0f, 200.0f);
}

// ---------------------------------------------------------------------------
// 初期化
// ---------------------------------------------------------------------------

bool SSGI::Initialize(GraphicsDevice* device, uint32_t width, uint32_t height)
{
    if (!device)
        return false;

    auto* d3dDevice = device->GetDevice();
    if (!d3dDevice)
        return false;

    m_device = d3dDevice;
    m_width  = width;
    m_height = height;

    // 専用SRVヒープ (3テクスチャ x 2フレーム = 6スロット)
    if (!m_srvHeap.Initialize(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 6, true))
        return false;

    // シェーダーコンパイラ
    if (!m_shader.Initialize())
        return false;

    // 定数バッファ (256バイトアラインメント)
    if (!m_cb.Initialize(d3dDevice, 256, 256))
        return false;

    // ルートシグネチャ: [0]=CBV(b0) + [1]=DescTable(t0,t1,t2 の3連続SRV) + s0(linear) + s1(point)
    {
        RootSignatureBuilder rsb;
        m_rootSignature = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(d3dDevice);
        if (!m_rootSignature) return false;
    }

    if (!CreatePipelines(d3dDevice))
        return false;

    // SSGI中間結果RT (テンポラル合成前の生SSGI出力)
    if (!m_intermediateRT.Create(d3dDevice, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT))
        return false;

    // テンポラル蓄積ヒストリRT (ダブルバッファ)
    if (!m_historyRT[0].Create(d3dDevice, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT))
        return false;
    if (!m_historyRT[1].Create(d3dDevice, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT))
        return false;

    // テンポラルリゾルブパイプライン
    if (!CreateTemporalPipeline(d3dDevice))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/SSGI.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/SSGITemporal.hlsl",
        [this](ID3D12Device* dev) { return CreateTemporalPipeline(dev); }
    );

    GX_LOG_INFO("SSGI initialized (%dx%d, samples=%d, temporal=on)", width, height, m_sampleCount);
    return true;
}

// ---------------------------------------------------------------------------
// パイプライン作成
// ---------------------------------------------------------------------------

bool SSGI::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/SSGI.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/SSGI.hlsl", L"PSSSGI", L"ps_6_0");
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

// ---------------------------------------------------------------------------
// テンポラルリゾルブパイプライン作成
// ---------------------------------------------------------------------------

bool SSGI::CreateTemporalPipeline(ID3D12Device* device)
{
    // テンポラルSRVヒープ (3テクスチャ: currentGI + history + depth, x 2フレーム = 6スロット + 余裕2 = 8)
    if (!m_temporalHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true))
        return false;

    // テンポラル定数バッファ
    if (!m_temporalCB.Initialize(device, 256, 256))
        return false;

    // テンポラルルートシグネチャ: [0]=CBV(b0) + [1]=DescTable(t0,t1,t2 の3連続SRV) + s0(linear)
    {
        RootSignatureBuilder rsb;
        m_temporalRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_temporalRS) return false;
    }

    auto vs = m_shader.CompileFromFile(L"Shaders/SSGITemporal.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/SSGITemporal.hlsl", L"PSTemporalResolve", L"ps_6_0");
    if (!ps.valid) return false;

    PipelineStateBuilder b;
    m_temporalPSO = b.SetRootSignature(m_temporalRS.Get())
        .SetVertexShader(vs.GetBytecode())
        .SetPixelShader(ps.GetBytecode())
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .Build(device);
    if (!m_temporalPSO) return false;

    return true;
}

// ---------------------------------------------------------------------------
// SRVヒープ更新 (SSGIメインパス)
// ---------------------------------------------------------------------------

void SSGI::UpdateSRVHeap(RenderTarget& srcHDR, RenderTarget& depthRT,
                          RenderTarget& normalRT, uint32_t frameIndex)
{
    uint32_t base = frameIndex * 3;

    // [base+0] = scene (HDR)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = srcHDR.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(srcHDR.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 0));
    }

    // [base+1] = depth
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(depthRT.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 1));
    }

    // [base+2] = normal
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = normalRT.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(normalRT.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 2));
    }
}

// ---------------------------------------------------------------------------
// テンポラルSRVヒープ更新 (RenderTarget深度版)
// ---------------------------------------------------------------------------

void SSGI::UpdateTemporalSRVHeap(RenderTarget& currentGI, RenderTarget& depthRT,
                                   uint32_t frameIndex)
{
    uint32_t base = frameIndex * 3;
    uint32_t readIdx = 1 - m_historyWriteIdx; // 読み取りは前回書き込んだ方

    // [base+0] = currentGI (SSGI中間結果)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = currentGI.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(currentGI.GetResource(), &srvDesc,
                                            m_temporalHeap.GetCPUHandle(base + 0));
    }

    // [base+1] = history (前フレームの蓄積結果)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = m_historyRT[readIdx].GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(m_historyRT[readIdx].GetResource(), &srvDesc,
                                            m_temporalHeap.GetCPUHandle(base + 1));
    }

    // [base+2] = depth (リプロジェクション用)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(depthRT.GetResource(), &srvDesc,
                                            m_temporalHeap.GetCPUHandle(base + 2));
    }
}

// ---------------------------------------------------------------------------
// テンポラルSRVヒープ更新 (DepthBuffer版)
// ---------------------------------------------------------------------------

void SSGI::UpdateTemporalSRVHeapFromDepthBuffer(RenderTarget& currentGI, DepthBuffer& depth,
                                                   uint32_t frameIndex)
{
    uint32_t base = frameIndex * 3;
    uint32_t readIdx = 1 - m_historyWriteIdx;

    // [base+0] = currentGI
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = currentGI.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(currentGI.GetResource(), &srvDesc,
                                            m_temporalHeap.GetCPUHandle(base + 0));
    }

    // [base+1] = history
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = m_historyRT[readIdx].GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(m_historyRT[readIdx].GetResource(), &srvDesc,
                                            m_temporalHeap.GetCPUHandle(base + 1));
    }

    // [base+2] = depth (TYPELESS -> R32_FLOAT)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                            m_temporalHeap.GetCPUHandle(base + 2));
    }
}

// ---------------------------------------------------------------------------
// テンポラルリゾルブ実行 (RenderTarget深度版)
// ---------------------------------------------------------------------------

void SSGI::ExecuteTemporalResolve(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                    RenderTarget& destHDR, RenderTarget& depthRT)
{
    if (!m_temporalPSO)
    {
        // テンポラルPSOが無い場合: 中間結果をそのままdestにコピー
        m_intermediateRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->CopyResource(destHDR.GetResource(), m_intermediateRT.GetResource());
        return;
    }

    uint32_t writeIdx = m_historyWriteIdx;

    // リソース遷移
    m_intermediateRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_historyRT[1 - writeIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depthRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // テンポラルSRVヒープ更新
    UpdateTemporalSRVHeap(m_intermediateRT, depthRT, frameIndex);

    // RTV設定
    auto destRTV = destHDR.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    // PSO + RS
    cmdList->SetPipelineState(m_temporalPSO.Get());
    cmdList->SetGraphicsRootSignature(m_temporalRS.Get());

    // ヒープバインド
    ID3D12DescriptorHeap* heaps[] = { m_temporalHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // テンポラル定数バッファ更新
    SSGITemporalConstants tc = {};
    XMStoreFloat4x4(XM(&tc.prevViewProj), XMMatrixTranspose(XMLoadFloat4x4(XM(&m_prevViewProj))));
    XMStoreFloat4x4(XM(&tc.invViewProj), XMMatrixTranspose(XMLoadFloat4x4(XM(&m_invViewProj))));
    tc.alpha        = m_temporalAlpha;
    tc.screenWidth  = static_cast<float>(m_width);
    tc.screenHeight = static_cast<float>(m_height);
    tc.hasHistory   = m_hasHistory ? 1.0f : 0.0f;

    void* p = m_temporalCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &tc, sizeof(tc));
        m_temporalCB.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_temporalCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_temporalHeap.GetGPUHandle(frameIndex * 3));

    // フルスクリーンドロー
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // destHDR -> historyRT[writeIdx] にコピー（次フレーム用ヒストリ蓄積）
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_historyRT[writeIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyResource(m_historyRT[writeIdx].GetResource(), destHDR.GetResource());
    m_historyRT[writeIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // ピンポンインデックスを切り替え
    m_historyWriteIdx = 1 - m_historyWriteIdx;
    m_hasHistory = true;
}

// ---------------------------------------------------------------------------
// テンポラルリゾルブ実行 (DepthBuffer版)
// ---------------------------------------------------------------------------

void SSGI::ExecuteTemporalResolveDepthBuffer(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                               RenderTarget& destHDR, DepthBuffer& depth)
{
    if (!m_temporalPSO)
    {
        m_intermediateRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->CopyResource(destHDR.GetResource(), m_intermediateRT.GetResource());
        return;
    }

    uint32_t writeIdx = m_historyWriteIdx;

    // リソース遷移
    m_intermediateRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_historyRT[1 - writeIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // テンポラルSRVヒープ更新
    UpdateTemporalSRVHeapFromDepthBuffer(m_intermediateRT, depth, frameIndex);

    // RTV設定
    auto destRTV = destHDR.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    // PSO + RS
    cmdList->SetPipelineState(m_temporalPSO.Get());
    cmdList->SetGraphicsRootSignature(m_temporalRS.Get());

    // ヒープバインド
    ID3D12DescriptorHeap* heaps[] = { m_temporalHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // テンポラル定数バッファ更新
    SSGITemporalConstants tc = {};
    XMStoreFloat4x4(XM(&tc.prevViewProj), XMMatrixTranspose(XMLoadFloat4x4(XM(&m_prevViewProj))));
    XMStoreFloat4x4(XM(&tc.invViewProj), XMMatrixTranspose(XMLoadFloat4x4(XM(&m_invViewProj))));
    tc.alpha        = m_temporalAlpha;
    tc.screenWidth  = static_cast<float>(m_width);
    tc.screenHeight = static_cast<float>(m_height);
    tc.hasHistory   = m_hasHistory ? 1.0f : 0.0f;

    void* p = m_temporalCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &tc, sizeof(tc));
        m_temporalCB.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_temporalCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_temporalHeap.GetGPUHandle(frameIndex * 3));

    // フルスクリーンドロー
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // destHDR -> historyRT[writeIdx] にコピー
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_historyRT[writeIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyResource(m_historyRT[writeIdx].GetResource(), destHDR.GetResource());
    m_historyRT[writeIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // ピンポンインデックスを切り替え
    m_historyWriteIdx = 1 - m_historyWriteIdx;
    m_hasHistory = true;
}

// ---------------------------------------------------------------------------
// DepthBuffer版 SRVヒープ更新
// ---------------------------------------------------------------------------

void SSGI::UpdateSRVHeapFromDepthBuffer(RenderTarget& srcHDR, DepthBuffer& depth,
                                          RenderTarget& normalRT, uint32_t frameIndex)
{
    uint32_t base = frameIndex * 3;

    // [base+0] = scene (HDR)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = srcHDR.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(srcHDR.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 0));
    }

    // [base+1] = depth (DepthBuffer: TYPELESS -> R32_FLOAT で読む)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 1));
    }

    // [base+2] = normal
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = normalRT.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(normalRT.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 2));
    }
}

// ---------------------------------------------------------------------------
// 実行 (RenderTarget深度版)
// ---------------------------------------------------------------------------

void SSGI::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                    RenderTarget& srcHDR, RenderTarget& destHDR,
                    RenderTarget& depthRT, RenderTarget& normalRT)
{
    if (!m_enabled || !cmdList || !m_pso)
        return;

    // === Pass 1: SSGI レイマーチング → 中間RT ===
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depthRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    normalRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_intermediateRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // SRVヒープ更新
    UpdateSRVHeap(srcHDR, depthRT, normalRT, frameIndex);

    // 中間RTにSSGI結果を描画
    auto intermediateRTV = m_intermediateRT.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &intermediateRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 定数バッファ更新
    SSGIConstants constants = {};
    constants.intensity    = m_intensity;
    constants.sampleCount  = m_sampleCount;
    constants.maxDistance   = m_maxDistance;
    constants.thickness    = m_thickness;
    constants.screenWidth  = static_cast<float>(m_width);
    constants.screenHeight = static_cast<float>(m_height);

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &constants, sizeof(constants));
        m_cb.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(frameIndex * 3));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // === Pass 2: テンポラルリゾルブ → destHDR ===
    ExecuteTemporalResolve(cmdList, frameIndex, destHDR, depthRT);
}

// ---------------------------------------------------------------------------
// 実行 (DepthBuffer版)
// ---------------------------------------------------------------------------

void SSGI::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                    RenderTarget& srcHDR, RenderTarget& destHDR,
                    DepthBuffer& depth, RenderTarget& normalRT)
{
    if (!m_enabled || !cmdList || !m_pso)
        return;

    // === Pass 1: SSGI レイマーチング → 中間RT ===
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    normalRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_intermediateRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // SRVヒープ更新
    UpdateSRVHeapFromDepthBuffer(srcHDR, depth, normalRT, frameIndex);

    // 中間RTにSSGI結果を描画
    auto intermediateRTV = m_intermediateRT.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &intermediateRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 定数バッファ更新
    SSGIConstants constants = {};
    constants.intensity    = m_intensity;
    constants.sampleCount  = m_sampleCount;
    constants.maxDistance   = m_maxDistance;
    constants.thickness    = m_thickness;
    constants.screenWidth  = static_cast<float>(m_width);
    constants.screenHeight = static_cast<float>(m_height);

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &constants, sizeof(constants));
        m_cb.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(frameIndex * 3));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // === Pass 2: テンポラルリゾルブ → destHDR ===
    ExecuteTemporalResolveDepthBuffer(cmdList, frameIndex, destHDR, depth);
}

// ---------------------------------------------------------------------------
// リサイズ
// ---------------------------------------------------------------------------

void SSGI::OnResize(GraphicsDevice* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    if (device)
    {
        m_device = device->GetDevice();
        // テンポラルRT再作成
        m_intermediateRT.Create(m_device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_historyRT[0].Create(m_device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_historyRT[1].Create(m_device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_hasHistory = false;
        m_historyWriteIdx = 0;
    }
}

void SSGI::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    if (device)
    {
        m_device = device;
        // テンポラルRT再作成
        m_intermediateRT.Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_historyRT[0].Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_historyRT[1].Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_hasHistory = false;
        m_historyWriteIdx = 0;
    }
}

} // namespace gx
