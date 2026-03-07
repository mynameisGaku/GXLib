/// @file DebugOverlay.cpp
/// @brief ImGui ベースインタラクティブデバッグオーバーレイ実装
#include "pch.h"
#include "Compat/DebugOverlay.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/QualitySettings.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioMixer.h"
#include "Audio/AudioBus.h"
#include "Core/JobSystem.h"
#include "Core/AssetDatabase.h"
#include "IO/BundleManager.h"

#include <imgui.h>
#include <Psapi.h>

namespace gx
{

void DebugOverlay::Draw(const DebugOverlayContext& ctx)
{
    if (!m_visible) return;

    // FPS 履歴を更新
    m_fpsHistory[m_fpsHistoryOffset] = ctx.fps;
    m_fpsHistoryOffset = (m_fpsHistoryOffset + 1) % 120;

    ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 480), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Debug Overlay", &m_visible))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("DebugTabs"))
    {
        if (ImGui::BeginTabItem("System"))
        {
            DrawSystemTab(ctx);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Renderer"))
        {
            DrawRendererTab(ctx);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Post FX"))
        {
            DrawPostFXTab(ctx);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Audio"))
        {
            DrawAudioTab(ctx);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Quality"))
        {
            DrawQualityTab(ctx);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
// System タブ
// ---------------------------------------------------------------------------
void DebugOverlay::DrawSystemTab(const DebugOverlayContext& ctx)
{
    // FPS グラフ
    char overlay[32];
    snprintf(overlay, sizeof(overlay), "%.1f FPS", ctx.fps);
    ImGui::PlotLines("##FPS", m_fpsHistory, 120, m_fpsHistoryOffset,
                     overlay, 0.0f, 240.0f, ImVec2(0, 60));
    ImGui::Text("Frame: %.2f ms", ctx.deltaTime * 1000.0f);
    ImGui::Text("Resolution: %u x %u", ctx.screenWidth, ctx.screenHeight);

    ImGui::Separator();

    // メモリ情報
    PROCESS_MEMORY_COUNTERS_EX pmc = {};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
    {
        ImGui::Text("Working Set: %.1f MB", pmc.WorkingSetSize / (1024.0 * 1024.0));
        ImGui::Text("Private:     %.1f MB", pmc.PrivateUsage / (1024.0 * 1024.0));
    }

    ImGui::Separator();

    // Jobs
    auto& jobs = JobSystem::Instance();
    if (jobs.IsInitialized())
        ImGui::Text("Job Workers: %u", jobs.GetWorkerCount());
    else
        ImGui::TextDisabled("JobSystem: (not initialized)");

    // Assets
    auto& db = AssetDatabase::Instance();
    auto& bm = BundleManager::Instance();
    ImGui::Text("Assets: %d  Bundles: %u (%u assets)",
                db.GetAssetCount(), bm.GetBundleCount(), bm.GetTotalAssetCount());
}

// ---------------------------------------------------------------------------
// Renderer タブ
// ---------------------------------------------------------------------------
void DebugOverlay::DrawRendererTab(const DebugOverlayContext& ctx)
{
    if (!ctx.renderer3D)
    {
        ImGui::TextDisabled("Renderer3D not available");
        return;
    }

    auto stats = ctx.renderer3D->GetRenderStats();
    ImGui::Text("Draw Calls:   %u", stats.drawCallCount);
    ImGui::Text("Triangles:    %u", stats.triangleCount);
    ImGui::Text("PSO Switches: %u", stats.psoSwitchCount);

    ImGui::Separator();

    bool shadow = ctx.renderer3D->IsShadowEnabled();
    if (ImGui::Checkbox("Shadows", &shadow))
        ctx.renderer3D->SetShadowEnabled(shadow);
}

// ---------------------------------------------------------------------------
// Post FX タブ
// ---------------------------------------------------------------------------
void DebugOverlay::DrawPostFXTab(const DebugOverlayContext& ctx)
{
    if (!ctx.postEffect)
    {
        ImGui::TextDisabled("PostEffectPipeline not available");
        return;
    }

    auto& pe = *ctx.postEffect;

    // ── グローバル設定 ──────────────────────────────────
    {
        const char* tonemapNames[] = { "Reinhard", "ACES", "Uncharted2" };
        int tonemapIdx = static_cast<int>(pe.GetTonemapMode());
        if (ImGui::Combo("Tonemap", &tonemapIdx, tonemapNames, 3))
            pe.SetTonemapMode(static_cast<TonemapMode>(tonemapIdx));

        float exposure = pe.GetExposure();
        if (ImGui::SliderFloat("Manual Exposure", &exposure, 0.1f, 10.0f, "%.2f"))
            pe.SetExposure(exposure);
    }

    ImGui::Separator();

    // ── Auto Exposure ───────────────────────────────────
    {
        auto& ae = pe.GetAutoExposure();
        bool enabled = ae.IsEnabled();
        if (ImGui::Checkbox("Auto Exposure", &enabled)) ae.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("AutoExposure##details"))
        {
            ImGui::Text("Current: %.3f", ae.GetCurrentExposure());
            float v;
            v = ae.GetKeyValue();
            if (ImGui::SliderFloat("Key Value", &v, 0.01f, 0.5f, "%.3f")) ae.SetKeyValue(v);
            v = ae.GetMinExposure();
            if (ImGui::SliderFloat("Min Exposure", &v, 0.01f, 2.0f, "%.2f")) ae.SetMinExposure(v);
            v = ae.GetMaxExposure();
            if (ImGui::SliderFloat("Max Exposure", &v, 0.5f, 10.0f, "%.1f")) ae.SetMaxExposure(v);
            v = ae.GetAdaptationSpeed();
            if (ImGui::SliderFloat("Adapt Speed", &v, 0.1f, 10.0f, "%.1f")) ae.SetAdaptationSpeed(v);
            ImGui::TreePop();
        }
    }

    // ── Sky Atmosphere ──────────────────────────────────
    {
        auto& sky = pe.GetSkyAtmosphere();
        bool enabled = sky.IsEnabled();
        if (ImGui::Checkbox("Sky Atmosphere", &enabled)) sky.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("SkyAtmosphere##details"))
        {
            float v;
            v = sky.GetSunIntensity();
            if (ImGui::SliderFloat("Sun Intensity", &v, 0.0f, 100.0f, "%.1f")) sky.SetSunIntensity(v);
            int steps = sky.GetNumSteps();
            if (ImGui::SliderInt("Steps", &steps, 4, 32)) sky.SetNumSteps(steps);
            ImGui::TreePop();
        }
    }

    // ── Volumetric Clouds ───────────────────────────────
    {
        auto& cl = pe.GetVolumetricClouds();
        bool enabled = cl.IsEnabled();
        if (ImGui::Checkbox("Volumetric Clouds", &enabled)) cl.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("VolumetricClouds##details"))
        {
            float v;
            v = cl.GetCoverage();
            if (ImGui::SliderFloat("Coverage", &v, 0.0f, 1.0f, "%.2f")) cl.SetCoverage(v);
            v = cl.GetDensity();
            if (ImGui::SliderFloat("Density", &v, 0.001f, 0.5f, "%.3f")) cl.SetDensity(v);
            v = cl.GetWindSpeed();
            if (ImGui::SliderFloat("Wind Speed", &v, 0.0f, 50.0f, "%.1f")) cl.SetWindSpeed(v);
            v = cl.GetSilverLiningIntensity();
            if (ImGui::SliderFloat("Silver Lining", &v, 0.0f, 2.0f, "%.2f")) cl.SetSilverLiningIntensity(v);
            v = cl.GetCloudBottom();
            if (ImGui::SliderFloat("Cloud Bottom", &v, 500.0f, 5000.0f, "%.0f")) cl.SetCloudBottom(v);
            v = cl.GetCloudTop();
            if (ImGui::SliderFloat("Cloud Top", &v, 1000.0f, 8000.0f, "%.0f")) cl.SetCloudTop(v);
            int n;
            n = cl.GetMarchSteps();
            if (ImGui::SliderInt("March Steps", &n, 8, 128)) cl.SetMarchSteps(n);
            n = cl.GetLightSteps();
            if (ImGui::SliderInt("Light Steps", &n, 2, 16)) cl.SetLightSteps(n);
            if (ImGui::TreeNode("Multi-Scatter##ms"))
            {
                n = cl.GetMSOctaves();
                if (ImGui::SliderInt("Octaves", &n, 1, 8)) cl.SetMSOctaves(n);
                v = cl.GetMSAttenuation();
                if (ImGui::SliderFloat("Attenuation", &v, 0.0f, 1.0f, "%.2f")) cl.SetMSAttenuation(v);
                v = cl.GetMSContribution();
                if (ImGui::SliderFloat("Contribution", &v, 0.0f, 1.0f, "%.2f")) cl.SetMSContribution(v);
                v = cl.GetMSEccentricity();
                if (ImGui::SliderFloat("Eccentricity", &v, 0.0f, 1.0f, "%.2f")) cl.SetMSEccentricity(v);
                v = cl.GetPowderAmount();
                if (ImGui::SliderFloat("Powder", &v, 0.0f, 1.0f, "%.2f")) cl.SetPowderAmount(v);
                ImGui::TreePop();
            }
            v = cl.GetAmbientBottom();
            if (ImGui::SliderFloat("Ambient Bottom", &v, 0.0f, 1.0f, "%.2f")) cl.SetAmbientBottom(v);
            v = cl.GetAmbientTop();
            if (ImGui::SliderFloat("Ambient Top", &v, 0.0f, 1.0f, "%.2f")) cl.SetAmbientTop(v);
            v = cl.GetAtmosphereDensity();
            if (ImGui::SliderFloat("Atmo Density", &v, 0.0f, 0.001f, "%.6f")) cl.SetAtmosphereDensity(v);
            ImGui::TreePop();
        }
    }

    // ── Bloom ───────────────────────────────────────────
    {
        auto& bl = pe.GetBloom();
        bool enabled = bl.IsEnabled();
        if (ImGui::Checkbox("Bloom", &enabled)) bl.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("Bloom##details"))
        {
            float v;
            v = bl.GetThreshold();
            if (ImGui::SliderFloat("Threshold", &v, 0.0f, 5.0f, "%.2f")) bl.SetThreshold(v);
            v = bl.GetIntensity();
            if (ImGui::SliderFloat("Intensity", &v, 0.0f, 3.0f, "%.2f")) bl.SetIntensity(v);
            ImGui::TreePop();
        }
    }

    // ── SSAO ────────────────────────────────────────────
    {
        auto& ao = pe.GetSSAO();
        bool enabled = ao.IsEnabled();
        if (ImGui::Checkbox("SSAO", &enabled)) ao.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("SSAO##details"))
        {
            float v;
            v = ao.GetRadius();
            if (ImGui::SliderFloat("Radius", &v, 0.05f, 3.0f, "%.2f")) ao.SetRadius(v);
            v = ao.GetBias();
            if (ImGui::SliderFloat("Bias", &v, 0.001f, 0.1f, "%.3f")) ao.SetBias(v);
            v = ao.GetPower();
            if (ImGui::SliderFloat("Power", &v, 0.5f, 8.0f, "%.1f")) ao.SetPower(v);
            ImGui::TreePop();
        }
    }

    // ── SSR ─────────────────────────────────────────────
    {
        auto& ssr = pe.GetSSR();
        bool enabled = ssr.IsEnabled();
        if (ImGui::Checkbox("SSR", &enabled)) ssr.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("SSR##details"))
        {
            float v; int n;
            v = ssr.GetMaxDistance();
            if (ImGui::SliderFloat("Max Distance", &v, 1.0f, 100.0f, "%.1f")) ssr.SetMaxDistance(v);
            v = ssr.GetStepSize();
            if (ImGui::SliderFloat("Step Size", &v, 0.01f, 1.0f, "%.3f")) ssr.SetStepSize(v);
            n = ssr.GetMaxSteps();
            if (ImGui::SliderInt("Max Steps", &n, 32, 512)) ssr.SetMaxSteps(n);
            v = ssr.GetThickness();
            if (ImGui::SliderFloat("Thickness", &v, 0.01f, 1.0f, "%.3f")) ssr.SetThickness(v);
            v = ssr.GetIntensity();
            if (ImGui::SliderFloat("Intensity", &v, 0.0f, 2.0f, "%.2f")) ssr.SetIntensity(v);
            ImGui::TreePop();
        }
    }

    // ── SSGI ────────────────────────────────────────────
    {
        auto& gi = pe.GetSSGI();
        bool enabled = gi.IsEnabled();
        if (ImGui::Checkbox("SSGI", &enabled)) gi.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("SSGI##details"))
        {
            float v; int n;
            v = gi.GetIntensity();
            if (ImGui::SliderFloat("Intensity", &v, 0.0f, 2.0f, "%.2f")) gi.SetIntensity(v);
            n = gi.GetSampleCount();
            if (ImGui::SliderInt("Samples", &n, 4, 128)) gi.SetSampleCount(n);
            v = gi.GetMaxDistance();
            if (ImGui::SliderFloat("Max Distance", &v, 1.0f, 200.0f, "%.1f")) gi.SetMaxDistance(v);
            v = gi.GetThickness();
            if (ImGui::SliderFloat("Thickness", &v, 0.01f, 2.0f, "%.2f")) gi.SetThickness(v);
            v = gi.GetTemporalAlpha();
            if (ImGui::SliderFloat("Temporal Alpha", &v, 0.01f, 1.0f, "%.2f")) gi.SetTemporalAlpha(v);
            ImGui::TreePop();
        }
    }

