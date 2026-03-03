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

    // Exposure
    float exposure = pe.GetExposure();
    if (ImGui::SliderFloat("Exposure", &exposure, 0.1f, 10.0f, "%.2f"))
        pe.SetExposure(exposure);

    // Tonemap
    const char* tonemapNames[] = { "Reinhard", "ACES", "Uncharted2" };
    int tonemapIdx = static_cast<int>(pe.GetTonemapMode());
    if (ImGui::Combo("Tonemap", &tonemapIdx, tonemapNames, 3))
        pe.SetTonemapMode(static_cast<TonemapMode>(tonemapIdx));

    ImGui::Separator();

    // エフェクトチェックボックス
    {
        bool v = pe.GetSSAO().IsEnabled();
        if (ImGui::Checkbox("SSAO", &v)) pe.GetSSAO().SetEnabled(v);
    }
    {
        bool v = pe.GetBloom().IsEnabled();
        if (ImGui::Checkbox("Bloom", &v)) pe.GetBloom().SetEnabled(v);
    }
    {
        bool v = pe.GetSSR().IsEnabled();
        if (ImGui::Checkbox("SSR", &v)) pe.GetSSR().SetEnabled(v);
    }
    {
        bool v = pe.GetTAA().IsEnabled();
        if (ImGui::Checkbox("TAA", &v)) pe.GetTAA().SetEnabled(v);
    }
    {
        bool v = pe.IsFXAAEnabled();
        if (ImGui::Checkbox("FXAA", &v)) pe.SetFXAAEnabled(v);
    }
    {
        bool v = pe.GetDoF().IsEnabled();
        if (ImGui::Checkbox("Depth of Field", &v)) pe.GetDoF().SetEnabled(v);
    }
    {
        bool v = pe.GetMotionBlur().IsEnabled();
        if (ImGui::Checkbox("Motion Blur", &v)) pe.GetMotionBlur().SetEnabled(v);
    }
    {
        bool v = pe.GetVolumetricLight().IsEnabled();
        if (ImGui::Checkbox("Volumetric Light", &v)) pe.GetVolumetricLight().SetEnabled(v);
    }
    {
        bool v = pe.GetSSGI().IsEnabled();
        if (ImGui::Checkbox("SSGI", &v)) pe.GetSSGI().SetEnabled(v);
    }
    {
        bool v = pe.GetContactShadows().IsEnabled();
        if (ImGui::Checkbox("Contact Shadows", &v)) pe.GetContactShadows().SetEnabled(v);
    }
    {
        bool v = pe.GetVolumetricClouds().IsEnabled();
        if (ImGui::Checkbox("Volumetric Clouds", &v)) pe.GetVolumetricClouds().SetEnabled(v);
    }
    {
        bool v = pe.GetLensFlare().IsEnabled();
        if (ImGui::Checkbox("Lens Flare", &v)) pe.GetLensFlare().SetEnabled(v);
    }
    if (pe.GetLensFlare().IsEnabled())
    {
        float intensity = pe.GetLensFlare().GetIntensity();
        if (ImGui::SliderFloat("LF Intensity", &intensity, 0.0f, 5.0f, "%.2f"))
            pe.GetLensFlare().SetIntensity(intensity);

        auto settings = pe.GetLensFlare().GetSettings();

        float starburst = settings.starburstStrength;
        if (ImGui::SliderFloat("LF Starburst", &starburst, 0.0f, 1.0f, "%.2f"))
        {
            settings.starburstStrength = starburst;
            pe.GetLensFlare().SetSettings(settings);
        }

        float blades = settings.apertureBlades;
        if (ImGui::SliderFloat("LF Blades", &blades, 3.0f, 12.0f, "%.0f"))
        {
            settings.apertureBlades = blades;
            pe.GetLensFlare().SetSettings(settings);
        }
    }

    ImGui::Separator();
    float ca = pe.GetChromaticAberration();
    if (ImGui::SliderFloat("Chromatic Aberration", &ca, 0.0f, 5.0f, "%.2f"))
        pe.SetChromaticAberration(ca);
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
