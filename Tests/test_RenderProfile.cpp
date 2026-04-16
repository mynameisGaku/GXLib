/// @file test_RenderProfile.cpp
/// @brief RenderProfile / RenderProfileStack ユニットテスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/RenderProfile.h"
#include <fstream>
#include <cstdio>

using namespace gx;

// ============================================================================
// Override<T> テスト
// ============================================================================

TEST(OverrideTest, DefaultNotOverridden)
{
    Override<float> o;
    EXPECT_FALSE(o.overridden);
    EXPECT_FLOAT_EQ(o.value, 0.0f);
}

TEST(OverrideTest, SetMarksOverridden)
{
    Override<float> o;
    o.Set(3.14f);
    EXPECT_TRUE(o.overridden);
    EXPECT_FLOAT_EQ(o.value, 3.14f);
}

TEST(OverrideTest, ClearResetsFlag)
{
    Override<int> o;
    o.Set(42);
    o.Clear();
    EXPECT_FALSE(o.overridden);
    EXPECT_EQ(o.value, 42); // value is preserved, only flag cleared
}

TEST(OverrideTest, MergeFromOverridden)
{
    Override<float> a, b;
    a.Set(1.0f);
    b.Set(2.0f);
    a.MergeFrom(b);
    EXPECT_TRUE(a.overridden);
    EXPECT_FLOAT_EQ(a.value, 2.0f);
}

TEST(OverrideTest, MergeFromNonOverriddenKeepsOriginal)
{
    Override<float> a, b;
    a.Set(1.0f);
    // b not overridden
    a.MergeFrom(b);
    EXPECT_TRUE(a.overridden);
    EXPECT_FLOAT_EQ(a.value, 1.0f);
}

// ============================================================================
// LerpValue テスト
// ============================================================================

