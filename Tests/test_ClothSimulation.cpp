/// @file test_ClothSimulation.cpp
/// @brief ClothSimulation テスト（CPU-onlyモード、device=nullptr）

#include <gtest/gtest.h>
#include "Graphics/3D/ClothSimulation.h"
#include <cmath>

using namespace gx;

// -----------------------------------------------------------------------
// Fixture
// -----------------------------------------------------------------------
class ClothSimulationTest : public ::testing::Test
{
protected:
    ClothSimulation cloth;

    void SetUp() override
    {
        ASSERT_TRUE(cloth.Initialize(nullptr, 4, 4));
        cloth.CreateGrid(4, 4, 1.0f);
    }

    void TearDown() override
    {
        cloth.Shutdown();
    }
};

// -----------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, InitializeCPUMode)
{
    ClothSimulation c;
    EXPECT_TRUE(c.Initialize(nullptr, 8, 8));
    c.Shutdown();
}

TEST_F(ClothSimulationTest, ShutdownResetsState)
{
    cloth.Shutdown();
    EXPECT_EQ(cloth.GetParticleCount(), 0u);
    EXPECT_EQ(cloth.GetConstraintCount(), 0u);
    EXPECT_EQ(cloth.GetColliderCount(), 0u);
    EXPECT_EQ(cloth.GetWidth(), 0u);
    EXPECT_EQ(cloth.GetHeight(), 0u);
}

// -----------------------------------------------------------------------
// CreateGrid
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, CreateGrid4x4Creates16Particles)
{
    EXPECT_EQ(cloth.GetParticleCount(), 16u);
}

TEST_F(ClothSimulationTest, CreateGridCorrectConstraintCount)
{
    // 4x4 grid:
    // Stretch: horizontal 3*4=12 + vertical 4*3=12 = 24
    // Shear: diagonal(\) 3*3=9 + diagonal(/) 3*3=9 = 18
    // Bend: horizontal 2*4=8 + vertical 4*2=8 = 16
    // Total = 24 + 18 + 16 = 58
    EXPECT_EQ(cloth.GetConstraintCount(), 58u);
}

TEST_F(ClothSimulationTest, CreateGridDimensions)
{
    EXPECT_EQ(cloth.GetWidth(), 4u);
    EXPECT_EQ(cloth.GetHeight(), 4u);
}

// -----------------------------------------------------------------------
// Particle State
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, ParticleInitialPosition)
{
    // Particle (0,0) should be at origin
    const auto& p00 = cloth.GetParticle(0);
    EXPECT_FLOAT_EQ(p00.position.x, 0.0f);
    EXPECT_FLOAT_EQ(p00.position.y, 0.0f);
    EXPECT_FLOAT_EQ(p00.position.z, 0.0f);

    // Particle (1,0) should be at (1,0,0)
    const auto& p10 = cloth.GetParticle(1);
    EXPECT_FLOAT_EQ(p10.position.x, 1.0f);
    EXPECT_FLOAT_EQ(p10.position.y, 0.0f);
    EXPECT_FLOAT_EQ(p10.position.z, 0.0f);

    // Particle (0,1) should be at (0,0,1) — grid on XZ plane
    const auto& p01 = cloth.GetParticle(4);
    EXPECT_FLOAT_EQ(p01.position.x, 0.0f);
    EXPECT_FLOAT_EQ(p01.position.y, 0.0f);
    EXPECT_FLOAT_EQ(p01.position.z, 1.0f);
}

TEST_F(ClothSimulationTest, ParticleDefaultInverseMassPositive)
{
    for (uint32_t i = 0; i < cloth.GetParticleCount(); ++i)
    {
        EXPECT_GT(cloth.GetParticle(i).inverseMass, 0.0f);
    }
}

// -----------------------------------------------------------------------
// Pin / Unpin
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, PinParticleSetsInverseMassZero)
{
    cloth.PinParticle(0);
    EXPECT_FLOAT_EQ(cloth.GetParticle(0).inverseMass, 0.0f);
}