    // ── Contact Shadows ─────────────────────────────────
    {
        auto& cs = pe.GetContactShadows();
        bool enabled = cs.IsEnabled();
        if (ImGui::Checkbox("Contact Shadows", &enabled)) cs.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("ContactShadows##details"))
        {
            float v; int n;
            v = cs.GetMaxDistance();
            if (ImGui::SliderFloat("Max Distance", &v, 0.01f, 2.0f, "%.3f")) cs.SetMaxDistance(v);
            n = cs.GetStepCount();
            if (ImGui::SliderInt("Steps", &n, 4, 64)) cs.SetStepCount(n);
            v = cs.GetThickness();
            if (ImGui::SliderFloat("Thickness", &v, 0.001f, 0.5f, "%.3f")) cs.SetThickness(v);
            v = cs.GetIntensity();
            if (ImGui::SliderFloat("Intensity", &v, 0.0f, 1.0f, "%.2f")) cs.SetIntensity(v);
            ImGui::TreePop();
        }
    }

    // ── Volumetric Light ────────────────────────────────
    {
        auto& vl = pe.GetVolumetricLight();
        bool enabled = vl.IsEnabled();
        if (ImGui::Checkbox("Volumetric Light", &enabled)) vl.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("VolumetricLight##details"))
        {
            float v; int n;
            v = vl.GetDensity();
            if (ImGui::SliderFloat("Density", &v, 0.0f, 5.0f, "%.2f")) vl.SetDensity(v);
            v = vl.GetDecay();
            if (ImGui::SliderFloat("Decay", &v, 0.9f, 1.0f, "%.3f")) vl.SetDecay(v);
            v = vl.GetWeight();
            if (ImGui::SliderFloat("Weight", &v, 0.0f, 0.2f, "%.3f")) vl.SetWeight(v);
            v = vl.GetExposure();
            if (ImGui::SliderFloat("Exposure", &v, 0.0f, 2.0f, "%.2f")) vl.SetExposure(v);
            v = vl.GetIntensity();
            if (ImGui::SliderFloat("Intensity", &v, 0.0f, 3.0f, "%.2f")) vl.SetIntensity(v);
            n = vl.GetNumSamples();
            if (ImGui::SliderInt("Samples", &n, 16, 256)) vl.SetNumSamples(n);
            ImGui::TreePop();
        }
    }

    // ── Lens Flare ──────────────────────────────────────
    {
        auto& lf = pe.GetLensFlare();
        bool enabled = lf.IsEnabled();
        if (ImGui::Checkbox("Lens Flare", &enabled)) lf.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("LensFlare##details"))
        {
            float v = lf.GetIntensity();
            if (ImGui::SliderFloat("Intensity", &v, 0.0f, 5.0f, "%.2f")) lf.SetIntensity(v);
            auto settings = lf.GetSettings();
            if (ImGui::SliderFloat("Starburst", &settings.starburstStrength, 0.0f, 1.0f, "%.2f"))
                lf.SetSettings(settings);
            if (ImGui::SliderFloat("Blades", &settings.apertureBlades, 3.0f, 12.0f, "%.0f"))
                lf.SetSettings(settings);
            ImGui::TreePop();
        }
    }

    // ── TAA ─────────────────────────────────────────────
    {
        auto& taa = pe.GetTAA();
        bool enabled = taa.IsEnabled();
        if (ImGui::Checkbox("TAA", &enabled)) taa.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("TAA##details"))
        {
            float v;
            v = taa.GetBlendFactor();
            if (ImGui::SliderFloat("Blend Factor", &v, 0.5f, 0.99f, "%.2f")) taa.SetBlendFactor(v);
            v = taa.GetSharpness();
            if (ImGui::SliderFloat("Sharpness", &v, 0.0f, 1.0f, "%.2f")) taa.SetSharpness(v);
            v = taa.GetResolutionScale();
            if (ImGui::SliderFloat("Resolution Scale", &v, 0.5f, 1.0f, "%.2f")) taa.SetResolutionScale(v);
            ImGui::TreePop();
        }
    }

    // ── Depth of Field ──────────────────────────────────
    {
        auto& dof = pe.GetDoF();
        bool enabled = dof.IsEnabled();
        if (ImGui::Checkbox("Depth of Field", &enabled)) dof.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("DoF##details"))
        {
            float v;
            v = dof.GetFocalDistance();
            if (ImGui::SliderFloat("Focal Distance", &v, 0.1f, 100.0f, "%.1f")) dof.SetFocalDistance(v);
            v = dof.GetFocalRange();
            if (ImGui::SliderFloat("Focal Range", &v, 0.1f, 50.0f, "%.1f")) dof.SetFocalRange(v);
            v = dof.GetBokehRadius();
            if (ImGui::SliderFloat("Bokeh Radius", &v, 1.0f, 20.0f, "%.1f")) dof.SetBokehRadius(v);
            ImGui::TreePop();
        }
    }

    // ── Motion Blur ─────────────────────────────────────
    {
        auto& mb = pe.GetMotionBlur();
        bool enabled = mb.IsEnabled();
        if (ImGui::Checkbox("Motion Blur", &enabled)) mb.SetEnabled(enabled);
        if (enabled && ImGui::TreeNode("MotionBlur##details"))
        {
            float v; int n;
            v = mb.GetIntensity();
            if (ImGui::SliderFloat("Intensity", &v, 0.0f, 3.0f, "%.2f")) mb.SetIntensity(v);
            n = mb.GetSampleCount();
            if (ImGui::SliderInt("Samples", &n, 4, 64)) mb.SetSampleCount(n);
            ImGui::TreePop();
        }
    }

    ImGui::Separator();

    // ── FXAA / Vignette / Color Grading / Chromatic Aberration ──
    {
        bool v = pe.IsFXAAEnabled();
        if (ImGui::Checkbox("FXAA", &v)) pe.SetFXAAEnabled(v);
    }
    {
        bool v = pe.IsVignetteEnabled();
        if (ImGui::Checkbox("Vignette", &v)) pe.SetVignetteEnabled(v);
        if (v)
        {
            float intensity = pe.GetVignetteIntensity();
            if (ImGui::SliderFloat("Vignette Intensity", &intensity, 0.0f, 2.0f, "%.2f"))
                pe.SetVignetteIntensity(intensity);
        }
    }
    {
        bool v = pe.IsColorGradingEnabled();
        if (ImGui::Checkbox("Color Grading", &v)) pe.SetColorGradingEnabled(v);
        if (v && ImGui::TreeNode("ColorGrading##details"))
        {
            float val;
            val = pe.GetContrast();
            if (ImGui::SliderFloat("Contrast", &val, 0.5f, 2.0f, "%.2f")) pe.SetContrast(val);
            val = pe.GetSaturation();
            if (ImGui::SliderFloat("Saturation", &val, 0.0f, 2.0f, "%.2f")) pe.SetSaturation(val);
            val = pe.GetTemperature();
            if (ImGui::SliderFloat("Temperature", &val, -1.0f, 1.0f, "%.2f")) pe.SetTemperature(val);
            ImGui::TreePop();
        }
    }
    {
        float ca = pe.GetChromaticAberration();
        if (ImGui::SliderFloat("Chromatic Aberration", &ca, 0.0f, 5.0f, "%.2f"))
            pe.SetChromaticAberration(ca);
    }
}