TEST(LerpValueTest, Float)
{
    EXPECT_FLOAT_EQ(LerpValue(0.0f, 10.0f, 0.5f), 5.0f);
    EXPECT_FLOAT_EQ(LerpValue(0.0f, 10.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(LerpValue(0.0f, 10.0f, 1.0f), 10.0f);
}

TEST(LerpValueTest, Int)
{
    EXPECT_EQ(LerpValue(0, 10, 0.5f), 5);
    EXPECT_EQ(LerpValue(0, 10, 0.3f), 3);
}

TEST(LerpValueTest, Bool)
{
    EXPECT_EQ(LerpValue(true, false, 0.3f), true);
    EXPECT_EQ(LerpValue(true, false, 0.5f), false);
}

TEST(LerpValueTest, Enum)
{
    EXPECT_EQ(LerpValue(TonemapMode::Reinhard, TonemapMode::ACES, 0.3f), TonemapMode::Reinhard);
    EXPECT_EQ(LerpValue(TonemapMode::Reinhard, TonemapMode::ACES, 0.7f), TonemapMode::ACES);
}

TEST(LerpValueTest, Vector3)
{
    Vector3 a{0, 0, 0}, b{10, 20, 30};
    auto r = LerpValue(a, b, 0.5f);
    EXPECT_FLOAT_EQ(r.x, 5.0f);
    EXPECT_FLOAT_EQ(r.y, 10.0f);
    EXPECT_FLOAT_EQ(r.z, 15.0f);
}

// ============================================================================
// LerpOverride テスト
// ============================================================================

TEST(LerpOverrideTest, BothOverridden)
{
    Override<float> a, b;
    a.Set(0.0f);
    b.Set(10.0f);
    auto r = LerpOverride(a, b, 0.5f);
    EXPECT_TRUE(r.overridden);
    EXPECT_FLOAT_EQ(r.value, 5.0f);
}

TEST(LerpOverrideTest, OnlyFirstOverridden)
{
    Override<float> a, b;
    a.Set(5.0f);
    auto r = LerpOverride(a, b, 0.5f);
    EXPECT_TRUE(r.overridden);
    EXPECT_FLOAT_EQ(r.value, 5.0f);
}

TEST(LerpOverrideTest, OnlySecondOverridden)
{
    Override<float> a, b;
    b.Set(7.0f);
    auto r = LerpOverride(a, b, 0.5f);
    EXPECT_TRUE(r.overridden);
    EXPECT_FLOAT_EQ(r.value, 7.0f);
}

TEST(LerpOverrideTest, NeitherOverridden)
{
    Override<float> a, b;
    auto r = LerpOverride(a, b, 0.5f);
    EXPECT_FALSE(r.overridden);
}

// ============================================================================
// RenderProfile テスト
// ============================================================================

TEST(RenderProfileTest, DefaultConstructionNotOverridden)
{
    RenderProfile p;
    EXPECT_FALSE(p.bloomEnabled.overridden);
    EXPECT_FALSE(p.tonemapMode.overridden);
    EXPECT_FALSE(p.fogMode.overridden);
}

TEST(RenderProfileTest, MergeFromOverridesOnly)
{
    RenderProfile base;
    base.bloomEnabled.Set(true);
    base.bloomThreshold.Set(1.5f);
    base.ssaoEnabled.Set(true);

    RenderProfile overlay;
    overlay.bloomThreshold.Set(0.8f);  // override bloom threshold only

    base.MergeFrom(overlay);
    EXPECT_TRUE(base.bloomEnabled.overridden);
    EXPECT_EQ(base.bloomEnabled.value, true);     // preserved
    EXPECT_FLOAT_EQ(base.bloomThreshold.value, 0.8f); // overwritten
    EXPECT_TRUE(base.ssaoEnabled.overridden);     // preserved
}

TEST(RenderProfileTest, LerpMidpoint)
{
    RenderProfile a, b;
    a.tonemapExposure.Set(1.0f);
    b.tonemapExposure.Set(3.0f);
    a.bloomIntensity.Set(0.0f);
    b.bloomIntensity.Set(1.0f);

    auto r = RenderProfile::Lerp(a, b, 0.5f);
    EXPECT_TRUE(r.tonemapExposure.overridden);
    EXPECT_FLOAT_EQ(r.tonemapExposure.value, 2.0f);
    EXPECT_FLOAT_EQ(r.bloomIntensity.value, 0.5f);
}

TEST(RenderProfileTest, LerpAtEndpoints)
{
    RenderProfile a, b;
    a.ssaoRadius.Set(0.2f);
    b.ssaoRadius.Set(1.0f);

    auto at0 = RenderProfile::Lerp(a, b, 0.0f);
    auto at1 = RenderProfile::Lerp(a, b, 1.0f);
    EXPECT_FLOAT_EQ(at0.ssaoRadius.value, 0.2f);
    EXPECT_FLOAT_EQ(at1.ssaoRadius.value, 1.0f);
}

TEST(RenderProfileTest, LerpPartialOverride)
{
    RenderProfile a, b;
    a.bloomIntensity.Set(0.5f);
    // b.bloomIntensity not overridden
    b.ssaoRadius.Set(1.0f);

    auto r = RenderProfile::Lerp(a, b, 0.5f);
    EXPECT_TRUE(r.bloomIntensity.overridden);
    EXPECT_FLOAT_EQ(r.bloomIntensity.value, 0.5f); // only a has it
    EXPECT_TRUE(r.ssaoRadius.overridden);
    EXPECT_FLOAT_EQ(r.ssaoRadius.value, 1.0f);     // only b has it
}

// ============================================================================
// ファクトリプリセット テスト
// ============================================================================

TEST(RenderProfileFactoryTest, Default)
{
    auto p = RenderProfile::CreateDefault();
    EXPECT_TRUE(p.tonemapMode.overridden);
    EXPECT_EQ(p.tonemapMode.value, TonemapMode::ACES);
    EXPECT_TRUE(p.bloomEnabled.overridden);
    EXPECT_TRUE(p.bloomEnabled.value);
    EXPECT_TRUE(p.fxaaEnabled.overridden);
    EXPECT_FALSE(p.fxaaEnabled.value);   // TAA有効のためFXAA不要
    EXPECT_TRUE(p.shadowEnabled.overridden);
    EXPECT_TRUE(p.shadowEnabled.value);

    // UE5相当の新デフォルト
    EXPECT_TRUE(p.motionBlurEnabled.value);
    EXPECT_FLOAT_EQ(p.motionBlurIntensity.value, 0.3f);
    EXPECT_TRUE(p.volumetricLightEnabled.value);
    EXPECT_TRUE(p.autoExposureEnabled.value);
    EXPECT_TRUE(p.contactShadowsEnabled.value);
    EXPECT_TRUE(p.vignetteEnabled.value);
    EXPECT_TRUE(p.colorGradingEnabled.value);
    EXPECT_TRUE(p.cascadeBlendEnabled.value);
    EXPECT_TRUE(p.pcssEnabled.value);
    EXPECT_TRUE(p.rtReflectionsEnabled.value);
    EXPECT_TRUE(p.rtgiEnabled.value);
    EXPECT_EQ(p.fogMode.value, FogMode::Exp);

    // SSGI (RTGI フォールバック)
    EXPECT_TRUE(p.ssgiEnabled.overridden);
    EXPECT_TRUE(p.ssgiEnabled.value);
    EXPECT_FLOAT_EQ(p.ssgiIntensity.value, 0.5f);
    EXPECT_FLOAT_EQ(p.ssgiMaxDistance.value, 50.0f);
    EXPECT_EQ(p.ssgiSampleCount.value, 32);
    EXPECT_FLOAT_EQ(p.ssgiThickness.value, 0.5f);
}

TEST(RenderProfileFactoryTest, Indoor)
{
    auto p = RenderProfile::CreateIndoor();
    EXPECT_TRUE(p.ssaoEnabled.overridden);
    EXPECT_TRUE(p.ssaoEnabled.value);
    EXPECT_TRUE(p.ssaoRadius.overridden);
    EXPECT_GT(p.ssaoRadius.value, 0.5f);  // 強め
    EXPECT_TRUE(p.fogMode.overridden);
    EXPECT_EQ(p.fogMode.value, FogMode::None);
    EXPECT_TRUE(p.tonemapExposure.overridden);
    EXPECT_LT(p.tonemapExposure.value, 1.0f);  // 暗め
}

TEST(RenderProfileFactoryTest, Outdoor)
{
    auto p = RenderProfile::CreateOutdoor();
    EXPECT_TRUE(p.fogMode.overridden);
    EXPECT_EQ(p.fogMode.value, FogMode::Exp);
    EXPECT_TRUE(p.volumetricLightEnabled.overridden);
    EXPECT_TRUE(p.volumetricLightEnabled.value);
    EXPECT_TRUE(p.pcssEnabled.overridden);
    EXPECT_TRUE(p.pcssEnabled.value);
}

TEST(RenderProfileFactoryTest, Cinematic)
{
    auto p = RenderProfile::CreateCinematic();
    EXPECT_TRUE(p.dofEnabled.overridden);
    EXPECT_TRUE(p.dofEnabled.value);
    EXPECT_TRUE(p.motionBlurEnabled.overridden);
    EXPECT_TRUE(p.motionBlurEnabled.value);
    EXPECT_TRUE(p.colorGradingEnabled.overridden);
    EXPECT_TRUE(p.colorGradingEnabled.value);
    EXPECT_TRUE(p.vignetteEnabled.overridden);
    EXPECT_TRUE(p.vignetteEnabled.value);
}

// ============================================================================
// JSON テスト
// ============================================================================

class RenderProfileJSONTest : public ::testing::Test
{
protected:
    gx::String m_tempPath = "test_render_profile_temp.json";

    void TearDown() override
    {
        std::remove(m_tempPath.c_str());
    }
};

TEST_F(RenderProfileJSONTest, RoundTrip)
{
    RenderProfile original;
    original.tonemapMode.Set(TonemapMode::Reinhard);
    original.tonemapExposure.Set(1.5f);
    original.bloomEnabled.Set(true);
    original.bloomThreshold.Set(0.9f);
    original.fogMode.Set(FogMode::Exp2);
    original.fogColor.Set(Vector3{0.5f, 0.6f, 0.7f});
    original.cascadeSplits.Set(gx::Array<float,4>{0.1f, 0.2f, 0.5f, 1.0f});
    original.rtReflectionsShadowSamples.Set(8u);

    ASSERT_TRUE(RenderProfile::Save(m_tempPath, original));

    RenderProfile loaded;
    ASSERT_TRUE(RenderProfile::Load(m_tempPath, loaded));

    EXPECT_TRUE(loaded.tonemapMode.overridden);
    EXPECT_EQ(loaded.tonemapMode.value, TonemapMode::Reinhard);
    EXPECT_FLOAT_EQ(loaded.tonemapExposure.value, 1.5f);
    EXPECT_TRUE(loaded.bloomEnabled.value);
    EXPECT_FLOAT_EQ(loaded.bloomThreshold.value, 0.9f);
    EXPECT_EQ(loaded.fogMode.value, FogMode::Exp2);
    EXPECT_NEAR(loaded.fogColor.value.x, 0.5f, 0.01f);
    EXPECT_NEAR(loaded.fogColor.value.y, 0.6f, 0.01f);
    EXPECT_NEAR(loaded.fogColor.value.z, 0.7f, 0.01f);
    EXPECT_NEAR(loaded.cascadeSplits.value[0], 0.1f, 0.01f);
    EXPECT_NEAR(loaded.cascadeSplits.value[3], 1.0f, 0.01f);
    EXPECT_EQ(loaded.rtReflectionsShadowSamples.value, 8u);
}

TEST_F(RenderProfileJSONTest, PartialLoadOnlyOverridesPresent)
{
    // Write partial JSON manually
    {
        std::ofstream ofs(m_tempPath.c_str());
        ofs << R"({
            "bloom": { "enabled": true, "intensity": 0.8 },
            "fog": { "mode": "Linear", "start": 10.0, "end": 100.0 }
        })";
    }

    RenderProfile loaded;
    ASSERT_TRUE(RenderProfile::Load(m_tempPath, loaded));

    // overridden
    EXPECT_TRUE(loaded.bloomEnabled.overridden);
    EXPECT_TRUE(loaded.bloomIntensity.overridden);
    EXPECT_TRUE(loaded.fogMode.overridden);
    EXPECT_EQ(loaded.fogMode.value, FogMode::Linear);

    // NOT overridden
    EXPECT_FALSE(loaded.ssaoEnabled.overridden);
    EXPECT_FALSE(loaded.tonemapMode.overridden);
    EXPECT_FALSE(loaded.dofEnabled.overridden);
}

