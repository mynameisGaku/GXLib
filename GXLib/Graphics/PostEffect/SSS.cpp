/// @file SSS.cpp
/// @brief Screen-Space Subsurface Scattering の実装
///
/// Jimenez et al. "Separable Subsurface Scattering" に基づくセパラブル2パスブラー。
/// RGB別ガウシアンカーネルをCPU側で事前計算し、水平→垂直の2パスで適用する。
/// 深度バッファによるブラー幅変調とSSSマスクによるピクセル選択で、
/// スキン等の半透明マテリアルのみに効果を限定する。
#include "pch_graphics.h"
#include "Graphics/PostEffect/SSS.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

#include <cmath>

namespace gx
{

// ---------------------------------------------------------------------------
// ガウシアンプロファイル — RGB各チャンネルのfalloffに基づく重み計算
// ---------------------------------------------------------------------------
Vector3 SSS::GaussianProfile(float variance, const Vector3& falloff)
{
    // Gaussian: G(x) = (1 / sqrt(2*pi*sigma^2)) * exp(-x^2 / (2*sigma^2))
    // falloffが大きいほどσが大きく、ブラーが広がる
    constexpr float k_InvSqrt2Pi = 0.3989422804f; // 1 / sqrt(2*pi)
    Vector3 result;
    float sigmaR = (std::max)(falloff.x, 0.001f);
    float sigmaG = (std::max)(falloff.y, 0.001f);
    float sigmaB = (std::max)(falloff.z, 0.001f);
    result.x = k_InvSqrt2Pi / sigmaR * std::exp(-variance / (2.0f * sigmaR * sigmaR));
    result.y = k_InvSqrt2Pi / sigmaG * std::exp(-variance / (2.0f * sigmaG * sigmaG));
    result.z = k_InvSqrt2Pi / sigmaB * std::exp(-variance / (2.0f * sigmaB * sigmaB));
    return result;
}

// ---------------------------------------------------------------------------
// カーネル計算 — RGB別ガウシアン重みの離散サンプリング
// ---------------------------------------------------------------------------
void SSS::ComputeKernel()
{
    int nSamples = (std::max)(1, (std::min)(m_settings.numSamples, k_MaxSamples));
    m_kernelSize = nSamples;

    // Range: オフセットは [-1, 1] の正規化範囲
    // nSamples点を等間隔に配置
    float range = 2.0f;
    float step  = range / static_cast<float>(nSamples);

    // falloff と strength3 から実効的なσを計算
    Vector3 sigma;
    sigma.x = m_settings.falloff.x * m_settings.strength3.x;
    sigma.y = m_settings.falloff.y * m_settings.strength3.y;
    sigma.z = m_settings.falloff.z * m_settings.strength3.z;

    // 各σの下限
    sigma.x = (std::max)(sigma.x, 0.001f);
    sigma.y = (std::max)(sigma.y, 0.001f);
    sigma.z = (std::max)(sigma.z, 0.001f);

    // 重み合計（正規化用）
    Vector3 totalWeight = { 0.0f, 0.0f, 0.0f };

    for (int i = 0; i < nSamples; ++i)
    {
        // オフセット: [-1, 1] 範囲で等間隔配置
        float offset = -1.0f + (static_cast<float>(i) + 0.5f) * step;
        float dist2  = offset * offset;

        Vector3 w = GaussianProfile(dist2, sigma);
        m_kernel[i].offset = offset;
        m_kernel[i].weight = w;

        totalWeight.x += w.x;
        totalWeight.y += w.y;
        totalWeight.z += w.z;
    }

    // 正規化: 全重みの合計が (1,1,1) になるようにスケーリング
    for (int i = 0; i < nSamples; ++i)
    {
        if (totalWeight.x > 0.0f) m_kernel[i].weight.x /= totalWeight.x;
        if (totalWeight.y > 0.0f) m_kernel[i].weight.y /= totalWeight.y;
        if (totalWeight.z > 0.0f) m_kernel[i].weight.z /= totalWeight.z;
    }
}

// ---------------------------------------------------------------------------
// 初期化
// ---------------------------------------------------------------------------
bool SSS::Initialize(GraphicsDevice* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    // カーネル事前計算（GPU不要）
    ComputeKernel();

    if (!device || !device->GetDevice())
    {
        GX_LOG_INFO("SSS: CPU-only mode (no GPU device)");
        return true;
    }

    m_device = device->GetDevice();

    // 中間RT（水平パス出力）
    if (!m_tempRT.Create(m_device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT))
        return false;

    // SRVヒープ: 3テクスチャ × 2パス × 2フレーム = 12スロット
    if (!m_srvHeap.Initialize(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 12, true))
        return false;

    // シェーダーコンパイラ
    if (!m_shader.Initialize())
        return false;

    // 定数バッファ: 各パスのCBは432B → 256B align → 512B、2パス分 = 1024B per frame
    // DynamicBufferが内部でダブルバッファリングするので、maxSize = 1024
    if (!m_cb.Initialize(m_device, 1024, 256))
        return false;

    // ルートシグネチャ: [0]=CBV(b0) + [1]=DescTable(t0,t1,t2 の3連続SRV) + s0(linear)
    {
        RootSignatureBuilder rsb;
        m_rootSignature = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(m_device);
        if (!m_rootSignature) return false;
    }

    if (!CreatePipelines(m_device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/SSS.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("SSS initialized (%dx%d, %d samples)", width, height, m_kernelSize);
    return true;
}

// ---------------------------------------------------------------------------
// パイプライン作成
// ---------------------------------------------------------------------------
bool SSS::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/SSS.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/SSS.hlsl", L"PSMain", L"ps_6_0");
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
// SRVヒープ更新
// ---------------------------------------------------------------------------
void SSS::UpdateSRVHeap(RenderTarget& scene, DepthBuffer& depth,
                        RenderTarget& stencilRT, uint32_t base)
{
    // [base+0] = scene / tempRT (HDR)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = scene.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(scene.GetResource(), &srvDesc,
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

    // [base+2] = SSS stencil mask (R channel)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        if (stencilRT.GetResource())
        {
            srvDesc.Format = stencilRT.GetFormat();
            m_device->CreateShaderResourceView(stencilRT.GetResource(), &srvDesc,
                                                m_srvHeap.GetCPUHandle(base + 2));
        }
        else
        {
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            m_device->CreateShaderResourceView(nullptr, &srvDesc,
                                                m_srvHeap.GetCPUHandle(base + 2));
        }
    }
}

// ---------------------------------------------------------------------------
// 実行: 水平パス → 垂直パス
// ---------------------------------------------------------------------------
void SSS::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                  RenderTarget& srcHDR, RenderTarget& destHDR,
                  DepthBuffer& depth, RenderTarget& sssStencilRT)
{
    if (!m_pso || !m_rootSignature)
        return;

    // --- 定数バッファ構造体 (256B aligned) ---
    // float2 direction, float sssWidth, float strength, float screenW, float screenH,
    // float farOverNear, int numSamples, float4 kernel[25]
    struct SSSCB
    {
        float directionX;
        float directionY;
        float sssWidth;
        float strengthVal;
        float screenWidth;
        float screenHeight;
        float farOverNear;
        int   numSamples;
        // kernel: float4 * 25 = 400B → total = 32 + 400 = 432B
        // 256B-aligned padded to 512B but we only write 432B into 256*2 space
        struct { float x; float y; float z; float w; } kernelData[k_MaxSamples];
    };
    static_assert(sizeof(SSSCB) <= 512, "SSSCB exceeds 512 bytes");

    // ヒープバインド
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);

