/// @file PostEffectSettings.cpp
/// @brief ポストエフェクト設定のJSON読み書き実装
#include "pch.h"
#include "Graphics/PostEffect/PostEffectSettings.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Core/Logger.h"

#include <fstream>

// nlohmann/json は巨大ヘッダなので翻訳単位を1つに限定
#include "ThirdParty/json.hpp"

using json = nlohmann::json;

namespace GX
{

bool PostEffectSettings::Load(const std::string& filePath, PostEffectPipeline& pipeline)
{
    std::ifstream ifs(filePath);
    if (!ifs.is_open())
    {
        GX_LOG_WARN("PostEffectSettings: File not found: %s", filePath.c_str());
        return false;
    }

    json root;
    try
    {
        ifs >> root;
    }
    catch (const json::parse_error& e)
    {
        GX_LOG_ERROR("PostEffectSettings: JSON parse error: %s", e.what());
        return false;
    }

    if (!root.contains("postEffects"))
    {
        GX_LOG_WARN("PostEffectSettings: Missing 'postEffects' key");
        return false;
    }

    auto& pe = root["postEffects"];

    // Tonemapping
    if (pe.contains("tonemapping"))
    {
        auto& tm = pe["tonemapping"];
        if (tm.contains("mode"))
        {
            std::string mode = tm["mode"].get<std::string>();
            if (mode == "Reinhard") pipeline.SetTonemapMode(TonemapMode::Reinhard);
            else if (mode == "ACES") pipeline.SetTonemapMode(TonemapMode::ACES);
            else if (mode == "Uncharted2") pipeline.SetTonemapMode(TonemapMode::Uncharted2);
        }
        if (tm.contains("exposure")) pipeline.SetExposure(tm["exposure"].get<float>());
    }

    // Bloom
    if (pe.contains("bloom"))
    {
        auto& b = pe["bloom"];
        if (b.contains("enabled"))   pipeline.GetBloom().SetEnabled(b["enabled"].get<bool>());
        if (b.contains("threshold")) pipeline.GetBloom().SetThreshold(b["threshold"].get<float>());
        if (b.contains("intensity")) pipeline.GetBloom().SetIntensity(b["intensity"].get<float>());
    }

    // FXAA
    if (pe.contains("fxaa"))
    {
        auto& f = pe["fxaa"];
        if (f.contains("enabled")) pipeline.SetFXAAEnabled(f["enabled"].get<bool>());
    }

    // SSAO
    if (pe.contains("ssao"))
    {
        auto& s = pe["ssao"];
        if (s.contains("enabled")) pipeline.GetSSAO().SetEnabled(s["enabled"].get<bool>());
        if (s.contains("radius"))  pipeline.GetSSAO().SetRadius(s["radius"].get<float>());
        if (s.contains("bias"))    pipeline.GetSSAO().SetBias(s["bias"].get<float>());
        if (s.contains("power"))   pipeline.GetSSAO().SetPower(s["power"].get<float>());
    }

    // DoF
    if (pe.contains("dof"))
    {
        auto& d = pe["dof"];
        if (d.contains("enabled"))       pipeline.GetDoF().SetEnabled(d["enabled"].get<bool>());
        if (d.contains("focalDistance"))  pipeline.GetDoF().SetFocalDistance(d["focalDistance"].get<float>());
        if (d.contains("focalRange"))    pipeline.GetDoF().SetFocalRange(d["focalRange"].get<float>());
        if (d.contains("bokehRadius"))   pipeline.GetDoF().SetBokehRadius(d["bokehRadius"].get<float>());
    }

    // Motion Blur
    if (pe.contains("motionBlur"))
    {
        auto& m = pe["motionBlur"];
        if (m.contains("enabled"))     pipeline.GetMotionBlur().SetEnabled(m["enabled"].get<bool>());
        if (m.contains("intensity"))   pipeline.GetMotionBlur().SetIntensity(m["intensity"].get<float>());
        if (m.contains("sampleCount")) pipeline.GetMotionBlur().SetSampleCount(m["sampleCount"].get<int>());
    }

    // SSR
    if (pe.contains("ssr"))
    {
        auto& s = pe["ssr"];
        if (s.contains("enabled"))   pipeline.GetSSR().SetEnabled(s["enabled"].get<bool>());
        if (s.contains("maxSteps"))  pipeline.GetSSR().SetMaxSteps(s["maxSteps"].get<int>());
        if (s.contains("intensity")) pipeline.GetSSR().SetIntensity(s["intensity"].get<float>());
    }

    // Outline
    if (pe.contains("outline"))
    {
        auto& o = pe["outline"];
        if (o.contains("enabled"))        pipeline.GetOutline().SetEnabled(o["enabled"].get<bool>());
        if (o.contains("depthThreshold")) pipeline.GetOutline().SetDepthThreshold(o["depthThreshold"].get<float>());
        if (o.contains("normalThreshold"))pipeline.GetOutline().SetNormalThreshold(o["normalThreshold"].get<float>());
        if (o.contains("intensity"))      pipeline.GetOutline().SetIntensity(o["intensity"].get<float>());
    }

    // TAA
    if (pe.contains("taa"))
    {
        auto& t = pe["taa"];
        if (t.contains("enabled"))     pipeline.GetTAA().SetEnabled(t["enabled"].get<bool>());
        if (t.contains("blendFactor")) pipeline.GetTAA().SetBlendFactor(t["blendFactor"].get<float>());
    }

    // AutoExposure
    if (pe.contains("autoExposure"))
    {
        auto& ae = pe["autoExposure"];
        if (ae.contains("enabled")) pipeline.GetAutoExposure().SetEnabled(ae["enabled"].get<bool>());
        if (ae.contains("speed"))   pipeline.GetAutoExposure().SetAdaptationSpeed(ae["speed"].get<float>());
        if (ae.contains("min"))     pipeline.GetAutoExposure().SetMinExposure(ae["min"].get<float>());
        if (ae.contains("max"))     pipeline.GetAutoExposure().SetMaxExposure(ae["max"].get<float>());
        if (ae.contains("keyValue"))pipeline.GetAutoExposure().SetKeyValue(ae["keyValue"].get<float>());
    }

    // VolumetricLight
    if (pe.contains("volumetricLight"))
    {
        auto& vl = pe["volumetricLight"];
        if (vl.contains("enabled"))   pipeline.GetVolumetricLight().SetEnabled(vl["enabled"].get<bool>());
        if (vl.contains("intensity")) pipeline.GetVolumetricLight().SetIntensity(vl["intensity"].get<float>());
        if (vl.contains("density"))   pipeline.GetVolumetricLight().SetDensity(vl["density"].get<float>());
        if (vl.contains("decay"))     pipeline.GetVolumetricLight().SetDecay(vl["decay"].get<float>());
    }

    // Vignette
    if (pe.contains("vignette"))
    {
        auto& v = pe["vignette"];
        if (v.contains("enabled"))              pipeline.SetVignetteEnabled(v["enabled"].get<bool>());
        if (v.contains("intensity"))            pipeline.SetVignetteIntensity(v["intensity"].get<float>());
        if (v.contains("chromaticAberration"))  pipeline.SetChromaticAberration(v["chromaticAberration"].get<float>());
    }

    // Color Grading
    if (pe.contains("colorGrading"))
    {
        auto& cg = pe["colorGrading"];
        if (cg.contains("enabled"))     pipeline.SetColorGradingEnabled(cg["enabled"].get<bool>());
        if (cg.contains("contrast"))    pipeline.SetContrast(cg["contrast"].get<float>());
        if (cg.contains("saturation"))  pipeline.SetSaturation(cg["saturation"].get<float>());
        if (cg.contains("temperature")) pipeline.SetTemperature(cg["temperature"].get<float>());
    }

    GX_LOG_INFO("PostEffectSettings: Loaded from %s", filePath.c_str());
    return true;
}

bool PostEffectSettings::Save(const std::string& filePath, const PostEffectPipeline& pipeline)
{
    json root;
    json pe;

    // Tonemapping
    {
        json tm;
        const char* modeNames[] = { "Reinhard", "ACES", "Uncharted2" };
        tm["mode"]     = modeNames[static_cast<uint32_t>(pipeline.GetTonemapMode())];
        tm["exposure"] = pipeline.GetExposure();
        pe["tonemapping"] = tm;
    }

    // Bloom
    {
        json b;
        b["enabled"]   = pipeline.GetBloom().IsEnabled();
        b["threshold"] = pipeline.GetBloom().GetThreshold();
        b["intensity"] = pipeline.GetBloom().GetIntensity();
        pe["bloom"] = b;
    }

    // FXAA
    {
        json f;
        f["enabled"] = pipeline.IsFXAAEnabled();
        pe["fxaa"] = f;
    }

    // SSAO
    {
        json s;
        s["enabled"] = pipeline.GetSSAO().IsEnabled();
        s["radius"]  = pipeline.GetSSAO().GetRadius();
        s["bias"]    = pipeline.GetSSAO().GetBias();
        s["power"]   = pipeline.GetSSAO().GetPower();
        pe["ssao"] = s;
    }

    // DoF
    {
        json d;
        d["enabled"]       = pipeline.GetDoF().IsEnabled();
        d["focalDistance"]  = pipeline.GetDoF().GetFocalDistance();
        d["focalRange"]    = pipeline.GetDoF().GetFocalRange();
        d["bokehRadius"]   = pipeline.GetDoF().GetBokehRadius();
        pe["dof"] = d;
    }

    // Motion Blur
    {
        json m;
        m["enabled"]     = pipeline.GetMotionBlur().IsEnabled();
        m["intensity"]   = pipeline.GetMotionBlur().GetIntensity();
        m["sampleCount"] = pipeline.GetMotionBlur().GetSampleCount();
        pe["motionBlur"] = m;
    }

    // SSR
    {
        json s;
        s["enabled"]   = pipeline.GetSSR().IsEnabled();
        s["maxSteps"]  = pipeline.GetSSR().GetMaxSteps();
        s["intensity"] = pipeline.GetSSR().GetIntensity();
        pe["ssr"] = s;
    }

    // Outline
    {
        json o;
        o["enabled"]        = pipeline.GetOutline().IsEnabled();
        o["depthThreshold"] = pipeline.GetOutline().GetDepthThreshold();
        o["normalThreshold"]= pipeline.GetOutline().GetNormalThreshold();
        o["intensity"]      = pipeline.GetOutline().GetIntensity();
        pe["outline"] = o;
    }

    // TAA
    {
        json t;
        t["enabled"]     = pipeline.GetTAA().IsEnabled();
        t["blendFactor"] = pipeline.GetTAA().GetBlendFactor();
        pe["taa"] = t;
    }

    // AutoExposure
    {
        json ae;
        ae["enabled"]  = pipeline.GetAutoExposure().IsEnabled();
        ae["speed"]    = pipeline.GetAutoExposure().GetAdaptationSpeed();
        ae["min"]      = pipeline.GetAutoExposure().GetMinExposure();
        ae["max"]      = pipeline.GetAutoExposure().GetMaxExposure();
        ae["keyValue"] = pipeline.GetAutoExposure().GetKeyValue();
        pe["autoExposure"] = ae;
    }

    // VolumetricLight
    {
        json vl;
        vl["enabled"]   = pipeline.GetVolumetricLight().IsEnabled();
        vl["intensity"] = pipeline.GetVolumetricLight().GetIntensity();
        vl["density"]   = pipeline.GetVolumetricLight().GetDensity();
        vl["decay"]     = pipeline.GetVolumetricLight().GetDecay();
        pe["volumetricLight"] = vl;
    }

    // Vignette
    {
        json v;
        v["enabled"]              = pipeline.IsVignetteEnabled();
        v["intensity"]            = pipeline.GetVignetteIntensity();
        v["chromaticAberration"]  = pipeline.GetChromaticAberration();
        pe["vignette"] = v;
    }

    // Color Grading
    {
        json cg;
        cg["enabled"]     = pipeline.IsColorGradingEnabled();
        cg["contrast"]    = pipeline.GetContrast();
        cg["saturation"]  = pipeline.GetSaturation();
        cg["temperature"] = pipeline.GetTemperature();
        pe["colorGrading"] = cg;
    }

    root["postEffects"] = pe;

    std::ofstream ofs(filePath);
    if (!ofs.is_open())
    {
        GX_LOG_ERROR("PostEffectSettings: Cannot open for writing: %s", filePath.c_str());
        return false;
    }

    ofs << root.dump(2);
    ofs.close();

    GX_LOG_INFO("PostEffectSettings: Saved to %s", filePath.c_str());
    return true;
}

} // namespace GX
