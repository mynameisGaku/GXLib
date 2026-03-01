/// @file SkyAtmosphere.cpp
/// @brief 大気空レンダリングの実装
///
/// 単散乱Rayleigh/Mieモデルを使用するフルスクリーンピクセルシェーダーパス。
/// VolumetricCloudsと同じ専用SRVヒープパターンに従う。
#include "pch_graphics.h"
#include "Graphics/3D/SkyAtmosphere.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace gx
{

bool SkyAtmosphere::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // 専用SRVヒープ: 深度テクスチャ1枚 x 2フレーム = 2スロット
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true))
        return false;

    if (!m_shader.Initialize())
        return false;

    // CBサイズ: 160バイト、CBアライメントのため256にパディング
    if (!m_cb.Initialize(device, 256, 256))
        return false;

    // ルートシグネチャ: [0] CBV(b0), [1] ディスクリプタテーブル(t0 深度), s0(リニアクランプ)
    {
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
    }

    if (!CreatePipelines(device))
        return false;

    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/SkyAtmosphere.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("SkyAtmosphere initialized (%dx%d)", width, height);
    return true;
}

bool SkyAtmosphere::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/SkyAtmosphere.hlsl", L"VS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/SkyAtmosphere.hlsl", L"PS", L"ps_6_0");
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

void SkyAtmosphere::UpdateSRVHeap(DepthBuffer& depth, uint32_t frameIndex)
{
    uint32_t slot = frameIndex;

    // [slot] = R32_FLOAT SRVとしての深度テクスチャ
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    m_device->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                        m_srvHeap.GetCPUHandle(slot));
}

void SkyAtmosphere::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                              RenderTarget& destHDR, DepthBuffer& depth,
                              const Camera3D& camera)
{
    if (!m_enabled) return;

    // リソース状態遷移
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    UpdateSRVHeap(depth, frameIndex);

    auto destRTV = destHDR.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width    = static_cast<float>(m_width);
    vp.Height   = static_cast<float>(m_height);
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

    // 定数バッファを構築
    XMMATRIX vpMat = camera.GetViewProjectionMatrix();
    XMMATRIX invVP = XMMatrixInverse(nullptr, vpMat);

    // 太陽方向を正規化（太陽に向かう方向）
    XMVECTOR sunDir = XMVector3Normalize(XMLoadFloat3(&m_sunDirection));
    XMFLOAT3 sunDirF;
    XMStoreFloat3(&sunDirF, sunDir);

    SkyAtmosphereConstants cb = {};
    XMStoreFloat4x4(&cb.invViewProjection, XMMatrixTranspose(invVP));
    cb.cameraPosition    = camera.GetPosition();
    cb.planetRadius      = m_planetRadius;
    cb.sunDirection      = sunDirF;
    cb.atmosphereRadius  = m_atmosphereRadius;
    cb.sunColor          = m_sunColor;
    cb.sunIntensity      = m_sunIntensity;
    cb.rayleighCoeff     = m_rayleighCoeff;
    cb.rayleighScaleHeight = m_rayleighScaleHeight;
    cb.mieCoeff          = m_mieCoeff;
    cb.mieScaleHeight    = m_mieScaleHeight;
    cb.mieG              = m_mieG;
    cb.numSteps          = m_numSteps;
    cb.screenDimensions  = { static_cast<float>(m_width), static_cast<float>(m_height) };

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &cb, sizeof(cb));
        m_cb.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(frameIndex));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void SkyAtmosphere::OnResize(ID3D12Device* /*device*/, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
}

XMFLOAT3 SkyAtmosphere::ComputeSunColor(float sunElevation)
{
    // Perez方式モデル: 天頂では太陽は白色、地平線では大気経路長の増加により赤みを帯びる
    // sunElevation: 0 = 地平線、PI/2 = 天頂
    constexpr float k_Pi = 3.14159265f;

    // 仰角を[0, PI/2]にクランプ
    float elev = (std::max)(0.0f, (std::min)(sunElevation, k_Pi * 0.5f));

    // 光学的深さ係数: 地平線での大気経路長は天頂の約38倍
    //（エアマスの簡略近似）。cos(天頂角) = sin(仰角)
    float sinElev = sinf(elev);
    float airmass = 1.0f / (std::max)(sinElev, 0.01f);

    // 海面高度でのRayleigh消散係数（波長依存）
    // どの波長が散乱されるかを決定する。
    // 短波長（青）はより多く散乱され、夕焼けでは赤/橙色が残る。
    float tauR = 5.5e-6f;  // 赤チャネル   (680nm)
    float tauG = 13.0e-6f; // 緑チャネル (550nm)
    float tauB = 22.4e-6f; // 青チャネル  (440nm)

    // スケールハイト経路（簡略化: スケールハイト8km、観測者は海面高度と仮定）
    float pathScale = 8000.0f * airmass;

    // 各チャネルのBeer-Lambert減衰
    float r = expf(-tauR * pathScale);
    float g = expf(-tauG * pathScale);
    float b = expf(-tauB * pathScale);

    // 天頂で色が約(1, 1, 1)になるよう正規化
    // 天頂時: エアマス ≈ 1、pathScale = 8000
    float rZenith = expf(-tauR * 8000.0f);
    float gZenith = expf(-tauG * 8000.0f);
    float bZenith = expf(-tauB * 8000.0f);

    float rNorm = (rZenith > 0.0f) ? r / rZenith : r;
    float gNorm = (gZenith > 0.0f) ? g / gZenith : g;
    float bNorm = (bZenith > 0.0f) ? b / bZenith : b;

    // [0, 1]にクランプ
    rNorm = (std::max)(0.0f, (std::min)(rNorm, 1.0f));
    gNorm = (std::max)(0.0f, (std::min)(gNorm, 1.0f));
    bNorm = (std::max)(0.0f, (std::min)(bNorm, 1.0f));

    return { rNorm, gNorm, bNorm };
}

} // namespace gx
