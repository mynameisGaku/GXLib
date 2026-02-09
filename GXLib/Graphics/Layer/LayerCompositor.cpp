/// @file LayerCompositor.cpp
/// @brief レイヤーコンポジターの実装
#include "pch.h"
#include "Graphics/Layer/LayerCompositor.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

bool LayerCompositor::Initialize(ID3D12Device* device, uint32_t w, uint32_t h)
{
    m_device = device;
    m_width  = w;
    m_height = h;

    if (!m_shader.Initialize())
        return false;

    if (!m_compositeCB.Initialize(device, 256, 256))
        return false;

    // マスク用SRVヒープ (2テクスチャ x 2フレーム = 4スロット)
    if (!m_maskSrvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4, true))
        return false;

    if (!CreatePipelines(device))
        return false;

    GX_LOG_INFO("LayerCompositor initialized (%dx%d)", w, h);
    return true;
}

bool LayerCompositor::CreatePipelines(ID3D12Device* device)
{
    // --- マスクなしRS: [0]=CBV(b0) + [1]=DescTable(t0) + s0(linear) ---
    {
        RootSignatureBuilder rsb;
        m_rsNoMask = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0)
            .Build(device);
        if (!m_rsNoMask) return false;
    }

    // --- マスクありRS: [0]=CBV(b0) + [1]=DescTable(t0,t1) + s0(linear) ---
    {
        RootSignatureBuilder rsb;
        m_rsMask = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0)
            .Build(device);
        if (!m_rsMask) return false;
    }

    // シェーダーコンパイル
    auto vs = m_shader.CompileFromFile(L"Shaders/LayerComposite.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;
    auto vsBytecode = vs.GetBytecode();

    auto psNoMask = m_shader.CompileFromFile(L"Shaders/LayerComposite.hlsl", L"PSComposite", L"ps_6_0");
    if (!psNoMask.valid) return false;

    auto psMask = m_shader.CompileFromFile(L"Shaders/LayerComposite.hlsl", L"PSCompositeMasked", L"ps_6_0");
    if (!psMask.valid) return false;

    auto psBytecodeNoMask = psNoMask.GetBytecode();
    auto psBytecodeMask   = psMask.GetBytecode();

    // ヘルパー: ブレンドモード別PSO作成 (マスクなし)
    auto buildNoMaskPSO = [&](auto blendSetFn) -> ComPtr<ID3D12PipelineState>
    {
        PipelineStateBuilder b;
        b.SetRootSignature(m_rsNoMask.Get())
         .SetVertexShader(vsBytecode)
         .SetPixelShader(psBytecodeNoMask)
         .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
         .SetDepthEnable(false)
         .SetCullMode(D3D12_CULL_MODE_NONE);
        blendSetFn(b);
        return b.Build(device);
    };

    // ヘルパー: ブレンドモード別PSO作成 (マスクあり)
    auto buildMaskPSO = [&](auto blendSetFn) -> ComPtr<ID3D12PipelineState>
    {
        PipelineStateBuilder b;
        b.SetRootSignature(m_rsMask.Get())
         .SetVertexShader(vsBytecode)
         .SetPixelShader(psBytecodeMask)
         .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
         .SetDepthEnable(false)
         .SetCullMode(D3D12_CULL_MODE_NONE);
        blendSetFn(b);
        return b.Build(device);
    };

    // --- マスクなし PSO ---
    m_psoAlpha = buildNoMaskPSO([](PipelineStateBuilder& b) { b.SetAlphaBlend(); });
    if (!m_psoAlpha) return false;

    m_psoAdd = buildNoMaskPSO([](PipelineStateBuilder& b) { b.SetAdditiveBlend(); });
    if (!m_psoAdd) return false;

    m_psoSub = buildNoMaskPSO([](PipelineStateBuilder& b) { b.SetSubtractiveBlend(); });
    if (!m_psoSub) return false;

    m_psoMul = buildNoMaskPSO([](PipelineStateBuilder& b) { b.SetMultiplyBlend(); });
    if (!m_psoMul) return false;

    // Screen blend: 1 - (1-Src) * (1-Dest) = Src + Dest - Src*Dest
    // Approximation: SrcBlend=ONE, DestBlend=INV_SRC_COLOR
    m_psoScreen = buildNoMaskPSO([](PipelineStateBuilder& b)
    {
        D3D12_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable           = TRUE;
        bd.RenderTarget[0].SrcBlend              = D3D12_BLEND_ONE;
        bd.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_COLOR;
        bd.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
        bd.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        b.SetBlendState(bd);
    });
    if (!m_psoScreen) return false;

    // None (opaque, no blending)
    m_psoNone = buildNoMaskPSO([](PipelineStateBuilder&) { /* default = no blend */ });
    if (!m_psoNone) return false;

    // --- マスクあり PSO (Alpha/Add) ---
    m_psoAlphaMask = buildMaskPSO([](PipelineStateBuilder& b) { b.SetAlphaBlend(); });
    if (!m_psoAlphaMask) return false;

    m_psoAddMask = buildMaskPSO([](PipelineStateBuilder& b) { b.SetAdditiveBlend(); });
    if (!m_psoAddMask) return false;

    return true;
}

