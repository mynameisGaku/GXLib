/// @file ReverbZone.cpp
/// @brief 空間リバーブゾーン管理の実装
#include "pch_audio.h"
#include "Audio/ReverbZone.h"

#include <cmath>
#include <algorithm>

namespace gx
{

// ============================================================================
// AddZone
// ============================================================================

uint32_t ReverbZoneManager::AddZone(const ReverbZoneConfig& config)
{
    ZoneState state;
    state.config = config;
    state.id = m_nextId++;
    state.currentBlend = 0.0f;
    m_zones.push_back(std::move(state));
    return state.id;
}

// ============================================================================
// RemoveZone
// ============================================================================

void ReverbZoneManager::RemoveZone(uint32_t zoneId)
{
    auto it = std::remove_if(m_zones.begin(), m_zones.end(),
        [zoneId](const ZoneState& z) { return z.id == zoneId; });
    m_zones.erase(it, m_zones.end());

    // アクティブゾーンが削除された場合はリセット
    if (m_activeZoneId == zoneId)
    {
        m_activeZoneId = 0;
        m_currentBlend = 0.0f;
    }
}

// ============================================================================
// UpdateZone
// ============================================================================

void ReverbZoneManager::UpdateZone(uint32_t zoneId, const ReverbZoneConfig& config)
{
    for (auto& zone : m_zones)
    {
        if (zone.id == zoneId)
        {
            zone.config = config;
            return;
        }
    }
}

// ============================================================================
// GetZone
// ============================================================================

const ReverbZoneConfig* ReverbZoneManager::GetZone(uint32_t zoneId) const
{
    for (const auto& zone : m_zones)
    {
        if (zone.id == zoneId)
        {
            return &zone.config;
        }
    }
    return nullptr;
}

// ============================================================================
// Update — リスナー位置に基づくブレンド計算
// ============================================================================

void ReverbZoneManager::Update(float listenerX, float listenerY, float listenerZ)
{
    // マネージャ無効時は何もしない
    if (!m_enabled)
    {
        m_activeZoneId = 0;
        m_currentBlend = 0.0f;
        return;
    }

    // 各ゾーンのブレンド係数を計算
    for (auto& zone : m_zones)
    {
        if (!zone.config.enabled)
        {
            zone.currentBlend = 0.0f;
            continue;
        }

        // リスナーとゾーン中心間の距離を計算
        float dx = listenerX - zone.config.position[0];
        float dy = listenerY - zone.config.position[1];
        float dz = listenerZ - zone.config.position[2];
        float distSq = dx * dx + dy * dy + dz * dz;
        float dist = std::sqrt(distSq);

        float inner = zone.config.innerRadius;
        float outer = zone.config.outerRadius;

        // ブレンド係数の計算
        if (dist <= inner)
        {
            // 内径以内: 完全適用
            zone.currentBlend = 1.0f;
        }
        else if (dist >= outer)
        {
            // 外径以外: 適用なし
            zone.currentBlend = 0.0f;
        }
        else
        {
            // 内径〜外径間: 線形補間
            float range = outer - inner;
            if (range > 0.0f)
            {
                zone.currentBlend = 1.0f - (dist - inner) / range;
            }
            else
            {
                zone.currentBlend = 0.0f;
            }
        }
    }

    // アクティブゾーンの選択
    // 最も高いブレンド係数を持つゾーンを選択。同値の場合は優先度が高い方を採用。
    uint32_t bestId = 0;
    float bestBlend = 0.0f;
    int bestPriority = std::numeric_limits<int>::min();

    for (const auto& zone : m_zones)
    {
        if (!zone.config.enabled)
            continue;
        if (zone.currentBlend <= 0.0f)
            continue;

        bool isBetter = false;
        if (zone.currentBlend > bestBlend)
        {
            isBetter = true;
        }
        else if (zone.currentBlend == bestBlend && zone.config.priority > bestPriority)
        {
            isBetter = true;
        }

        if (isBetter)
        {
            bestId = zone.id;
            bestBlend = zone.currentBlend;
            bestPriority = zone.config.priority;
        }
    }

    m_activeZoneId = bestId;
    m_currentBlend = bestBlend;
}

// ============================================================================
// アクセサ
// ============================================================================

uint32_t ReverbZoneManager::GetActiveZoneId() const
{
    return m_activeZoneId;
}

float ReverbZoneManager::GetCurrentBlendFactor() const
{
    return m_currentBlend;
}

void ReverbZoneManager::SetMaxActiveZones(int max)
{
    m_maxActive = (max > 0) ? max : 1;
}

int ReverbZoneManager::GetMaxActiveZones() const
{
    return m_maxActive;
}

size_t ReverbZoneManager::GetZoneCount() const
{
    return m_zones.size();
}

void ReverbZoneManager::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
    {
        m_activeZoneId = 0;
        m_currentBlend = 0.0f;
    }
}

bool ReverbZoneManager::IsEnabled() const
{
    return m_enabled;
}

} // namespace gx
