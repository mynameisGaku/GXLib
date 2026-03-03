/// @file test_GraphicsDevice.cpp
/// @brief Graphics Device / MaterialManager / Material / ShaderModelGPUParams CPU-only tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Material.h"
#include "Graphics/3D/ShaderModelConstants.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ============================================================================
// MaterialManager: Create / Get / Release / Freelist
// ============================================================================

TEST(GraphicsDeviceTest, MaterialManagerCreate)
{
    MaterialManager mgr;
    Material mat;
    int handle = mgr.CreateMaterial(mat);
    EXPECT_GE(handle, 0);
}

TEST(GraphicsDeviceTest, MaterialManagerGet)
{
    MaterialManager mgr;
    Material mat;
    mat.constants.metallicFactor = 0.9f;
    int h = mgr.CreateMaterial(mat);
    Material* got = mgr.GetMaterial(h);
    ASSERT_NE(got, nullptr);
    EXPECT_NEAR(got->constants.metallicFactor, 0.9f, kEps);
}

TEST(GraphicsDeviceTest, MaterialManagerRelease)
{
    MaterialManager mgr;
    Material mat;
    int h = mgr.CreateMaterial(mat);
    mgr.ReleaseMaterial(h);
    EXPECT_EQ(mgr.GetMaterial(h), nullptr);
}

TEST(GraphicsDeviceTest, MaterialManagerReuseHandle)
{
    MaterialManager mgr;
    Material mat;
    int h1 = mgr.CreateMaterial(mat);
    mgr.ReleaseMaterial(h1);
    int h2 = mgr.CreateMaterial(mat);
    // Freelist should reuse the released handle
    EXPECT_EQ(h1, h2);
}

TEST(GraphicsDeviceTest, MaterialManagerMaxMaterials)
{
    EXPECT_EQ(MaterialManager::k_MaxMaterials, 256u);
}

TEST(GraphicsDeviceTest, MaterialSetTexture)
{
    MaterialManager mgr;
    Material mat;
    int h = mgr.CreateMaterial(mat);
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Albedo, 100));
    Material* got = mgr.GetMaterial(h);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->albedoMapHandle, 100);
}

TEST(GraphicsDeviceTest, MaterialSetShaderHandle)
{
    MaterialManager mgr;
    Material mat;
    int h = mgr.CreateMaterial(mat);
    EXPECT_TRUE(mgr.SetShaderHandle(h, 55));
    Material* got = mgr.GetMaterial(h);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->shaderHandle, 55);
}

// ============================================================================
// Material struct defaults and POM
// ============================================================================

TEST(GraphicsDeviceTest, MaterialDefaultValues)
{
    Material mat;
    // Default albedo = white
    EXPECT_NEAR(mat.constants.albedoFactor.r, 1.0f, kEps);
    EXPECT_NEAR(mat.constants.albedoFactor.g, 1.0f, kEps);
    EXPECT_NEAR(mat.constants.albedoFactor.b, 1.0f, kEps);
    EXPECT_NEAR(mat.constants.albedoFactor.a, 1.0f, kEps);
    // Default metallic = 0, roughness = 0.5
    EXPECT_NEAR(mat.constants.metallicFactor, 0.0f, kEps);
    EXPECT_NEAR(mat.constants.roughnessFactor, 0.5f, kEps);
    // Default AO strength = 1
    EXPECT_NEAR(mat.constants.aoStrength, 1.0f, kEps);
    // Default emissive = 0
    EXPECT_NEAR(mat.constants.emissiveStrength, 0.0f, kEps);
    // Default texture handles = -1
    EXPECT_EQ(mat.albedoMapHandle, -1);
    EXPECT_EQ(mat.normalMapHandle, -1);
    EXPECT_EQ(mat.metRoughMapHandle, -1);
    EXPECT_EQ(mat.shaderHandle, -1);
    // Default shader model = Standard
    EXPECT_EQ(mat.shaderModel, gxfmt::ShaderModel::Standard);
}

TEST(GraphicsDeviceTest, MaterialTextureHandleDefaults)
{
    Material mat;
    EXPECT_EQ(mat.aoMapHandle, -1);
    EXPECT_EQ(mat.emissiveMapHandle, -1);
    EXPECT_EQ(mat.toonRampMapHandle, -1);
    EXPECT_EQ(mat.subsurfaceMapHandle, -1);
    EXPECT_EQ(mat.clearCoatMaskMapHandle, -1);
}

TEST(GraphicsDeviceTest, MaterialShaderParams)
{
    Material mat;
    // Default shader model params match ShaderModelParams member initializers
    EXPECT_NEAR(mat.shaderParams.baseColor[0], 1.0f, kEps);
    EXPECT_NEAR(mat.shaderParams.metallic, 0.0f, kEps);
    EXPECT_NEAR(mat.shaderParams.roughness, 0.5f, kEps);
}

// ============================================================================
// ShaderModel and flags
// ============================================================================

