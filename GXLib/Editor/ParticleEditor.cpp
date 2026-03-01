/// @file ParticleEditor.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/ParticleEditor.h"
#include "Core/Logger.h"
#include <algorithm>

namespace gx
{

void ParticleEditor::SetEmitterSettings(const ParticleEmitterSettings& settings)
{
    m_settings = settings;
    m_dirty = true;
}

void ParticleEditor::SetEmissionRate(float rate)
{
    m_settings.emissionRate = std::max(0.0f, rate);
    m_dirty = true;
}

void ParticleEditor::SetMaxParticles(uint32_t count)
{
    m_settings.maxParticles = count;
    m_dirty = true;
}

void ParticleEditor::SetLifetime(float seconds)
{
    m_settings.lifetime = std::max(0.01f, seconds);
    m_dirty = true;
}

void ParticleEditor::SetStartSpeed(float speed)
{
    m_settings.startSpeed = speed;
    m_dirty = true;
}

void ParticleEditor::SetStartSize(float size)
{
    m_settings.startSize = std::max(0.0f, size);
    m_dirty = true;
}

void ParticleEditor::SetGravity(float gravity)
{
    m_settings.gravity = gravity;
    m_dirty = true;
}

void ParticleEditor::SetEmitterShape(EditorEmitterShape shape)
{
    m_settings.shapeViz.shape = shape;
    m_dirty = true;
}

void ParticleEditor::SetShapeRadius(float radius)
{
    m_settings.shapeViz.radius = std::max(0.01f, radius);
    m_dirty = true;
}

void ParticleEditor::LoadPreset(Preset preset)
{
    m_settings = ParticleEmitterSettings{};

    switch (preset)
    {
    case Preset::Fire:
        m_settings.name = "Fire";
        m_settings.emissionRate = 50.0f;
        m_settings.maxParticles = 500;
        m_settings.lifetime = 1.5f;
        m_settings.startSpeed = 3.0f;
        m_settings.startSize = 0.2f;
        m_settings.gravity = 1.0f;
        m_settings.sizeCurve.AddPoint(0.0f, 1.0f);
        m_settings.sizeCurve.AddPoint(1.0f, 0.0f);
        m_settings.alphaCurve.AddPoint(0.0f, 1.0f);
        m_settings.alphaCurve.AddPoint(0.8f, 0.5f);
        m_settings.alphaCurve.AddPoint(1.0f, 0.0f);
        break;
    case Preset::Smoke:
        m_settings.name = "Smoke";
        m_settings.emissionRate = 20.0f;
        m_settings.maxParticles = 200;
        m_settings.lifetime = 4.0f;
        m_settings.startSpeed = 1.0f;
        m_settings.startSize = 0.5f;
        m_settings.gravity = 0.5f;
        m_settings.sizeCurve.AddPoint(0.0f, 0.5f);
        m_settings.sizeCurve.AddPoint(1.0f, 2.0f);
        break;
    case Preset::Sparks:
        m_settings.name = "Sparks";
        m_settings.emissionRate = 100.0f;
        m_settings.maxParticles = 1000;
        m_settings.lifetime = 0.8f;
        m_settings.startSpeed = 10.0f;
        m_settings.startSize = 0.05f;
        m_settings.gravity = -9.81f;
        break;
    case Preset::Snow:
        m_settings.name = "Snow";
        m_settings.emissionRate = 30.0f;
        m_settings.maxParticles = 500;
        m_settings.lifetime = 5.0f;
        m_settings.startSpeed = 0.5f;
        m_settings.startSize = 0.08f;
        m_settings.gravity = -1.0f;
        m_settings.shapeViz.shape = EditorEmitterShape::Box;
        m_settings.shapeViz.width = 10.0f;
        m_settings.shapeViz.depth = 10.0f;
        break;
    }
    m_dirty = true;
}

std::string ParticleEditor::GetPresetName(Preset preset)
{
    switch (preset)
    {
    case Preset::Fire:   return "Fire";
    case Preset::Smoke:  return "Smoke";
    case Preset::Sparks: return "Sparks";
    case Preset::Snow:   return "Snow";
    default: return "Unknown";
    }
}

void ParticleEditor::Play()
{
    m_playing = true;
    m_paused = false;
}

void ParticleEditor::Pause()
{
    if (m_playing) m_paused = true;
}

void ParticleEditor::Stop()
{
    m_playing = false;
    m_paused = false;
    m_playbackTime = 0.0f;
}

void ParticleEditor::Restart()
{
    m_playbackTime = 0.0f;
    m_playing = true;
    m_paused = false;
}

} // namespace gx
