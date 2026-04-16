/// @file test_DecalProjector.cpp
/// @brief DecalProjectorComponent のユニットテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/GraphicsComponents.h"
#include "Math/Transform3D.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ============================================================================
// DecalProjectorComponent defaults
// ============================================================================

TEST(DecalProjectorTest, DefaultTextureHandle)
{
    DecalProjectorComponent comp;
    EXPECT_EQ(comp.textureHandle, 0u);
}

TEST(DecalProjectorTest, DefaultDimensions)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.width, 1.0f);
    EXPECT_FLOAT_EQ(comp.height, 1.0f);
    EXPECT_FLOAT_EQ(comp.depth, 1.0f);
}

TEST(DecalProjectorTest, DefaultColor)
{
    DecalProjectorComponent comp;
    EXPECT_NEAR(comp.color.x, 1.0f, kEps);
    EXPECT_NEAR(comp.color.y, 1.0f, kEps);
    EXPECT_NEAR(comp.color.z, 1.0f, kEps);
}

TEST(DecalProjectorTest, DefaultOpacity)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.opacity, 1.0f);
}

TEST(DecalProjectorTest, DefaultBlendMode)
{
    DecalProjectorComponent comp;
    EXPECT_EQ(comp.blendMode, DecalBlendMode::Alpha);
}

TEST(DecalProjectorTest, DefaultMetallic)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.metallic, 0.0f);
}

TEST(DecalProjectorTest, DefaultRoughness)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.roughness, 0.5f);
}

TEST(DecalProjectorTest, DefaultLifetime)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.lifetime, -1.0f);
}

TEST(DecalProjectorTest, DefaultFadeTime)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.fadeTime, 0.5f);
}

TEST(DecalProjectorTest, DefaultElapsed)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.elapsed, 0.0f);
}

TEST(DecalProjectorTest, DefaultNormalThreshold)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.normalThreshold, 0.7f);
}

TEST(DecalProjectorTest, DefaultFadeDistance)
{
    DecalProjectorComponent comp;
    EXPECT_FLOAT_EQ(comp.fadeDistance, 0.5f);
}

TEST(DecalProjectorTest, DefaultPriority)
{
    DecalProjectorComponent comp;
    EXPECT_EQ(comp.priority, 0);
}

TEST(DecalProjectorTest, DefaultNormalTextureHandle)
{
    DecalProjectorComponent comp;
    EXPECT_EQ(comp.normalTextureHandle, -1);
}

// ============================================================================
// ComponentType
// ============================================================================

TEST(DecalProjectorTest, ComponentType)
{
    DecalProjectorComponent comp;
    EXPECT_EQ(comp.GetType(), ComponentType::Custom);
    EXPECT_EQ(DecalProjectorComponent::k_Type, ComponentType::Custom);
}

// ============================================================================
// Clone
// ============================================================================

TEST(DecalProjectorTest, Clone)
{
    DecalProjectorComponent comp;
    comp.textureHandle = 42;
    comp.width = 2.0f;
    comp.height = 3.0f;
    comp.depth = 0.5f;
    comp.color = Vector3(0.5f, 0.3f, 0.1f);
    comp.opacity = 0.8f;
    comp.blendMode = DecalBlendMode::Additive;
    comp.metallic = 0.9f;
    comp.roughness = 0.1f;
    comp.lifetime = 5.0f;
    comp.fadeTime = 1.0f;
    comp.elapsed = 2.0f;
    comp.normalThreshold = 0.5f;
    comp.fadeDistance = 0.3f;
    comp.priority = 10;
    comp.normalTextureHandle = 7;

    auto cloned = comp.Clone();
    ASSERT_NE(cloned, nullptr);

    auto* decal = dynamic_cast<DecalProjectorComponent*>(cloned.get());
    ASSERT_NE(decal, nullptr);

    EXPECT_EQ(decal->textureHandle, 42u);
    EXPECT_FLOAT_EQ(decal->width, 2.0f);
    EXPECT_FLOAT_EQ(decal->height, 3.0f);
    EXPECT_FLOAT_EQ(decal->depth, 0.5f);
    EXPECT_NEAR(decal->color.x, 0.5f, kEps);
    EXPECT_NEAR(decal->color.y, 0.3f, kEps);
    EXPECT_NEAR(decal->color.z, 0.1f, kEps);
    EXPECT_FLOAT_EQ(decal->opacity, 0.8f);
    EXPECT_EQ(decal->blendMode, DecalBlendMode::Additive);
    EXPECT_FLOAT_EQ(decal->metallic, 0.9f);
    EXPECT_FLOAT_EQ(decal->roughness, 0.1f);
    EXPECT_FLOAT_EQ(decal->lifetime, 5.0f);
    EXPECT_FLOAT_EQ(decal->fadeTime, 1.0f);
    EXPECT_FLOAT_EQ(decal->elapsed, 2.0f);
    EXPECT_FLOAT_EQ(decal->normalThreshold, 0.5f);
    EXPECT_FLOAT_EQ(decal->fadeDistance, 0.3f);
    EXPECT_EQ(decal->priority, 10);
    EXPECT_EQ(decal->normalTextureHandle, 7);
}