TEST(GraphicsDeviceTest, MaterialShaderModel)
{
    Material mat;
    mat.shaderModel = gxfmt::ShaderModel::Toon;
    EXPECT_EQ(mat.shaderModel, gxfmt::ShaderModel::Toon);
    mat.shaderModel = gxfmt::ShaderModel::Phong;
    EXPECT_EQ(mat.shaderModel, gxfmt::ShaderModel::Phong);
}

TEST(GraphicsDeviceTest, MaterialFlags)
{
    MaterialConstants mc{};
    mc.flags = MaterialFlags::HasAlbedoMap;
    EXPECT_TRUE(mc.flags & MaterialFlags::HasAlbedoMap);
    EXPECT_FALSE(mc.flags & MaterialFlags::HasNormalMap);
}

TEST(GraphicsDeviceTest, MaterialFlagsMultiple)
{
    MaterialConstants mc{};
    mc.flags = MaterialFlags::HasAlbedoMap
             | MaterialFlags::HasNormalMap
             | MaterialFlags::HasEmissiveMap;
    EXPECT_TRUE(mc.flags & MaterialFlags::HasAlbedoMap);
    EXPECT_TRUE(mc.flags & MaterialFlags::HasNormalMap);
    EXPECT_FALSE(mc.flags & MaterialFlags::HasMetRoughMap);
    EXPECT_FALSE(mc.flags & MaterialFlags::HasAOMap);
    EXPECT_TRUE(mc.flags & MaterialFlags::HasEmissiveMap);
}

TEST(GraphicsDeviceTest, MaterialConstantsDefaults)
{
    MaterialConstants mc{};
    // Color defaults to white (1, 1, 1, 1)
    EXPECT_NEAR(mc.albedoFactor.r, 1.0f, kEps);
    EXPECT_NEAR(mc.metallicFactor, 0.0f, kEps);
    EXPECT_NEAR(mc.roughnessFactor, 0.0f, kEps);
    EXPECT_EQ(mc.flags, 0u);
}

// ============================================================================
// ShaderModelGPUParams conversion
// ============================================================================

TEST(GraphicsDeviceTest, ShaderModelGPUParamsSize)
{
    EXPECT_EQ(sizeof(ShaderModelGPUParams), 256u);
}

TEST(GraphicsDeviceTest, ConvertFromLegacy)
{
    MaterialConstants legacy{};
    legacy.albedoFactor = { 0.8f, 0.6f, 0.4f, 1.0f };
    legacy.metallicFactor = 0.7f;
    legacy.roughnessFactor = 0.3f;
    legacy.aoStrength = 0.9f;
    legacy.emissiveStrength = 1.5f;
    legacy.emissiveFactor = { 1.0f, 0.5f, 0.0f };
    legacy.flags = MaterialFlags::HasAlbedoMap | MaterialFlags::HasNormalMap;

    ShaderModelGPUParams gpu = ConvertFromLegacy(legacy);

    EXPECT_NEAR(gpu.baseColor.x, 0.8f, kEps);
    EXPECT_NEAR(gpu.baseColor.y, 0.6f, kEps);
    EXPECT_NEAR(gpu.baseColor.z, 0.4f, kEps);
    EXPECT_NEAR(gpu.baseColor.w, 1.0f, kEps);
    EXPECT_NEAR(gpu.metallic, 0.7f, kEps);
    EXPECT_NEAR(gpu.roughness, 0.3f, kEps);
    EXPECT_NEAR(gpu.aoStrength, 0.9f, kEps);
    EXPECT_NEAR(gpu.emissiveStrength, 1.5f, kEps);
    EXPECT_NEAR(gpu.emissiveFactor.x, 1.0f, kEps);
    EXPECT_EQ(gpu.shaderModelId, 0u); // Standard
    EXPECT_EQ(gpu.flags, MaterialFlags::HasAlbedoMap | MaterialFlags::HasNormalMap);
}

TEST(GraphicsDeviceTest, ConvertToGPUParams)
{
    gxfmt::ShaderModelParams src{};
    src.baseColor[0] = 1.0f;
    src.baseColor[1] = 0.0f;
    src.baseColor[2] = 0.0f;
    src.baseColor[3] = 1.0f;
    src.metallic = 1.0f;
    src.roughness = 0.1f;
    src.normalScale = 2.0f;

    ShaderModelGPUParams gpu = ConvertToGPUParams(
        src, gxfmt::ShaderModel::Standard, MaterialFlags::HasAlbedoMap);

    EXPECT_NEAR(gpu.baseColor.x, 1.0f, kEps);
    EXPECT_NEAR(gpu.baseColor.y, 0.0f, kEps);
    EXPECT_NEAR(gpu.metallic, 1.0f, kEps);
    EXPECT_NEAR(gpu.roughness, 0.1f, kEps);
    EXPECT_NEAR(gpu.normalScale, 2.0f, kEps);
    EXPECT_EQ(gpu.shaderModelId, static_cast<uint32_t>(gxfmt::ShaderModel::Standard));
    EXPECT_EQ(gpu.flags, MaterialFlags::HasAlbedoMap);
}
