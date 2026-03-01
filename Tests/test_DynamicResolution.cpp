/// @file test_DynamicResolution.cpp
/// @brief Dynamic Resolution Scaling tests

#include <gtest/gtest.h>
#include "Graphics/PostEffect/DynamicResolution.h"

using namespace gx;

TEST(DynamicResolutionTest, DefaultState)
{
    DynamicResolution drs;
    EXPECT_EQ(drs.GetNativeWidth(), 0u);
    EXPECT_EQ(drs.GetNativeHeight(), 0u);
    EXPECT_FLOAT_EQ(drs.GetCurrentScale(), 1.0f);
}

TEST(DynamicResolutionTest, Initialize)
{
    DynamicResolution drs;
    EXPECT_TRUE(drs.Initialize(1920, 1080));
    EXPECT_EQ(drs.GetNativeWidth(), 1920u);
    EXPECT_EQ(drs.GetNativeHeight(), 1080u);
    EXPECT_EQ(drs.GetRenderWidth(), 1920u);
    EXPECT_EQ(drs.GetRenderHeight(), 1080u);
}

TEST(DynamicResolutionTest, InitializeInvalidDimensions)
{
    DynamicResolution drs;
    EXPECT_FALSE(drs.Initialize(0, 0));
}

TEST(DynamicResolutionTest, ScaleDownUnderLoad)
{
    DynamicResolution drs;
    DynamicResolutionConfig cfg;
    cfg.targetFrameTimeMs = 16.67f;
    cfg.adaptationSpeed = 1.0f; // Instant adaptation for testing
    drs.Initialize(1920, 1080, cfg);

    drs.UpdateScale(33.34f); // 2x over budget
    EXPECT_LT(drs.GetCurrentScale(), 1.0f);
    EXPECT_LE(drs.GetRenderWidth(), 1920u);
}

TEST(DynamicResolutionTest, ScaleUpUnderBudget)
{
    DynamicResolution drs;
    DynamicResolutionConfig cfg;
    cfg.targetFrameTimeMs = 16.67f;
    cfg.adaptationSpeed = 1.0f;
    cfg.minScale = 0.5f;
    drs.Initialize(1920, 1080, cfg);

    // First scale down
    drs.UpdateScale(33.34f);
    float scaledDown = drs.GetCurrentScale();

    // Then give it easy budget
    drs.UpdateScale(8.0f);
    EXPECT_GT(drs.GetCurrentScale(), scaledDown);
}

TEST(DynamicResolutionTest, MinScaleClamp)
{
    DynamicResolution drs;
    DynamicResolutionConfig cfg;
    cfg.minScale = 0.5f;
    cfg.adaptationSpeed = 1.0f;
    drs.Initialize(1920, 1080, cfg);

    // Heavy load
    for (int i = 0; i < 10; i++)
        drs.UpdateScale(100.0f);

    EXPECT_GE(drs.GetCurrentScale(), 0.5f);
}

TEST(DynamicResolutionTest, MaxScaleClamp)
{
    DynamicResolution drs;
    DynamicResolutionConfig cfg;
    cfg.maxScale = 1.0f;
    cfg.adaptationSpeed = 1.0f;
    drs.Initialize(1920, 1080, cfg);

    for (int i = 0; i < 10; i++)
        drs.UpdateScale(1.0f); // Very fast

    EXPECT_LE(drs.GetCurrentScale(), 1.0f);
}

TEST(DynamicResolutionTest, DisabledNoChange)
{
    DynamicResolution drs;
    DynamicResolutionConfig cfg;
    cfg.enabled = false;
    drs.Initialize(1920, 1080, cfg);

    drs.UpdateScale(33.34f);
    EXPECT_FLOAT_EQ(drs.GetCurrentScale(), 1.0f);
}

TEST(DynamicResolutionTest, ResetScale)
{
    DynamicResolution drs;
    DynamicResolutionConfig cfg;
    cfg.adaptationSpeed = 1.0f;
    drs.Initialize(1920, 1080, cfg);

    drs.UpdateScale(50.0f);
    EXPECT_LT(drs.GetCurrentScale(), 1.0f);

    drs.ResetScale();
    EXPECT_FLOAT_EQ(drs.GetCurrentScale(), 1.0f);
}

TEST(DynamicResolutionTest, ConfigAccess)
{
    DynamicResolution drs;
    DynamicResolutionConfig cfg;
    cfg.targetFrameTimeMs = 8.33f; // 120 fps
    drs.Initialize(3840, 2160, cfg);

    EXPECT_FLOAT_EQ(drs.GetConfig().targetFrameTimeMs, 8.33f);
    EXPECT_EQ(drs.GetNativeWidth(), 3840u);
}
