/// @file DepthOfField.cpp
/// @brief Depth of Field (被写界深度) の実装
///
/// 4パス構成:
/// 1. CoC生成: 深度→ビュー空間Z→CoC値 (R16_FLOAT)
/// 2. 水平ブラー: full-res HDR → half-res (ダウンサンプル+水平ブラー)
/// 3. 垂直ブラー: half-res → half-res
/// 4. 合成: シャープHDRとブラーHDRをCoC値でlerp
///
/// 合成パスでは3テクスチャを使うため、専用の3スロットSRVヒープを使用
/// (D3D12はSetDescriptorHeaps時に1つのCBV_SRV_UAVヒープしかバインドできない)
#include "pch.h"
#include "Graphics/PostEffect/DepthOfField.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

bool DepthOfField::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    uint32_t halfW = (std::max)(width / 2u, 1u);
    uint32_t halfH = (std::max)(height / 2u, 1u);

    // CoC map (R16_FLOAT, full-res)
    if (!m_cocRT.Create(device, width, height, DXGI_FORMAT_R16_FLOAT))
        return false;
    // ブラー中間 (HDR, half-res)
    if (!m_blurTempRT.Create(device, halfW, halfH, DXGI_FORMAT_R16G16B16A16_FLOAT))
        return false;
    // ブラー結果 (HDR, half-res)
    if (!m_blurRT.Create(device, halfW, halfH, DXGI_FORMAT_R16G16B16A16_FLOAT))
        return false;

    // 合成用SRVヒープ (shader-visible, 3スロット)
    // [0]=sharp(srcHDR), [1]=blurred(blurRT), [2]=CoC
    if (!m_compositeSRVHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 6, true))
        return false;

    // シェーダーコンパイラ
    if (!m_shader.Initialize())
        return false;

    // 定数バッファ
    if (!m_cocCB.Initialize(device, 256, 256))
        return false;
    if (!m_blurCB.Initialize(device, 256, 256))
        return false;
    if (!m_compositeCB.Initialize(device, 256, 256))
        return false;

    // === ルートシグネチャ ===

    // CoC生成: [0]=CBV(b0) + [1]=DescTable(t0 depth) + s0(point clamp)
    {
        RootSignatureBuilder rsb;
        m_cocRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_cocRS) return false;
    }

    // ブラー: [0]=CBV(b0) + [1]=DescTable(t0 scene/blurTemp) + s0(linear clamp)
    {
        RootSignatureBuilder rsb;
        m_blurRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_blurRS) return false;
    }

    // 合成: [0]=CBV(b0) + [1]=DescTable(t0,t1,t2 の3連続SRV) + s0(linear clamp) + s1(point clamp)
    {
        RootSignatureBuilder rsb;
        m_compositeRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_compositeRS) return false;
    }

    if (!CreatePipelines(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/DepthOfField.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("DepthOfField initialized (%dx%d, blur=%dx%d)", width, height, halfW, halfH);
    return true;
}

bool DepthOfField::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/DepthOfField.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;
    auto vsBytecode = vs.GetBytecode();

    // CoC生成 PSO (R16_FLOAT出力)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/DepthOfField.hlsl", L"PSCoC", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_cocPSO = b.SetRootSignature(m_cocRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_cocPSO) return false;
    }

    // 水平ブラー PSO (HDR half-res)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/DepthOfField.hlsl", L"PSBlurH", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_blurHPSO = b.SetRootSignature(m_blurRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_blurHPSO) return false;
    }

    // 垂直ブラー PSO (HDR half-res)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/DepthOfField.hlsl", L"PSBlurV", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_blurVPSO = b.SetRootSignature(m_blurRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_blurVPSO) return false;
    }

    // 合成 PSO (HDR full-res)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/DepthOfField.hlsl", L"PSComposite", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_compositePSO = b.SetRootSignature(m_compositeRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_compositePSO) return false;
    }

    return true;
}

