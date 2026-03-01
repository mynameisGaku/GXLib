#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/CascadedShadowMap.h"

using namespace gx;

// ---------------------------------------------------------------------------
// CascadeBlend parameter tests
// ---------------------------------------------------------------------------

TEST(CascadeBlend, DefaultBlendWidthIs01)
{
    CascadedShadowMap csm;
    EXPECT_FLOAT_EQ(csm.GetCascadeBlendWidth(), 0.1f);
}

TEST(CascadeBlend, DefaultBlendDisabled)
{
    CascadedShadowMap csm;
    EXPECT_FALSE(csm.IsCascadeBlendEnabled());
}

TEST(CascadeBlend, EnableDisableToggle)
{
    CascadedShadowMap csm;
    csm.SetCascadeBlendEnabled(true);
    EXPECT_TRUE(csm.IsCascadeBlendEnabled());
    csm.SetCascadeBlendEnabled(false);
    EXPECT_FALSE(csm.IsCascadeBlendEnabled());
}

TEST(CascadeBlend, SetBlendWidthRoundTrip)
{
    CascadedShadowMap csm;
    csm.SetCascadeBlendWidth(0.25f);
    EXPECT_FLOAT_EQ(csm.GetCascadeBlendWidth(), 0.25f);
}

TEST(CascadeBlend, BlendWidthZeroIsValid)
{
    CascadedShadowMap csm;
    csm.SetCascadeBlendWidth(0.0f);
    EXPECT_FLOAT_EQ(csm.GetCascadeBlendWidth(), 0.0f);
}

TEST(CascadeBlend, BlendWidthOneIsValid)
{
    CascadedShadowMap csm;
    csm.SetCascadeBlendWidth(1.0f);
    EXPECT_FLOAT_EQ(csm.GetCascadeBlendWidth(), 1.0f);
}

TEST(CascadeBlend, ShadowConstantsHasCascadeBlendWidth)
{
    ShadowConstants sc{};
    sc.cascadeBlendWidth = 0.3f;
    EXPECT_FLOAT_EQ(sc.cascadeBlendWidth, 0.3f);
}
