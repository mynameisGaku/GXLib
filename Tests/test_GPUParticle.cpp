/// @file test_GPUParticle.cpp
/// @brief Phase 5: GPUParticleSystemエミッター管理、設定、CPUモードテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/GPUParticleSystem.h"
#include <set>

using namespace gx;

// ============================================================================
// EmitterShapeとParticleBlendMode列挙型
// ============================================================================

TEST(GPUParticleEnums, EmitterShapeValues)
{
    EXPECT_NE(static_cast<int>(EmitterShape::Point), static_cast<int>(EmitterShape::Sphere));
    EXPECT_NE(static_cast<int>(EmitterShape::Box), static_cast<int>(EmitterShape::Cone));
    EXPECT_NE(static_cast<int>(EmitterShape::Ring), static_cast<int>(EmitterShape::Point));
}

TEST(GPUParticleEnums, BlendModeValues)
{
    EXPECT_NE(static_cast<int>(ParticleBlendMode::Alpha), static_cast<int>(ParticleBlendMode::Additive));
    EXPECT_NE(static_cast<int>(ParticleBlendMode::Multiply), static_cast<int>(ParticleBlendMode::Alpha));
}

// ============================================================================
// GPUParticleEmitterConfigデフォルト値
// ============================================================================

TEST(GPUParticleConfig, Defaults)
{
    GPUParticleEmitterConfig config;
    EXPECT_EQ(config.maxParticles, 1000u);
    EXPECT_FLOAT_EQ(config.emissionRate, 100.0f);
    EXPECT_EQ(config.shape, EmitterShape::Point);
    EXPECT_FLOAT_EQ(config.drag, 0.1f);
    EXPECT_FLOAT_EQ(config.startSizeMin, 0.1f);
    EXPECT_FLOAT_EQ(config.startSizeMax, 0.3f);
    EXPECT_FLOAT_EQ(config.endSize, 0.0f);
    EXPECT_FLOAT_EQ(config.lifetimeMin, 1.0f);
    EXPECT_FLOAT_EQ(config.lifetimeMax, 3.0f);
    EXPECT_EQ(config.blendMode, ParticleBlendMode::Additive);
    EXPECT_EQ(config.textureHandle, -1);
}

TEST(GPUParticleConfig, CustomValues)
{
    GPUParticleEmitterConfig config;
    config.maxParticles = 5000;
    config.emissionRate = 500.0f;
    config.shape = EmitterShape::Sphere;
    config.lifetimeMin = 0.5f;
    config.lifetimeMax = 2.0f;
    config.blendMode = ParticleBlendMode::Alpha;

    EXPECT_EQ(config.maxParticles, 5000u);
    EXPECT_FLOAT_EQ(config.emissionRate, 500.0f);
    EXPECT_EQ(config.shape, EmitterShape::Sphere);
    EXPECT_FLOAT_EQ(config.lifetimeMin, 0.5f);
    EXPECT_FLOAT_EQ(config.lifetimeMax, 2.0f);
    EXPECT_EQ(config.blendMode, ParticleBlendMode::Alpha);
}

// ============================================================================
// GPUParticleSystem（nullptrデバイスでのCPUモード）
// ============================================================================

TEST(GPUParticleSystem, InitializeWithNullDevice)
{
    GPUParticleSystem ps;
    // nullptrデバイスで初期化 - CPUのみモードに入る
    bool result = ps.Initialize(nullptr, 10000);
    // CPUモード追跡のため成功するはず
    EXPECT_EQ(ps.GetMaxTotalParticles(), 10000u);
    ps.Shutdown();
}

TEST(GPUParticleSystem, CreateEmitter)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    config.maxParticles = 100;
    int id = ps.CreateEmitter(config);
    EXPECT_GT(id, 0);
    EXPECT_EQ(ps.GetEmitterCount(), 1u);

    ps.Shutdown();
}

TEST(GPUParticleSystem, CreateMultipleEmitters)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    int id1 = ps.CreateEmitter(config);
    int id2 = ps.CreateEmitter(config);
    int id3 = ps.CreateEmitter(config);

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_EQ(ps.GetEmitterCount(), 3u);

    ps.Shutdown();
}