void DepthOfField::UpdateCompositeSRVHeap(RenderTarget& srcHDR, uint32_t frameIndex)
{
    // 3テクスチャ(sharp+blur+CoC)を1つのDescriptorTableでバインドするため、
    // 専用shader-visibleヒープにSRVを直接作成する。
    // D3D12はSetDescriptorHeapsで1つのCBV_SRV_UAVヒープしかバインドできないため、
    // 各RTの個別ヒープからのCopyDescriptorsSimpleは使えない
    // フレームごとに3スロットずつオフセット (double-buffer)

    uint32_t base = frameIndex * 3;

    auto createSRV = [&](uint32_t slot, ID3D12Resource* resource, DXGI_FORMAT format)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = format;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(resource, &srvDesc, m_compositeSRVHeap.GetCPUHandle(slot));
    };

    // [base+0] = sharp (srcHDR) - R16G16B16A16_FLOAT
    createSRV(base + 0, srcHDR.GetResource(), srcHDR.GetFormat());
    // [base+1] = blurred (m_blurRT) - R16G16B16A16_FLOAT
    createSRV(base + 1, m_blurRT.GetResource(), m_blurRT.GetFormat());
    // [base+2] = CoC (m_cocRT) - R16_FLOAT
    createSRV(base + 2, m_cocRT.GetResource(), m_cocRT.GetFormat());
}