// ---------------------------------------------------------------------------
// Audio タブ
// ---------------------------------------------------------------------------
void DebugOverlay::DrawAudioTab(const DebugOverlayContext& ctx)
{
    if (!ctx.audioManager)
    {
        ImGui::TextDisabled("AudioManager not available");
        return;
    }

    auto& mixer = ctx.audioManager->GetMixer();

    auto drawBus = [](const char* label, AudioBus& bus) {
        ImGui::PushID(label);
        float vol = bus.GetVolume();
        if (ImGui::SliderFloat(label, &vol, 0.0f, 1.0f, "%.2f"))
            bus.SetVolume(vol);
        ImGui::SameLine();
        bool muted = bus.IsMuted();
        if (ImGui::Checkbox("Mute", &muted))
            bus.SetMuted(muted);
        ImGui::PopID();
    };

    drawBus("Master", mixer.GetMasterBus());
    drawBus("BGM",    mixer.GetBGMBus());
    drawBus("SE",     mixer.GetSEBus());
    drawBus("Voice",  mixer.GetVoiceBus());
}

// ---------------------------------------------------------------------------
// Quality タブ
// ---------------------------------------------------------------------------
void DebugOverlay::DrawQualityTab(const DebugOverlayContext& ctx)
{
    if (!ctx.quality || !ctx.postEffect)
    {
        ImGui::TextDisabled("QualitySettings not available");
        return;
    }

    const char* levelNames[] = { "Low", "Medium", "High", "Ultra", "Custom" };
    int level = static_cast<int>(ctx.quality->GetQualityLevel());
    if (ImGui::Combo("Quality Level", &level, levelNames, 5))
    {
        if (level < 4)  // Custom は選択不可
        {
            ctx.quality->SetQualityLevel(static_cast<QualityLevel>(level));
            ctx.quality->Apply(*ctx.postEffect);
        }
    }

    ImGui::Separator();

    auto& p = ctx.quality->GetParams();
    ImGui::Text("Shadow Map:  %u", p.shadowMapSize);
    ImGui::Text("SSAO:        %s (R=%.2f, S=%d)",
                p.ssaoEnabled ? "ON" : "OFF", p.ssaoRadius, p.ssaoSamples);
    ImGui::Text("Bloom:       %s (T=%.2f, I=%.2f)",
                p.bloomEnabled ? "ON" : "OFF", p.bloomThreshold, p.bloomIntensity);
    ImGui::Text("SSR:         %s", p.ssrEnabled ? "ON" : "OFF");
    ImGui::Text("TAA:         %s", p.taaEnabled ? "ON" : "OFF");
    ImGui::Text("FXAA:        %s", p.fxaaEnabled ? "ON" : "OFF");
    ImGui::Text("VRS Mode:    %u", p.vrsMode);
}

} // namespace gx