TEST_F(ClothSimulationTest, UnpinParticleRestoresMass)
{
    cloth.PinParticle(0);
    cloth.UnpinParticle(0, 2.0f);
    EXPECT_GT(cloth.GetParticle(0).inverseMass, 0.0f);
    EXPECT_FLOAT_EQ(cloth.GetParticle(0).inverseMass, 0.5f); // 1/2
}

TEST_F(ClothSimulationTest, IsParticlePinned)
{
    EXPECT_FALSE(cloth.IsParticlePinned(0));
    cloth.PinParticle(0);
    EXPECT_TRUE(cloth.IsParticlePinned(0));
    cloth.UnpinParticle(0);
    EXPECT_FALSE(cloth.IsParticlePinned(0));
}

// -----------------------------------------------------------------------
// Gravity & Physics
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, GravityMovesUnpinnedDown)
{
    float initialY = cloth.GetParticle(5).position.y;
    cloth.Update(1.0f / 60.0f);
    EXPECT_LT(cloth.GetParticle(5).position.y, initialY);
}

TEST_F(ClothSimulationTest, PinnedParticleStaysFixed)
{
    cloth.PinParticle(0);
    float initialY = cloth.GetParticle(0).position.y;
    cloth.Update(1.0f / 60.0f);
    EXPECT_FLOAT_EQ(cloth.GetParticle(0).position.y, initialY);
}

TEST_F(ClothSimulationTest, ConstraintsPreventExcessiveSeparation)
{
    // Pin top-left corner
    cloth.PinParticle(0);

    // Run several update steps
    for (int i = 0; i < 30; ++i)
        cloth.Update(1.0f / 60.0f);

    // Particle 1 is horizontally adjacent to particle 0 (restLength=1.0)
    // Due to constraints, it shouldn't be unreasonably far from particle 0
    float dx = cloth.GetParticle(1).position.x - cloth.GetParticle(0).position.x;
    float dy = cloth.GetParticle(1).position.y - cloth.GetParticle(0).position.y;
    float dz = cloth.GetParticle(1).position.z - cloth.GetParticle(0).position.z;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Should be roughly within 2x rest length (constraints keep things together)
    EXPECT_LT(dist, 3.0f);
}

// -----------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, DefaultGravity)
{
    const auto& s = cloth.GetSettings();
    EXPECT_FLOAT_EQ(s.gravity.x, 0.0f);
    EXPECT_FLOAT_EQ(s.gravity.y, -9.8f);
    EXPECT_FLOAT_EQ(s.gravity.z, 0.0f);
}

TEST_F(ClothSimulationTest, DefaultDamping)
{
    EXPECT_FLOAT_EQ(cloth.GetSettings().damping, 0.99f);
}

TEST_F(ClothSimulationTest, DefaultSolverIterations)
{
    EXPECT_EQ(cloth.GetSettings().solverIterations, 8u);
}

TEST_F(ClothSimulationTest, SetSettingsRoundTrip)
{
    ClothSettings s;
    s.gravity = { 0.0f, -5.0f, 0.0f };
    s.damping = 0.95f;
    s.solverIterations = 16;
    s.windForce = { 1.0f, 0.0f, 0.0f };
    cloth.SetSettings(s);

    const auto& result = cloth.GetSettings();
    EXPECT_FLOAT_EQ(result.gravity.y, -5.0f);
    EXPECT_FLOAT_EQ(result.damping, 0.95f);
    EXPECT_EQ(result.solverIterations, 16u);
    EXPECT_FLOAT_EQ(result.windForce.x, 1.0f);
}

// -----------------------------------------------------------------------
// Colliders
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, AddCollider)
{
    ClothCollider c;
    c.shape = ClothColliderShape::Sphere;
    c.position = { 0.0f, -5.0f, 0.0f };
    c.radius = 3.0f;

    int id = cloth.AddCollider(c);
    EXPECT_GE(id, 0);
    EXPECT_EQ(cloth.GetColliderCount(), 1u);
}