void DepthOfField::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                            RenderTarget& srcHDR, RenderTarget& destHDR,
                            DepthBuffer& depth, const Camera3D& camera)
{
    uint32_t halfW = (std::max)(m_width / 2u, 1u);
    uint32_t halfH = (std::max)(m_height / 2u, 1u);

    // フルスクリーンビューポート
    D3D12_VIEWPORT vpFull = {};
    vpFull.Width  = static_cast<float>(m_width);
    vpFull.Height = static_cast<float>(m_height);
    vpFull.MaxDepth = 1.0f;

    D3D12_RECT scFull = {};
    scFull.right  = static_cast<LONG>(m_width);
    scFull.bottom = static_cast<LONG>(m_height);

    // ハーフスクリーンビューポート
    D3D12_VIEWPORT vpHalf = {};
    vpHalf.Width  = static_cast<float>(halfW);
    vpHalf.Height = static_cast<float>(halfH);
    vpHalf.MaxDepth = 1.0f;

    D3D12_RECT scHalf = {};
    scHalf.right  = static_cast<LONG>(halfW);
    scHalf.bottom = static_cast<LONG>(halfH);

    // 射影行列の逆行列
    XMMATRIX proj = camera.GetProjectionMatrix();
    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);

    // ================================================================
    // Pass 1: CoC生成 (depth → CoC map)
    // ================================================================
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_cocRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto cocRTV = m_cocRT.GetRTVHandle();
    const float clearBlack[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmdList->ClearRenderTargetView(cocRTV, clearBlack, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &cocRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vpFull);
    cmdList->RSSetScissorRects(1, &scFull);

    cmdList->SetPipelineState(m_cocPSO.Get());
    cmdList->SetGraphicsRootSignature(m_cocRS.Get());

    ID3D12DescriptorHeap* depthHeaps[] = { depth.GetOwnSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, depthHeaps);

    DoFCoCConstants cocConst = {};
    XMStoreFloat4x4(&cocConst.invProjection, XMMatrixTranspose(invProj));
    cocConst.focalDistance = m_focalDistance;
    cocConst.focalRange    = m_focalRange;
    cocConst.cocScale      = m_bokehRadius;
    cocConst.screenWidth   = static_cast<float>(m_width);
    cocConst.screenHeight  = static_cast<float>(m_height);
    cocConst.nearZ         = camera.GetNearZ();
    cocConst.farZ          = camera.GetFarZ();

    void* p = m_cocCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &cocConst, sizeof(cocConst));
        m_cocCB.Unmap(frameIndex);
    }
    cmdList->SetGraphicsRootConstantBufferView(0, m_cocCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, depth.GetOwnSRVHeap().GetGPUHandle(0));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // ================================================================
    // Pass 2: 水平ブラー (srcHDR → blurTempRT at half-res)
    // ================================================================
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_blurTempRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto blurTempRTV = m_blurTempRT.GetRTVHandle();
    cmdList->ClearRenderTargetView(blurTempRTV, clearBlack, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &blurTempRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vpHalf);
    cmdList->RSSetScissorRects(1, &scHalf);

    cmdList->SetPipelineState(m_blurHPSO.Get());
    cmdList->SetGraphicsRootSignature(m_blurRS.Get());

    ID3D12DescriptorHeap* srcHeaps[] = { srcHDR.GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, srcHeaps);

    DoFBlurConstants blurH = {};
    blurH.texelSizeX = 1.0f / static_cast<float>(m_width);
    blurH.texelSizeY = 1.0f / static_cast<float>(m_height);

    p = m_blurCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &blurH, sizeof(blurH));
        m_blurCB.Unmap(frameIndex);
    }
    cmdList->SetGraphicsRootConstantBufferView(0, m_blurCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, srcHDR.GetSRVGPUHandle());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // ================================================================
    // Pass 3: 垂直ブラー (blurTempRT → blurRT at half-res)
    // ================================================================
    m_blurTempRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_blurRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto blurRTV = m_blurRT.GetRTVHandle();
    cmdList->ClearRenderTargetView(blurRTV, clearBlack, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &blurRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vpHalf);
    cmdList->RSSetScissorRects(1, &scHalf);

    cmdList->SetPipelineState(m_blurVPSO.Get());
    cmdList->SetGraphicsRootSignature(m_blurRS.Get());

    ID3D12DescriptorHeap* blurTempHeaps[] = { m_blurTempRT.GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, blurTempHeaps);

    DoFBlurConstants blurV = {};
    blurV.texelSizeX = 1.0f / static_cast<float>(halfW);
    blurV.texelSizeY = 1.0f / static_cast<float>(halfH);

    p = m_blurCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &blurV, sizeof(blurV));
        m_blurCB.Unmap(frameIndex);
    }
    cmdList->SetGraphicsRootConstantBufferView(0, m_blurCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_blurTempRT.GetSRVGPUHandle());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // ================================================================
    // Pass 4: 合成 (sharp srcHDR + blurred blurRT + CoC → destHDR)
    // 3テクスチャを1つのSRVヒープにコピーしてバインド
    // ================================================================
    m_cocRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_blurRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 合成用SRVヒープを更新 (各RTのSRVを1ヒープにコピー)
    UpdateCompositeSRVHeap(srcHDR, frameIndex);

    auto destRTV = destHDR.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vpFull);
    cmdList->RSSetScissorRects(1, &scFull);

    cmdList->SetPipelineState(m_compositePSO.Get());
    cmdList->SetGraphicsRootSignature(m_compositeRS.Get());

    ID3D12DescriptorHeap* compositeHeaps[] = { m_compositeSRVHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, compositeHeaps);

    // CB
    DoFCompositeConstants compConst = {};
    p = m_compositeCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &compConst, sizeof(compConst));
        m_compositeCB.Unmap(frameIndex);
    }
    cmdList->SetGraphicsRootConstantBufferView(0, m_compositeCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_compositeSRVHeap.GetGPUHandle(frameIndex * 3));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // 後処理: DepthBuffer を DEPTH_WRITE に戻す
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void DepthOfField::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    uint32_t halfW = (std::max)(width / 2u, 1u);
    uint32_t halfH = (std::max)(height / 2u, 1u);

    m_cocRT.Create(device, width, height, DXGI_FORMAT_R16_FLOAT);
    m_blurTempRT.Create(device, halfW, halfH, DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_blurRT.Create(device, halfW, halfH, DXGI_FORMAT_R16G16B16A16_FLOAT);
}

} // namespace GX
