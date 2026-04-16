/// @file test_DecalDeferred.cpp
/// @brief Deferred Decal 拡張パラメータテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Decal.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ==================== DecalBlendMode enum ====================

TEST(DecalDeferredTest, BlendMode_Alpha_IsZero)
{
    EXPECT_EQ(static_cast<uint32_t>(DecalBlendMode::Alpha), 0u);
}

TEST(DecalDeferredTest, BlendMode_Additive_IsOne)
{
    EXPECT_EQ(static_cast<uint32_t>(DecalBlendMode::Additive), 1u);
}

TEST(DecalDeferredTest, BlendMode_Multiply_IsTwo)
{
    EXPECT_EQ(static_cast<uint32_t>(DecalBlendMode::Multiply), 2u);
}

TEST(DecalDeferredTest, BlendMode_Replace_IsThree)
{
    EXPECT_EQ(static_cast<uint32_t>(DecalBlendMode::Replace), 3u);
}

TEST(DecalDeferredTest, AllBlendModesDistinct)
{
    uint32_t a = static_cast<uint32_t>(DecalBlendMode::Alpha);
    uint32_t b = static_cast<uint32_t>(DecalBlendMode::Additive);
    uint32_t c = static_cast<uint32_t>(DecalBlendMode::Multiply);
    uint32_t d = static_cast<uint32_t>(DecalBlendMode::Replace);
    EXPECT_NE(a, b); EXPECT_NE(a, c); EXPECT_NE(a, d);
    EXPECT_NE(b, c); EXPECT_NE(b, d); EXPECT_NE(c, d);
}

// ==================== DecalData defaults ====================

TEST(DecalDeferredTest, DefaultBlendMode_IsAlpha)
{
    DecalData d;
    EXPECT_EQ(d.blendMode, DecalBlendMode::Alpha);
}

TEST(DecalDeferredTest, DefaultPriority_IsZero)
{
    DecalData d;
    EXPECT_EQ(d.priority, 0);
}

TEST(DecalDeferredTest, DefaultMetallic_IsZero)
{
    DecalData d;
    EXPECT_NEAR(d.metallic, 0.0f, kEps);
}

TEST(DecalDeferredTest, DefaultRoughness_IsHalf)
{
    DecalData d;
    EXPECT_NEAR(d.roughness, 0.5f, kEps);
}

TEST(DecalDeferredTest, DefaultNormalTexture_IsInvalid)
{
    DecalData d;
    EXPECT_EQ(d.normalTextureHandle, -1);
}

// ==================== Priority sorting ====================

TEST(DecalDeferredTest, PrioritySorting)
{
    DecalData a; a.priority = 1;
    DecalData b; b.priority = 5;
    DecalData c; c.priority = 3;
    gx::Vector<DecalData> decals = {a, b, c};
    std::sort(decals.begin(), decals.end(),
              [](const DecalData& x, const DecalData& y) { return x.priority < y.priority; });
    EXPECT_LE(decals[0].priority, decals[1].priority);
    EXPECT_LE(decals[1].priority, decals[2].priority);
}

TEST(DecalDeferredTest, SetBlendMode)
{
    DecalData d;
    d.blendMode = DecalBlendMode::Additive;
    EXPECT_EQ(d.blendMode, DecalBlendMode::Additive);
}

TEST(DecalDeferredTest, SetMetallicRoughness)
{
    DecalData d;
    d.metallic = 0.8f;
    d.roughness = 0.2f;
    EXPECT_NEAR(d.metallic, 0.8f, kEps);
    EXPECT_NEAR(d.roughness, 0.2f, kEps);
}

TEST(DecalDeferredTest, SetNormalTexture)
{
    DecalData d;
    d.normalTextureHandle = 42;
    EXPECT_EQ(d.normalTextureHandle, 42);
}

TEST(DecalDeferredTest, SetPriority)
{
    DecalData d;
    d.priority = 10;
    EXPECT_EQ(d.priority, 10);
}
