/// @file VolumetricLight.cpp
/// @brief ボリューメトリックライト（ゴッドレイ）の実装
///
/// GPU Gems 3 スクリーン空間ラディアルブラー方式。
/// SSR/MotionBlur/Outlineと同じ 2-SRV 専用ヒープパターンを使用。
#include "pch.h"
#include "Graphics/PostEffect/VolumetricLight.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace GX
{

bool VolumetricLight::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

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

    if (!CreatePipelines(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/VolumetricLight.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("VolumetricLight initialized (%dx%d)", width, height);
    return true;
}

bool VolumetricLight::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/VolumetricLight.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/VolumetricLight.hlsl", L"PSMain", L"ps_6_0");
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

void VolumetricLight::UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex)
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

void VolumetricLight::UpdateSunInfo(const Camera3D& camera)
{
    XMVECTOR sunDir = XMVector3Normalize(XMLoadFloat3(&m_lightDirection));
    XMVECTOR camPos = XMLoadFloat3(&camera.GetPosition());

    // 太陽は光の逆方向の遠方
    XMVECTOR sunWorld = camPos - sunDir * 1000.0f;

    XMMATRIX viewProj = camera.GetViewProjectionMatrix();
    XMVECTOR sunClip = XMVector3TransformCoord(sunWorld, viewProj);

    // NDC→UV変換
    XMFLOAT3 sunNDC = {0.0f, 0.0f, 0.0f};
    XMStoreFloat3(&sunNDC, sunClip);
    float sunU = sunNDC.x * 0.5f + 0.5f;
    float sunV = -sunNDC.y * 0.5f + 0.5f;

    // ビュー空間Zで前方チェック
    XMMATRIX viewMat = camera.GetViewMatrix();
    XMVECTOR sunView = XMVector3TransformCoord(sunWorld, viewMat);
    XMFLOAT3 sunViewF;
    XMStoreFloat3(&sunViewF, sunView);

    float sunVisible = 1.0f;

    // 太陽がカメラの背後にある場合はフェードアウト
    if (sunViewF.z <= 0.0f)
    {
        sunVisible = 0.0f;
    }
    else
    {
        // 画面外の場合は距離に応じてフェードアウト
        float distFromCenter = sqrtf((sunU - 0.5f) * (sunU - 0.5f) + (sunV - 0.5f) * (sunV - 0.5f));
        // 画面内(~0.7)では100%、それ以降フェードアウト、2.0以上で0
        float fadeT = (distFromCenter - 0.7f) / 1.3f;
        fadeT = (std::max)(0.0f, (std::min)(1.0f, fadeT));
        sunVisible = 1.0f - fadeT;
    }

    m_lastSunVisible = sunVisible;
    m_lastSunScreenPos = { sunU, sunV };
}

void VolumetricLight::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                               RenderTarget& srcHDR, RenderTarget& destHDR,
                               DepthBuffer& depth, const Camera3D& camera)
{
    // 太陽位置を更新
    UpdateSunInfo(camera);

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
    VolumetricLightConstants constants = {};
    constants.sunScreenPos = m_lastSunScreenPos;
    constants.density    = m_density;
    constants.decay      = m_decay;
    constants.weight     = m_weight;
    constants.exposure   = m_exposure;
    constants.numSamples = m_numSamples;
    constants.intensity  = m_intensity;
    constants.lightColor = m_lightColor;
    constants.sunVisible = m_lastSunVisible;

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

void VolumetricLight::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
}

} // namespace GX
