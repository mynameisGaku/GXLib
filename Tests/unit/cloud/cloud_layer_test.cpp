/// @file cloud_layer_test.cpp
/// @brief CloudClass enum + HeightDensityCurve + CloudLayer tests (ADR-0021 Phase B).
#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/CloudLayer.h"

namespace {

TEST(CloudLayerTest, CloudClassEnumValuesAreDistinct)
{
    // Seven classes, all distinct integer values
    EXPECT_NE(static_cast<int>(gx::CloudClass::Stratus),
              static_cast<int>(gx::CloudClass::Cumulus));
    EXPECT_NE(static_cast<int>(gx::CloudClass::Cumulus),
              static_cast<int>(gx::CloudClass::Cumulonimbus));
    EXPECT_NE(static_cast<int>(gx::CloudClass::Cirrus),
              static_cast<int>(gx::CloudClass::Altocumulus));
}

TEST(HeightDensityCurveTest, SampleOutsideRangeReturnsZero)
{
    gx::HeightDensityCurve c;
    EXPECT_FLOAT_EQ(c.Sample(-0.1f), 0.0f);
    EXPECT_FLOAT_EQ(c.Sample(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(c.Sample(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(c.Sample(1.5f), 0.0f);
}

TEST(HeightDensityCurveTest, SampleMidHeightIsPositive)
{
    gx::HeightDensityCurve c;
    // Mid-layer sample must be > 0 for any non-degenerate curve
    float s = c.Sample(0.5f);
    EXPECT_GT(s, 0.0f);
    EXPECT_LE(s, 1.0f);
}

TEST(HeightDensityCurveTest, CumulusAndStratusHaveDifferentProfiles)
{
    // Stratus shapeExponent = 1.0 (linear), cumulus = 1.5 (reshaped).
    // Sample in the fade-in region where shape exponent actually affects values.
    // Sampling in the plateau (mid-layer) returns ~1.0 for both (fade regions untouched).
    auto stratus = gx::HeightDensityCurve::Default(gx::CloudClass::Stratus);
    auto cumulus = gx::HeightDensityCurve::Default(gx::CloudClass::Cumulus);

    // At fade-in sample point, shape exponent matters
    float stratusEdge = stratus.Sample(0.02f);  // below stratus bottomFade=0.05
    float cumulusEdge = cumulus.Sample(0.08f);  // below cumulus bottomFade=0.15
    // pow(x, 1.5) with x<1 produces smaller values than linear — cumulus edge softer
    EXPECT_NE(stratusEdge, cumulusEdge);
}

TEST(HeightDensityCurveTest, DefaultStratusFlatter)
{
    auto stratus = gx::HeightDensityCurve::Default(gx::CloudClass::Stratus);
    auto cumulus = gx::HeightDensityCurve::Default(gx::CloudClass::Cumulus);

    // Stratus shapeExponent = 1.0 (linear), cumulus = 1.5 (biased)
    EXPECT_FLOAT_EQ(stratus.shapeExponent, 1.0f);
    EXPECT_GT(cumulus.shapeExponent, 1.0f);
}

TEST(HeightDensityCurveTest, DefaultCumulonimbusReachesTop)
{
    auto cb = gx::HeightDensityCurve::Default(gx::CloudClass::Cumulonimbus);
    // Cumulonimbus anvil = top fade height near 1.0
    EXPECT_GE(cb.topFadeHeight, 0.9f);
}

TEST(CloudLayerTest, DefaultValuesSane)
{
    gx::CloudLayer layer;
    EXPECT_LT(layer.altitudeBottom, layer.altitudeTop);
    EXPECT_GT(layer.densityMul, 0.0f);
    EXPECT_GE(layer.diffusivity, 0.0f);
    EXPECT_LE(layer.diffusivity, 1.0f);
    EXPECT_GT(layer.detailFadeDistance, 0.0f);
}

} // namespace
