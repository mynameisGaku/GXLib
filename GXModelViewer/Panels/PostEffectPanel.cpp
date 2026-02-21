/// @file PostEffectPanel.cpp
/// @brief ポストエフェクトパネル実装
///
/// 各エフェクトをCollapsingHeaderで分類し、ON/OFFチェックボックスと
/// パラメータスライダーを提供する。変更は即座にPipelineオブジェクトに反映される。

#include "PostEffectPanel.h"
#include <imgui.h>

#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/PostEffect/Bloom.h"
#include "Graphics/PostEffect/SSAO.h"
#include "Graphics/PostEffect/SSR.h"
#include "Graphics/PostEffect/TAA.h"
#include "Graphics/PostEffect/DepthOfField.h"
#include "Graphics/PostEffect/MotionBlur.h"
#include "Graphics/PostEffect/OutlineEffect.h"
#include "Graphics/PostEffect/VolumetricLight.h"
#include "Graphics/PostEffect/AutoExposure.h"

void PostEffectPanel::Draw(GX::PostEffectPipeline& pipeline)
{
    if (!ImGui::Begin("Post Effects"))
    {
        ImGui::End();
        return;
    }
    DrawContent(pipeline);
    ImGui::End();
}

void PostEffectPanel::DrawContent(GX::PostEffectPipeline& pipeline)
{
    // --- Bloom ---
    if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
    {
        GX::Bloom& bloom = pipeline.GetBloom();
        bool enabled = bloom.IsEnabled();
        if (ImGui::Checkbox("Enabled##Bloom", &enabled))
            bloom.SetEnabled(enabled);

        if (enabled)
        {
            float threshold = bloom.GetThreshold();
            if (ImGui::SliderFloat("Threshold##Bloom", &threshold, 0.0f, 5.0f, "%.2f"))
                bloom.SetThreshold(threshold);

            float intensity = bloom.GetIntensity();
            if (ImGui::SliderFloat("Intensity##Bloom", &intensity, 0.0f, 3.0f, "%.2f"))
                bloom.SetIntensity(intensity);
        }
    }

    // --- SSAO ---
    if (ImGui::CollapsingHeader("SSAO"))
    {
        GX::SSAO& ssao = pipeline.GetSSAO();
        bool enabled = ssao.IsEnabled();
        if (ImGui::Checkbox("Enabled##SSAO", &enabled))
            ssao.SetEnabled(enabled);

        if (enabled)
        {
            float radius = ssao.GetRadius();
            if (ImGui::SliderFloat("Radius##SSAO", &radius, 0.01f, 5.0f, "%.3f"))
                ssao.SetRadius(radius);

            float bias = ssao.GetBias();
            if (ImGui::SliderFloat("Bias##SSAO", &bias, 0.0f, 0.2f, "%.4f"))
                ssao.SetBias(bias);

            float power = ssao.GetPower();
            if (ImGui::SliderFloat("Power##SSAO", &power, 0.1f, 8.0f, "%.2f"))
                ssao.SetPower(power);
        }
    }

    // --- SSR ---
    if (ImGui::CollapsingHeader("SSR"))
    {
        GX::SSR& ssr = pipeline.GetSSR();
        bool enabled = ssr.IsEnabled();
        if (ImGui::Checkbox("Enabled##SSR", &enabled))
            ssr.SetEnabled(enabled);

        if (enabled)
        {
            float maxDist = ssr.GetMaxDistance();
            if (ImGui::SliderFloat("Max Distance##SSR", &maxDist, 1.0f, 100.0f, "%.1f"))
                ssr.SetMaxDistance(maxDist);

            float stepSize = ssr.GetStepSize();
            if (ImGui::SliderFloat("Step Size##SSR", &stepSize, 0.01f, 1.0f, "%.3f"))
                ssr.SetStepSize(stepSize);

            float thickness = ssr.GetThickness();
            if (ImGui::SliderFloat("Thickness##SSR", &thickness, 0.01f, 1.0f, "%.3f"))
                ssr.SetThickness(thickness);

            int maxSteps = ssr.GetMaxSteps();
            if (ImGui::SliderInt("Max Steps##SSR", &maxSteps, 16, 512))
                ssr.SetMaxSteps(maxSteps);

            float intensity = ssr.GetIntensity();
            if (ImGui::SliderFloat("Intensity##SSR", &intensity, 0.0f, 2.0f, "%.2f"))
                ssr.SetIntensity(intensity);
        }
    }

    // --- TAA ---
    if (ImGui::CollapsingHeader("TAA"))
    {
        GX::TAA& taa = pipeline.GetTAA();
        bool enabled = taa.IsEnabled();
        if (ImGui::Checkbox("Enabled##TAA", &enabled))
            taa.SetEnabled(enabled);

        if (enabled)
        {
            float blend = taa.GetBlendFactor();
            if (ImGui::SliderFloat("Blend Factor##TAA", &blend, 0.5f, 0.99f, "%.3f"))
                taa.SetBlendFactor(blend);
        }
    }

    // --- Depth of Field ---
    if (ImGui::CollapsingHeader("Depth of Field"))
    {
        GX::DepthOfField& dof = pipeline.GetDoF();
        bool enabled = dof.IsEnabled();
        if (ImGui::Checkbox("Enabled##DoF", &enabled))
            dof.SetEnabled(enabled);

        if (enabled)
        {
            float focalDist = dof.GetFocalDistance();
            if (ImGui::SliderFloat("Focus Distance##DoF", &focalDist, 0.1f, 100.0f, "%.1f"))
                dof.SetFocalDistance(focalDist);

            float focalRange = dof.GetFocalRange();
            if (ImGui::SliderFloat("Focus Range##DoF", &focalRange, 0.1f, 50.0f, "%.1f"))
                dof.SetFocalRange(focalRange);

            float bokeh = dof.GetBokehRadius();
            if (ImGui::SliderFloat("Bokeh Radius##DoF", &bokeh, 1.0f, 32.0f, "%.1f"))
                dof.SetBokehRadius(bokeh);
        }
    }

    // --- Motion Blur ---
    if (ImGui::CollapsingHeader("Motion Blur"))
    {
        GX::MotionBlur& mb = pipeline.GetMotionBlur();
        bool enabled = mb.IsEnabled();
        if (ImGui::Checkbox("Enabled##MotionBlur", &enabled))
            mb.SetEnabled(enabled);

        if (enabled)
        {
            float intensity = mb.GetIntensity();
            if (ImGui::SliderFloat("Intensity##MotionBlur", &intensity, 0.0f, 3.0f, "%.2f"))
                mb.SetIntensity(intensity);

            int samples = mb.GetSampleCount();
            if (ImGui::SliderInt("Sample Count##MotionBlur", &samples, 4, 64))
                mb.SetSampleCount(samples);
        }
    }

    // --- Outline ---
    if (ImGui::CollapsingHeader("Outline"))
    {
        GX::OutlineEffect& outline = pipeline.GetOutline();
        bool enabled = outline.IsEnabled();
        if (ImGui::Checkbox("Enabled##Outline", &enabled))
            outline.SetEnabled(enabled);

        if (enabled)
        {
            float depthThresh = outline.GetDepthThreshold();
            if (ImGui::SliderFloat("Depth Threshold##Outline", &depthThresh, 0.01f, 5.0f, "%.3f"))
                outline.SetDepthThreshold(depthThresh);

            float normalThresh = outline.GetNormalThreshold();
            if (ImGui::SliderFloat("Normal Threshold##Outline", &normalThresh, 0.01f, 1.0f, "%.3f"))
                outline.SetNormalThreshold(normalThresh);

            float intensity = outline.GetIntensity();
            if (ImGui::SliderFloat("Intensity##Outline", &intensity, 0.0f, 3.0f, "%.2f"))
                outline.SetIntensity(intensity);

            // Line color as RGBA
            // OutlineEffect stores XMFLOAT4 but we use float[4] for ImGui
            const auto& lc = outline.GetLineColor();
            float color[4] = { lc.x, lc.y, lc.z, lc.w };
            if (ImGui::ColorEdit4("Color##Outline", color))
            {
                DirectX::XMFLOAT4 newColor(color[0], color[1], color[2], color[3]);
                outline.SetLineColor(newColor);
            }
        }
    }

    // --- Volumetric Light ---
    if (ImGui::CollapsingHeader("Volumetric Light"))
    {
        GX::VolumetricLight& vl = pipeline.GetVolumetricLight();
        bool enabled = vl.IsEnabled();
        if (ImGui::Checkbox("Enabled##VolumetricLight", &enabled))
            vl.SetEnabled(enabled);

        if (enabled)
        {
            float intensity = vl.GetIntensity();
            if (ImGui::SliderFloat("Intensity##VL", &intensity, 0.0f, 3.0f, "%.2f"))
                vl.SetIntensity(intensity);

            float decay = vl.GetDecay();
            if (ImGui::SliderFloat("Decay##VL", &decay, 0.8f, 1.0f, "%.3f"))
                vl.SetDecay(decay);

            float density = vl.GetDensity();
            if (ImGui::SliderFloat("Density##VL", &density, 0.1f, 3.0f, "%.2f"))
                vl.SetDensity(density);

            float weight = vl.GetWeight();
            if (ImGui::SliderFloat("Weight##VL", &weight, 0.001f, 0.2f, "%.4f"))
                vl.SetWeight(weight);

            float exposure = vl.GetExposure();
            if (ImGui::SliderFloat("Exposure##VL", &exposure, 0.01f, 2.0f, "%.3f"))
                vl.SetExposure(exposure);

            int samples = vl.GetNumSamples();
            if (ImGui::SliderInt("Samples##VL", &samples, 16, 256))
                vl.SetNumSamples(samples);
        }
    }

    // --- Color Grading ---
    if (ImGui::CollapsingHeader("Color Grading"))
    {
        bool enabled = pipeline.IsColorGradingEnabled();
        if (ImGui::Checkbox("Enabled##ColorGrading", &enabled))
            pipeline.SetColorGradingEnabled(enabled);

        if (enabled)
        {
            float temperature = pipeline.GetTemperature();
            if (ImGui::SliderFloat("Temperature##CG", &temperature, -1.0f, 1.0f, "%.3f"))
                pipeline.SetTemperature(temperature);

            float saturation = pipeline.GetSaturation();
            if (ImGui::SliderFloat("Saturation##CG", &saturation, 0.0f, 3.0f, "%.2f"))
                pipeline.SetSaturation(saturation);

            float contrast = pipeline.GetContrast();
            if (ImGui::SliderFloat("Contrast##CG", &contrast, 0.1f, 3.0f, "%.2f"))
                pipeline.SetContrast(contrast);
        }
    }

    // --- Auto Exposure ---
    if (ImGui::CollapsingHeader("Auto Exposure"))
    {
        GX::AutoExposure& ae = pipeline.GetAutoExposure();
        bool enabled = ae.IsEnabled();
        if (ImGui::Checkbox("Enabled##AutoExposure", &enabled))
            ae.SetEnabled(enabled);

        if (enabled)
        {
            float minEV = ae.GetMinExposure();
            if (ImGui::SliderFloat("Min Exposure##AE", &minEV, 0.01f, 5.0f, "%.2f"))
                ae.SetMinExposure(minEV);

            float maxEV = ae.GetMaxExposure();
            if (ImGui::SliderFloat("Max Exposure##AE", &maxEV, 1.0f, 20.0f, "%.2f"))
                ae.SetMaxExposure(maxEV);

            float speed = ae.GetAdaptationSpeed();
            if (ImGui::SliderFloat("Adapt Speed##AE", &speed, 0.1f, 10.0f, "%.2f"))
                ae.SetAdaptationSpeed(speed);

            float key = ae.GetKeyValue();
            if (ImGui::SliderFloat("Key Value##AE", &key, 0.01f, 1.0f, "%.3f"))
                ae.SetKeyValue(key);

            ImGui::Text("Current: %.3f", ae.GetCurrentExposure());
        }
    }

    // --- Tonemapping ---
    if (ImGui::CollapsingHeader("Tonemapping", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char* modeNames[] = { "Reinhard", "ACES", "Uncharted2" };
        int current = static_cast<int>(pipeline.GetTonemapMode());
        if (ImGui::Combo("Mode##Tonemap", &current, modeNames, 3))
            pipeline.SetTonemapMode(static_cast<GX::TonemapMode>(current));

        float exposure = pipeline.GetExposure();
        if (ImGui::SliderFloat("Exposure##Tonemap", &exposure, 0.1f, 10.0f, "%.2f"))
            pipeline.SetExposure(exposure);
    }

    // --- FXAA ---
    if (ImGui::CollapsingHeader("FXAA"))
    {
        bool fxaa = pipeline.IsFXAAEnabled();
        if (ImGui::Checkbox("Enabled##FXAA", &fxaa))
            pipeline.SetFXAAEnabled(fxaa);
    }

    // --- Vignette ---
    if (ImGui::CollapsingHeader("Vignette"))
    {
        bool enabled = pipeline.IsVignetteEnabled();
        if (ImGui::Checkbox("Enabled##Vignette", &enabled))
            pipeline.SetVignetteEnabled(enabled);

        if (enabled)
        {
            float intensity = pipeline.GetVignetteIntensity();
            if (ImGui::SliderFloat("Intensity##Vignette", &intensity, 0.0f, 2.0f, "%.2f"))
                pipeline.SetVignetteIntensity(intensity);

            float chromatic = pipeline.GetChromaticAberration();
            if (ImGui::SliderFloat("Chromatic Aberration##Vignette", &chromatic, 0.0f, 0.02f, "%.4f"))
                pipeline.SetChromaticAberration(chromatic);
        }
    }
}
