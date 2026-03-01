/// @file AudioMixerUI.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/AudioMixerUI.h"
#include "Core/Logger.h"
#include <algorithm>

namespace gx
{

void AudioMixerUI::Initialize(uint32_t busCount)
{
    busCount = std::max(1u, busCount);
    m_strips.resize(busCount);
    for (uint32_t i = 0; i < busCount; ++i)
    {
        m_strips[i] = ChannelStripData{};
        m_strips[i].busName = DefaultBusName(i);
    }
    m_routings.clear();
}

const ChannelStripData& AudioMixerUI::GetChannelStrip(uint32_t busIndex) const
{
    if (busIndex >= m_strips.size()) return m_dummyStrip;
    return m_strips[busIndex];
}

void AudioMixerUI::SetVolume(uint32_t busIndex, float volume)
{
    if (busIndex >= m_strips.size()) return;
    m_strips[busIndex].volume = std::clamp(volume, 0.0f, 2.0f);
}

void AudioMixerUI::SetPan(uint32_t busIndex, float pan)
{
    if (busIndex >= m_strips.size()) return;
    m_strips[busIndex].pan = std::clamp(pan, -1.0f, 1.0f);
}

void AudioMixerUI::SetMuted(uint32_t busIndex, bool muted)
{
    if (busIndex >= m_strips.size()) return;
    m_strips[busIndex].muted = muted;
}

void AudioMixerUI::SetSoloed(uint32_t busIndex, bool soloed)
{
    if (busIndex >= m_strips.size()) return;
    m_strips[busIndex].soloed = soloed;
}

void AudioMixerUI::SetVULevel(uint32_t busIndex, float left, float right)
{
    if (busIndex >= m_strips.size()) return;
    m_strips[busIndex].vuLeft = std::max(0.0f, left);
    m_strips[busIndex].vuRight = std::max(0.0f, right);

    // Update peaks
    if (left > m_strips[busIndex].peakLeft)
        m_strips[busIndex].peakLeft = left;
    if (right > m_strips[busIndex].peakRight)
        m_strips[busIndex].peakRight = right;
}

void AudioMixerUI::UpdateVUMeters(float dt)
{
    float decay = k_VUDecayRate * dt;
    for (auto& strip : m_strips)
    {
        strip.vuLeft  = std::max(0.0f, strip.vuLeft  - decay);
        strip.vuRight = std::max(0.0f, strip.vuRight - decay);
        strip.peakLeft  = std::max(0.0f, strip.peakLeft  - decay);
        strip.peakRight = std::max(0.0f, strip.peakRight - decay);
    }
}

bool AudioMixerUI::GetEffectiveMuted(uint32_t busIndex) const
{
    if (busIndex >= m_strips.size()) return true;

    // If any bus is soloed, non-solo buses are effectively muted
    uint32_t soloCount = GetSoloCount();
    if (soloCount > 0)
    {
        return !m_strips[busIndex].soloed;
    }

    return m_strips[busIndex].muted;
}

uint32_t AudioMixerUI::GetSoloCount() const
{
    uint32_t count = 0;
    for (const auto& strip : m_strips)
    {
        if (strip.soloed) ++count;
    }
    return count;
}

void AudioMixerUI::AddRouting(uint32_t srcBusIndex, uint32_t dstBusIndex, float sendLevel)
{
    BusRoutingConnection conn;
    conn.srcBusIndex = srcBusIndex;
    conn.dstBusIndex = dstBusIndex;
    conn.sendLevel = sendLevel;
    m_routings.push_back(conn);
}

void AudioMixerUI::RemoveRouting(uint32_t srcBusIndex, uint32_t dstBusIndex)
{
    m_routings.erase(
        std::remove_if(m_routings.begin(), m_routings.end(),
            [&](const BusRoutingConnection& c) {
                return c.srcBusIndex == srcBusIndex && c.dstBusIndex == dstBusIndex;
            }),
        m_routings.end());
}

std::string AudioMixerUI::GetBusName(uint32_t busIndex) const
{
    if (busIndex >= m_strips.size()) return "";
    return m_strips[busIndex].busName;
}

void AudioMixerUI::SetBusName(uint32_t busIndex, const std::string& name)
{
    if (busIndex >= m_strips.size()) return;
    m_strips[busIndex].busName = name;
}

void AudioMixerUI::ResetToDefault()
{
    for (uint32_t i = 0; i < m_strips.size(); ++i)
    {
        m_strips[i].volume = 1.0f;
        m_strips[i].pan = 0.0f;
        m_strips[i].muted = false;
        m_strips[i].soloed = false;
        m_strips[i].vuLeft = 0.0f;
        m_strips[i].vuRight = 0.0f;
        m_strips[i].peakLeft = 0.0f;
        m_strips[i].peakRight = 0.0f;
        m_strips[i].busName = DefaultBusName(i);
    }
    m_routings.clear();
}

std::string AudioMixerUI::DefaultBusName(uint32_t index)
{
    switch (index)
    {
    case 0: return "Master";
    case 1: return "BGM";
    case 2: return "SE";
    case 3: return "Voice";
    default: return "Bus_" + std::to_string(index);
    }
}

} // namespace gx