void LayerCompositor::Composite(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                 D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV,
                                 LayerStack& layerStack)
{
    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;

    // バックバッファを黒でクリア
    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(backBufferRTV, clearColor, 0, nullptr);

    // RTV + Viewport/Scissor 設定
    cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width    = static_cast<float>(m_width);
    vp.Height   = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    // Z-order 昇順で各レイヤーを合成
    const auto& layers = layerStack.GetSortedLayers();
    for (auto* layer : layers)
    {
        if (!layer->IsVisible() || layer->GetOpacity() <= 0.0f)
            continue;

        // SRV状態にする
        layer->GetRenderTarget().TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        if (layer->HasMask())
        {
            DrawLayerMasked(*layer, *layer->GetMask());
        }
        else
        {
            DrawLayer(*layer);
        }

        // RTVを再設定（ヒープ変更後に必要）
        cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);
    }
}

void LayerCompositor::DrawLayer(RenderLayer& layer)
{
    // PSO選択
    ID3D12PipelineState* pso = nullptr;
    switch (layer.GetBlendMode())
    {
    case LayerBlendMode::Alpha:  pso = m_psoAlpha.Get();  break;
    case LayerBlendMode::Add:    pso = m_psoAdd.Get();    break;
    case LayerBlendMode::Sub:    pso = m_psoSub.Get();    break;
    case LayerBlendMode::Mul:    pso = m_psoMul.Get();    break;
    case LayerBlendMode::Screen: pso = m_psoScreen.Get(); break;
    case LayerBlendMode::None:   pso = m_psoNone.Get();   break;
    }

    m_cmdList->SetPipelineState(pso);
    m_cmdList->SetGraphicsRootSignature(m_rsNoMask.Get());

    // レイヤーRTのSRVヒープをバインド
    ID3D12DescriptorHeap* heaps[] = { layer.GetRenderTarget().GetSRVHeap().GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);

    // 定数バッファ
    CompositeConstants cc = {};
    cc.opacity = layer.GetOpacity();
    cc.hasMask = 0.0f;
    void* p = m_compositeCB.Map(m_frameIndex);
    if (p)
    {
        memcpy(p, &cc, sizeof(cc));
        m_compositeCB.Unmap(m_frameIndex);
    }
    m_cmdList->SetGraphicsRootConstantBufferView(0, m_compositeCB.GetGPUVirtualAddress(m_frameIndex));
    m_cmdList->SetGraphicsRootDescriptorTable(1, layer.GetRenderTarget().GetSRVGPUHandle());

    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawInstanced(3, 1, 0, 0);
}

void LayerCompositor::DrawLayerMasked(RenderLayer& layer, RenderLayer& mask)
{
    // PSO選択 (マスクあり)
    ID3D12PipelineState* pso = nullptr;
    switch (layer.GetBlendMode())
    {
    case LayerBlendMode::Add:    pso = m_psoAddMask.Get();   break;
    default:                     pso = m_psoAlphaMask.Get(); break;
    }

    m_cmdList->SetPipelineState(pso);
    m_cmdList->SetGraphicsRootSignature(m_rsMask.Get());

    // マスクRTもSRV状態にする
    mask.GetRenderTarget().TransitionTo(m_cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // 専用SRVヒープに layer + mask の SRV を書き込み
    uint32_t base = m_frameIndex * 2;

    // [base+0] = layer
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        m_device->CreateShaderResourceView(layer.GetRenderTarget().GetResource(), &srvDesc,
                                            m_maskSrvHeap.GetCPUHandle(base + 0));
    }

    // [base+1] = mask
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = mask.GetRenderTarget().GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        m_device->CreateShaderResourceView(mask.GetRenderTarget().GetResource(), &srvDesc,
                                            m_maskSrvHeap.GetCPUHandle(base + 1));
    }

    // ヒープバインド
    ID3D12DescriptorHeap* heaps[] = { m_maskSrvHeap.GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);

    // 定数バッファ
    CompositeConstants cc = {};
    cc.opacity = layer.GetOpacity();
    cc.hasMask = 1.0f;
    void* p = m_compositeCB.Map(m_frameIndex);
    if (p)
    {
        memcpy(p, &cc, sizeof(cc));
        m_compositeCB.Unmap(m_frameIndex);
    }
    m_cmdList->SetGraphicsRootConstantBufferView(0, m_compositeCB.GetGPUVirtualAddress(m_frameIndex));
    m_cmdList->SetGraphicsRootDescriptorTable(1, m_maskSrvHeap.GetGPUHandle(base));

    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawInstanced(3, 1, 0, 0);
}

void LayerCompositor::OnResize(ID3D12Device* device, uint32_t w, uint32_t h)
{
    m_width  = w;
    m_height = h;
}

} // namespace GX
