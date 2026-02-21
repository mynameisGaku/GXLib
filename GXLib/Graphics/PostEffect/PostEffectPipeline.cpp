/// @file PostEffectPipeline.cpp
/// @brief ポストエフェクトパイプラインの実装
///
/// エフェクトチェーン:
/// HDR Scene → [Bloom] → [ColorGrading(HDR)] → [Tonemapping(HDR→LDR)]
///           → [FXAA(LDR)] → [Vignette+ChromAb(LDR)] → Backbuffer
#include "pch.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/PostEffect/PostEffectSettings.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Graphics/Device/BarrierBatch.h"
#include "Graphics/RayTracing/RTReflections.h"
#include "Graphics/RayTracing/RTGI.h"
#include "Core/Logger.h"

namespace GX
{

bool PostEffectPipeline::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // HDR用RT (高ダイナミックレンジ)
    if (!m_hdrRT.Create(device, width, height, k_HDRFormat)) return false;
    if (!m_hdrPingPongRT.Create(device, width, height, k_HDRFormat)) return false;

    // GBuffer法線RT (DXR反射用)
    if (!m_normalRT.Create(device, width, height, k_HDRFormat)) return false;

    // GBufferアルベドRT (GI用)
    if (!m_albedoRT.Create(device, width, height, k_LDRFormat)) return false;

    // LDR用RT (低ダイナミックレンジ)
    if (!m_ldrRT[0].Create(device, width, height, k_LDRFormat)) return false;
    if (!m_ldrRT[1].Create(device, width, height, k_LDRFormat)) return false;

    // シェーダーコンパイラ
    if (!m_shader.Initialize()) return false;

    // 定数バッファ
    if (!m_tonemapCB.Initialize(device, 256, 256)) return false;
    if (!m_fxaaCB.Initialize(device, 256, 256)) return false;
    if (!m_vignetteCB.Initialize(device, 256, 256)) return false;
    if (!m_colorGradingCB.Initialize(device, 256, 256)) return false;

    // パイプライン
    if (!CreatePipelines(device)) return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/Tonemapping.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/FXAA.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/Vignette.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/ColorGrading.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    // SSAO
    if (!m_ssao.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize SSAO");
        return false;
    }

    // ブルーム
    if (!m_bloom.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize Bloom");
        return false;
    }

    // DoF (被写界深度)
    if (!m_dof.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize DoF");
        return false;
    }

    // モーションブラー
    if (!m_motionBlur.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize MotionBlur");
        return false;
    }

    // SSR (スクリーン空間反射)
    if (!m_ssr.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize SSR");
        return false;
    }

    // アウトライン
    if (!m_outline.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize OutlineEffect");
        return false;
    }

    // ボリューメトリックライト
    if (!m_volumetricLight.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize VolumetricLight");
        return false;
    }

    // TAA (時間的AA)
    if (!m_taa.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize TAA");
        return false;
    }

    // 自動露出
    if (!m_autoExposure.Initialize(device, width, height))
    {
        GX_LOG_ERROR("PostEffectPipeline: Failed to initialize AutoExposure");
        return false;
    }

    GX_LOG_INFO("PostEffectPipeline initialized (%dx%d) with SSAO/SSR/VolumetricLight/Bloom/DoF/MotionBlur/Outline/TAA/FXAA/Vignette/ColorGrading", width, height);
    return true;
}