TEST(GPUParticleSystem, DestroyEmitter)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    int id = ps.CreateEmitter(config);
    EXPECT_EQ(ps.GetEmitterCount(), 1u);

    ps.DestroyEmitter(id);
    EXPECT_EQ(ps.GetEmitterCount(), 0u);

    ps.Shutdown();
}

TEST(GPUParticleSystem, SetEmitterPosition)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    int id = ps.CreateEmitter(config);

    // クラッシュしないこと
    ps.SetEmitterPosition(id, { 10.0f, 20.0f, 30.0f });

    ps.Shutdown();
}

TEST(GPUParticleSystem, SetEmitterEnabled)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    int id = ps.CreateEmitter(config);

    ps.SetEmitterEnabled(id, false);
    ps.SetEmitterEnabled(id, true);

    // クラッシュしないこと
    ps.Shutdown();
}

TEST(GPUParticleSystem, Burst)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    int id = ps.CreateEmitter(config);

    // バーストはクラッシュしないこと
    ps.Burst(id, 50);

    ps.Shutdown();
}

TEST(GPUParticleSystem, GetActiveParticleCountInitial)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    EXPECT_EQ(ps.GetActiveParticleCount(), 0u);

    ps.Shutdown();
}

TEST(GPUParticleSystem, GetEmitterCountEmpty)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    EXPECT_EQ(ps.GetEmitterCount(), 0u);

    ps.Shutdown();
}

TEST(GPUParticleSystem, DestroyInvalidEmitter)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    // 存在しないエミッターの破棄はクラッシュしないこと
    ps.DestroyEmitter(9999);
    EXPECT_EQ(ps.GetEmitterCount(), 0u);

    ps.Shutdown();
}

// ============================================================================
// 追加テスト: GPUParticleEmitterConfig
// ============================================================================

TEST(GPUParticleConfig, InitialVelocityDefaults)
{
    GPUParticleEmitterConfig config;
    EXPECT_FLOAT_EQ(config.initialVelocityMin.x, -1.0f);
    EXPECT_FLOAT_EQ(config.initialVelocityMin.y, 1.0f);
    EXPECT_FLOAT_EQ(config.initialVelocityMin.z, -1.0f);
    EXPECT_FLOAT_EQ(config.initialVelocityMax.x, 1.0f);
    EXPECT_FLOAT_EQ(config.initialVelocityMax.y, 3.0f);
    EXPECT_FLOAT_EQ(config.initialVelocityMax.z, 1.0f);
}

TEST(GPUParticleConfig, GravityDefault)
{
    GPUParticleEmitterConfig config;
    EXPECT_FLOAT_EQ(config.gravity.x, 0.0f);
    EXPECT_FLOAT_EQ(config.gravity.y, -9.81f);
    EXPECT_FLOAT_EQ(config.gravity.z, 0.0f);
}

TEST(GPUParticleConfig, StartColorDefault)
{
    GPUParticleEmitterConfig config;
    EXPECT_FLOAT_EQ(config.startColor.r, 1.0f);
    EXPECT_FLOAT_EQ(config.startColor.g, 1.0f);
    EXPECT_FLOAT_EQ(config.startColor.b, 1.0f);
    EXPECT_FLOAT_EQ(config.startColor.a, 1.0f);
}

TEST(GPUParticleConfig, EndColorDefault)
{
    GPUParticleEmitterConfig config;
    EXPECT_FLOAT_EQ(config.endColor.r, 1.0f);
    EXPECT_FLOAT_EQ(config.endColor.g, 1.0f);
    EXPECT_FLOAT_EQ(config.endColor.b, 1.0f);
    EXPECT_FLOAT_EQ(config.endColor.a, 0.0f);
}

TEST(GPUParticleConfig, CustomVelocity)
{
    GPUParticleEmitterConfig config;
    config.initialVelocityMin = { -5.0f, 0.0f, -5.0f };
    config.initialVelocityMax = { 5.0f, 10.0f, 5.0f };
    EXPECT_FLOAT_EQ(config.initialVelocityMin.x, -5.0f);
    EXPECT_FLOAT_EQ(config.initialVelocityMax.y, 10.0f);
}

TEST(GPUParticleConfig, CustomGravity)
{
    GPUParticleEmitterConfig config;
    config.gravity = { 0.0f, 0.0f, 0.0f }; // 無重力
    EXPECT_FLOAT_EQ(config.gravity.y, 0.0f);
}

