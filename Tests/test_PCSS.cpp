#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/CascadedShadowMap.h"

using namespace gx;

// ---------------------------------------------------------------------------
// PCSS (Percentage-Closer Soft Shadows) parameter tests
// ---------------------------------------------------------------------------

TEST(PCSS, DefaultDisabled)
{
    CascadedShadowMap csm;
    EXPECT_FALSE(csm.IsPCSSEnabled());
}

TEST(PCSS, EnableDisableToggle)
{
    CascadedShadowMap csm;
    csm.SetPCSSEnabled(true);
    EXPECT_TRUE(csm.IsPCSSEnabled());
    csm.SetPCSSEnabled(false);
    EXPECT_FALSE(csm.IsPCSSEnabled());
}

TEST(PCSS, DefaultLightSizeIsOne)
{
    CascadedShadowMap csm;
    EXPECT_FLOAT_EQ(csm.GetLightSize(), 1.0f);
}

TEST(PCSS, SetLightSizeRoundTrip)
{
    CascadedShadowMap csm;
    csm.SetLightSize(2.5f);
    EXPECT_FLOAT_EQ(csm.GetLightSize(), 2.5f);
}

TEST(PCSS, LightSizeZeroIsValid)
{
    CascadedShadowMap csm;
    csm.SetLightSize(0.0f);
    EXPECT_FLOAT_EQ(csm.GetLightSize(), 0.0f);
}

TEST(PCSS, LightSizeLargeValue)
{
    CascadedShadowMap csm;
    csm.SetLightSize(100.0f);
    EXPECT_FLOAT_EQ(csm.GetLightSize(), 100.0f);
}

TEST(PCSS, ShadowConstantsHasShadowLightSize)
{
    ShadowConstants sc{};
    sc.shadowLightSize = 3.0f;
    EXPECT_FLOAT_EQ(sc.shadowLightSize, 3.0f);
}

TEST(PCSS, ShadowConstantsHasPcssEnabled)
{
    ShadowConstants sc{};
    sc.pcssEnabled = 1.0f;
    EXPECT_FLOAT_EQ(sc.pcssEnabled, 1.0f);

    sc.pcssEnabled = 0.0f;
    EXPECT_FLOAT_EQ(sc.pcssEnabled, 0.0f);
}