bool PostEffectPipeline::CreatePipelines(ID3D12Device* device)
{
    // 共通ルートシグネチャ: b0 + t0 + s0
    {
        RootSignatureBuilder rsb;
        m_commonRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_commonRS) return false;
    }

    auto vs = m_shader.CompileFromFile(L"Shaders/Tonemapping.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;
    auto vsBytecode = vs.GetBytecode();

    // ヘルパー: PSO作成
    auto buildPSO = [&](const wchar_t* file, const wchar_t* entry, DXGI_FORMAT rtFmt)
        -> ComPtr<ID3D12PipelineState>
    {
        auto ps = m_shader.CompileFromFile(file, entry, L"ps_6_0");
        if (!ps.valid) return nullptr;
        PipelineStateBuilder b;
        return b.SetRootSignature(m_commonRS.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(rtFmt)
            .SetDepthEnable(false).SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
    };

    // Tonemapping: HDR → LDR
    m_tonemapPSO = buildPSO(L"Shaders/Tonemapping.hlsl", L"PSMain", k_LDRFormat);
    if (!m_tonemapPSO) return false;

    // FXAA: LDR → LDR
    m_fxaaPSO = buildPSO(L"Shaders/FXAA.hlsl", L"PSMain", k_LDRFormat);
    if (!m_fxaaPSO) return false;

    // Vignette: LDR → LDR
    m_vignettePSO = buildPSO(L"Shaders/Vignette.hlsl", L"PSMain", k_LDRFormat);
    if (!m_vignettePSO) return false;

    // Color Grading: HDR → HDR
    m_colorGradingHDRPSO = buildPSO(L"Shaders/ColorGrading.hlsl", L"PSMain", k_HDRFormat);
    if (!m_colorGradingHDRPSO) return false;

    return true;
}

void PostEffectPipeline::DrawFullscreen(ID3D12PipelineState* pso, ID3D12RootSignature* rs,
                                         RenderTarget& dest, RenderTarget& src,
                                         DynamicBuffer& cb, const void* cbData, uint32_t cbSize)
{
    // バリアバッチング: dest→RT と src→SRV を1回の API 呼び出しでまとめて発行
    BarrierBatch<2> barriers(m_cmdList);
    if (dest.GetCurrentState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        barriers.Transition(dest.GetResource(), dest.GetCurrentState(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        dest.SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    if (src.GetCurrentState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    {
        barriers.Transition(src.GetResource(), src.GetCurrentState(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        src.SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    barriers.Flush();

    auto rtv = dest.GetRTVHandle();
    m_cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(dest.GetWidth());
    vp.Height = static_cast<float>(dest.GetHeight());
    vp.MaxDepth = 1.0f;
    m_cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(dest.GetWidth());
    sc.bottom = static_cast<LONG>(dest.GetHeight());
    m_cmdList->RSSetScissorRects(1, &sc);

    m_cmdList->SetPipelineState(pso);
    m_cmdList->SetGraphicsRootSignature(rs);

    ID3D12DescriptorHeap* heaps[] = { src.GetSRVHeap().GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);

    // CB更新
    void* p = cb.Map(m_frameIndex);
    if (p)
    {
        memcpy(p, cbData, cbSize);
        cb.Unmap(m_frameIndex);
    }
    m_cmdList->SetGraphicsRootConstantBufferView(0, cb.GetGPUVirtualAddress(m_frameIndex));
    m_cmdList->SetGraphicsRootDescriptorTable(1, src.GetSRVGPUHandle());

    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawInstanced(3, 1, 0, 0);
}

void PostEffectPipeline::DrawFullscreenToRTV(ID3D12PipelineState* pso, ID3D12RootSignature* rs,
                                              D3D12_CPU_DESCRIPTOR_HANDLE destRTV,
                                              RenderTarget& src,
                                              DynamicBuffer& cb, const void* cbData, uint32_t cbSize)
{
    src.TransitionTo(m_cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    m_cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    m_cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    m_cmdList->RSSetScissorRects(1, &sc);

    m_cmdList->SetPipelineState(pso);
    m_cmdList->SetGraphicsRootSignature(rs);

    ID3D12DescriptorHeap* heaps[] = { src.GetSRVHeap().GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);

    void* p = cb.Map(m_frameIndex);
    if (p)
    {
        memcpy(p, cbData, cbSize);
        cb.Unmap(m_frameIndex);
    }
    m_cmdList->SetGraphicsRootConstantBufferView(0, cb.GetGPUVirtualAddress(m_frameIndex));
    m_cmdList->SetGraphicsRootDescriptorTable(1, src.GetSRVGPUHandle());

    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawInstanced(3, 1, 0, 0);
}

void PostEffectPipeline::BeginScene(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                     D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, Camera3D& camera)
{
    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;

    // TAA有効時: ジッターをカメラに適用
    if (m_taa.IsEnabled())
    {
        auto jitter = m_taa.GetCurrentJitter();
        camera.SetJitter(jitter.x, jitter.y);
    }
    else
    {
        camera.ClearJitter();
    }

    m_hdrRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_normalRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_albedoRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[3] = {
        m_hdrRT.GetRTVHandle(),
        m_normalRT.GetRTVHandle(),
        m_albedoRT.GetRTVHandle()
    };
    cmdList->ClearRenderTargetView(rtvs[0], clearColor, 0, nullptr);
    cmdList->ClearRenderTargetView(rtvs[1], clearColor, 0, nullptr);
    cmdList->ClearRenderTargetView(rtvs[2], clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmdList->OMSetRenderTargets(3, rtvs, FALSE, &dsvHandle);

    D3D12_VIEWPORT viewport = {};
    viewport.Width  = static_cast<float>(m_width);
    viewport.Height = static_cast<float>(m_height);
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = {};
    scissor.right  = static_cast<LONG>(m_width);
    scissor.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &scissor);
}

void PostEffectPipeline::EndScene()
{
    m_hdrRT.TransitionTo(m_cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_normalRT.TransitionTo(m_cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_albedoRT.TransitionTo(m_cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostEffectPipeline::Resolve(D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV,
                                  DepthBuffer& depthBuffer, const Camera3D& camera,
                                  float deltaTime)
{
    // 毎フレーム太陽位置を計算（HUDデバッグ用、enabled に関係なく）
    m_volumetricLight.UpdateSunInfo(camera);

    // ========================================
    // HDRチェーン: ping-pongパターンで入力/出力RTを交互に切り替える
    // 同じRTを同時にSRV(入力)とRTV(出力)に使えないため、2枚のRTを交互に使用
    // ========================================
    RenderTarget* currentHDR = &m_hdrRT;

    // RTGI (GI加算合成, SSAOの前)
    if (m_rtGI && m_rtGI->IsEnabled() && m_cmdList4)
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_rtGI->SetNormalRT(&m_normalRT);
        m_rtGI->Execute(m_cmdList4, m_frameIndex, *currentHDR, *dest, depthBuffer, camera, m_albedoRT);
        currentHDR = dest;
    }

    // SSAO (HDRシーンにインプレース乗算合成)
    if (m_ssao.IsEnabled())
    {
        m_ssao.Execute(m_cmdList, m_frameIndex, *currentHDR, depthBuffer, camera);
        // hdrRT にインプレース乗算合成されるので currentHDR は変わらない
        // hdrRT の状態を SRV に戻す（次のエフェクト用）
        currentHDR->TransitionTo(m_cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // 反射: DXR RT反射 (排他) または SSR
    bool rtUsed = false;
    if (m_rtReflections && m_rtReflections->IsEnabled() && m_cmdList4)
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_rtReflections->SetNormalRT(&m_normalRT);
        m_rtReflections->Execute(m_cmdList4, m_frameIndex, *currentHDR, *dest, depthBuffer, camera);
        currentHDR = dest;
        rtUsed = true;
    }

    // SSR (HDR空間, SSAOの後) — RT反射と排他
    if (!rtUsed && m_ssr.IsEnabled())
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_ssr.Execute(m_cmdList, m_frameIndex, *currentHDR, *dest, depthBuffer, m_normalRT, camera);
        currentHDR = dest;
    }

    // ボリューメトリックライト (HDR空間, SSRの後・Bloomの前)
    if (m_volumetricLight.IsEnabled())
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_volumetricLight.Execute(m_cmdList, m_frameIndex, *currentHDR, *dest, depthBuffer, camera);
        currentHDR = dest;
    }

    // ブルーム
    if (m_bloom.IsEnabled())
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_bloom.Execute(m_cmdList, m_frameIndex, *currentHDR, *dest);
        dest->TransitionTo(m_cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        currentHDR = dest;
    }

    // DoF (HDR空間, Bloomの後)
    if (m_dof.IsEnabled())
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_dof.Execute(m_cmdList, m_frameIndex, *currentHDR, *dest, depthBuffer, camera);
        currentHDR = dest;
    }

    // モーションブラー (HDR空間, DoFの後)
    if (m_motionBlur.IsEnabled())
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_motionBlur.Execute(m_cmdList, m_frameIndex, *currentHDR, *dest, depthBuffer, camera);
        currentHDR = dest;
    }
    // 前フレームVP行列を保存 — Executeの後に呼ぶこと
    // 先に呼ぶと今フレームと前フレームのVPが同じになり、速度ベクトルが常にゼロになってしまう
    m_motionBlur.UpdatePreviousVP(camera);

    // アウトライン (HDR空間, MotionBlurの後)
    if (m_outline.IsEnabled())
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_outline.Execute(m_cmdList, m_frameIndex, *currentHDR, *dest, depthBuffer, camera);
        currentHDR = dest;
    }

    // TAA (HDR空間, Outlineの後・ColorGradingの前)
    if (m_taa.IsEnabled())
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        m_taa.Execute(m_cmdList, m_frameIndex, *currentHDR, *dest, depthBuffer, camera);
        currentHDR = dest;
    }
    // 前フレームVP行列を保存（次フレームのTAAで使用）
    m_taa.UpdatePreviousVP(camera);
    m_taa.AdvanceFrame();

    // カラーグレーディング (HDR空間)
    if (m_colorGradingEnabled)
    {
        RenderTarget* dest = (currentHDR == &m_hdrRT) ? &m_hdrPingPongRT : &m_hdrRT;
        ColorGradingConstants cgc = {};
        cgc.exposure    = 0.0f;
        cgc.contrast    = m_contrast;
        cgc.saturation  = m_saturation;
        cgc.temperature = m_temperature;
        DrawFullscreen(m_colorGradingHDRPSO.Get(), m_commonRS.Get(),
                       *dest, *currentHDR, m_colorGradingCB, &cgc, sizeof(cgc));
        currentHDR = dest;
    }

    // ========================================
    // 自動露出 (トーンマッピング直前)
    // ========================================
    float exposure = m_exposure;
    if (m_autoExposure.IsEnabled())
        exposure = m_autoExposure.ComputeExposure(m_cmdList, m_frameIndex, *currentHDR, deltaTime);

    // ========================================
    // トーンマッピング + LDRチェーン
    // ========================================
    bool hasLDREffects = m_fxaaEnabled || m_vignetteEnabled;

    if (!hasLDREffects)
    {
        // LDRエフェクトなし: バックバッファに直接トーンマップ
        TonemapConstants tc = {};
        tc.mode     = static_cast<uint32_t>(m_tonemapMode);
        tc.exposure = exposure;
        DrawFullscreenToRTV(m_tonemapPSO.Get(), m_commonRS.Get(),
                            backBufferRTV, *currentHDR, m_tonemapCB, &tc, sizeof(tc));
        return;
    }

    // トーンマップ → ldrRT[0]
    TonemapConstants tc = {};
    tc.mode     = static_cast<uint32_t>(m_tonemapMode);
    tc.exposure = exposure;
    DrawFullscreen(m_tonemapPSO.Get(), m_commonRS.Get(),
                   m_ldrRT[0], *currentHDR, m_tonemapCB, &tc, sizeof(tc));

    int ldrIdx = 0;

    // ========================================
    // LDRチェーン: [FXAA] → [Vignette]
    // 最後のエフェクトは直接バックバッファに描画する
    // ========================================
    bool vignetteIsLast = m_vignetteEnabled;
    bool fxaaIsLast = m_fxaaEnabled && !m_vignetteEnabled;

    // FXAA
    if (m_fxaaEnabled)
    {
        FXAAConstants fc = {};
        fc.rcpFrameX      = 1.0f / static_cast<float>(m_width);
        fc.rcpFrameY      = 1.0f / static_cast<float>(m_height);
        fc.qualitySubpix  = 0.75f;
        fc.edgeThreshold  = 0.166f;

        if (fxaaIsLast)
        {
            // FXAAが最後のエフェクト → 直接バックバッファに描画
            DrawFullscreenToRTV(m_fxaaPSO.Get(), m_commonRS.Get(),
                                backBufferRTV, m_ldrRT[ldrIdx], m_fxaaCB, &fc, sizeof(fc));
            return;
        }

        // FXAA → ldrRT[次]
        int dstIdx = 1 - ldrIdx;
        DrawFullscreen(m_fxaaPSO.Get(), m_commonRS.Get(),
                       m_ldrRT[dstIdx], m_ldrRT[ldrIdx], m_fxaaCB, &fc, sizeof(fc));
        ldrIdx = dstIdx;
    }

    // ビネット + 色収差
    if (m_vignetteEnabled)
    {
        VignetteConstants vc = {};
        vc.intensity        = m_vignetteIntensity;
        vc.radius           = 0.8f;
        vc.chromaticStrength = m_chromaticStrength;

        // ビネットはLDRの最終エフェクト → 直接バックバッファに描画
        DrawFullscreenToRTV(m_vignettePSO.Get(), m_commonRS.Get(),
                            backBufferRTV, m_ldrRT[ldrIdx], m_vignetteCB, &vc, sizeof(vc));
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE PostEffectPipeline::GetHDRRTVHandle() const
{
    return m_hdrRT.GetRTVHandle();
}

void PostEffectPipeline::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
    m_hdrRT.Create(device, width, height, k_HDRFormat);
    m_hdrPingPongRT.Create(device, width, height, k_HDRFormat);
    m_normalRT.Create(device, width, height, k_HDRFormat);
    m_albedoRT.Create(device, width, height, k_LDRFormat);
    m_ldrRT[0].Create(device, width, height, k_LDRFormat);
    m_ldrRT[1].Create(device, width, height, k_LDRFormat);
    m_ssao.OnResize(device, width, height);
    m_bloom.OnResize(device, width, height);
    m_dof.OnResize(device, width, height);
    m_motionBlur.OnResize(device, width, height);
    m_ssr.OnResize(device, width, height);
    m_outline.OnResize(device, width, height);
    m_volumetricLight.OnResize(device, width, height);
    m_taa.OnResize(device, width, height);
    m_autoExposure.OnResize(device, width, height);
    if (m_rtReflections)
        m_rtReflections->OnResize(device, width, height);
    if (m_rtGI)
        m_rtGI->OnResize(device, width, height);
}

bool PostEffectPipeline::LoadSettings(const std::string& filePath)
{
    return PostEffectSettings::Load(filePath, *this);
}

bool PostEffectPipeline::SaveSettings(const std::string& filePath) const
{
    return PostEffectSettings::Save(filePath, *this);
}

} // namespace GX
