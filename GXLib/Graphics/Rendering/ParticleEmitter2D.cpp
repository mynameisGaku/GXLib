#include "pch.h"
/// @file ParticleEmitter2D.cpp
/// @brief 2Dパーティクルエミッター実装

#include "Graphics/Rendering/ParticleEmitter2D.h"
#include "Math/MathUtil.h"

namespace GX
{

void ParticleEmitter2D::Initialize(const EmitterConfig2D& config)
{
    m_config = config;
    m_particles.resize(config.maxParticles);
    m_aliveCount = 0;
    m_emissionAccum = 0.0f;
}

void ParticleEmitter2D::Update(float deltaTime)
{
    // 既存パーティクルの更新
    m_aliveCount = 0;
    for (auto& p : m_particles)
    {
        if (!p.alive) continue;

        p.life -= deltaTime;
        if (p.life <= 0.0f)
        {
            p.alive = false;
            continue;
        }

        // 重力
        p.velocity.x += m_config.gravity.x * deltaTime;
        p.velocity.y += m_config.gravity.y * deltaTime;

        // ドラッグ
        if (m_config.drag > 0.0f)
        {
            float factor = 1.0f - m_config.drag * deltaTime;
            if (factor < 0.0f) factor = 0.0f;
            p.velocity *= factor;
        }

        // 位置更新
        p.position += p.velocity * deltaTime;

        // 回転更新
        p.rotation += p.angularVelocity * deltaTime;

        // 寿命に基づく補間
        float t = 1.0f - (p.life / p.maxLife);
        t = MathUtil::Clamp(t, 0.0f, 1.0f);

        // サイズ補間
        p.size = p.startSize + (p.endSize - p.startSize) * t;

        // 色補間
        p.color = Color::Lerp(p.startColor, p.endColor, t);

        ++m_aliveCount;
    }

    // 新規パーティクルの発生
    if (m_active && m_config.emissionRate > 0.0f)
    {
        m_emissionAccum += m_config.emissionRate * deltaTime;
        while (m_emissionAccum >= 1.0f)
        {
            SpawnParticle();
            m_emissionAccum -= 1.0f;
        }
    }
}

void ParticleEmitter2D::Burst(int count)
{
    for (int i = 0; i < count; ++i)
    {
        SpawnParticle();
    }
}

void ParticleEmitter2D::Draw(SpriteBatch& batch, int fallbackTexture)
{
    batch.SetBlendMode(m_config.blendMode);

    for (const auto& p : m_particles)
    {
        if (!p.alive) continue;

        batch.SetDrawColor(p.color.r, p.color.g, p.color.b, p.color.a);

        int texHandle = m_config.textureHandle;
        if (texHandle < 0) texHandle = fallbackTexture;
        if (texHandle < 0) continue; // テクスチャなし→スキップ

        float extRate = p.size / 16.0f; // normalize to base size
        batch.DrawRotaGraph(p.position.x, p.position.y,
                            extRate, p.rotation,
                            texHandle, true);
    }

    // 描画色をリセット
    batch.SetDrawColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void ParticleEmitter2D::SpawnParticle()
{
    int idx = FindDeadParticle();
    if (idx < 0) return;

    Particle2D& p = m_particles[idx];
    p.alive = true;

    // 寿命
    std::uniform_real_distribution<float> lifeDist(m_config.lifeMin, m_config.lifeMax);
    p.life = lifeDist(m_rng);
    p.maxLife = p.life;

    // サイズ
    p.startSize = m_config.sizeStart;
    p.endSize = m_config.sizeEnd;
    p.size = p.startSize;

    // 色
    p.startColor = m_config.colorStart;
    p.endColor = m_config.colorEnd;
    p.color = p.startColor;

    // 位置（形状に基づく）
    p.position = m_position;
    switch (m_config.shape)
    {
    case EmitterShape2D::Point:
        break;
    case EmitterShape2D::Circle:
    {
        std::uniform_real_distribution<float> angleDist(0.0f, XM_2PI);
        std::uniform_real_distribution<float> radiusDist(0.0f, m_config.shapeRadius);
        float a = angleDist(m_rng);
        float r = std::sqrt(radiusDist(m_rng) / (std::max)(m_config.shapeRadius, 0.001f)) * m_config.shapeRadius;
        p.position.x += std::cos(a) * r;
        p.position.y += std::sin(a) * r;
        break;
    }
    case EmitterShape2D::Rectangle:
    {
        std::uniform_real_distribution<float> xDist(-m_config.shapeWidth * 0.5f, m_config.shapeWidth * 0.5f);
        std::uniform_real_distribution<float> yDist(-m_config.shapeHeight * 0.5f, m_config.shapeHeight * 0.5f);
        p.position.x += xDist(m_rng);
        p.position.y += yDist(m_rng);
        break;
    }
    case EmitterShape2D::Line:
    {
        std::uniform_real_distribution<float> xDist(-m_config.shapeWidth * 0.5f, m_config.shapeWidth * 0.5f);
        p.position.x += xDist(m_rng);
        break;
    }
    }

    // 速度（方向＋スプレッド）
    std::uniform_real_distribution<float> speedDist(m_config.speedMin, m_config.speedMax);
    float speed = speedDist(m_rng);

    float dirRad = m_config.directionAngle * (XM_PI / 180.0f);
    std::uniform_real_distribution<float> spreadDist(-m_config.directionSpread, m_config.directionSpread);
    float spreadRad = spreadDist(m_rng) * (XM_PI / 180.0f);
    float finalAngle = dirRad + spreadRad;

    p.velocity.x = std::cos(finalAngle) * speed;
    p.velocity.y = std::sin(finalAngle) * speed;

    // 回転
    p.rotation = 0.0f;
    if (m_config.angularVelocityMin != 0.0f || m_config.angularVelocityMax != 0.0f)
    {
        std::uniform_real_distribution<float> angVelDist(m_config.angularVelocityMin, m_config.angularVelocityMax);
        p.angularVelocity = angVelDist(m_rng);
    }
    else
    {
        p.angularVelocity = 0.0f;
    }

    ++m_aliveCount;
}

int ParticleEmitter2D::FindDeadParticle()
{
    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i)
    {
        if (!m_particles[i].alive) return i;
    }
    return -1;
}

} // namespace GX
