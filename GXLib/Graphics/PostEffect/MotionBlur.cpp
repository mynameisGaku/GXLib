/// @file MotionBlur.cpp
/// @brief カメラベース Motion Blur の実装
///
/// 深度再構成方式: 深度バッファからワールド座標を復元し、
/// 前フレームVP行列で再投影して速度ベクトルを計算、HDR空間でブラーを行う。
///
/// 2テクスチャ(scene + depth)を使うため、DoFと同様に専用SRVヒープを使用
#include "pch.h"
#include "Graphics/PostEffect/MotionBlur.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

bool MotionBlur::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // 前フレームVP行列を単位行列で初期化
    XMStoreFloat4x4(&m_previousVP, XMMatrixIdentity());

    // 専用SRVヒープ (2テクスチャ × 2フレーム = 4スロット)
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4, true))
        return false;

    // シェーダーコンパイラ
    if (!m_shader.Initialize())
        return false;

    // 定数バッファ
    if (!m_cb.Initialize(device, 256, 256))
        return false;

    // ルートシグネチャ: [0]=CBV(b0) + [1]=DescTable(t0,t1 の2連続SRV) + s0(linear) + s1(point)
    {
        RootSignatureBuilder rsb;
        m_rootSignature = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_rootSignature) return false;
    }

    // PSO
    auto vs = m_shader.CompileFromFile(L"Shaders/MotionBlur.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/MotionBlur.hlsl", L"PSMotionBlur", L"ps_6_0");
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

    GX_LOG_INFO("MotionBlur initialized (%dx%d)", width, height);
    return true;
}

void MotionBlur::UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex)
{
    uint32_t base = frameIndex * 2;

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
        m_device->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 1));
    }
}

void MotionBlur::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                          RenderTarget& srcHDR, RenderTarget& destHDR,
                          DepthBuffer& depth, const Camera3D& camera)
{
    // 初回フレーム（前フレームVPが無い）はスキップ
    if (!m_hasPreviousVP)
        return;

    // リソース遷移
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // SRVヒープ更新
    UpdateSRVHeap(srcHDR, depth, frameIndex);

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
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // ヒープバインド
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // 定数バッファ更新
    XMMATRIX viewProj = camera.GetViewProjectionMatrix();
    XMMATRIX invVP = XMMatrixInverse(nullptr, viewProj);

    MotionBlurConstants constants = {};
    XMStoreFloat4x4(&constants.invViewProjection, XMMatrixTranspose(invVP));
    XMStoreFloat4x4(&constants.previousViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&m_previousVP)));
    constants.intensity   = m_intensity;
    constants.sampleCount = m_sampleCount;

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &constants, sizeof(constants));
        m_cb.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(frameIndex * 2));

    // フルスクリーンドロー
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // 深度バッファを DEPTH_WRITE に戻す
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void MotionBlur::UpdatePreviousVP(const Camera3D& camera)
{
    XMStoreFloat4x4(&m_previousVP, camera.GetViewProjectionMatrix());
    m_hasPreviousVP = true;
}

void MotionBlur::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
}

} // namespace GX