TEST_F(RenderProfileJSONTest, SaveOnlyOverridden)
{
    RenderProfile p;
    p.bloomEnabled.Set(true);
    // Everything else NOT overridden

    ASSERT_TRUE(RenderProfile::Save(m_tempPath, p));

    // Read back raw JSON
    std::ifstream ifs(m_tempPath.c_str());
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    // Should contain bloom but NOT ssao, fog, etc.
    EXPECT_NE(content.find("bloom"), std::string::npos);
    EXPECT_EQ(content.find("ssao"), std::string::npos);
    EXPECT_EQ(content.find("fog"), std::string::npos);
    EXPECT_EQ(content.find("tonemapping"), std::string::npos);
}

TEST_F(RenderProfileJSONTest, LoadNonExistentFile)
{
    RenderProfile p;
    EXPECT_FALSE(RenderProfile::Load("nonexistent_file.json", p));
}

TEST_F(RenderProfileJSONTest, LoadInvalidJSON)
{
    {
        std::ofstream ofs(m_tempPath.c_str());
        ofs << "this is not json {{{";
    }

    RenderProfile p;
    EXPECT_FALSE(RenderProfile::Load(m_tempPath, p));
}

// ============================================================================
// RenderProfileStack テスト
// ============================================================================

TEST(RenderProfileStackTest, EmptyResolveReturnsEmpty)
{
    RenderProfileStack stack;
    auto r = stack.Resolve();
    EXPECT_FALSE(r.bloomEnabled.overridden);
    EXPECT_FALSE(r.tonemapMode.overridden);
}

