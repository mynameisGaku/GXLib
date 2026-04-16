/// @file test_VolumetricFog.cpp
/// @brief VolumetricFog 設定・FogVolume管理のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/VolumetricFog.h"

using namespace gx;

// ==================== デフォルト設定 ====================

TEST(VolumetricFogTest, DefaultSettings_Disabled)
{
    VolumetricFogSettings settings;
    EXPECT_FALSE(settings.enabled);
}

TEST(VolumetricFogTest, DefaultSettings_MaxDistance)
{
    VolumetricFogSettings settings;
    EXPECT_FLOAT_EQ(settings.maxDistance, 200.0f);
}

TEST(VolumetricFogTest, DefaultSettings_Resolution)
{
    VolumetricFogSettings settings;
    EXPECT_EQ(settings.resolutionX, 160u);
    EXPECT_EQ(settings.resolutionY, 90u);
    EXPECT_EQ(settings.resolutionZ, 64u);
}

TEST(VolumetricFogTest, DefaultSettings_GlobalDensity)
{
    VolumetricFogSettings settings;
    EXPECT_FLOAT_EQ(settings.globalDensity, 0.02f);
}

TEST(VolumetricFogTest, DefaultSettings_GlobalAbsorption)
{
    VolumetricFogSettings settings;
    EXPECT_FLOAT_EQ(settings.globalAbsorption, 0.01f);
}

// ==================== FogVolume デフォルト ====================

TEST(VolumetricFogTest, FogVolume_DefaultShape)
{
    FogVolume volume;
    EXPECT_EQ(volume.shape, FogVolumeShape::Box);
}

TEST(VolumetricFogTest, FogVolume_DefaultDensity)
{
    FogVolume volume;
    EXPECT_FLOAT_EQ(volume.density, 1.0f);
}

TEST(VolumetricFogTest, FogVolume_DefaultAbsorption)
{
    FogVolume volume;
    EXPECT_FLOAT_EQ(volume.absorption, 0.1f);
}

TEST(VolumetricFogTest, FogVolume_DefaultScatteringAnisotropy)
{
    FogVolume volume;
    EXPECT_FLOAT_EQ(volume.scatteringAnisotropy, 0.3f);
}

TEST(VolumetricFogTest, FogVolume_ShapeEnum)
{
    EXPECT_EQ(static_cast<uint32_t>(FogVolumeShape::Box), 0u);
    EXPECT_EQ(static_cast<uint32_t>(FogVolumeShape::Sphere), 1u);
}

// ==================== VolumetricFog クラス ====================

TEST(VolumetricFogTest, IsEnabled_Default)
{
    VolumetricFog fog;
    EXPECT_FALSE(fog.IsEnabled());
}

TEST(VolumetricFogTest, SetSettings_RoundTrip)
{
    VolumetricFog fog;
    VolumetricFogSettings settings;
    settings.enabled       = true;
    settings.maxDistance    = 500.0f;
    settings.resolutionX   = 320;
    settings.resolutionY   = 180;
    settings.resolutionZ   = 128;
    settings.globalDensity = 0.05f;
    settings.globalAbsorption = 0.03f;

    fog.SetSettings(settings);
    const auto& result = fog.GetSettings();

    EXPECT_TRUE(result.enabled);
    EXPECT_FLOAT_EQ(result.maxDistance, 500.0f);
    EXPECT_EQ(result.resolutionX, 320u);
    EXPECT_EQ(result.resolutionY, 180u);
    EXPECT_EQ(result.resolutionZ, 128u);
    EXPECT_FLOAT_EQ(result.globalDensity, 0.05f);
    EXPECT_FLOAT_EQ(result.globalAbsorption, 0.03f);
}

TEST(VolumetricFogTest, AddFogVolume_IncreasesCount)
{
    VolumetricFog fog;
    EXPECT_EQ(fog.GetFogVolumeCount(), 0u);

    FogVolume v1;
    v1.density = 2.0f;
    fog.AddFogVolume(v1);
    EXPECT_EQ(fog.GetFogVolumeCount(), 1u);

    FogVolume v2;
    v2.shape = FogVolumeShape::Sphere;
    fog.AddFogVolume(v2);
    EXPECT_EQ(fog.GetFogVolumeCount(), 2u);
}

TEST(VolumetricFogTest, RemoveFogVolume_DecreasesCount)
{
    VolumetricFog fog;
    FogVolume v;
    int id0 = fog.AddFogVolume(v);
    fog.AddFogVolume(v);
    EXPECT_EQ(fog.GetFogVolumeCount(), 2u);

    fog.RemoveFogVolume(id0);
    EXPECT_EQ(fog.GetFogVolumeCount(), 1u);
}

TEST(VolumetricFogTest, RemoveFogVolume_InvalidId)
{
    VolumetricFog fog;
    // 無効なIDで削除しても例外が出ないことを確認
    fog.RemoveFogVolume(-1);
    fog.RemoveFogVolume(100);
    EXPECT_EQ(fog.GetFogVolumeCount(), 0u);
}

TEST(VolumetricFogTest, IsEnabled_AfterSetSettings)
{
    VolumetricFog fog;
    VolumetricFogSettings settings;
    settings.enabled = true;
    fog.SetSettings(settings);
    EXPECT_TRUE(fog.IsEnabled());
}
