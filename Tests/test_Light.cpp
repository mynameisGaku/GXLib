/// @file test_Light.cpp
/// @brief Light factory functions and LightData struct tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Light.h"

using namespace GX;

static constexpr float kEps = 1e-5f;

// ==================== CreateDirectional ====================

TEST(LightTest, Directional_Type)
{
    LightData ld = Light::CreateDirectional({0, -1, 0}, {1, 1, 1}, 1.0f);
    EXPECT_EQ(ld.type, static_cast<uint32_t>(LightType::Directional));
}

TEST(LightTest, Directional_Color)
{
    LightData ld = Light::CreateDirectional({0, -1, 0}, {0.5f, 0.6f, 0.7f}, 2.0f);
    EXPECT_NEAR(ld.color.x, 0.5f, kEps);
    EXPECT_NEAR(ld.color.y, 0.6f, kEps);
    EXPECT_NEAR(ld.color.z, 0.7f, kEps);
    EXPECT_NEAR(ld.intensity, 2.0f, kEps);
}

TEST(LightTest, Directional_DirectionNormalized)
{
    LightData ld = Light::CreateDirectional({1, 1, 1}, {1, 1, 1}, 1.0f);
    float len = sqrtf(ld.direction.x * ld.direction.x +
                      ld.direction.y * ld.direction.y +
                      ld.direction.z * ld.direction.z);
    EXPECT_NEAR(len, 1.0f, 1e-3f);
}

TEST(LightTest, Directional_DirectionValues)
{
    LightData ld = Light::CreateDirectional({0, -1, 0}, {1, 1, 1}, 1.0f);
    EXPECT_NEAR(ld.direction.y, -1.0f, kEps);
}

TEST(LightTest, Directional_RangeIgnored)
{
    LightData ld = Light::CreateDirectional({0, -1, 0}, {1, 1, 1}, 1.0f);
    // Directional light has no meaningful range
    // Just verify the struct is valid
    EXPECT_FALSE(std::isnan(ld.range));
}

// ==================== CreatePoint ====================

TEST(LightTest, Point_Type)
{
    LightData ld = Light::CreatePoint({5, 3, 2}, 10.0f, {1, 0, 0}, 1.5f);
    EXPECT_EQ(ld.type, static_cast<uint32_t>(LightType::Point));
}

TEST(LightTest, Point_Position)
{
    LightData ld = Light::CreatePoint({5.0f, 3.0f, 2.0f}, 10.0f, {1, 0, 0}, 1.0f);
    EXPECT_NEAR(ld.position.x, 5.0f, kEps);
    EXPECT_NEAR(ld.position.y, 3.0f, kEps);
    EXPECT_NEAR(ld.position.z, 2.0f, kEps);
}

TEST(LightTest, Point_Range)
{
    LightData ld = Light::CreatePoint({0, 0, 0}, 25.0f, {1, 1, 1}, 1.0f);
    EXPECT_NEAR(ld.range, 25.0f, kEps);
}

TEST(LightTest, Point_Color)
{
    LightData ld = Light::CreatePoint({0, 0, 0}, 10.0f, {1.0f, 0.0f, 0.0f}, 3.0f);
    EXPECT_NEAR(ld.color.x, 1.0f, kEps);
    EXPECT_NEAR(ld.color.y, 0.0f, kEps);
    EXPECT_NEAR(ld.color.z, 0.0f, kEps);
    EXPECT_NEAR(ld.intensity, 3.0f, kEps);
}

TEST(LightTest, Point_Intensity)
{
    LightData ld = Light::CreatePoint({0, 0, 0}, 5.0f, {1, 1, 1}, 100.0f);
    EXPECT_NEAR(ld.intensity, 100.0f, kEps);
}

// ==================== CreateSpot ====================

TEST(LightTest, Spot_Type)
{
    LightData ld = Light::CreateSpot({0, 5, 0}, {0, -1, 0}, 20.0f, 45.0f, {1, 1, 1}, 1.0f);
    EXPECT_EQ(ld.type, static_cast<uint32_t>(LightType::Spot));
}

TEST(LightTest, Spot_Position)
{
    LightData ld = Light::CreateSpot({1.0f, 2.0f, 3.0f}, {0, -1, 0}, 20.0f, 45.0f, {1, 1, 1}, 1.0f);
    EXPECT_NEAR(ld.position.x, 1.0f, kEps);
    EXPECT_NEAR(ld.position.y, 2.0f, kEps);
    EXPECT_NEAR(ld.position.z, 3.0f, kEps);
}

TEST(LightTest, Spot_Range)
{
    LightData ld = Light::CreateSpot({0, 0, 0}, {0, -1, 0}, 50.0f, 30.0f, {1, 1, 1}, 1.0f);
    EXPECT_NEAR(ld.range, 50.0f, kEps);
}

TEST(LightTest, Spot_DirectionNormalized)
{
    LightData ld = Light::CreateSpot({0, 0, 0}, {1, 2, 3}, 10.0f, 45.0f, {1, 1, 1}, 1.0f);
    float len = sqrtf(ld.direction.x * ld.direction.x +
                      ld.direction.y * ld.direction.y +
                      ld.direction.z * ld.direction.z);
    EXPECT_NEAR(len, 1.0f, 1e-3f);
}

TEST(LightTest, Spot_AngleDeg)
{
    LightData ld = Light::CreateSpot({0, 0, 0}, {0, -1, 0}, 10.0f, 60.0f, {1, 1, 1}, 1.0f);
    // spotAngle should store some form of the angle (degrees or cos half-angle)
    EXPECT_FALSE(std::isnan(ld.spotAngle));
    EXPECT_NE(ld.spotAngle, 0.0f);
}

TEST(LightTest, Spot_Color)
{
    LightData ld = Light::CreateSpot({0, 0, 0}, {0, -1, 0}, 10.0f, 45.0f,
                                      {0.2f, 0.8f, 0.5f}, 5.0f);
    EXPECT_NEAR(ld.color.x, 0.2f, kEps);
    EXPECT_NEAR(ld.color.y, 0.8f, kEps);
    EXPECT_NEAR(ld.color.z, 0.5f, kEps);
    EXPECT_NEAR(ld.intensity, 5.0f, kEps);
}

// ==================== LightConstants ====================

TEST(LightTest, Constants_MaxLights)
{
    EXPECT_EQ(LightConstants::k_MaxLights, 16u);
}

TEST(LightTest, Constants_DefaultStruct)
{
    LightConstants lc{};
    EXPECT_EQ(lc.numLights, 0u);
}

// ==================== LightType enum ====================

TEST(LightTest, LightType_Values)
{
    EXPECT_EQ(static_cast<uint32_t>(LightType::Directional), 0u);
    EXPECT_EQ(static_cast<uint32_t>(LightType::Point), 1u);
    EXPECT_EQ(static_cast<uint32_t>(LightType::Spot), 2u);
}

// ==================== Multiple lights ====================

TEST(LightTest, MultipleLights_DifferentTypes)
{
    LightData dir = Light::CreateDirectional({0, -1, 0}, {1, 1, 1}, 1.0f);
    LightData pt  = Light::CreatePoint({0, 0, 0}, 10.0f, {1, 0, 0}, 2.0f);
    LightData sp  = Light::CreateSpot({0, 5, 0}, {0, -1, 0}, 20.0f, 45.0f, {0, 1, 0}, 3.0f);

    EXPECT_NE(dir.type, pt.type);
    EXPECT_NE(pt.type, sp.type);
    EXPECT_NE(dir.type, sp.type);
}