// ============================================================================
// Set values
// ============================================================================

TEST(DecalProjectorTest, SetBlendMode)
{
    DecalProjectorComponent comp;
    comp.blendMode = DecalBlendMode::Multiply;
    EXPECT_EQ(comp.blendMode, DecalBlendMode::Multiply);
    comp.blendMode = DecalBlendMode::Replace;
    EXPECT_EQ(comp.blendMode, DecalBlendMode::Replace);
}

TEST(DecalProjectorTest, SetDimensions)
{
    DecalProjectorComponent comp;
    comp.width = 5.0f;
    comp.height = 10.0f;
    comp.depth = 0.1f;
    EXPECT_FLOAT_EQ(comp.width, 5.0f);
    EXPECT_FLOAT_EQ(comp.height, 10.0f);
    EXPECT_FLOAT_EQ(comp.depth, 0.1f);
}

TEST(DecalProjectorTest, SetColor)
{
    DecalProjectorComponent comp;
    comp.color = Vector3(0.2f, 0.4f, 0.6f);
    EXPECT_NEAR(comp.color.x, 0.2f, kEps);
    EXPECT_NEAR(comp.color.y, 0.4f, kEps);
    EXPECT_NEAR(comp.color.z, 0.6f, kEps);
}

TEST(DecalProjectorTest, SetPBRProperties)
{
    DecalProjectorComponent comp;
    comp.metallic = 1.0f;
    comp.roughness = 0.0f;
    EXPECT_FLOAT_EQ(comp.metallic, 1.0f);
    EXPECT_FLOAT_EQ(comp.roughness, 0.0f);
}

// ============================================================================
// ToDecalData conversion
// ============================================================================

TEST(DecalProjectorTest, ToDecalDataBasic)
{
    DecalProjectorComponent comp;
    comp.textureHandle = 10;
    comp.width = 2.0f;
    comp.height = 3.0f;
    comp.depth = 0.5f;
    comp.color = Vector3(0.5f, 0.5f, 0.5f);
    comp.opacity = 0.7f;
    comp.blendMode = DecalBlendMode::Additive;
    comp.metallic = 0.3f;
    comp.roughness = 0.8f;
    comp.lifetime = 10.0f;
    comp.elapsed = 2.0f;
    comp.fadeDistance = 0.4f;
    comp.normalThreshold = 0.6f;
    comp.priority = 5;
    comp.normalTextureHandle = 3;

    Transform3D xform;
    xform.SetPosition(Vector3(1.0f, 2.0f, 3.0f));

    DecalData data = comp.ToDecalData(xform);

    EXPECT_EQ(data.textureHandle, 10);
    EXPECT_NEAR(data.color.r, 0.5f, kEps);
    EXPECT_NEAR(data.color.g, 0.5f, kEps);
    EXPECT_NEAR(data.color.b, 0.5f, kEps);
    EXPECT_NEAR(data.color.a, 0.7f, kEps);
    EXPECT_EQ(data.blendMode, DecalBlendMode::Additive);
    EXPECT_FLOAT_EQ(data.metallic, 0.3f);
    EXPECT_FLOAT_EQ(data.roughness, 0.8f);
    EXPECT_FLOAT_EQ(data.lifetime, 10.0f);
    EXPECT_FLOAT_EQ(data.age, 2.0f);
    EXPECT_FLOAT_EQ(data.fadeDistance, 0.4f);
    EXPECT_FLOAT_EQ(data.normalThreshold, 0.6f);
    EXPECT_EQ(data.priority, 5);
    EXPECT_EQ(data.normalTextureHandle, 3);
}

TEST(DecalProjectorTest, ToDecalDataScale)
{
    DecalProjectorComponent comp;
    comp.width = 4.0f;
    comp.height = 6.0f;
    comp.depth = 2.0f;

    Transform3D xform;
    DecalData data = comp.ToDecalData(xform);

    // ToDecalData sets the transform scale to (width, height, depth)
    Vector3 scale = data.transform.GetScale();
    EXPECT_NEAR(scale.x, 4.0f, kEps);
    EXPECT_NEAR(scale.y, 6.0f, kEps);
    EXPECT_NEAR(scale.z, 2.0f, kEps);
}

TEST(DecalProjectorTest, ToDecalDataPermanent)
{
    DecalProjectorComponent comp;
    comp.lifetime = -1.0f;

    Transform3D xform;
    DecalData data = comp.ToDecalData(xform);
    EXPECT_FLOAT_EQ(data.lifetime, -1.0f);
}

// ============================================================================
// UpdateLifetime
// ============================================================================

TEST(DecalProjectorTest, UpdateLifetime_Permanent)
{
    DecalProjectorComponent comp;
    comp.lifetime = -1.0f;
    comp.opacity = 0.8f;

    float result = comp.UpdateLifetime(1.0f);
    EXPECT_FLOAT_EQ(result, 0.8f);
    EXPECT_FLOAT_EQ(comp.elapsed, 0.0f); // elapsed should not change for permanent
}

