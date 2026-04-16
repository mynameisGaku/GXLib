/// @file RenderProfile.cpp
/// @brief RenderProfile / RenderProfileStack 実装
#include "pch_graphics.h"
#include "Graphics/RenderProfile.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/RayTracing/RTReflections.h"
#include "Graphics/RayTracing/RTGI.h"
#include "Core/Logger.h"

#include <fstream>
#include <algorithm>

// nlohmann/json はこの翻訳単位でのみ include
#include "ThirdParty/json.hpp"

using json = nlohmann::json;

namespace gx
{

// ============================================================================
// RenderProfile::MergeFrom
// ============================================================================

void RenderProfile::MergeFrom(const RenderProfile& o)
{
    // Tonemapping
    tonemapMode.MergeFrom(o.tonemapMode);
    tonemapExposure.MergeFrom(o.tonemapExposure);

    // Bloom
    bloomEnabled.MergeFrom(o.bloomEnabled);
    bloomThreshold.MergeFrom(o.bloomThreshold);
    bloomIntensity.MergeFrom(o.bloomIntensity);

    // SSAO
    ssaoEnabled.MergeFrom(o.ssaoEnabled);
    ssaoRadius.MergeFrom(o.ssaoRadius);
    ssaoBias.MergeFrom(o.ssaoBias);
    ssaoPower.MergeFrom(o.ssaoPower);

    // DoF
    dofEnabled.MergeFrom(o.dofEnabled);
    dofFocalDistance.MergeFrom(o.dofFocalDistance);
    dofFocalRange.MergeFrom(o.dofFocalRange);
    dofBokehRadius.MergeFrom(o.dofBokehRadius);

    // MotionBlur
    motionBlurEnabled.MergeFrom(o.motionBlurEnabled);
    motionBlurIntensity.MergeFrom(o.motionBlurIntensity);
    motionBlurSampleCount.MergeFrom(o.motionBlurSampleCount);

    // SSR
    ssrEnabled.MergeFrom(o.ssrEnabled);
    ssrMaxDistance.MergeFrom(o.ssrMaxDistance);
    ssrStepSize.MergeFrom(o.ssrStepSize);
    ssrMaxSteps.MergeFrom(o.ssrMaxSteps);
    ssrThickness.MergeFrom(o.ssrThickness);
    ssrIntensity.MergeFrom(o.ssrIntensity);

    // Outline
    outlineEnabled.MergeFrom(o.outlineEnabled);
    outlineDepthThreshold.MergeFrom(o.outlineDepthThreshold);
    outlineNormalThreshold.MergeFrom(o.outlineNormalThreshold);
    outlineIntensity.MergeFrom(o.outlineIntensity);

    // TAA
    taaEnabled.MergeFrom(o.taaEnabled);
    taaBlendFactor.MergeFrom(o.taaBlendFactor);
    taaSharpness.MergeFrom(o.taaSharpness);

    // VolumetricLight
    volumetricLightEnabled.MergeFrom(o.volumetricLightEnabled);
    volumetricLightIntensity.MergeFrom(o.volumetricLightIntensity);
    volumetricLightDensity.MergeFrom(o.volumetricLightDensity);
    volumetricLightDecay.MergeFrom(o.volumetricLightDecay);

    // AutoExposure
    autoExposureEnabled.MergeFrom(o.autoExposureEnabled);
    autoExposureSpeed.MergeFrom(o.autoExposureSpeed);
    autoExposureMin.MergeFrom(o.autoExposureMin);
    autoExposureMax.MergeFrom(o.autoExposureMax);
    autoExposureKeyValue.MergeFrom(o.autoExposureKeyValue);

    // ContactShadows
    contactShadowsEnabled.MergeFrom(o.contactShadowsEnabled);
    contactShadowsMaxDistance.MergeFrom(o.contactShadowsMaxDistance);
    contactShadowsStepCount.MergeFrom(o.contactShadowsStepCount);
    contactShadowsThickness.MergeFrom(o.contactShadowsThickness);
    contactShadowsIntensity.MergeFrom(o.contactShadowsIntensity);

    // Vignette / ChromaticAberration
    vignetteEnabled.MergeFrom(o.vignetteEnabled);
    vignetteIntensity.MergeFrom(o.vignetteIntensity);
    chromaticAberrationStrength.MergeFrom(o.chromaticAberrationStrength);

    // ColorGrading
    colorGradingEnabled.MergeFrom(o.colorGradingEnabled);
    colorGradingContrast.MergeFrom(o.colorGradingContrast);
    colorGradingSaturation.MergeFrom(o.colorGradingSaturation);
    colorGradingTemperature.MergeFrom(o.colorGradingTemperature);

    // FXAA
    fxaaEnabled.MergeFrom(o.fxaaEnabled);

    // LensFlare
    lensFlareEnabled.MergeFrom(o.lensFlareEnabled);
    lensFlareIntensity.MergeFrom(o.lensFlareIntensity);
    lensFlareThreshold.MergeFrom(o.lensFlareThreshold);
    lensFlareApertureBlades.MergeFrom(o.lensFlareApertureBlades);
    lensFlareStarburstStrength.MergeFrom(o.lensFlareStarburstStrength);

    // Fog
    fogMode.MergeFrom(o.fogMode);
    fogColor.MergeFrom(o.fogColor);
    fogStart.MergeFrom(o.fogStart);
    fogEnd.MergeFrom(o.fogEnd);
    fogDensity.MergeFrom(o.fogDensity);

    // Shadows
    shadowEnabled.MergeFrom(o.shadowEnabled);
    cascadeSplits.MergeFrom(o.cascadeSplits);
    cascadeBlendEnabled.MergeFrom(o.cascadeBlendEnabled);
    cascadeBlendWidth.MergeFrom(o.cascadeBlendWidth);
    pcssEnabled.MergeFrom(o.pcssEnabled);
    pcssLightSize.MergeFrom(o.pcssLightSize);

    // Ambient
    ambientColor.MergeFrom(o.ambientColor);

    // RT Reflections
    rtReflectionsEnabled.MergeFrom(o.rtReflectionsEnabled);
    rtReflectionsMaxDistance.MergeFrom(o.rtReflectionsMaxDistance);
    rtReflectionsIntensity.MergeFrom(o.rtReflectionsIntensity);
    rtReflectionsShadowSamples.MergeFrom(o.rtReflectionsShadowSamples);

    // RT GI
    rtgiEnabled.MergeFrom(o.rtgiEnabled);
    rtgiIntensity.MergeFrom(o.rtgiIntensity);
    rtgiMaxDistance.MergeFrom(o.rtgiMaxDistance);
    rtgiTemporalAlpha.MergeFrom(o.rtgiTemporalAlpha);
    rtgiSpatialIterations.MergeFrom(o.rtgiSpatialIterations);

    // SSGI
    ssgiEnabled.MergeFrom(o.ssgiEnabled);
    ssgiIntensity.MergeFrom(o.ssgiIntensity);
    ssgiMaxDistance.MergeFrom(o.ssgiMaxDistance);
    ssgiSampleCount.MergeFrom(o.ssgiSampleCount);
    ssgiThickness.MergeFrom(o.ssgiThickness);
}

// ============================================================================
// RenderProfile::Lerp
// ============================================================================

RenderProfile RenderProfile::Lerp(const RenderProfile& a, const RenderProfile& b, float t)
{
    RenderProfile r;

    // Tonemapping
    r.tonemapMode     = LerpOverride(a.tonemapMode, b.tonemapMode, t);
    r.tonemapExposure = LerpOverride(a.tonemapExposure, b.tonemapExposure, t);

    // Bloom
    r.bloomEnabled   = LerpOverride(a.bloomEnabled, b.bloomEnabled, t);
    r.bloomThreshold = LerpOverride(a.bloomThreshold, b.bloomThreshold, t);
    r.bloomIntensity = LerpOverride(a.bloomIntensity, b.bloomIntensity, t);

    // SSAO
    r.ssaoEnabled = LerpOverride(a.ssaoEnabled, b.ssaoEnabled, t);
    r.ssaoRadius  = LerpOverride(a.ssaoRadius, b.ssaoRadius, t);
    r.ssaoBias    = LerpOverride(a.ssaoBias, b.ssaoBias, t);
    r.ssaoPower   = LerpOverride(a.ssaoPower, b.ssaoPower, t);

    // DoF
    r.dofEnabled       = LerpOverride(a.dofEnabled, b.dofEnabled, t);
    r.dofFocalDistance  = LerpOverride(a.dofFocalDistance, b.dofFocalDistance, t);
    r.dofFocalRange    = LerpOverride(a.dofFocalRange, b.dofFocalRange, t);
    r.dofBokehRadius   = LerpOverride(a.dofBokehRadius, b.dofBokehRadius, t);

    // MotionBlur
    r.motionBlurEnabled     = LerpOverride(a.motionBlurEnabled, b.motionBlurEnabled, t);
    r.motionBlurIntensity   = LerpOverride(a.motionBlurIntensity, b.motionBlurIntensity, t);
    r.motionBlurSampleCount = LerpOverride(a.motionBlurSampleCount, b.motionBlurSampleCount, t);

    // SSR
    r.ssrEnabled     = LerpOverride(a.ssrEnabled, b.ssrEnabled, t);
    r.ssrMaxDistance  = LerpOverride(a.ssrMaxDistance, b.ssrMaxDistance, t);
    r.ssrStepSize    = LerpOverride(a.ssrStepSize, b.ssrStepSize, t);
    r.ssrMaxSteps    = LerpOverride(a.ssrMaxSteps, b.ssrMaxSteps, t);
    r.ssrThickness   = LerpOverride(a.ssrThickness, b.ssrThickness, t);
    r.ssrIntensity   = LerpOverride(a.ssrIntensity, b.ssrIntensity, t);

    // Outline
    r.outlineEnabled        = LerpOverride(a.outlineEnabled, b.outlineEnabled, t);
    r.outlineDepthThreshold = LerpOverride(a.outlineDepthThreshold, b.outlineDepthThreshold, t);
    r.outlineNormalThreshold= LerpOverride(a.outlineNormalThreshold, b.outlineNormalThreshold, t);
    r.outlineIntensity      = LerpOverride(a.outlineIntensity, b.outlineIntensity, t);

    // TAA
    r.taaEnabled     = LerpOverride(a.taaEnabled, b.taaEnabled, t);
    r.taaBlendFactor = LerpOverride(a.taaBlendFactor, b.taaBlendFactor, t);
    r.taaSharpness   = LerpOverride(a.taaSharpness, b.taaSharpness, t);

    // VolumetricLight
    r.volumetricLightEnabled   = LerpOverride(a.volumetricLightEnabled, b.volumetricLightEnabled, t);
    r.volumetricLightIntensity = LerpOverride(a.volumetricLightIntensity, b.volumetricLightIntensity, t);
    r.volumetricLightDensity   = LerpOverride(a.volumetricLightDensity, b.volumetricLightDensity, t);
    r.volumetricLightDecay     = LerpOverride(a.volumetricLightDecay, b.volumetricLightDecay, t);

    // AutoExposure
    r.autoExposureEnabled  = LerpOverride(a.autoExposureEnabled, b.autoExposureEnabled, t);
    r.autoExposureSpeed    = LerpOverride(a.autoExposureSpeed, b.autoExposureSpeed, t);
    r.autoExposureMin      = LerpOverride(a.autoExposureMin, b.autoExposureMin, t);
    r.autoExposureMax      = LerpOverride(a.autoExposureMax, b.autoExposureMax, t);
    r.autoExposureKeyValue = LerpOverride(a.autoExposureKeyValue, b.autoExposureKeyValue, t);

    // ContactShadows
    r.contactShadowsEnabled     = LerpOverride(a.contactShadowsEnabled, b.contactShadowsEnabled, t);
    r.contactShadowsMaxDistance  = LerpOverride(a.contactShadowsMaxDistance, b.contactShadowsMaxDistance, t);
    r.contactShadowsStepCount   = LerpOverride(a.contactShadowsStepCount, b.contactShadowsStepCount, t);
    r.contactShadowsThickness   = LerpOverride(a.contactShadowsThickness, b.contactShadowsThickness, t);
    r.contactShadowsIntensity   = LerpOverride(a.contactShadowsIntensity, b.contactShadowsIntensity, t);

    // Vignette / ChromaticAberration
    r.vignetteEnabled             = LerpOverride(a.vignetteEnabled, b.vignetteEnabled, t);
    r.vignetteIntensity           = LerpOverride(a.vignetteIntensity, b.vignetteIntensity, t);
    r.chromaticAberrationStrength = LerpOverride(a.chromaticAberrationStrength, b.chromaticAberrationStrength, t);

    // ColorGrading
    r.colorGradingEnabled     = LerpOverride(a.colorGradingEnabled, b.colorGradingEnabled, t);
    r.colorGradingContrast    = LerpOverride(a.colorGradingContrast, b.colorGradingContrast, t);
    r.colorGradingSaturation  = LerpOverride(a.colorGradingSaturation, b.colorGradingSaturation, t);
    r.colorGradingTemperature = LerpOverride(a.colorGradingTemperature, b.colorGradingTemperature, t);

    // FXAA
    r.fxaaEnabled = LerpOverride(a.fxaaEnabled, b.fxaaEnabled, t);

    // LensFlare
    r.lensFlareEnabled           = LerpOverride(a.lensFlareEnabled, b.lensFlareEnabled, t);
    r.lensFlareIntensity         = LerpOverride(a.lensFlareIntensity, b.lensFlareIntensity, t);
    r.lensFlareThreshold         = LerpOverride(a.lensFlareThreshold, b.lensFlareThreshold, t);
    r.lensFlareApertureBlades    = LerpOverride(a.lensFlareApertureBlades, b.lensFlareApertureBlades, t);
    r.lensFlareStarburstStrength = LerpOverride(a.lensFlareStarburstStrength, b.lensFlareStarburstStrength, t);

    // Fog
    r.fogMode    = LerpOverride(a.fogMode, b.fogMode, t);
    r.fogColor   = LerpOverride(a.fogColor, b.fogColor, t);
    r.fogStart   = LerpOverride(a.fogStart, b.fogStart, t);
    r.fogEnd     = LerpOverride(a.fogEnd, b.fogEnd, t);
    r.fogDensity = LerpOverride(a.fogDensity, b.fogDensity, t);

    // Shadows
    r.shadowEnabled       = LerpOverride(a.shadowEnabled, b.shadowEnabled, t);
    r.cascadeSplits       = LerpOverride(a.cascadeSplits, b.cascadeSplits, t);
    r.cascadeBlendEnabled = LerpOverride(a.cascadeBlendEnabled, b.cascadeBlendEnabled, t);
    r.cascadeBlendWidth   = LerpOverride(a.cascadeBlendWidth, b.cascadeBlendWidth, t);
    r.pcssEnabled         = LerpOverride(a.pcssEnabled, b.pcssEnabled, t);
    r.pcssLightSize       = LerpOverride(a.pcssLightSize, b.pcssLightSize, t);

    // Ambient
    r.ambientColor = LerpOverride(a.ambientColor, b.ambientColor, t);

    // RT Reflections
    r.rtReflectionsEnabled       = LerpOverride(a.rtReflectionsEnabled, b.rtReflectionsEnabled, t);
    r.rtReflectionsMaxDistance    = LerpOverride(a.rtReflectionsMaxDistance, b.rtReflectionsMaxDistance, t);
    r.rtReflectionsIntensity     = LerpOverride(a.rtReflectionsIntensity, b.rtReflectionsIntensity, t);
    r.rtReflectionsShadowSamples = LerpOverride(a.rtReflectionsShadowSamples, b.rtReflectionsShadowSamples, t);

    // RT GI
    r.rtgiEnabled           = LerpOverride(a.rtgiEnabled, b.rtgiEnabled, t);
    r.rtgiIntensity         = LerpOverride(a.rtgiIntensity, b.rtgiIntensity, t);
    r.rtgiMaxDistance        = LerpOverride(a.rtgiMaxDistance, b.rtgiMaxDistance, t);
    r.rtgiTemporalAlpha     = LerpOverride(a.rtgiTemporalAlpha, b.rtgiTemporalAlpha, t);
    r.rtgiSpatialIterations = LerpOverride(a.rtgiSpatialIterations, b.rtgiSpatialIterations, t);

    // SSGI
    r.ssgiEnabled     = LerpOverride(a.ssgiEnabled, b.ssgiEnabled, t);
    r.ssgiIntensity   = LerpOverride(a.ssgiIntensity, b.ssgiIntensity, t);
    r.ssgiMaxDistance  = LerpOverride(a.ssgiMaxDistance, b.ssgiMaxDistance, t);
    r.ssgiSampleCount = LerpOverride(a.ssgiSampleCount, b.ssgiSampleCount, t);
    r.ssgiThickness   = LerpOverride(a.ssgiThickness, b.ssgiThickness, t);

    return r;
}

// ============================================================================
// RenderProfile::Apply
// ============================================================================

void RenderProfile::Apply(PostEffectPipeline& pipeline, Renderer3D* renderer) const
{
    // --- Tonemapping ---
    if (tonemapMode.overridden)     pipeline.SetTonemapMode(tonemapMode.value);
    if (tonemapExposure.overridden) pipeline.SetExposure(tonemapExposure.value);

    // --- Bloom ---
    if (bloomEnabled.overridden)   pipeline.GetBloom().SetEnabled(bloomEnabled.value);
    if (bloomThreshold.overridden) pipeline.GetBloom().SetThreshold(bloomThreshold.value);
    if (bloomIntensity.overridden) pipeline.GetBloom().SetIntensity(bloomIntensity.value);

    // --- SSAO ---
    if (ssaoEnabled.overridden) pipeline.GetSSAO().SetEnabled(ssaoEnabled.value);
    if (ssaoRadius.overridden)  pipeline.GetSSAO().SetRadius(ssaoRadius.value);
    if (ssaoBias.overridden)    pipeline.GetSSAO().SetBias(ssaoBias.value);
    if (ssaoPower.overridden)   pipeline.GetSSAO().SetPower(ssaoPower.value);

    // --- DoF ---
    if (dofEnabled.overridden)       pipeline.GetDoF().SetEnabled(dofEnabled.value);
    if (dofFocalDistance.overridden)  pipeline.GetDoF().SetFocalDistance(dofFocalDistance.value);
    if (dofFocalRange.overridden)    pipeline.GetDoF().SetFocalRange(dofFocalRange.value);
    if (dofBokehRadius.overridden)   pipeline.GetDoF().SetBokehRadius(dofBokehRadius.value);

    // --- MotionBlur ---
    if (motionBlurEnabled.overridden)     pipeline.GetMotionBlur().SetEnabled(motionBlurEnabled.value);
    if (motionBlurIntensity.overridden)   pipeline.GetMotionBlur().SetIntensity(motionBlurIntensity.value);
    if (motionBlurSampleCount.overridden) pipeline.GetMotionBlur().SetSampleCount(motionBlurSampleCount.value);

    // --- SSR ---
    if (ssrEnabled.overridden)     pipeline.GetSSR().SetEnabled(ssrEnabled.value);
    if (ssrMaxDistance.overridden)  pipeline.GetSSR().SetMaxDistance(ssrMaxDistance.value);
    if (ssrStepSize.overridden)    pipeline.GetSSR().SetStepSize(ssrStepSize.value);
    if (ssrMaxSteps.overridden)    pipeline.GetSSR().SetMaxSteps(ssrMaxSteps.value);
    if (ssrThickness.overridden)   pipeline.GetSSR().SetThickness(ssrThickness.value);
    if (ssrIntensity.overridden)   pipeline.GetSSR().SetIntensity(ssrIntensity.value);

    // --- Outline ---
    if (outlineEnabled.overridden)        pipeline.GetOutline().SetEnabled(outlineEnabled.value);
    if (outlineDepthThreshold.overridden) pipeline.GetOutline().SetDepthThreshold(outlineDepthThreshold.value);
    if (outlineNormalThreshold.overridden)pipeline.GetOutline().SetNormalThreshold(outlineNormalThreshold.value);
    if (outlineIntensity.overridden)      pipeline.GetOutline().SetIntensity(outlineIntensity.value);

    // --- TAA ---
    if (taaEnabled.overridden)     pipeline.GetTAA().SetEnabled(taaEnabled.value);
    if (taaBlendFactor.overridden) pipeline.GetTAA().SetBlendFactor(taaBlendFactor.value);
    if (taaSharpness.overridden)   pipeline.GetTAA().SetSharpness(taaSharpness.value);

    // --- VolumetricLight ---
    if (volumetricLightEnabled.overridden)   pipeline.GetVolumetricLight().SetEnabled(volumetricLightEnabled.value);
    if (volumetricLightIntensity.overridden) pipeline.GetVolumetricLight().SetIntensity(volumetricLightIntensity.value);
    if (volumetricLightDensity.overridden)   pipeline.GetVolumetricLight().SetDensity(volumetricLightDensity.value);
    if (volumetricLightDecay.overridden)     pipeline.GetVolumetricLight().SetDecay(volumetricLightDecay.value);

    // --- AutoExposure ---
    if (autoExposureEnabled.overridden)  pipeline.GetAutoExposure().SetEnabled(autoExposureEnabled.value);
    if (autoExposureSpeed.overridden)    pipeline.GetAutoExposure().SetAdaptationSpeed(autoExposureSpeed.value);
    if (autoExposureMin.overridden)      pipeline.GetAutoExposure().SetMinExposure(autoExposureMin.value);
    if (autoExposureMax.overridden)      pipeline.GetAutoExposure().SetMaxExposure(autoExposureMax.value);
    if (autoExposureKeyValue.overridden) pipeline.GetAutoExposure().SetKeyValue(autoExposureKeyValue.value);

    // --- ContactShadows ---
    if (contactShadowsEnabled.overridden)     pipeline.GetContactShadows().SetEnabled(contactShadowsEnabled.value);
    if (contactShadowsMaxDistance.overridden)  pipeline.GetContactShadows().SetMaxDistance(contactShadowsMaxDistance.value);
    if (contactShadowsStepCount.overridden)   pipeline.GetContactShadows().SetStepCount(contactShadowsStepCount.value);
    if (contactShadowsThickness.overridden)   pipeline.GetContactShadows().SetThickness(contactShadowsThickness.value);
    if (contactShadowsIntensity.overridden)   pipeline.GetContactShadows().SetIntensity(contactShadowsIntensity.value);

    // --- Vignette / ChromaticAberration ---
    if (vignetteEnabled.overridden)             pipeline.SetVignetteEnabled(vignetteEnabled.value);
    if (vignetteIntensity.overridden)           pipeline.SetVignetteIntensity(vignetteIntensity.value);
    if (chromaticAberrationStrength.overridden) pipeline.SetChromaticAberration(chromaticAberrationStrength.value);

    // --- ColorGrading ---
    if (colorGradingEnabled.overridden)     pipeline.SetColorGradingEnabled(colorGradingEnabled.value);
    if (colorGradingContrast.overridden)    pipeline.SetContrast(colorGradingContrast.value);
    if (colorGradingSaturation.overridden)  pipeline.SetSaturation(colorGradingSaturation.value);
    if (colorGradingTemperature.overridden) pipeline.SetTemperature(colorGradingTemperature.value);

    // --- FXAA ---
    if (fxaaEnabled.overridden) pipeline.SetFXAAEnabled(fxaaEnabled.value);

    // --- LensFlare ---
    if (lensFlareEnabled.overridden)         pipeline.GetLensFlare().SetEnabled(lensFlareEnabled.value);
    if (lensFlareIntensity.overridden)         pipeline.GetLensFlare().SetIntensity(lensFlareIntensity.value);
    if (lensFlareThreshold.overridden)         pipeline.GetLensFlare().SetThreshold(lensFlareThreshold.value);
    if (lensFlareApertureBlades.overridden)
    {
        auto s = pipeline.GetLensFlare().GetSettings();
        s.apertureBlades = lensFlareApertureBlades.value;
        pipeline.GetLensFlare().SetSettings(s);
    }
    if (lensFlareStarburstStrength.overridden)
    {
        auto s = pipeline.GetLensFlare().GetSettings();
        s.starburstStrength = lensFlareStarburstStrength.value;
        pipeline.GetLensFlare().SetSettings(s);
    }

    // --- Fog (Renderer3D) ---
    if (renderer && fogMode.overridden)
    {
        Vector3 color = fogColor.overridden ? fogColor.value : Vector3{0.6f, 0.65f, 0.7f};
        float start    = fogStart.overridden ? fogStart.value : 50.0f;
        float end      = fogEnd.overridden ? fogEnd.value : 200.0f;
        float density  = fogDensity.overridden ? fogDensity.value : 0.01f;
        renderer->SetFog(fogMode.value, color, start, end, density);
    }

    // --- Shadows (Renderer3D + CSM) ---
    if (renderer)
    {
        if (shadowEnabled.overridden)
            renderer->SetShadowEnabled(shadowEnabled.value);

        auto& csm = renderer->GetCSM();
        if (cascadeSplits.overridden)
        {
            const auto& s = cascadeSplits.value;
            csm.SetCascadeSplits(s[0], s[1], s[2], s[3]);
        }
        if (cascadeBlendEnabled.overridden) csm.SetCascadeBlendEnabled(cascadeBlendEnabled.value);
        if (cascadeBlendWidth.overridden)   csm.SetCascadeBlendWidth(cascadeBlendWidth.value);
        if (pcssEnabled.overridden)         csm.SetPCSSEnabled(pcssEnabled.value);
        if (pcssLightSize.overridden)       csm.SetLightSize(pcssLightSize.value);
    }

    // --- RT Reflections ---
    if (auto* rt = pipeline.GetRTReflections())
    {
        if (rtReflectionsEnabled.overridden)       rt->SetEnabled(rtReflectionsEnabled.value);
        if (rtReflectionsMaxDistance.overridden)    rt->SetMaxDistance(rtReflectionsMaxDistance.value);
        if (rtReflectionsIntensity.overridden)     rt->SetIntensity(rtReflectionsIntensity.value);
        if (rtReflectionsShadowSamples.overridden) rt->SetShadowSamples(rtReflectionsShadowSamples.value);
    }

    // --- RT GI ---
    if (auto* gi = pipeline.GetRTGI())
    {
        if (rtgiEnabled.overridden)           gi->SetEnabled(rtgiEnabled.value);
        if (rtgiIntensity.overridden)         gi->SetIntensity(rtgiIntensity.value);
        if (rtgiMaxDistance.overridden)        gi->SetMaxDistance(rtgiMaxDistance.value);
        if (rtgiTemporalAlpha.overridden)     gi->SetTemporalAlpha(rtgiTemporalAlpha.value);
        if (rtgiSpatialIterations.overridden) gi->SetSpatialIterations(rtgiSpatialIterations.value);
    }

    // --- SSGI ---
    if (ssgiEnabled.overridden)     pipeline.GetSSGI().SetEnabled(ssgiEnabled.value);
    if (ssgiIntensity.overridden)   pipeline.GetSSGI().SetIntensity(ssgiIntensity.value);
    if (ssgiMaxDistance.overridden)  pipeline.GetSSGI().SetMaxDistance(ssgiMaxDistance.value);
    if (ssgiSampleCount.overridden) pipeline.GetSSGI().SetSampleCount(ssgiSampleCount.value);
    if (ssgiThickness.overridden)   pipeline.GetSSGI().SetThickness(ssgiThickness.value);
}

// ============================================================================
// JSON helpers
// ============================================================================

static const char* TonemapModeToString(TonemapMode m)
{
    switch (m)
    {
    case TonemapMode::Reinhard:   return "Reinhard";
    case TonemapMode::ACES:       return "ACES";
    case TonemapMode::Uncharted2: return "Uncharted2";
    }
    return "ACES";
}

static TonemapMode StringToTonemapMode(const gx::String& s)
{
    if (s == "Reinhard")   return TonemapMode::Reinhard;
    if (s == "Uncharted2") return TonemapMode::Uncharted2;
    return TonemapMode::ACES;
}

static const char* FogModeToString(FogMode m)
{
    switch (m)
    {
    case FogMode::None:   return "None";
    case FogMode::Linear: return "Linear";
    case FogMode::Exp:    return "Exp";
    case FogMode::Exp2:   return "Exp2";
    }
    return "None";
}

static FogMode StringToFogMode(const gx::String& s)
{
    if (s == "Linear") return FogMode::Linear;
    if (s == "Exp")    return FogMode::Exp;
    if (s == "Exp2")   return FogMode::Exp2;
    return FogMode::None;
}

// ============================================================================
// JSON Load helpers
// ============================================================================

static void LoadFloat(const json& obj, const char* key, Override<float>& out)
{
    if (obj.contains(key)) out.Set(obj[key].get<float>());
}

static void LoadInt(const json& obj, const char* key, Override<int>& out)
{
    if (obj.contains(key)) out.Set(obj[key].get<int>());
}

static void LoadUint(const json& obj, const char* key, Override<uint32_t>& out)
{
    if (obj.contains(key)) out.Set(obj[key].get<uint32_t>());
}

static void LoadBool(const json& obj, const char* key, Override<bool>& out)
{
    if (obj.contains(key)) out.Set(obj[key].get<bool>());
}

static void LoadFloat3(const json& obj, const char* key, Override<Vector3>& out)
{
    if (obj.contains(key) && obj[key].is_array() && obj[key].size() >= 3)
    {
        auto& a = obj[key];
        out.Set(Vector3{a[0].get<float>(), a[1].get<float>(), a[2].get<float>()});
    }
}

static void LoadFloat4Array(const json& obj, const char* key, Override<gx::Array<float,4>>& out)
{
    if (obj.contains(key) && obj[key].is_array() && obj[key].size() >= 4)
    {
        auto& a = obj[key];
        out.Set(gx::Array<float,4>{a[0].get<float>(), a[1].get<float>(), a[2].get<float>(), a[3].get<float>()});
    }
}

// ============================================================================
// JSON Save helpers
// ============================================================================

static void SaveFloat(json& obj, const char* key, const Override<float>& v)
{
    if (v.overridden) obj[key] = v.value;
}

static void SaveInt(json& obj, const char* key, const Override<int>& v)
{
    if (v.overridden) obj[key] = v.value;
}

static void SaveUint(json& obj, const char* key, const Override<uint32_t>& v)
{
    if (v.overridden) obj[key] = v.value;
}

static void SaveBool(json& obj, const char* key, const Override<bool>& v)
{
    if (v.overridden) obj[key] = v.value;
}

static void SaveFloat3(json& obj, const char* key, const Override<Vector3>& v)
{
    if (v.overridden) obj[key] = { v.value.x, v.value.y, v.value.z };
}

static void SaveFloat4Array(json& obj, const char* key, const Override<gx::Array<float,4>>& v)
{
    if (v.overridden) obj[key] = { v.value[0], v.value[1], v.value[2], v.value[3] };
}

// ============================================================================
// RenderProfile::Load
// ============================================================================

bool RenderProfile::Load(const gx::String& path, RenderProfile& out)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        GX_LOG_WARN("RenderProfile: File not found: %s", path.c_str());
        return false;
    }

    json root;
    try
    {
        ifs >> root;
    }
    catch (const json::parse_error& e)
    {
        GX_LOG_ERROR("RenderProfile: JSON parse error: %s", e.what());
        return false;
    }

    out = RenderProfile{};

    // Tonemapping
    if (root.contains("tonemapping"))
    {
        auto& t = root["tonemapping"];
        if (t.contains("mode")) out.tonemapMode.Set(StringToTonemapMode(t["mode"].get<gx::String>()));
        LoadFloat(t, "exposure", out.tonemapExposure);
    }

    // Bloom
    if (root.contains("bloom"))
    {
        auto& b = root["bloom"];
        LoadBool(b, "enabled", out.bloomEnabled);
        LoadFloat(b, "threshold", out.bloomThreshold);
        LoadFloat(b, "intensity", out.bloomIntensity);
    }

    // SSAO
    if (root.contains("ssao"))
    {
        auto& s = root["ssao"];
        LoadBool(s, "enabled", out.ssaoEnabled);
        LoadFloat(s, "radius", out.ssaoRadius);
        LoadFloat(s, "bias", out.ssaoBias);
        LoadFloat(s, "power", out.ssaoPower);
    }

    // DoF
    if (root.contains("dof"))
    {
        auto& d = root["dof"];
        LoadBool(d, "enabled", out.dofEnabled);
        LoadFloat(d, "focalDistance", out.dofFocalDistance);
        LoadFloat(d, "focalRange", out.dofFocalRange);
        LoadFloat(d, "bokehRadius", out.dofBokehRadius);
    }

    // MotionBlur
    if (root.contains("motionBlur"))
    {
        auto& m = root["motionBlur"];
        LoadBool(m, "enabled", out.motionBlurEnabled);
        LoadFloat(m, "intensity", out.motionBlurIntensity);
        LoadInt(m, "sampleCount", out.motionBlurSampleCount);
    }

    // SSR
    if (root.contains("ssr"))
    {
        auto& s = root["ssr"];
        LoadBool(s, "enabled", out.ssrEnabled);
        LoadFloat(s, "maxDistance", out.ssrMaxDistance);
        LoadFloat(s, "stepSize", out.ssrStepSize);
        LoadInt(s, "maxSteps", out.ssrMaxSteps);
        LoadFloat(s, "thickness", out.ssrThickness);
        LoadFloat(s, "intensity", out.ssrIntensity);
    }

    // Outline
    if (root.contains("outline"))
    {
        auto& o = root["outline"];
        LoadBool(o, "enabled", out.outlineEnabled);
        LoadFloat(o, "depthThreshold", out.outlineDepthThreshold);
        LoadFloat(o, "normalThreshold", out.outlineNormalThreshold);
        LoadFloat(o, "intensity", out.outlineIntensity);
    }

    // TAA
    if (root.contains("taa"))
    {
        auto& t = root["taa"];
        LoadBool(t, "enabled", out.taaEnabled);
        LoadFloat(t, "blendFactor", out.taaBlendFactor);
        LoadFloat(t, "sharpness", out.taaSharpness);
    }

    // VolumetricLight
    if (root.contains("volumetricLight"))
    {
        auto& vl = root["volumetricLight"];
        LoadBool(vl, "enabled", out.volumetricLightEnabled);
        LoadFloat(vl, "intensity", out.volumetricLightIntensity);
        LoadFloat(vl, "density", out.volumetricLightDensity);
        LoadFloat(vl, "decay", out.volumetricLightDecay);
    }

    // AutoExposure
    if (root.contains("autoExposure"))
    {
        auto& ae = root["autoExposure"];
        LoadBool(ae, "enabled", out.autoExposureEnabled);
        LoadFloat(ae, "speed", out.autoExposureSpeed);
        LoadFloat(ae, "min", out.autoExposureMin);
        LoadFloat(ae, "max", out.autoExposureMax);
        LoadFloat(ae, "keyValue", out.autoExposureKeyValue);
    }

    // ContactShadows
    if (root.contains("contactShadows"))
    {
        auto& cs = root["contactShadows"];
        LoadBool(cs, "enabled", out.contactShadowsEnabled);
        LoadFloat(cs, "maxDistance", out.contactShadowsMaxDistance);
        LoadInt(cs, "stepCount", out.contactShadowsStepCount);
        LoadFloat(cs, "thickness", out.contactShadowsThickness);
        LoadFloat(cs, "intensity", out.contactShadowsIntensity);
    }

    // Vignette
    if (root.contains("vignette"))
    {
        auto& v = root["vignette"];
        LoadBool(v, "enabled", out.vignetteEnabled);
        LoadFloat(v, "intensity", out.vignetteIntensity);
        LoadFloat(v, "chromaticAberration", out.chromaticAberrationStrength);
    }

    // ColorGrading
    if (root.contains("colorGrading"))
    {
        auto& cg = root["colorGrading"];
        LoadBool(cg, "enabled", out.colorGradingEnabled);
        LoadFloat(cg, "contrast", out.colorGradingContrast);
        LoadFloat(cg, "saturation", out.colorGradingSaturation);
        LoadFloat(cg, "temperature", out.colorGradingTemperature);
    }

    // FXAA
    if (root.contains("fxaa"))
    {
        auto& f = root["fxaa"];
        LoadBool(f, "enabled", out.fxaaEnabled);
    }

    // LensFlare
    if (root.contains("lensFlare"))
    {
        auto& lf = root["lensFlare"];
        LoadBool(lf, "enabled", out.lensFlareEnabled);
        LoadFloat(lf, "intensity", out.lensFlareIntensity);
        LoadFloat(lf, "threshold", out.lensFlareThreshold);
        LoadFloat(lf, "apertureBlades", out.lensFlareApertureBlades);
        LoadFloat(lf, "starburstStrength", out.lensFlareStarburstStrength);
    }

    // Fog
    if (root.contains("fog"))
    {
        auto& fg = root["fog"];
        if (fg.contains("mode")) out.fogMode.Set(StringToFogMode(fg["mode"].get<gx::String>()));
        LoadFloat3(fg, "color", out.fogColor);
        LoadFloat(fg, "start", out.fogStart);
        LoadFloat(fg, "end", out.fogEnd);
        LoadFloat(fg, "density", out.fogDensity);
    }

    // Shadows
    if (root.contains("shadows"))
    {
        auto& sh = root["shadows"];
        LoadBool(sh, "enabled", out.shadowEnabled);
        LoadFloat4Array(sh, "cascadeSplits", out.cascadeSplits);
        LoadBool(sh, "cascadeBlendEnabled", out.cascadeBlendEnabled);
        LoadFloat(sh, "cascadeBlendWidth", out.cascadeBlendWidth);
        LoadBool(sh, "pcssEnabled", out.pcssEnabled);
        LoadFloat(sh, "lightSize", out.pcssLightSize);
    }

    // Ambient
    if (root.contains("ambient"))
    {
        auto& am = root["ambient"];
        LoadFloat3(am, "color", out.ambientColor);
    }

    // RT Reflections
    if (root.contains("rtReflections"))
    {
        auto& rt = root["rtReflections"];
        LoadBool(rt, "enabled", out.rtReflectionsEnabled);
        LoadFloat(rt, "maxDistance", out.rtReflectionsMaxDistance);
        LoadFloat(rt, "intensity", out.rtReflectionsIntensity);
        LoadUint(rt, "shadowSamples", out.rtReflectionsShadowSamples);
    }

    // RT GI
    if (root.contains("rtgi"))
    {
        auto& gi = root["rtgi"];
        LoadBool(gi, "enabled", out.rtgiEnabled);
        LoadFloat(gi, "intensity", out.rtgiIntensity);
        LoadFloat(gi, "maxDistance", out.rtgiMaxDistance);
        LoadFloat(gi, "temporalAlpha", out.rtgiTemporalAlpha);
        LoadInt(gi, "spatialIterations", out.rtgiSpatialIterations);
    }

    // SSGI
    if (root.contains("ssgi"))
    {
        auto& sg = root["ssgi"];
        LoadBool(sg, "enabled", out.ssgiEnabled);
        LoadFloat(sg, "intensity", out.ssgiIntensity);
        LoadFloat(sg, "maxDistance", out.ssgiMaxDistance);
        LoadInt(sg, "sampleCount", out.ssgiSampleCount);
        LoadFloat(sg, "thickness", out.ssgiThickness);
    }

    GX_LOG_INFO("RenderProfile: Loaded from %s", path.c_str());
    return true;
}