TEST(RenderProfileStackTest, SingleProfile)
{
    RenderProfileStack stack;
    RenderProfile p;
    p.bloomEnabled.Set(true);
    p.bloomIntensity.Set(0.8f);
    stack.AddProfile("base", p, 0);

    auto r = stack.Resolve();
    EXPECT_TRUE(r.bloomEnabled.overridden);
    EXPECT_TRUE(r.bloomEnabled.value);
    EXPECT_FLOAT_EQ(r.bloomIntensity.value, 0.8f);
}

TEST(RenderProfileStackTest, PriorityOrdering)
{
    RenderProfileStack stack;

    RenderProfile low;
    low.tonemapExposure.Set(1.0f);
    stack.AddProfile("low", low, 0);

    RenderProfile high;
    high.tonemapExposure.Set(2.0f);
    stack.AddProfile("high", high, 10);

    auto r = stack.Resolve();
    EXPECT_FLOAT_EQ(r.tonemapExposure.value, 2.0f);  // high priority wins
}

TEST(RenderProfileStackTest, WeightBlend)
{
    RenderProfileStack stack;

    RenderProfile base;
    base.tonemapExposure.Set(1.0f);
    stack.AddProfile("base", base, 0);

    RenderProfile overlay;
    overlay.tonemapExposure.Set(3.0f);
    auto id = stack.AddProfile("overlay", overlay, 10, 0.5f);  // weight=0.5
    (void)id;

    auto r = stack.Resolve();
    EXPECT_TRUE(r.tonemapExposure.overridden);
    // base=1.0, overlay merges to 3.0, lerp(1.0, 3.0, 0.5) = 2.0
    EXPECT_NEAR(r.tonemapExposure.value, 2.0f, 0.01f);
}

TEST(RenderProfileStackTest, RemoveProfile)
{
    RenderProfileStack stack;

    RenderProfile p;
    p.bloomEnabled.Set(true);
    auto id = stack.AddProfile("temp", p, 0);

    EXPECT_EQ(stack.GetEntryCount(), 1u);
    stack.RemoveProfile(id);
    EXPECT_EQ(stack.GetEntryCount(), 0u);

    auto r = stack.Resolve();
    EXPECT_FALSE(r.bloomEnabled.overridden);
}

