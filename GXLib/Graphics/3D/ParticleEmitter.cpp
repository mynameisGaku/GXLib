/// @file ParticleEmitter.cpp
/// @brief パーティクルエミッターの実装
#include "pch.h"
#include "Graphics/3D/ParticleEmitter.h"

namespace GX
{

void ParticleEmitter::Initialize(const ParticleEmitterConfig& config)
{
    m_config = config;
    m_particles.reserve(config.maxParticles);
}

void ParticleEmitter::Update(float deltaTime)
{
    if (deltaTime <= 0.0f) return;

    // 1. 新しい粒子を生成（emissionRateに従う）
    if (m_active && m_config.emissionRate > 0.0f)
    {
        m_emissionAccum += m_config.emissionRate * deltaTime;
        while (m_emissionAccum >= 1.0f &&
               m_particles.size() < m_config.maxParticles)
        {
            SpawnParticle();
            m_emissionAccum -= 1.0f;
        }
    }

    // 2. 既存粒子の物理更新と寿命管理
    // インデックスベースで走査する。イテレータだとswap-and-pop時に
    // pop_back()が末尾イテレータを無効化し、MSVCデバッグで
    // "vector iterators incompatible" が発生するため。
    for (size_t i = 0; i < m_particles.size(); )
    {
        auto& p = m_particles[i];
        p.life -= deltaTime;
        if (p.life <= 0.0f)
        {
            // 寿命切れ: 末尾と入れ替えて削除（順序不定でOK）
            p = m_particles.back();
            m_particles.pop_back();
            continue;  // 同じインデックスに入れ替わった要素を再チェック
        }

        // 重力加速
        p.velocity.x += m_config.gravity.x * deltaTime;
        p.velocity.y += m_config.gravity.y * deltaTime;
        p.velocity.z += m_config.gravity.z * deltaTime;

        // 空気抵抗
        if (m_config.drag > 0.0f)
        {
            float factor = 1.0f - m_config.drag * deltaTime;
            if (factor < 0.0f) factor = 0.0f;
            p.velocity.x *= factor;
            p.velocity.y *= factor;
            p.velocity.z *= factor;
        }

        // 位置更新
        p.position.x += p.velocity.x * deltaTime;
        p.position.y += p.velocity.y * deltaTime;
        p.position.z += p.velocity.z * deltaTime;

        // 寿命による色とサイズの補間
        float t = 1.0f - (p.life / p.maxLife);  // 0=生成直後、1=寿命終了
        p.color.x = m_config.colorStart.x + (m_config.colorEnd.x - m_config.colorStart.x) * t;
        p.color.y = m_config.colorStart.y + (m_config.colorEnd.y - m_config.colorStart.y) * t;
        p.color.z = m_config.colorStart.z + (m_config.colorEnd.z - m_config.colorStart.z) * t;
        p.color.w = m_config.colorStart.w + (m_config.colorEnd.w - m_config.colorStart.w) * t;

        // サイズ変化
        if (m_config.sizeOverLife != 0.0f)
        {
            p.size += m_config.sizeOverLife * deltaTime;
            if (p.size < 0.0f) p.size = 0.0f;
        }

        ++i;
    }
}

void ParticleEmitter::Burst(uint32_t count)
{
    for (uint32_t i = 0; i < count && m_particles.size() < m_config.maxParticles; ++i)
    {
        SpawnParticle();
    }
}

void ParticleEmitter::SpawnParticle()
{
    Particle p = {};

    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> distNeg1to1(-1.0f, 1.0f);

    // 寿命をランダムに設定
    p.maxLife = m_config.lifeMin + dist01(m_rng) * (m_config.lifeMax - m_config.lifeMin);
    p.life = p.maxLife;

    // サイズをランダムに設定
    p.size = m_config.sizeMin + dist01(m_rng) * (m_config.sizeMax - m_config.sizeMin);

    // 初期色
    p.color = m_config.colorStart;

    // 回転（ランダム）
    p.rotation = dist01(m_rng) * XM_2PI;

    // 発生位置と速度を形状に応じて計算
    float speed = m_config.speedMin + dist01(m_rng) * (m_config.speedMax - m_config.speedMin);

    switch (m_config.shape)
    {
    case ParticleShape::Point:
    {
        p.position = m_position;
        // 方向ベクトル周りにランダム分散
        XMFLOAT3 dir = {
            m_direction.x + distNeg1to1(m_rng) * 0.5f,
            m_direction.y + distNeg1to1(m_rng) * 0.5f,
            m_direction.z + distNeg1to1(m_rng) * 0.5f
        };
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 0.001f)
        {
            dir.x /= len; dir.y /= len; dir.z /= len;
        }
        p.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
        break;
    }
    case ParticleShape::Sphere:
    {
        // 球面上のランダムな点
        float theta = dist01(m_rng) * XM_2PI;
        float phi = std::acos(distNeg1to1(m_rng));
        float sinPhi = std::sin(phi);
        XMFLOAT3 dir = {
            sinPhi * std::cos(theta),
            sinPhi * std::sin(theta),
            std::cos(phi)
        };
        float r = dist01(m_rng) * m_config.shapeRadius;
        p.position = {
            m_position.x + dir.x * r,
            m_position.y + dir.y * r,
            m_position.z + dir.z * r
        };
        p.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
        break;
    }
    case ParticleShape::Cone:
    {
        p.position = m_position;
        float halfAngle = m_config.coneAngle * 0.5f * (XM_PI / 180.0f);
        float cosHalf = std::cos(halfAngle);
        // コサイン重み付きでコーン内のランダム方向を生成
        float z = cosHalf + dist01(m_rng) * (1.0f - cosHalf);
        float r2 = std::sqrt(1.0f - z * z);
        float phi = dist01(m_rng) * XM_2PI;
        XMFLOAT3 localDir = { r2 * std::cos(phi), r2 * std::sin(phi), z };

        // ローカル方向をm_directionに回転する
        // 簡易版: m_directionがY+の場合はそのまま使用
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        XMVECTOR dir = XMLoadFloat3(&m_direction);
        XMVECTOR axis = XMVector3Cross(up, dir);
        float dot = XMVectorGetX(XMVector3Dot(up, dir));
        if (XMVectorGetX(XMVector3Length(axis)) < 0.001f)
        {
            // 平行またはほぼ平行
            p.velocity = {
                localDir.x * speed,
                localDir.z * speed * (dot > 0 ? 1.0f : -1.0f),
                localDir.y * speed
            };
        }
        else
        {
            float angle = std::acos(std::max(-1.0f, std::min(1.0f, dot)));
            XMMATRIX rot = XMMatrixRotationAxis(axis, angle);
            XMVECTOR ldir = XMLoadFloat3(&localDir);
            XMVECTOR wdir = XMVector3TransformNormal(ldir, rot);
            XMFLOAT3 wdirF;
            XMStoreFloat3(&wdirF, wdir);
            p.velocity = { wdirF.x * speed, wdirF.y * speed, wdirF.z * speed };
        }
        break;
    }
    case ParticleShape::Box:
    {
        p.position = {
            m_position.x + distNeg1to1(m_rng) * m_config.boxHalfExtents.x,
            m_position.y + distNeg1to1(m_rng) * m_config.boxHalfExtents.y,
            m_position.z + distNeg1to1(m_rng) * m_config.boxHalfExtents.z
        };
        XMFLOAT3 dir = {
            m_direction.x + distNeg1to1(m_rng) * 0.3f,
            m_direction.y + distNeg1to1(m_rng) * 0.3f,
            m_direction.z + distNeg1to1(m_rng) * 0.3f
        };
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len > 0.001f)
        {
            dir.x /= len; dir.y /= len; dir.z /= len;
        }
        p.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
        break;
    }
    }

    m_particles.push_back(p);
}

} // namespace GX