// ============================================================================
// RenderProfile::Save
// ============================================================================

bool RenderProfile::Save(const gx::String& path, const RenderProfile& p)
{
    json root;

    // Tonemapping
    {
        json obj;
        if (p.tonemapMode.overridden) obj["mode"] = TonemapModeToString(p.tonemapMode.value);
        SaveFloat(obj, "exposure", p.tonemapExposure);
        if (!obj.empty()) root["tonemapping"] = obj;
    }

    // Bloom
    {
        json obj;
        SaveBool(obj, "enabled", p.bloomEnabled);
        SaveFloat(obj, "threshold", p.bloomThreshold);
        SaveFloat(obj, "intensity", p.bloomIntensity);
        if (!obj.empty()) root["bloom"] = obj;
    }

    // SSAO
    {
        json obj;
        SaveBool(obj, "enabled", p.ssaoEnabled);
        SaveFloat(obj, "radius", p.ssaoRadius);
        SaveFloat(obj, "bias", p.ssaoBias);
        SaveFloat(obj, "power", p.ssaoPower);
        if (!obj.empty()) root["ssao"] = obj;
    }

    // DoF
    {
        json obj;
        SaveBool(obj, "enabled", p.dofEnabled);
        SaveFloat(obj, "focalDistance", p.dofFocalDistance);
        SaveFloat(obj, "focalRange", p.dofFocalRange);
        SaveFloat(obj, "bokehRadius", p.dofBokehRadius);
        if (!obj.empty()) root["dof"] = obj;
    }

    // MotionBlur
    {
        json obj;
        SaveBool(obj, "enabled", p.motionBlurEnabled);
        SaveFloat(obj, "intensity", p.motionBlurIntensity);
        SaveInt(obj, "sampleCount", p.motionBlurSampleCount);
        if (!obj.empty()) root["motionBlur"] = obj;
    }

    // SSR
    {
        json obj;
        SaveBool(obj, "enabled", p.ssrEnabled);
        SaveFloat(obj, "maxDistance", p.ssrMaxDistance);
        SaveFloat(obj, "stepSize", p.ssrStepSize);
        SaveInt(obj, "maxSteps", p.ssrMaxSteps);
        SaveFloat(obj, "thickness", p.ssrThickness);
        SaveFloat(obj, "intensity", p.ssrIntensity);
        if (!obj.empty()) root["ssr"] = obj;
    }

    // Outline
    {
        json obj;
        SaveBool(obj, "enabled", p.outlineEnabled);
        SaveFloat(obj, "depthThreshold", p.outlineDepthThreshold);
        SaveFloat(obj, "normalThreshold", p.outlineNormalThreshold);
        SaveFloat(obj, "intensity", p.outlineIntensity);
        if (!obj.empty()) root["outline"] = obj;
    }

    // TAA
    {
        json obj;
        SaveBool(obj, "enabled", p.taaEnabled);
        SaveFloat(obj, "blendFactor", p.taaBlendFactor);
        SaveFloat(obj, "sharpness", p.taaSharpness);
        if (!obj.empty()) root["taa"] = obj;
    }

    // VolumetricLight
    {
        json obj;
        SaveBool(obj, "enabled", p.volumetricLightEnabled);
        SaveFloat(obj, "intensity", p.volumetricLightIntensity);
        SaveFloat(obj, "density", p.volumetricLightDensity);
        SaveFloat(obj, "decay", p.volumetricLightDecay);
        if (!obj.empty()) root["volumetricLight"] = obj;
    }

    // AutoExposure
    {
        json obj;
        SaveBool(obj, "enabled", p.autoExposureEnabled);
        SaveFloat(obj, "speed", p.autoExposureSpeed);
        SaveFloat(obj, "min", p.autoExposureMin);
        SaveFloat(obj, "max", p.autoExposureMax);
        SaveFloat(obj, "keyValue", p.autoExposureKeyValue);
        if (!obj.empty()) root["autoExposure"] = obj;
    }

    // ContactShadows
    {
        json obj;
        SaveBool(obj, "enabled", p.contactShadowsEnabled);
        SaveFloat(obj, "maxDistance", p.contactShadowsMaxDistance);
        SaveInt(obj, "stepCount", p.contactShadowsStepCount);
        SaveFloat(obj, "thickness", p.contactShadowsThickness);
        SaveFloat(obj, "intensity", p.contactShadowsIntensity);
        if (!obj.empty()) root["contactShadows"] = obj;
    }

    // Vignette
    {
        json obj;
        SaveBool(obj, "enabled", p.vignetteEnabled);
        SaveFloat(obj, "intensity", p.vignetteIntensity);
        SaveFloat(obj, "chromaticAberration", p.chromaticAberrationStrength);
        if (!obj.empty()) root["vignette"] = obj;
    }

    // ColorGrading
    {
        json obj;
        SaveBool(obj, "enabled", p.colorGradingEnabled);
        SaveFloat(obj, "contrast", p.colorGradingContrast);
        SaveFloat(obj, "saturation", p.colorGradingSaturation);
        SaveFloat(obj, "temperature", p.colorGradingTemperature);
        if (!obj.empty()) root["colorGrading"] = obj;
    }

    // FXAA
    {
        json obj;
        SaveBool(obj, "enabled", p.fxaaEnabled);
        if (!obj.empty()) root["fxaa"] = obj;
    }

    // LensFlare
    {
        json obj;
        SaveBool(obj, "enabled", p.lensFlareEnabled);
        SaveFloat(obj, "intensity", p.lensFlareIntensity);
        SaveFloat(obj, "threshold", p.lensFlareThreshold);
        SaveFloat(obj, "apertureBlades", p.lensFlareApertureBlades);
        SaveFloat(obj, "starburstStrength", p.lensFlareStarburstStrength);
        if (!obj.empty()) root["lensFlare"] = obj;
    }

    // Fog
    {
        json obj;
        if (p.fogMode.overridden) obj["mode"] = FogModeToString(p.fogMode.value);
        SaveFloat3(obj, "color", p.fogColor);
        SaveFloat(obj, "start", p.fogStart);
        SaveFloat(obj, "end", p.fogEnd);
        SaveFloat(obj, "density", p.fogDensity);
        if (!obj.empty()) root["fog"] = obj;
    }

    // Shadows
    {
        json obj;
        SaveBool(obj, "enabled", p.shadowEnabled);
        SaveFloat4Array(obj, "cascadeSplits", p.cascadeSplits);
        SaveBool(obj, "cascadeBlendEnabled", p.cascadeBlendEnabled);
        SaveFloat(obj, "cascadeBlendWidth", p.cascadeBlendWidth);
        SaveBool(obj, "pcssEnabled", p.pcssEnabled);
        SaveFloat(obj, "lightSize", p.pcssLightSize);
        if (!obj.empty()) root["shadows"] = obj;
    }

    // Ambient
    {
        json obj;
        SaveFloat3(obj, "color", p.ambientColor);
        if (!obj.empty()) root["ambient"] = obj;
    }

    // RT Reflections
    {
        json obj;
        SaveBool(obj, "enabled", p.rtReflectionsEnabled);
        SaveFloat(obj, "maxDistance", p.rtReflectionsMaxDistance);
        SaveFloat(obj, "intensity", p.rtReflectionsIntensity);
        SaveUint(obj, "shadowSamples", p.rtReflectionsShadowSamples);
        if (!obj.empty()) root["rtReflections"] = obj;
    }

    // RT GI
    {
        json obj;
        SaveBool(obj, "enabled", p.rtgiEnabled);
        SaveFloat(obj, "intensity", p.rtgiIntensity);
        SaveFloat(obj, "maxDistance", p.rtgiMaxDistance);
        SaveFloat(obj, "temporalAlpha", p.rtgiTemporalAlpha);
        SaveInt(obj, "spatialIterations", p.rtgiSpatialIterations);
        if (!obj.empty()) root["rtgi"] = obj;
    }

    // SSGI
    {
        json obj;
        SaveBool(obj, "enabled", p.ssgiEnabled);
        SaveFloat(obj, "intensity", p.ssgiIntensity);
        SaveFloat(obj, "maxDistance", p.ssgiMaxDistance);
        SaveInt(obj, "sampleCount", p.ssgiSampleCount);
        SaveFloat(obj, "thickness", p.ssgiThickness);
        if (!obj.empty()) root["ssgi"] = obj;
    }

    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        GX_LOG_ERROR("RenderProfile: Cannot open for writing: %s", path.c_str());
        return false;
    }

    ofs << root.dump(2);
    ofs.close();

    GX_LOG_INFO("RenderProfile: Saved to %s", path.c_str());
    return true;
}

