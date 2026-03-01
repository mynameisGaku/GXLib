/// @file test_Material.cpp
/// @brief Material / MaterialManager / MaterialConstants のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Material.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ==================== MaterialConstants ====================

TEST(MaterialTest, Constants_DefaultAlbedo)
{
    MaterialConstants mc{};
    EXPECT_NEAR(mc.albedoFactor.x, 0.0f, kEps);
    EXPECT_NEAR(mc.albedoFactor.y, 0.0f, kEps);
    EXPECT_NEAR(mc.albedoFactor.z, 0.0f, kEps);
    EXPECT_NEAR(mc.albedoFactor.w, 0.0f, kEps);
}

TEST(MaterialTest, Constants_SetValues)
{
    MaterialConstants mc{};
    mc.albedoFactor = {1.0f, 0.5f, 0.2f, 1.0f};
    mc.metallicFactor = 0.8f;
    mc.roughnessFactor = 0.3f;
    mc.aoStrength = 1.0f;
    EXPECT_NEAR(mc.metallicFactor, 0.8f, kEps);
    EXPECT_NEAR(mc.roughnessFactor, 0.3f, kEps);
    EXPECT_NEAR(mc.aoStrength, 1.0f, kEps);
}

TEST(MaterialTest, Constants_EmissiveFactor)
{
    MaterialConstants mc{};
    mc.emissiveFactor = {2.0f, 1.0f, 0.5f};
    mc.emissiveStrength = 3.0f;
    EXPECT_NEAR(mc.emissiveFactor.x, 2.0f, kEps);
    EXPECT_NEAR(mc.emissiveStrength, 3.0f, kEps);
}

TEST(MaterialTest, Constants_Flags)
{
    MaterialConstants mc{};
    mc.flags = MaterialFlags::HasAlbedoMap | MaterialFlags::HasNormalMap;
    EXPECT_TRUE(mc.flags & MaterialFlags::HasAlbedoMap);
    EXPECT_TRUE(mc.flags & MaterialFlags::HasNormalMap);
    EXPECT_FALSE(mc.flags & MaterialFlags::HasMetRoughMap);
}

// ==================== MaterialFlags ====================

TEST(MaterialTest, Flags_AllUnique)
{
    uint32_t all[] = {
        MaterialFlags::HasAlbedoMap,
        MaterialFlags::HasNormalMap,
        MaterialFlags::HasMetRoughMap,
        MaterialFlags::HasAOMap,
        MaterialFlags::HasEmissiveMap,
        MaterialFlags::HasToonRampMap,
        MaterialFlags::HasSubsurfaceMap,
        MaterialFlags::HasClearCoatMaskMap,
    };
    for (int i = 0; i < 8; ++i)
        for (int j = i + 1; j < 8; ++j)
            EXPECT_NE(all[i], all[j]);
}

TEST(MaterialTest, Flags_ArePowerOf2)
{
    EXPECT_EQ(MaterialFlags::HasAlbedoMap, 1u << 0);
    EXPECT_EQ(MaterialFlags::HasNormalMap, 1u << 1);
    EXPECT_EQ(MaterialFlags::HasMetRoughMap, 1u << 2);
    EXPECT_EQ(MaterialFlags::HasAOMap, 1u << 3);
    EXPECT_EQ(MaterialFlags::HasEmissiveMap, 1u << 4);
    EXPECT_EQ(MaterialFlags::HasToonRampMap, 1u << 5);
}

// ==================== Material struct ====================

TEST(MaterialTest, Struct_DefaultHandles)
{
    Material mat{};
    EXPECT_EQ(mat.albedoMapHandle, -1);
    EXPECT_EQ(mat.normalMapHandle, -1);
}

TEST(MaterialTest, Struct_ShaderModel)
{
    Material mat{};
    mat.shaderModel = gxfmt::ShaderModel::Standard;
    EXPECT_EQ(mat.shaderModel, gxfmt::ShaderModel::Standard);
}

TEST(MaterialTest, Struct_SetTextureHandles)
{
    Material mat{};
    mat.albedoMapHandle = 42;
    mat.normalMapHandle = 7;
    mat.metRoughMapHandle = 13;
    EXPECT_EQ(mat.albedoMapHandle, 42);
    EXPECT_EQ(mat.normalMapHandle, 7);
    EXPECT_EQ(mat.metRoughMapHandle, 13);
}