    // カーネルデータをCBに書き込む共通ヘルパー
    auto fillCB = [&](SSSCB& cb, float dirX, float dirY)
    {
        cb.directionX   = dirX;
        cb.directionY   = dirY;
        cb.sssWidth     = m_settings.width;
        cb.strengthVal  = m_settings.strength;
        cb.screenWidth  = static_cast<float>(m_width);
        cb.screenHeight = static_cast<float>(m_height);
        cb.farOverNear  = 1000.0f; // far/near ratio (typical)
        cb.numSamples   = m_kernelSize;
        for (int i = 0; i < k_MaxSamples; ++i)
        {
            if (i < m_kernelSize)
            {
                cb.kernelData[i].x = m_kernel[i].offset;
                cb.kernelData[i].y = m_kernel[i].weight.x;
                cb.kernelData[i].z = m_kernel[i].weight.y;
                cb.kernelData[i].w = m_kernel[i].weight.z;
            }
            else
            {
                cb.kernelData[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
            }
        }
    };

    // 両パスのCBデータを一括書き込み (frameIndex面に2パス分を書く)
    // パス0: offset 0, パス1: offset 512
    constexpr uint32_t k_CBPassStride = 512; // 256B-aligned (432B→512B)
    {
        SSSCB hcb = {};
        fillCB(hcb, 1.0f, 0.0f);
        SSSCB vcb = {};
        fillCB(vcb, 0.0f, 1.0f);

        void* p = m_cb.Map(frameIndex);
        if (p)
        {
            memcpy(p, &hcb, sizeof(hcb));
            memcpy(static_cast<uint8_t*>(p) + k_CBPassStride, &vcb, sizeof(vcb));
            m_cb.Unmap(frameIndex);
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS cbBase = m_cb.GetGPUVirtualAddress(frameIndex);

    // =======================================================================
    // Pass 1: Horizontal — srcHDR → m_tempRT, direction=(1,0)
    // =======================================================================
    {
        srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        sssStencilRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_tempRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // SRV更新
        uint32_t srvBase = frameIndex * 6; // pass0: base, pass1: base+3
        UpdateSRVHeap(srcHDR, depth, sssStencilRT, srvBase);

        auto rtv = m_tempRT.GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &sc);

        cmdList->SetPipelineState(m_pso.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootConstantBufferView(0, cbBase);
        cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(srvBase));

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // =======================================================================
    // Pass 2: Vertical — m_tempRT → destHDR, direction=(0,1)
    // =======================================================================
    {
        m_tempRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // SRV更新（入力がm_tempRTに変わる）
        uint32_t srvBase = frameIndex * 6 + 3;
        UpdateSRVHeap(m_tempRT, depth, sssStencilRT, srvBase);

        auto rtv = destHDR.GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &sc);

        cmdList->SetPipelineState(m_pso.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootConstantBufferView(0, cbBase + k_CBPassStride);
        cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(srvBase));

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // 深度バッファを DEPTH_WRITE に戻す
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

// ---------------------------------------------------------------------------
// リサイズ
// ---------------------------------------------------------------------------
void SSS::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    if (device)
        m_tempRT.Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
}

} // namespace gx