// ============================================================================
// Factory Presets
// ============================================================================

RenderProfile RenderProfile::CreateDefault()
{
    RenderProfile p;

    // Tonemapping
    p.tonemapMode.Set(TonemapMode::ACES);
    p.tonemapExposure.Set(1.0f);

    // Bloom
    p.bloomEnabled.Set(true);
    p.bloomThreshold.Set(2.0f);
    p.bloomIntensity.Set(0.5f);

    // SSAO
    p.ssaoEnabled.Set(true);
    p.ssaoRadius.Set(0.5f);
    p.ssaoBias.Set(0.025f);
    p.ssaoPower.Set(2.0f);

    // DoF
    p.dofEnabled.Set(false);
    p.dofFocalDistance.Set(10.0f);
    p.dofFocalRange.Set(20.0f);
    p.dofBokehRadius.Set(3.0f);

    // MotionBlur
    p.motionBlurEnabled.Set(true);
    p.motionBlurIntensity.Set(0.3f);
    p.motionBlurSampleCount.Set(16);

    // SSR
    p.ssrEnabled.Set(true);
    p.ssrMaxDistance.Set(100.0f);
    p.ssrStepSize.Set(0.5f);
    p.ssrMaxSteps.Set(64);
    p.ssrThickness.Set(0.5f);
    p.ssrIntensity.Set(1.0f);

    // Outline
    p.outlineEnabled.Set(false);
    p.outlineDepthThreshold.Set(0.1f);
    p.outlineNormalThreshold.Set(0.5f);
    p.outlineIntensity.Set(1.0f);

    // TAA
    p.taaEnabled.Set(true);
    p.taaBlendFactor.Set(0.1f);
    p.taaSharpness.Set(0.5f);

    // VolumetricLight
    p.volumetricLightEnabled.Set(true);
    p.volumetricLightIntensity.Set(0.8f);
    p.volumetricLightDensity.Set(0.5f);
    p.volumetricLightDecay.Set(0.95f);

    // AutoExposure
    p.autoExposureEnabled.Set(true);
    p.autoExposureSpeed.Set(1.5f);
    p.autoExposureMin.Set(0.5f);
    p.autoExposureMax.Set(2.0f);
    p.autoExposureKeyValue.Set(0.18f);

    // ContactShadows
    p.contactShadowsEnabled.Set(true);
    p.contactShadowsMaxDistance.Set(0.5f);
    p.contactShadowsStepCount.Set(16);
    p.contactShadowsThickness.Set(0.05f);
    p.contactShadowsIntensity.Set(1.0f);

    // Vignette / ChromaticAberration
    p.vignetteEnabled.Set(true);
    p.vignetteIntensity.Set(0.3f);
    p.chromaticAberrationStrength.Set(0.003f);

    // ColorGrading
    p.colorGradingEnabled.Set(true);
    p.colorGradingContrast.Set(1.0f);
    p.colorGradingSaturation.Set(1.0f);
    p.colorGradingTemperature.Set(0.0f);

    // FXAA (TAA が有効なので不要)
    p.fxaaEnabled.Set(false);

    // LensFlare (デフォルト OFF)
    p.lensFlareEnabled.Set(false);
    p.lensFlareIntensity.Set(1.0f);
    p.lensFlareThreshold.Set(1.5f);
    p.lensFlareApertureBlades.Set(6.0f);
    p.lensFlareStarburstStrength.Set(0.3f);

    // Fog (Exponential Height Fog)
    p.fogMode.Set(FogMode::Exp);
    p.fogColor.Set(Vector3{0.6f, 0.65f, 0.7f});
    p.fogStart.Set(50.0f);
    p.fogEnd.Set(200.0f);
    p.fogDensity.Set(0.005f);

    // Shadows
    p.shadowEnabled.Set(true);
    p.cascadeSplits.Set(gx::Array<float,4>{0.05f, 0.15f, 0.4f, 1.0f});
    p.cascadeBlendEnabled.Set(true);
    p.cascadeBlendWidth.Set(0.1f);
    p.pcssEnabled.Set(true);
    p.pcssLightSize.Set(1.5f);

    // Ambient
    p.ambientColor.Set(Vector3{0.1f, 0.1f, 0.15f});

    // RT Reflections (Lumen Reflections 相当; Apply() は null 安全)
    p.rtReflectionsEnabled.Set(true);
    p.rtReflectionsMaxDistance.Set(50.0f);
    p.rtReflectionsIntensity.Set(1.0f);
    p.rtReflectionsShadowSamples.Set(4);

    // RT GI (Lumen GI 相当; Apply() は null 安全)
    p.rtgiEnabled.Set(true);
    p.rtgiIntensity.Set(1.0f);
    p.rtgiMaxDistance.Set(30.0f);
    p.rtgiTemporalAlpha.Set(0.1f);
    p.rtgiSpatialIterations.Set(3);

    // SSGI (RTGI フォールバック用; Resolve() で RTGI と排他)
    p.ssgiEnabled.Set(true);
    p.ssgiIntensity.Set(0.5f);
    p.ssgiMaxDistance.Set(50.0f);
    p.ssgiSampleCount.Set(32);
    p.ssgiThickness.Set(0.5f);

    return p;
}