TEST(GPUParticleConfig, CustomColors)
{
    GPUParticleEmitterConfig config;
    config.startColor = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤
    config.endColor = { 0.0f, 0.0f, 1.0f, 0.0f };   // 青→透明
    EXPECT_FLOAT_EQ(config.startColor.r, 1.0f);
    EXPECT_FLOAT_EQ(config.startColor.g, 0.0f);
    EXPECT_FLOAT_EQ(config.endColor.b, 1.0f);
    EXPECT_FLOAT_EQ(config.endColor.a, 0.0f);
}

// ============================================================================
// 追加テスト: EmitterShape全値
// ============================================================================

TEST(GPUParticleEnums, AllEmitterShapesDistinct)
{
    std::set<int> values;
    values.insert(static_cast<int>(EmitterShape::Point));
    values.insert(static_cast<int>(EmitterShape::Sphere));
    values.insert(static_cast<int>(EmitterShape::Box));
    values.insert(static_cast<int>(EmitterShape::Cone));
    values.insert(static_cast<int>(EmitterShape::Ring));
    EXPECT_EQ(values.size(), 5u);
}

TEST(GPUParticleEnums, AllBlendModesDistinct)
{
    std::set<int> values;
    values.insert(static_cast<int>(ParticleBlendMode::Alpha));
    values.insert(static_cast<int>(ParticleBlendMode::Additive));
    values.insert(static_cast<int>(ParticleBlendMode::Multiply));
    EXPECT_EQ(values.size(), 3u);
}

// ============================================================================
// 追加テスト: GPUParticleSystem操作
// ============================================================================

TEST(GPUParticleSystem, MaxTotalParticlesCustom)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 50000);
    EXPECT_EQ(ps.GetMaxTotalParticles(), 50000u);
    ps.Shutdown();
}

TEST(GPUParticleSystem, CreateEmitterWithSphereShape)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    config.shape = EmitterShape::Sphere;
    config.maxParticles = 500;
    int id = ps.CreateEmitter(config);
    EXPECT_GT(id, 0);

    ps.Shutdown();
}

TEST(GPUParticleSystem, CreateEmitterWithBoxShape)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    config.shape = EmitterShape::Box;
    int id = ps.CreateEmitter(config);
    EXPECT_GT(id, 0);

    ps.Shutdown();
}

TEST(GPUParticleSystem, DestroyAndRecreateEmitter)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    int id1 = ps.CreateEmitter(config);
    EXPECT_EQ(ps.GetEmitterCount(), 1u);

    ps.DestroyEmitter(id1);
    EXPECT_EQ(ps.GetEmitterCount(), 0u);

    int id2 = ps.CreateEmitter(config);
    EXPECT_EQ(ps.GetEmitterCount(), 1u);
    EXPECT_NE(id1, id2);

    ps.Shutdown();
}

TEST(GPUParticleSystem, SetEmitterPositionMultipleTimes)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    int id = ps.CreateEmitter(config);

    // 複数回の位置設定がクラッシュしないこと
    ps.SetEmitterPosition(id, { 0.0f, 0.0f, 0.0f });
    ps.SetEmitterPosition(id, { 10.0f, 20.0f, 30.0f });
    ps.SetEmitterPosition(id, { -5.0f, -10.0f, -15.0f });

    ps.Shutdown();
}

TEST(GPUParticleSystem, BurstMultipleTimes)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    int id = ps.CreateEmitter(config);

    // 複数回のバーストがクラッシュしないこと
    ps.Burst(id, 10);
    ps.Burst(id, 100);
    ps.Burst(id, 1000);

    ps.Shutdown();
}

TEST(GPUParticleSystem, ShutdownClearsEmitters)
{
    GPUParticleSystem ps;
    ps.Initialize(nullptr, 10000);

    GPUParticleEmitterConfig config;
    ps.CreateEmitter(config);
    ps.CreateEmitter(config);
    EXPECT_EQ(ps.GetEmitterCount(), 2u);

    ps.Shutdown();
    // Shutdown後、再初期化した場合エミッターはクリアされているはず
    ps.Initialize(nullptr, 10000);
    EXPECT_EQ(ps.GetEmitterCount(), 0u);
    ps.Shutdown();
}
