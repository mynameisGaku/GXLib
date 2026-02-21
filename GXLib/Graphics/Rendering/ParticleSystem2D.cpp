#include "pch.h"
/// @file ParticleSystem2D.cpp
/// @brief 2Dパーティクルシステム実装

#include "Graphics/Rendering/ParticleSystem2D.h"

namespace GX
{

int ParticleSystem2D::AddEmitter(const EmitterConfig2D& config)
{
    int handle;
    if (!m_freeList.empty())
    {
        handle = m_freeList.back();
        m_freeList.pop_back();
        m_emitters[handle].emitter.Initialize(config);
        m_emitters[handle].valid = true;
    }
    else
    {
        handle = static_cast<int>(m_emitters.size());
        EmitterEntry entry;
        entry.emitter.Initialize(config);
        entry.valid = true;
        m_emitters.push_back(std::move(entry));
    }
    return handle;
}

void ParticleSystem2D::RemoveEmitter(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_emitters.size())) return;
    if (!m_emitters[handle].valid) return;
    m_emitters[handle].valid = false;
    m_freeList.push_back(handle);
}

void ParticleSystem2D::SetPosition(int handle, const Vector2& pos)
{
    if (handle < 0 || handle >= static_cast<int>(m_emitters.size())) return;
    if (!m_emitters[handle].valid) return;
    m_emitters[handle].emitter.SetPosition(pos);
}

void ParticleSystem2D::SetPosition(int handle, float x, float y)
{
    SetPosition(handle, Vector2(x, y));
}

void ParticleSystem2D::Burst(int handle, int count)
{
    if (handle < 0 || handle >= static_cast<int>(m_emitters.size())) return;
    if (!m_emitters[handle].valid) return;
    m_emitters[handle].emitter.Burst(count);
}

ParticleEmitter2D* ParticleSystem2D::GetEmitter(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_emitters.size())) return nullptr;
    if (!m_emitters[handle].valid) return nullptr;
    return &m_emitters[handle].emitter;
}

void ParticleSystem2D::Update(float deltaTime)
{
    for (auto& entry : m_emitters)
    {
        if (entry.valid)
        {
            entry.emitter.Update(deltaTime);
        }
    }
}

void ParticleSystem2D::Draw(SpriteBatch& batch)
{
    // テクスチャ無しパーティクル用の16x16白テクスチャを遅延作成
    // DrawRotaGraphのextRate = size / 16.0f なので16px基準が必要
    if (m_whiteTexture < 0)
    {
        uint32_t white[16 * 16];
        for (auto& px : white) px = 0xFFFFFFFF;
        m_whiteTexture = batch.GetTextureManager().CreateTextureFromMemory(white, 16, 16);
    }

    for (auto& entry : m_emitters)
    {
        if (entry.valid)
        {
            entry.emitter.Draw(batch, m_whiteTexture);
        }
    }
}

void ParticleSystem2D::Clear()
{
    m_emitters.clear();
    m_freeList.clear();
}

uint32_t ParticleSystem2D::GetAliveCount() const
{
    uint32_t total = 0;
    for (const auto& entry : m_emitters)
    {
        if (entry.valid)
        {
            total += entry.emitter.GetAliveCount();
        }
    }
    return total;
}

} // namespace GX
