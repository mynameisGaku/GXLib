/// @file SSAO.cpp
/// @brief Screen Space Ambient Occlusion の実装
#include "pch.h"
#include "Graphics/PostEffect/SSAO.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"
#include <random>

namespace GX
{

void SSAO::GenerateKernel()
{
    std::mt19937 rng(42); // 固定シード
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> distNeg(-1.0f, 1.0f);

    for (uint32_t i = 0; i < k_KernelSize; ++i)
    {
        // 半球上のランダム方向
        XMFLOAT4 sample;
        sample.x = distNeg(rng);
        sample.y = distNeg(rng);
        sample.z = dist01(rng); // 半球（法線方向のみ）
        sample.w = 0.0f;

        // 正規化
        XMVECTOR v = XMLoadFloat4(&sample);
        v = XMVector3Normalize(v);

        // ランダムな長さ（中心に近いほど密に）
        float scale = static_cast<float>(i) / static_cast<float>(k_KernelSize);
        scale = 0.1f + scale * scale * 0.9f; // lerp(0.1, 1.0, scale^2)
        v = XMVectorScale(v, scale);

        XMStoreFloat4(&m_kernel[i], v);
    }
}

bool SSAO::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    GenerateKernel();

    // AO出力RT (R8_UNORM)
    if (!m_ssaoRT.Create(device, width, height, DXGI_FORMAT_R8_UNORM))
        return false;
    if (!m_blurTempRT.Create(device, width, height, DXGI_FORMAT_R8_UNORM))
        return false;

    // シェーダーコンパイラ
    if (!m_shader.Initialize())
        return false;

    // 定数バッファ（SSAOConstants = 1184B → 256-align = 1280B）
    if (!m_generateCB.Initialize(device, 1280, 1280))
        return false;
    if (!m_blurCB.Initialize(device, 256, 256))
        return false;

    // --- ルートシグネチャ: AO生成用 (b0 + t0 + s0(point clamp)) ---
    {
        RootSignatureBuilder rsb;
        m_generateRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_generateRS) return false;
    }

    // --- ルートシグネチャ: ブラー＋合成用 (b0 + t0 + s0(point clamp)) ---
    {
        RootSignatureBuilder rsb;
        m_blurRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_blurRS) return false;
    }

    if (!CreatePipelines(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/SSAO.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("SSAO initialized (%dx%d, %d samples)", width, height, k_KernelSize);
    return true;
}

bool SSAO::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/SSAO.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;
    auto vsBytecode = vs.GetBytecode();

    // AO生成 PSO (R8_UNORM出力)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/SSAO.hlsl", L"PSGenerate", L"ps_6_0");
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

    // 水平ブラー PSO
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/SSAO.hlsl", L"PSBlurH", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_blurHPSO = b.SetRootSignature(m_blurRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R8_UNORM)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_blurHPSO) return false;
    }

    // 垂直ブラー PSO
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/SSAO.hlsl", L"PSBlurV", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_blurVPSO = b.SetRootSignature(m_blurRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R8_UNORM)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_blurVPSO) return false;
    }

    // 合成 PSO (HDR R16G16B16A16_FLOAT + MultiplyBlend)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/SSAO.hlsl", L"PSComposite", L"ps_6_0");
        if (!ps.valid) return false;
        PipelineStateBuilder b;
        m_compositePSO = b.SetRootSignature(m_blurRS.Get())
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