RenderProfile RenderProfile::CreateIndoor()
{
    RenderProfile p;

    // SSAO 強め
    p.ssaoEnabled.Set(true);
    p.ssaoRadius.Set(0.8f);
    p.ssaoBias.Set(0.02f);
    p.ssaoPower.Set(3.0f);

    // 暗め露出
    p.tonemapMode.Set(TonemapMode::ACES);
    p.tonemapExposure.Set(0.8f);

    // フォグ無し
    p.fogMode.Set(FogMode::None);

    // 暗めアンビエント
    p.ambientColor.Set(Vector3{0.05f, 0.05f, 0.08f});

    // コンタクトシャドウ有効
    p.contactShadowsEnabled.Set(true);
    p.contactShadowsMaxDistance.Set(0.3f);
    p.contactShadowsStepCount.Set(24);
    p.contactShadowsThickness.Set(0.03f);
    p.contactShadowsIntensity.Set(1.0f);

    return p;
}

RenderProfile RenderProfile::CreateOutdoor()
{
    RenderProfile p;

    // フォグ
    p.fogMode.Set(FogMode::Exp);
    p.fogColor.Set(Vector3{0.6f, 0.65f, 0.7f});
    p.fogDensity.Set(0.005f);

    // ボリューメトリックライト
    p.volumetricLightEnabled.Set(true);
    p.volumetricLightIntensity.Set(1.5f);
    p.volumetricLightDensity.Set(0.8f);
    p.volumetricLightDecay.Set(0.95f);

    // PCSS
    p.pcssEnabled.Set(true);
    p.pcssLightSize.Set(2.0f);
    p.cascadeSplits.Set(gx::Array<float,4>{0.05f, 0.15f, 0.4f, 1.0f});

    // 明るめ露出
    p.tonemapExposure.Set(1.2f);

    // オートエクスポージャー
    p.autoExposureEnabled.Set(true);
    p.autoExposureSpeed.Set(1.5f);
    p.autoExposureMin.Set(0.2f);
    p.autoExposureMax.Set(5.0f);
    p.autoExposureKeyValue.Set(0.2f);

    return p;
}

