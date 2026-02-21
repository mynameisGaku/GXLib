/// @file TAA.cpp
/// @brief Temporal Anti-Aliasing の実装
///
/// MotionBlur.cppパターンを踏襲。3テクスチャ(scene + history + depth)を
/// 専用SRVヒープで管理し、Halton(2,3)ジッターでサブピクセル蓄積を行う。
#include "pch.h"
#include "Graphics/PostEffect/TAA.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace GX
{

/// Halton準乱数列: 低不一致性を持つ準乱数で、ランダムよりも均一にピクセル内を
/// カバーする。基数2と3の組み合わせで2D準乱数を生成 (TAAの標準手法)
float TAA::Halton(int index, int base)
{
    float f = 1.0f;
    float r = 0.0f;
    int i = index;
    while (i > 0)
    {
        f /= static_cast<float>(base);
        r += f * static_cast<float>(i % base);
        i /= base;
    }
    return r;
}

XMFLOAT2 TAA::GetCurrentJitter() const
{
    if (!m_enabled || m_width == 0 || m_height == 0)
        return { 0.0f, 0.0f };

    // 8サンプルサイクル (index 1-8, 0はHaltonで0を返すため避ける)
    int idx = (static_cast<int>(m_frameCount) % 8) + 1;
    float jx = Halton(idx, 2) - 0.5f; // [-0.5, 0.5] ピクセル
    float jy = Halton(idx, 3) - 0.5f;

    // ピクセルオフセット → NDCオフセット
    float ndcX = jx * 2.0f / static_cast<float>(m_width);
    float ndcY = jy * 2.0f / static_cast<float>(m_height);

    return { ndcX, ndcY };
}

bool TAA::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    XMStoreFloat4x4(&m_previousVP, XMMatrixIdentity());

    // 履歴RT
    if (!m_historyRT.Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT))
        return false;

    // 専用SRVヒープ (3テクスチャ × 2フレーム = 6スロット)
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 6, true))
        return false;

    if (!m_shader.Initialize())
        return false;

    if (!m_cb.Initialize(device, 256, 256))
        return false;

    // RS: [0]=CBV(b0) + [1]=DescTable(t0,t1,t2 の3連続SRV) + s0(linear) + s1(point)
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
            .Build(device);
        if (!m_rootSignature) return false;
    }

    if (!CreatePipelines(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/TAA.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("TAA initialized (%dx%d)", width, height);
    return true;
}

bool TAA::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/TAA.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/TAA.hlsl", L"PSTAA", L"ps_6_0");
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

void TAA::UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex)
{
    uint32_t base = frameIndex * 3;

    // [base+0] = 現フレームシーン (HDR)
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

    // [base+1] = 履歴 (前フレームの結果)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = m_historyRT.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(m_historyRT.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 1));
    }

    // [base+2] = 深度
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 2));
    }
}

void TAA::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                   RenderTarget& srcHDR, RenderTarget& destHDR,
                   DepthBuffer& depth, const Camera3D& camera)
{
    // 初回フレーム: 履歴RTが空なのでTAAブレンドはできない。
    // srcHDRをそのまま出力とし、履歴RTにもコピーして次フレーム以降に備える
    if (!m_hasHistory || !m_hasPreviousVP)
    {
        srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->CopyResource(destHDR.GetResource(), srcHDR.GetResource());

        destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_historyRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->CopyResource(m_historyRT.GetResource(), destHDR.GetResource());

        m_historyRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_hasHistory = true;
        return;
    }

    // リソース遷移
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_historyRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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

    // 定数バッファ (非ジッターのVP行列を使用)
    XMMATRIX viewProj = camera.GetViewProjectionMatrix(); // 非ジッター
    XMMATRIX invVP = XMMatrixInverse(nullptr, viewProj);

    TAAConstants constants = {};
    XMStoreFloat4x4(&constants.invViewProjection, XMMatrixTranspose(invVP));
    XMStoreFloat4x4(&constants.previousViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&m_previousVP)));
    constants.jitterOffset = GetCurrentJitter();
    constants.blendFactor  = m_blendFactor;
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

    // フルスクリーンドロー
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // destHDR → historyRT にコピー
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_historyRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyResource(m_historyRT.GetResource(), destHDR.GetResource());
    m_historyRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // 深度バッファを DEPTH_WRITE に戻す
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void TAA::UpdatePreviousVP(const Camera3D& camera)
{
    // 非ジッターのVP行列を保存
    XMStoreFloat4x4(&m_previousVP, camera.GetViewProjectionMatrix());
    m_hasPreviousVP = true;
}

void TAA::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
    m_historyRT.Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_hasHistory = false;
}

} // namespace GX
