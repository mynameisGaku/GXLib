#pragma once
/// @file ParticleEmitter2D.h
/// @brief 2Dパーティクルエミッター

#include "pch.h"
#include "Math/Vector2.h"
#include "Math/Color.h"
#include "Graphics/Rendering/SpriteBatch.h"

namespace GX
{

/// @brief 2Dパーティクルの放出形状
enum class EmitterShape2D
{
    Point,      ///< 1点から発生
    Circle,     ///< 円内からランダムに発生
    Rectangle,  ///< 矩形内からランダムに発生
    Line,       ///< 線分上からランダムに発生
};

/// @brief 個々の2Dパーティクル
struct Particle2D
{
    Vector2 position;
    Vector2 velocity;
    float   rotation = 0.0f;
    float   angularVelocity = 0.0f;
    float   size = 16.0f;
    float   startSize = 16.0f;
    float   endSize = 0.0f;
    Color   color = { 1.0f, 1.0f, 1.0f, 1.0f };
    Color   startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    Color   endColor = { 1.0f, 1.0f, 1.0f, 0.0f };
    float   life = 1.0f;
    float   maxLife = 1.0f;
    bool    alive = false;
};

/// @brief 2Dエミッター設定
struct EmitterConfig2D
{
    EmitterShape2D shape = EmitterShape2D::Point;
    float shapeRadius = 0.0f;
    float shapeWidth = 0.0f;
    float shapeHeight = 0.0f;

    float emissionRate = 50.0f;
    int   burstCount = 0;

    float lifeMin = 0.5f, lifeMax = 1.5f;
    float speedMin = 50.0f, speedMax = 150.0f;
    float directionAngle = -90.0f;   ///< 度 (上向き=-90)
    float directionSpread = 30.0f;   ///< 度 (±spread)

    float sizeStart = 16.0f, sizeEnd = 0.0f;
    Color colorStart = { 1.0f, 1.0f, 1.0f, 1.0f };
    Color colorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };

    Vector2 gravity = { 0, 300.0f };
    float drag = 0.0f;
    float angularVelocityMin = 0.0f, angularVelocityMax = 0.0f;

    int textureHandle = -1;
    BlendMode blendMode = BlendMode::Add;

    uint32_t maxParticles = 500;
};

/// @brief 2Dパーティクルエミッター
class ParticleEmitter2D
{
public:
    ParticleEmitter2D() = default;

    /// @brief エミッターを初期化する
    void Initialize(const EmitterConfig2D& config);

    /// @brief エミッターの位置を設定する
    void SetPosition(const Vector2& pos) { m_position = pos; }
    void SetPosition(float x, float y) { m_position = { x, y }; }

    /// @brief エミッターの位置を取得する
    const Vector2& GetPosition() const { return m_position; }

    /// @brief パーティクルを更新する
    void Update(float deltaTime);

    /// @brief 一度にcount個のパーティクルをバースト発射する
    void Burst(int count);

    /// @brief SpriteBatchに描画を発行する
    /// @param fallbackTexture テクスチャ無し時に使う白テクスチャハンドル（-1なら描画しない）
    void Draw(SpriteBatch& batch, int fallbackTexture = -1);

    /// @brief エミッターの有効/無効を設定する
    void SetActive(bool active) { m_active = active; }
    bool IsActive() const { return m_active; }

    /// @brief 生存パーティクル数を取得する
    uint32_t GetAliveCount() const { return m_aliveCount; }

    /// @brief 設定への参照を取得する
    const EmitterConfig2D& GetConfig() const { return m_config; }
    EmitterConfig2D& GetConfigMutable() { return m_config; }

private:
    void SpawnParticle();
    int FindDeadParticle();

    EmitterConfig2D m_config;
    std::vector<Particle2D> m_particles;
    Vector2 m_position = { 0, 0 };
    float m_emissionAccum = 0.0f;
    bool m_active = true;
    uint32_t m_aliveCount = 0;

    std::mt19937 m_rng{ std::random_device{}() };
};

} // namespace GX