// ==================== MaterialManager ====================

TEST(MaterialManagerTest, CreateMaterial_ReturnsHandle)
{
    MaterialManager mgr;
    Material mat{};
    mat.constants.albedoFactor = {1, 1, 1, 1};
    int h = mgr.CreateMaterial(mat);
    EXPECT_GE(h, 0);
}

TEST(MaterialManagerTest, GetMaterial_Valid)
{
    MaterialManager mgr;
    Material mat{};
    mat.constants.metallicFactor = 0.75f;
    int h = mgr.CreateMaterial(mat);
    Material* got = mgr.GetMaterial(h);
    EXPECT_NE(got, nullptr);
    EXPECT_NEAR(got->constants.metallicFactor, 0.75f, kEps);
}

TEST(MaterialManagerTest, GetMaterial_InvalidHandle)
{
    MaterialManager mgr;
    Material* got = mgr.GetMaterial(-1);
    EXPECT_EQ(got, nullptr);
    got = mgr.GetMaterial(999);
    EXPECT_EQ(got, nullptr);
}

TEST(MaterialManagerTest, ReleaseMaterial)
{
    MaterialManager mgr;
    Material mat{};
    int h = mgr.CreateMaterial(mat);
    mgr.ReleaseMaterial(h);
    Material* got = mgr.GetMaterial(h);
    EXPECT_EQ(got, nullptr);
}

TEST(MaterialManagerTest, ReuseHandleAfterRelease)
{
    MaterialManager mgr;
    Material mat{};
    int h1 = mgr.CreateMaterial(mat);
    mgr.ReleaseMaterial(h1);
    int h2 = mgr.CreateMaterial(mat);
    // フリーリストがハンドルを再利用するはず
    EXPECT_EQ(h1, h2);
}

TEST(MaterialManagerTest, MultipleCreates)
{
    MaterialManager mgr;
    Material mat{};
    int h1 = mgr.CreateMaterial(mat);
    int h2 = mgr.CreateMaterial(mat);
    int h3 = mgr.CreateMaterial(mat);
    EXPECT_NE(h1, h2);
    EXPECT_NE(h2, h3);
}

TEST(MaterialManagerTest, SetTexture)
{
    MaterialManager mgr;
    Material mat{};
    int h = mgr.CreateMaterial(mat);
    bool ok = mgr.SetTexture(h, MaterialTextureSlot::Albedo, 42);
    EXPECT_TRUE(ok);
    Material* got = mgr.GetMaterial(h);
    EXPECT_EQ(got->albedoMapHandle, 42);
}

TEST(MaterialManagerTest, SetTexture_InvalidHandle)
{
    MaterialManager mgr;
    bool ok = mgr.SetTexture(-1, MaterialTextureSlot::Albedo, 42);
    EXPECT_FALSE(ok);
}

TEST(MaterialManagerTest, SetTexture_AllSlots)
{
    MaterialManager mgr;
    Material mat{};
    int h = mgr.CreateMaterial(mat);
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Albedo, 1));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Normal, 2));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::MetalRoughness, 3));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::AO, 4));
    EXPECT_TRUE(mgr.SetTexture(h, MaterialTextureSlot::Emissive, 5));
    Material* got = mgr.GetMaterial(h);
    EXPECT_EQ(got->albedoMapHandle, 1);
    EXPECT_EQ(got->normalMapHandle, 2);
    EXPECT_EQ(got->metRoughMapHandle, 3);
    EXPECT_EQ(got->aoMapHandle, 4);
    EXPECT_EQ(got->emissiveMapHandle, 5);
}

TEST(MaterialManagerTest, SetShaderHandle)
{
    MaterialManager mgr;
    Material mat{};
    int h = mgr.CreateMaterial(mat);
    bool ok = mgr.SetShaderHandle(h, 99);
    EXPECT_TRUE(ok);
    Material* got = mgr.GetMaterial(h);
    EXPECT_EQ(got->shaderHandle, 99);
}

TEST(MaterialManagerTest, SetShaderHandle_Invalid)
{
    MaterialManager mgr;
    bool ok = mgr.SetShaderHandle(-1, 99);
    EXPECT_FALSE(ok);
}

TEST(MaterialManagerTest, MaxMaterials_Constant)
{
    EXPECT_EQ(MaterialManager::k_MaxMaterials, 256u);
}