void SSAO::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                    RenderTarget& hdrRT, DepthBuffer& depthBuffer, const Camera3D& camera)
{
    // ビューポートとシザー
    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);

    // 射影行列と逆射影行列
    XMMATRIX proj = camera.GetProjectionMatrix();
    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);

    // SSAOConstants を構築
    SSAOConstants ssaoConst = {};
    XMStoreFloat4x4(&ssaoConst.projection, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&ssaoConst.invProjection, XMMatrixTranspose(invProj));
    memcpy(ssaoConst.samples, m_kernel, sizeof(m_kernel));
    ssaoConst.radius       = m_radius;
    ssaoConst.bias         = m_bias;
    ssaoConst.power        = m_power;
    ssaoConst.screenWidth  = static_cast<float>(m_width);
    ssaoConst.screenHeight = static_cast<float>(m_height);
    ssaoConst.nearZ        = camera.GetNearZ();
    ssaoConst.farZ         = camera.GetFarZ();
    ssaoConst.padding      = 0.0f;

    // ================================================================
    // Pass 1: AO生成
    // ================================================================
    depthBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_ssaoRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto ssaoRTV = m_ssaoRT.GetRTVHandle();
    const float clearWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    cmdList->ClearRenderTargetView(ssaoRTV, clearWhite, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &ssaoRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_generatePSO.Get());
    cmdList->SetGraphicsRootSignature(m_generateRS.Get());

    ID3D12DescriptorHeap* depthHeaps[] = { depthBuffer.GetOwnSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, depthHeaps);

    // CB更新
    void* p = m_generateCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &ssaoConst, sizeof(ssaoConst));
        m_generateCB.Unmap(frameIndex);
    }
    cmdList->SetGraphicsRootConstantBufferView(0, m_generateCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, depthBuffer.GetOwnSRVHeap().GetGPUHandle(0));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // ================================================================
    // Pass 2: 水平ブラー (ssaoRT → blurTempRT)
    // ================================================================
    m_ssaoRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_blurTempRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto blurTempRTV = m_blurTempRT.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &blurTempRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_blurHPSO.Get());
    cmdList->SetGraphicsRootSignature(m_blurRS.Get());

    ID3D12DescriptorHeap* ssaoHeaps[] = { m_ssaoRT.GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, ssaoHeaps);

    SSAOBlurConstants blurH = {};
    blurH.blurDirX = 1.0f / static_cast<float>(m_width);
    blurH.blurDirY = 0.0f;

    p = m_blurCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &blurH, sizeof(blurH));
        m_blurCB.Unmap(frameIndex);
    }
    cmdList->SetGraphicsRootConstantBufferView(0, m_blurCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_ssaoRT.GetSRVGPUHandle());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // ================================================================
    // Pass 3: 垂直ブラー (blurTempRT → ssaoRT)
    // ================================================================
    m_blurTempRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_ssaoRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    ssaoRTV = m_ssaoRT.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &ssaoRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_blurVPSO.Get());
    cmdList->SetGraphicsRootSignature(m_blurRS.Get());

    ID3D12DescriptorHeap* blurTempHeaps[] = { m_blurTempRT.GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, blurTempHeaps);

    SSAOBlurConstants blurV = {};
    blurV.blurDirX = 0.0f;
    blurV.blurDirY = 1.0f / static_cast<float>(m_height);

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
    // Pass 4: 乗算合成 (ssaoRT → hdrRT)
    // ================================================================
    m_ssaoRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    hdrRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto hdrRTV = hdrRT.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &hdrRTV, FALSE, nullptr);
    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_compositePSO.Get());
    cmdList->SetGraphicsRootSignature(m_blurRS.Get());

    ID3D12DescriptorHeap* ssaoFinalHeaps[] = { m_ssaoRT.GetSRVHeap().GetHeap() };
    cmdList->SetDescriptorHeaps(1, ssaoFinalHeaps);

    // 合成パスではCBは不使用だが、ルートシグネチャにはb0があるので
    // ブラーCBをダミーとして設定
    cmdList->SetGraphicsRootConstantBufferView(0, m_blurCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_ssaoRT.GetSRVGPUHandle());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // ================================================================
    // 後処理: DepthBuffer を DEPTH_WRITE に戻す
    // ================================================================
    depthBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void SSAO::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
    m_ssaoRT.Create(device, width, height, DXGI_FORMAT_R8_UNORM);
    m_blurTempRT.Create(device, width, height, DXGI_FORMAT_R8_UNORM);
}

} // namespace GX
