/// @file QualitySettings.cpp
/// @brief グラフィックス品質プリセットの実装
#include "pch_graphics.h"
#include "Graphics/QualitySettings.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Core/Logger.h"

namespace gx
{

QualityParams QualityParams::FromPreset(QualityLevel level)
{
    QualityParams p;
    switch (level)
    {
    case QualityLevel::Low:
        p.shadowMapSize   = 512;
        p.ssaoEnabled     = false;
        p.bloomEnabled    = false;
        p.ssrEnabled      = false;
        p.motionBlurEnabled = false;
        p.taaEnabled      = false;
        p.fxaaEnabled     = true;
        p.contactShadowsEnabled = false;
        p.dofEnabled      = false;
        p.volumetricLightEnabled = false;
        break;

    case QualityLevel::Medium:
        p.shadowMapSize   = 1024;
        p.ssaoEnabled     = true;
        p.ssaoRadius      = 0.3f;
        p.ssaoSamples     = 8;
        p.bloomEnabled    = true;
        p.bloomThreshold  = 1.2f;
        p.bloomIntensity  = 0.5f;
        p.ssrEnabled      = false;
        p.motionBlurEnabled = false;
        p.taaEnabled      = true;
        p.fxaaEnabled     = true;
        p.contactShadowsEnabled = false;
        p.dofEnabled      = false;
        p.volumetricLightEnabled = false;
        break;

    case QualityLevel::High:
        p.shadowMapSize   = 2048;
        p.ssaoEnabled     = true;
        p.ssaoRadius      = 0.5f;
        p.ssaoSamples     = 16;
        p.bloomEnabled    = true;
        p.bloomThreshold  = 1.0f;
        p.bloomIntensity  = 0.8f;
        p.ssrEnabled      = true;
        p.motionBlurEnabled = true;
        p.taaEnabled      = true;
        p.fxaaEnabled     = true;
        p.contactShadowsEnabled = true;
        p.dofEnabled      = false;
        p.volumetricLightEnabled = false;
        break;

    case QualityLevel::Ultra:
        p.shadowMapSize   = 4096;
        p.ssaoEnabled     = true;
        p.ssaoRadius      = 0.5f;
        p.ssaoSamples     = 32;
        p.bloomEnabled    = true;
        p.bloomThreshold  = 0.8f;
        p.bloomIntensity  = 1.0f;
        p.ssrEnabled      = true;
        p.motionBlurEnabled = true;
        p.taaEnabled      = true;
        p.fxaaEnabled     = true;
        p.contactShadowsEnabled = true;
        p.dofEnabled      = true;
        p.volumetricLightEnabled = true;
        break;

    default:
        break;
    }
    return p;
}

void QualitySettings::SetQualityLevel(QualityLevel level)
{
    m_level = level;
    if (level != QualityLevel::Custom)
        m_params = QualityParams::FromPreset(level);
}

void QualitySettings::SetParams(const QualityParams& params)
{
    m_params = params;
    m_level = QualityLevel::Custom;
}

void QualitySettings::Apply(PostEffectPipeline& pipeline) const
{
    // SSAO
    pipeline.GetSSAO().SetEnabled(m_params.ssaoEnabled);
    if (m_params.ssaoEnabled)
    {
        pipeline.GetSSAO().SetRadius(m_params.ssaoRadius);
    }

    // Bloom
    pipeline.GetBloom().SetEnabled(m_params.bloomEnabled);
    if (m_params.bloomEnabled)
    {
        pipeline.GetBloom().SetThreshold(m_params.bloomThreshold);
        pipeline.GetBloom().SetIntensity(m_params.bloomIntensity);
    }

    // SSR
    pipeline.GetSSR().SetEnabled(m_params.ssrEnabled);

    // MotionBlur
    pipeline.GetMotionBlur().SetEnabled(m_params.motionBlurEnabled);

    // TAA
    pipeline.GetTAA().SetEnabled(m_params.taaEnabled);

    // FXAA
    pipeline.SetFXAAEnabled(m_params.fxaaEnabled);

    // ContactShadows
    pipeline.GetContactShadows().SetEnabled(m_params.contactShadowsEnabled);

    // DoF
    pipeline.GetDoF().SetEnabled(m_params.dofEnabled);

    // VolumetricLight
    pipeline.GetVolumetricLight().SetEnabled(m_params.volumetricLightEnabled);
}

bool QualitySettings::SaveToFile(const std::string& filePath) const
{
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << "qualityLevel=" << static_cast<int>(m_level) << "\n";
    file << "shadowMapSize=" << m_params.shadowMapSize << "\n";
    file << "ssaoEnabled=" << m_params.ssaoEnabled << "\n";
    file << "ssaoRadius=" << m_params.ssaoRadius << "\n";
    file << "ssaoSamples=" << m_params.ssaoSamples << "\n";
    file << "bloomEnabled=" << m_params.bloomEnabled << "\n";
    file << "bloomThreshold=" << m_params.bloomThreshold << "\n";
    file << "bloomIntensity=" << m_params.bloomIntensity << "\n";
    file << "ssrEnabled=" << m_params.ssrEnabled << "\n";
    file << "motionBlurEnabled=" << m_params.motionBlurEnabled << "\n";
    file << "taaEnabled=" << m_params.taaEnabled << "\n";
    file << "fxaaEnabled=" << m_params.fxaaEnabled << "\n";
    file << "contactShadowsEnabled=" << m_params.contactShadowsEnabled << "\n";
    file << "dofEnabled=" << m_params.dofEnabled << "\n";
    file << "volumetricLightEnabled=" << m_params.volumetricLightEnabled << "\n";
    return true;
}

bool QualitySettings::LoadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line))
    {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        try {
            if (key == "qualityLevel") m_level = static_cast<QualityLevel>(std::stoi(val));
            else if (key == "shadowMapSize") m_params.shadowMapSize = static_cast<uint32_t>(std::stoi(val));
            else if (key == "ssaoEnabled") m_params.ssaoEnabled = (val == "1");
            else if (key == "ssaoRadius") m_params.ssaoRadius = std::stof(val);
            else if (key == "ssaoSamples") m_params.ssaoSamples = std::stoi(val);
            else if (key == "bloomEnabled") m_params.bloomEnabled = (val == "1");
            else if (key == "bloomThreshold") m_params.bloomThreshold = std::stof(val);
            else if (key == "bloomIntensity") m_params.bloomIntensity = std::stof(val);
            else if (key == "ssrEnabled") m_params.ssrEnabled = (val == "1");
            else if (key == "motionBlurEnabled") m_params.motionBlurEnabled = (val == "1");
            else if (key == "taaEnabled") m_params.taaEnabled = (val == "1");
            else if (key == "fxaaEnabled") m_params.fxaaEnabled = (val == "1");
            else if (key == "contactShadowsEnabled") m_params.contactShadowsEnabled = (val == "1");
            else if (key == "dofEnabled") m_params.dofEnabled = (val == "1");
            else if (key == "volumetricLightEnabled") m_params.volumetricLightEnabled = (val == "1");
        } catch (...) {}
    }
    return true;
}

} // namespace gx