TEST_F(ClothSimulationTest, RemoveCollider)
{
    ClothCollider c;
    c.shape = ClothColliderShape::Sphere;
    c.position = { 0.0f, -5.0f, 0.0f };
    c.radius = 3.0f;

    int id = cloth.AddCollider(c);
    cloth.RemoveCollider(id);
    EXPECT_EQ(cloth.GetColliderCount(), 0u);
}

TEST_F(ClothSimulationTest, SphereColliderPrevenetsPenetration)
{
    // Place a large sphere just below the cloth
    ClothCollider c;
    c.shape = ClothColliderShape::Sphere;
    c.position = { 1.5f, -2.0f, 1.5f };
    c.radius = 2.0f;
    cloth.AddCollider(c);

    // Run simulation — particles should not penetrate the sphere
    for (int i = 0; i < 120; ++i)
        cloth.Update(1.0f / 60.0f);

    // Check all particles are outside sphere
    for (uint32_t i = 0; i < cloth.GetParticleCount(); ++i)
    {
        if (cloth.IsParticlePinned(i)) continue;
        const auto& p = cloth.GetParticle(i);
        float dx = p.position.x - c.position.x;
        float dy = p.position.y - c.position.y;
        float dz = p.position.z - c.position.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        EXPECT_GE(dist, c.radius - 0.01f);
    }
}

// -----------------------------------------------------------------------
// Wind
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, WindAffectsParticleMovement)
{
    ClothSettings s = cloth.GetSettings();
    s.gravity = { 0.0f, 0.0f, 0.0f };  // Disable gravity
    s.windForce = { 10.0f, 0.0f, 0.0f };
    cloth.SetSettings(s);

    float initialX = cloth.GetParticle(5).position.x;
    for (int i = 0; i < 10; ++i)
        cloth.Update(1.0f / 60.0f);

    // Wind pushes in +X direction
    EXPECT_GT(cloth.GetParticle(5).position.x, initialX);
}

// -----------------------------------------------------------------------
// Counts & Dimensions
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, GetParticleCount)
{
    EXPECT_EQ(cloth.GetParticleCount(), 16u);
}

TEST_F(ClothSimulationTest, GetConstraintCount)
{
    EXPECT_GT(cloth.GetConstraintCount(), 0u);
}

TEST_F(ClothSimulationTest, GetColliderCount)
{
    EXPECT_EQ(cloth.GetColliderCount(), 0u);
    ClothCollider c;
    c.shape = ClothColliderShape::Sphere;
    c.position = { 0.0f, 0.0f, 0.0f };
    c.radius = 1.0f;
    cloth.AddCollider(c);
    EXPECT_EQ(cloth.GetColliderCount(), 1u);
}

// -----------------------------------------------------------------------
// Multiple Updates
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, MultipleUpdatesProduceProgressiveMotion)
{
    float y1 = cloth.GetParticle(5).position.y;
    cloth.Update(1.0f / 60.0f);
    float y2 = cloth.GetParticle(5).position.y;
    cloth.Update(1.0f / 60.0f);
    float y3 = cloth.GetParticle(5).position.y;

    // Each update should move particle further down
    EXPECT_LT(y2, y1);
    EXPECT_LT(y3, y2);
}

// -----------------------------------------------------------------------
// Different Grid Sizes
// -----------------------------------------------------------------------
TEST_F(ClothSimulationTest, CreateGridDifferentSize)
{
    ClothSimulation c2;
    ASSERT_TRUE(c2.Initialize(nullptr, 8, 6));
    c2.CreateGrid(8, 6, 0.5f);
    EXPECT_EQ(c2.GetParticleCount(), 48u);
    EXPECT_EQ(c2.GetWidth(), 8u);
    EXPECT_EQ(c2.GetHeight(), 6u);
    c2.Shutdown();
}
