/// @file test_GPUParticleCollision.cpp
/// @brief GPUParticleSystem 衝突パラメータテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/GPUParticleSystem.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

TEST(GPUParticleCollisionTest, DefaultCollisionDisabled)
{
    GPUParticleSystem ps;
    EXPECT_FALSE(ps.IsCollisionEnabled());
}

TEST(GPUParticleCollisionTest, SetCollisionEnabled)
{
    GPUParticleSystem ps;
    ps.SetCollisionEnabled(true);
    EXPECT_TRUE(ps.IsCollisionEnabled());
}

TEST(GPUParticleCollisionTest, SetCollisionDisabled)
{
    GPUParticleSystem ps;
    ps.SetCollisionEnabled(true);
    ps.SetCollisionEnabled(false);
    EXPECT_FALSE(ps.IsCollisionEnabled());
}

TEST(GPUParticleCollisionTest, DefaultBounceFactor)
{
    GPUParticleSystem ps;
    EXPECT_NEAR(ps.GetBounceFactor(), 0.5f, kEps);
}

TEST(GPUParticleCollisionTest, SetBounceFactor)
{
    GPUParticleSystem ps;
    ps.SetBounceFactor(0.8f);
    EXPECT_NEAR(ps.GetBounceFactor(), 0.8f, kEps);
}

TEST(GPUParticleCollisionTest, BounceFactorZero)
{
    GPUParticleSystem ps;
    ps.SetBounceFactor(0.0f);
    EXPECT_NEAR(ps.GetBounceFactor(), 0.0f, kEps);
}

TEST(GPUParticleCollisionTest, BounceFactorOne)
{
    GPUParticleSystem ps;
    ps.SetBounceFactor(1.0f);
    EXPECT_NEAR(ps.GetBounceFactor(), 1.0f, kEps);
}

TEST(GPUParticleCollisionTest, DefaultFriction)
{
    GPUParticleSystem ps;
    EXPECT_NEAR(ps.GetFriction(), 0.1f, kEps);
}

TEST(GPUParticleCollisionTest, SetFriction)
{
    GPUParticleSystem ps;
    ps.SetFriction(0.3f);
    EXPECT_NEAR(ps.GetFriction(), 0.3f, kEps);
}

TEST(GPUParticleCollisionTest, FrictionZero)
{
    GPUParticleSystem ps;
    ps.SetFriction(0.0f);
    EXPECT_NEAR(ps.GetFriction(), 0.0f, kEps);
}

TEST(GPUParticleCollisionTest, MaxParticles_Default)
{
    GPUParticleSystem ps;
    EXPECT_EQ(ps.GetMaxParticles(), 100000u);
}

TEST(GPUParticleCollisionTest, CollisionAndDrag_Independent)
{
    GPUParticleSystem ps;
    ps.SetCollisionEnabled(true);
    ps.SetBounceFactor(0.7f);
    ps.SetFriction(0.2f);
    ps.SetDrag(0.5f);
    EXPECT_TRUE(ps.IsCollisionEnabled());
    EXPECT_NEAR(ps.GetBounceFactor(), 0.7f, kEps);
    EXPECT_NEAR(ps.GetFriction(), 0.2f, kEps);
}