RenderProfile RenderProfile::CreateCinematic()
{
    RenderProfile p;

    // DoF
    p.dofEnabled.Set(true);
    p.dofFocalDistance.Set(5.0f);
    p.dofFocalRange.Set(3.0f);
    p.dofBokehRadius.Set(10.0f);

    // モーションブラー
    p.motionBlurEnabled.Set(true);
    p.motionBlurIntensity.Set(0.8f);
    p.motionBlurSampleCount.Set(24);

    // カラーグレーディング
    p.colorGradingEnabled.Set(true);
    p.colorGradingContrast.Set(1.1f);
    p.colorGradingSaturation.Set(1.2f);
    p.colorGradingTemperature.Set(-0.05f);

    // ビネット
    p.vignetteEnabled.Set(true);
    p.vignetteIntensity.Set(0.6f);
    p.chromaticAberrationStrength.Set(0.005f);

    // フィルムっぽい露出
    p.tonemapMode.Set(TonemapMode::ACES);
    p.tonemapExposure.Set(1.1f);

    // Bloom
    p.bloomEnabled.Set(true);
    p.bloomThreshold.Set(0.8f);
    p.bloomIntensity.Set(0.7f);

    return p;
}

// ============================================================================
// RenderProfileStack
// ============================================================================

uint32_t RenderProfileStack::AddProfile(const gx::String& name, const RenderProfile& profile,
                                         int priority, float weight)
{
    uint32_t id = m_nextId++;
    m_entries.push_back(Entry{id, name, profile, priority, weight});
    return id;
}

