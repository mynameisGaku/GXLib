/// @file cloud_field_test.cpp
/// @brief CloudField aggregate tests — layers + weather map + enable flag (ADR-0021 Phase B).
#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/CloudField.h"

namespace {

TEST(CloudFieldTest, DefaultConstructedFieldHasNoLayers)
{
    gx::CloudField field;
    EXPECT_EQ(field.GetLayerCount(), 0u);
    EXPECT_TRUE(field.IsEnabled());
    EXPECT_EQ(field.GetAtmosphereLUT(), nullptr);
    EXPECT_FALSE(field.GetWeatherMap().IsInitialized());
}

TEST(CloudFieldTest, AddLayerSucceedsUpToMax)
{
    gx::CloudField field;
    for (size_t i = 0; i < gx::CloudField::k_MaxLayers; ++i)
    {
        EXPECT_TRUE(field.AddLayer(gx::CloudLayer{}));
    }
    EXPECT_EQ(field.GetLayerCount(), gx::CloudField::k_MaxLayers);
    // One more should fail
    EXPECT_FALSE(field.AddLayer(gx::CloudLayer{}));
}

TEST(CloudFieldTest, InitializeDefaultLayersCreatesThree)
{
    gx::CloudField field;
    field.InitializeDefaultLayers();
    EXPECT_EQ(field.GetLayerCount(), 3u);

    // Low (stratus) altitude < mid (cumulus) < high (cirrus)
    EXPECT_LT(field.GetLayer(0).altitudeBottom, field.GetLayer(1).altitudeBottom);
    EXPECT_LT(field.GetLayer(1).altitudeBottom, field.GetLayer(2).altitudeBottom);

    // Dominant class matches expected
    EXPECT_EQ(field.GetLayer(0).dominantClass, gx::CloudClass::Stratus);
    EXPECT_EQ(field.GetLayer(1).dominantClass, gx::CloudClass::Cumulus);
    EXPECT_EQ(field.GetLayer(2).dominantClass, gx::CloudClass::Cirrus);

    // High cirrus has reduced animationSpeed
    EXPECT_LT(field.GetLayer(2).animationSpeed, 1.0f);
}

TEST(CloudFieldTest, SetEnabledTogglesFlag)
{
    gx::CloudField field;
    EXPECT_TRUE(field.IsEnabled());
    field.SetEnabled(false);
    EXPECT_FALSE(field.IsEnabled());
    field.SetEnabled(true);
    EXPECT_TRUE(field.IsEnabled());
}

TEST(CloudFieldTest, ClearLayersEmptiesField)
{
    gx::CloudField field;
    field.InitializeDefaultLayers();
    ASSERT_GT(field.GetLayerCount(), 0u);
    field.ClearLayers();
    EXPECT_EQ(field.GetLayerCount(), 0u);
}

TEST(CloudFieldTest, WeatherMapAccessor)
{
    gx::CloudField field;
    auto& wm = field.GetWeatherMap();
    ASSERT_TRUE(wm.Initialize(8, 8, nullptr));
    EXPECT_TRUE(field.GetWeatherMap().IsInitialized());
}

TEST(CloudFieldTest, GetLayerOutOfBoundsReturnsFallback)
{
    gx::CloudField field;
    // No layers added — GetLayer(0) returns static fallback (documented behavior)
    const auto& layer = field.GetLayer(0);
    EXPECT_EQ(layer.altitudeBottom, 1000.0f);  // default value
}

} // namespace