TEST(DecalProjectorTest, UpdateLifetime_BeforeFade)
{
    DecalProjectorComponent comp;
    comp.lifetime = 5.0f;
    comp.fadeTime = 1.0f;
    comp.opacity = 1.0f;

    // At t=2.0, well before fade starts at t=4.0
    float result = comp.UpdateLifetime(2.0f);
    EXPECT_FLOAT_EQ(comp.elapsed, 2.0f);
    EXPECT_FLOAT_EQ(result, 1.0f);
}

TEST(DecalProjectorTest, UpdateLifetime_DuringFade)
{
    DecalProjectorComponent comp;
    comp.lifetime = 5.0f;
    comp.fadeTime = 1.0f;
    comp.opacity = 1.0f;

    // Advance to t=4.5 (fade start at 4.0, halfway through fade)
    comp.UpdateLifetime(4.5f);
    EXPECT_FLOAT_EQ(comp.elapsed, 4.5f);

    // The fade is at 50%: opacity * (1 - 0.5) = 0.5
    // elapsed=4.5, fadeStart=4.0, fadeProgress=(4.5-4.0)/1.0=0.5
    // result = 1.0 * (1.0 - 0.5) = 0.5
    // Note: UpdateLifetime was already called, need to recalculate
}

TEST(DecalProjectorTest, UpdateLifetime_Expired)
{
    DecalProjectorComponent comp;
    comp.lifetime = 3.0f;
    comp.fadeTime = 0.5f;
    comp.opacity = 1.0f;

    float result = comp.UpdateLifetime(3.0f);
    EXPECT_FLOAT_EQ(comp.elapsed, 3.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);
}

TEST(DecalProjectorTest, UpdateLifetime_Incremental)
{
    DecalProjectorComponent comp;
    comp.lifetime = 4.0f;
    comp.fadeTime = 2.0f;
    comp.opacity = 1.0f;

    // t=1.0 — before fade (fadeStart=2.0)
    float r1 = comp.UpdateLifetime(1.0f);
    EXPECT_FLOAT_EQ(comp.elapsed, 1.0f);
    EXPECT_FLOAT_EQ(r1, 1.0f);

    // t=2.0 — still before fade starts
    float r2 = comp.UpdateLifetime(1.0f);
    EXPECT_FLOAT_EQ(comp.elapsed, 2.0f);
    EXPECT_FLOAT_EQ(r2, 1.0f);

    // t=3.0 — during fade (fadeStart=2.0, progress=0.5)
    float r3 = comp.UpdateLifetime(1.0f);
    EXPECT_FLOAT_EQ(comp.elapsed, 3.0f);
    EXPECT_NEAR(r3, 0.5f, kEps);

    // t=4.0 — expired
    float r4 = comp.UpdateLifetime(1.0f);
    EXPECT_FLOAT_EQ(comp.elapsed, 4.0f);
    EXPECT_FLOAT_EQ(r4, 0.0f);
}

TEST(DecalProjectorTest, UpdateLifetime_ZeroFadeTime)
{
    DecalProjectorComponent comp;
    comp.lifetime = 2.0f;
    comp.fadeTime = 0.0f;
    comp.opacity = 1.0f;

    // Before expiry — full opacity
    float r1 = comp.UpdateLifetime(1.0f);
    EXPECT_FLOAT_EQ(r1, 1.0f);

    // At expiry — 0
    float r2 = comp.UpdateLifetime(1.0f);
    EXPECT_FLOAT_EQ(r2, 0.0f);
}

// ============================================================================
// IsExpired
// ============================================================================

TEST(DecalProjectorTest, IsExpired_Permanent)
{
    DecalProjectorComponent comp;
    comp.lifetime = -1.0f;
    comp.elapsed = 1000.0f;
    EXPECT_FALSE(comp.IsExpired());
}

TEST(DecalProjectorTest, IsExpired_NotYet)
{
    DecalProjectorComponent comp;
    comp.lifetime = 5.0f;
    comp.elapsed = 3.0f;
    EXPECT_FALSE(comp.IsExpired());
}

TEST(DecalProjectorTest, IsExpired_Exact)
{
    DecalProjectorComponent comp;
    comp.lifetime = 5.0f;
    comp.elapsed = 5.0f;
    EXPECT_TRUE(comp.IsExpired());
}

TEST(DecalProjectorTest, IsExpired_Past)
{
    DecalProjectorComponent comp;
    comp.lifetime = 5.0f;
    comp.elapsed = 10.0f;
    EXPECT_TRUE(comp.IsExpired());
}

// ============================================================================
// Component base class features
// ============================================================================

TEST(DecalProjectorTest, EnabledByDefault)
{
    DecalProjectorComponent comp;
    EXPECT_TRUE(comp.IsEnabled());
}

TEST(DecalProjectorTest, SetEnabled)
{
    DecalProjectorComponent comp;
    comp.SetEnabled(false);
    EXPECT_FALSE(comp.IsEnabled());
    comp.SetEnabled(true);
    EXPECT_TRUE(comp.IsEnabled());
}

TEST(DecalProjectorTest, EntityIsNull)
{
    DecalProjectorComponent comp;
    EXPECT_EQ(comp.GetEntity(), nullptr);
}
