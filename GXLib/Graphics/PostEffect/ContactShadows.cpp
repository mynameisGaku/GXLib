/// @file ContactShadows.cpp
/// @brief Screen Space Contact Shadows の実装

#include "pch_graphics.h"
#include "Graphics/PostEffect/ContactShadows.h"
#include "Math/MathConvert.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace gx
{

bool ContactShadows::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    // シャドウマスクRT (R8_UNORM)
    if (!m_shadowRT.Create(device, width, height, DXGI_FORMAT_R8_UNORM))
    {
        GX_LOG_ERROR("ContactShadows: Failed to create shadow RT");
        return false;
    }

    // シェーダーコンパイラ
    if (!m_shader.Initialize())
        return false;

    // 定数バッファ (256B アライメント)
    if (!m_generateCB.Initialize(device, 256, 256))
        return false;

    // 生成パス用ルートシグネチャ: CBV(b0) + DescTable(SRV t0, pixel) + static sampler
    {
        RootSignatureBuilder rsb;
        m_generateRS = rsb
            .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_generateRS) return false;
    }

    // 合成パス用ルートシグネチャ: CBV(b0) + DescTable(SRV t0, pixel) + static sampler
    {
        RootSignatureBuilder rsb;
        m_compositeRS = rsb
            .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_compositeRS) return false;
    }

    if (!CreatePipelines(device))
        return false;

    // ホットリロード登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/ContactShadows.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("ContactShadows initialized (%dx%d)", width, height);
    return true;
}

bool ContactShadows::CreatePipelines(ID3D12Device* device)
{
    // 共通フルスクリーンVS
    auto vs = m_shader.CompileFromFile(L"Shaders/ContactShadows.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;
    auto vsBytecode = vs.GetBytecode();

    // 生成PSO (R8_UNORM出力)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/ContactShadows.hlsl", L"PSGenerate", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_generatePSO = b.SetRootSignature(m_generateRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R8_UNORM)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_generatePSO) return false;
    }

    // 合成PSO (HDR R16G16B16A16_FLOAT + MultiplyBlend)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/ContactShadows.hlsl", L"PSComposite", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_compositePSO = b.SetRootSignature(m_compositeRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .SetMultiplyBlend()
            .Build(device);
        if (!m_compositePSO) return false;
    }

    return true;
}

void ContactShadows::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                              RenderTarget& hdrRT, DepthBuffer& depthBuffer,
                              const Camera3D& camera, const Vector3& lightDirWorld)
{
    // ビューポートとシザー
    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);

    // 行列
    XMMATRIX view = ToXMMATRIX(camera.GetViewMatrix());
    XMMATRIX proj = ToXMMATRIX(camera.GetProjectionMatrix());
    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);

    // ライト方向をビュー空間に変換
    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(XM(&lightDirWorld)));
    XMVECTOR lightDirView = XMVector3TransformNormal(lightDir, view);
    lightDirView = XMVector3Normalize(lightDirView);

    // 定数バッファ構築
    ContactShadowConstants consts = {};
    XMStoreFloat4x4(XM(&consts.view), XMMatrixTranspose(view));
    XMStoreFloat4x4(XM(&consts.projection), XMMatrixTranspose(proj));
    XMStoreFloat4x4(XM(&consts.invProjection), XMMatrixTranspose(invProj));
    XMStoreFloat3(XM(&consts.lightDirView), lightDirView);
    consts.maxDistance  = m_maxDistance;
    consts.screenWidth  = static_cast<float>(m_width);
    consts.screenHeight = static_cast<float>(m_height);
    consts.stepCount    = m_stepCount;
    consts.thickness    = m_thickness;
    consts.intensity    = m_intensity;
    consts.nearZ        = camera.GetNearZ();
    consts.farZ         = camera.GetFarZ();
    consts.padding      = 0.0f;

    // ================================================================
    // Pass 1: コンタクトシャドウ生成
    // ================================================================
    depthBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_shadowRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto shadowRTV = m_shadowRT.GetRTVHandle();
    const float clearWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    cmdList->ClearRenderTargetView(shadowRTV, clearWhite, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &shadowRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_generatePSO.Get());
    cmdList->SetGraphicsRootSignature(m_generateRS.Get());

    ID3D12DescriptorHeap* depthHeaps[] = { depthBuffer.GetOwnSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, depthHeaps);

    void* p = m_generateCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &consts, sizeof(consts));
        m_generateCB.Unmap(frameIndex);
    }
    cmdList->SetGraphicsRootConstantBufferView(0, m_generateCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, depthBuffer.GetOwnSRVHeap().GetGPUHandle(0));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // ================================================================
    // Pass 2: 乗算合成 (shadowRT → hdrRT)
    // ================================================================
    m_shadowRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    hdrRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto hdrRTV = hdrRT.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &hdrRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_compositePSO.Get());
    cmdList->SetGraphicsRootSignature(m_compositeRS.Get());

    ID3D12DescriptorHeap* shadowHeaps[] = { m_shadowRT.GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, shadowHeaps);

    // ダミーCBV設定（RSにb0があるため）
    cmdList->SetGraphicsRootConstantBufferView(0, m_generateCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_shadowRT.GetSRVGPUHandle());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

void ContactShadows::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
    m_shadowRT.Create(device, width, height, DXGI_FORMAT_R8_UNORM);
}

} // namespace gx
