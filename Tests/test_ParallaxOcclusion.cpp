/// @file test_ParallaxOcclusion.cpp
/// @brief Material texture slot and shader model parameter tests

#include <gtest/gtest.h>
#include "Graphics/3D/Material.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

TEST(ParallaxOcclusionTest, MaterialDefaultTextureHandles)
{
    Material mat;
    EXPECT_EQ(mat.albedoMapHandle, -1);
    EXPECT_EQ(mat.normalMapHandle, -1);
    EXPECT_EQ(mat.metRoughMapHandle, -1);
    EXPECT_EQ(mat.aoMapHandle, -1);
    EXPECT_EQ(mat.emissiveMapHandle, -1);
}

TEST(ParallaxOcclusionTest, MaterialShaderModelDefault)
{
    Material mat;
    EXPECT_EQ(mat.shaderModel, gxfmt::ShaderModel::Standard);
}

TEST(ParallaxOcclusionTest, MaterialShaderModelChange)
{
    Material mat;
    mat.shaderModel = gxfmt::ShaderModel::Toon;
    EXPECT_EQ(mat.shaderModel, gxfmt::ShaderModel::Toon);
    mat.shaderModel = gxfmt::ShaderModel::Phong;
    EXPECT_EQ(mat.shaderModel, gxfmt::ShaderModel::Phong);
}

TEST(ParallaxOcclusionTest, MaterialConstantsFlags)
{
    Material mat;
    mat.constants.flags = MaterialFlags::HasAlbedoMap | MaterialFlags::HasNormalMap;
    EXPECT_TRUE(mat.constants.flags & MaterialFlags::HasAlbedoMap);
    EXPECT_TRUE(mat.constants.flags & MaterialFlags::HasNormalMap);
    EXPECT_FALSE(mat.constants.flags & MaterialFlags::HasEmissiveMap);
}

TEST(ParallaxOcclusionTest, MaterialConstantsDefaults)
{
    Material mat;
    EXPECT_NEAR(mat.constants.albedoFactor.x, 1.0f, kEps);
    EXPECT_NEAR(mat.constants.metallicFactor, 0.0f, kEps);
    EXPECT_NEAR(mat.constants.roughnessFactor, 0.5f, kEps);
    EXPECT_NEAR(mat.constants.aoStrength, 1.0f, kEps);
    EXPECT_NEAR(mat.constants.emissiveStrength, 0.0f, kEps);
}

TEST(ParallaxOcclusionTest, MaterialManagerSetTexture)
{
    MaterialManager mgr;
    Material mat;
    int h = mgr.CreateMaterial(mat);
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Normal, 42));
    Material* got = mgr.GetMaterial(h);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->normalMapHandle, 42);
}

TEST(ParallaxOcclusionTest, MaterialShaderParamsDefault)
{
    gxfmt::ShaderModelParams params{};
    EXPECT_NEAR(params.metallic, 0.0f, kEps);
    EXPECT_NEAR(params.roughness, 0.5f, kEps);
    EXPECT_NEAR(params.normalScale, 1.0f, kEps);
    EXPECT_NEAR(params.aoStrength, 1.0f, kEps);
}