TEST(RenderProfileStackTest, SetWeightAndPriority)
{
    RenderProfileStack stack;

    RenderProfile p;
    p.tonemapExposure.Set(5.0f);
    auto id = stack.AddProfile("p", p, 0, 0.0f);  // weight=0 (inactive)

    auto r = stack.Resolve();
    EXPECT_FALSE(r.tonemapExposure.overridden);  // weight=0 → skipped

    stack.SetWeight(id, 1.0f);
    r = stack.Resolve();
    EXPECT_TRUE(r.tonemapExposure.overridden);
    EXPECT_FLOAT_EQ(r.tonemapExposure.value, 5.0f);
}

TEST(RenderProfileStackTest, Clear)
{
    RenderProfileStack stack;

    RenderProfile p;
    p.bloomEnabled.Set(true);
    stack.AddProfile("a", p, 0);
    stack.AddProfile("b", p, 1);

    EXPECT_EQ(stack.GetEntryCount(), 2u);
    stack.Clear();
    EXPECT_EQ(stack.GetEntryCount(), 0u);
}

// ============================================================================
// LerpValue — gx::Array<float,4>
// ============================================================================

TEST(LerpValueTest, Array4)
{
    gx::Array<float,4> a{0.0f, 0.0f, 0.0f, 0.0f};
    gx::Array<float,4> b{1.0f, 2.0f, 3.0f, 4.0f};
    auto r = LerpValue(a, b, 0.5f);
    EXPECT_FLOAT_EQ(r[0], 0.5f);
    EXPECT_FLOAT_EQ(r[1], 1.0f);
    EXPECT_FLOAT_EQ(r[2], 1.5f);
    EXPECT_FLOAT_EQ(r[3], 2.0f);
}

// ============================================================================
// JSON — All effect sections round-trip
// ============================================================================

class RenderProfileFullJSONTest : public ::testing::Test
{
protected:
    gx::String m_tempPath = "test_render_profile_full_temp.json";
    void TearDown() override { std::remove(m_tempPath.c_str()); }
};

TEST_F(RenderProfileFullJSONTest, AllSectionsRoundTrip)
{
    auto original = RenderProfile::CreateDefault();
    ASSERT_TRUE(RenderProfile::Save(m_tempPath, original));

    RenderProfile loaded;
    ASSERT_TRUE(RenderProfile::Load(m_tempPath, loaded));

    // Spot-check representative fields from each section
    EXPECT_EQ(loaded.tonemapMode.value, TonemapMode::ACES);
    EXPECT_TRUE(loaded.bloomEnabled.value);
    EXPECT_TRUE(loaded.ssaoEnabled.value);
    EXPECT_FALSE(loaded.dofEnabled.value);
    EXPECT_TRUE(loaded.motionBlurEnabled.value);
    EXPECT_TRUE(loaded.ssrEnabled.value);
    EXPECT_FALSE(loaded.outlineEnabled.value);
    EXPECT_TRUE(loaded.taaEnabled.value);
    EXPECT_TRUE(loaded.volumetricLightEnabled.value);
    EXPECT_TRUE(loaded.autoExposureEnabled.value);
    EXPECT_TRUE(loaded.contactShadowsEnabled.value);
    EXPECT_TRUE(loaded.vignetteEnabled.value);
    EXPECT_TRUE(loaded.colorGradingEnabled.value);
    EXPECT_FALSE(loaded.fxaaEnabled.value);
    EXPECT_EQ(loaded.fogMode.value, FogMode::Exp);
    EXPECT_TRUE(loaded.shadowEnabled.value);
    EXPECT_TRUE(loaded.rtReflectionsEnabled.value);
    EXPECT_TRUE(loaded.rtgiEnabled.value);
    EXPECT_TRUE(loaded.ambientColor.overridden);

    // SSGI round-trip
    EXPECT_TRUE(loaded.ssgiEnabled.overridden);
    EXPECT_TRUE(loaded.ssgiEnabled.value);
    EXPECT_FLOAT_EQ(loaded.ssgiIntensity.value, 0.5f);
    EXPECT_FLOAT_EQ(loaded.ssgiMaxDistance.value, 50.0f);
    EXPECT_EQ(loaded.ssgiSampleCount.value, 32);
    EXPECT_FLOAT_EQ(loaded.ssgiThickness.value, 0.5f);
}