void RenderProfileStack::RemoveProfile(uint32_t id)
{
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
                       [id](const Entry& e) { return e.id == id; }),
        m_entries.end());
}

void RenderProfileStack::SetWeight(uint32_t id, float weight)
{
    for (auto& e : m_entries)
    {
        if (e.id == id) { e.weight = weight; return; }
    }
}

void RenderProfileStack::SetPriority(uint32_t id, int priority)
{
    for (auto& e : m_entries)
    {
        if (e.id == id) { e.priority = priority; return; }
    }
}

RenderProfile RenderProfileStack::Resolve() const
{
    if (m_entries.empty()) return RenderProfile{};

    // priority 昇順にソート (安定ソート)
    gx::Vector<const Entry*> sorted;
    sorted.reserve(m_entries.size());
    for (const auto& e : m_entries)
        sorted.push_back(&e);

    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Entry* a, const Entry* b) { return a->priority < b->priority; });

    RenderProfile resolved;

    for (const auto* entry : sorted)
    {
        if (entry->weight >= 1.0f)
        {
            // weight=1.0 → 完全マージ
            resolved.MergeFrom(entry->profile);
        }
        else if (entry->weight > 0.0f)
        {
            // weight<1.0 → マージ後に既存結果と Lerp
            RenderProfile merged = resolved;
            merged.MergeFrom(entry->profile);
            resolved = RenderProfile::Lerp(resolved, merged, entry->weight);
        }
        // weight <= 0.0 → スキップ
    }

    return resolved;
}

void RenderProfileStack::Apply(PostEffectPipeline& pipeline, Renderer3D* renderer) const
{
    Resolve().Apply(pipeline, renderer);
}

} // namespace gx
