/// @file test_Color.cpp
/// @brief Color 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Color.h"

using namespace GX;

TEST(ColorTest, DefaultConstructor)
{
    Color c;
    EXPECT_FLOAT_EQ(c.r, 1.0f);
    EXPECT_FLOAT_EQ(c.g, 1.0f);
    EXPECT_FLOAT_EQ(c.b, 1.0f);
    EXPECT_FLOAT_EQ(c.a, 1.0f);
}

TEST(ColorTest, FloatConstructor)
{
    Color c(0.5f, 0.6f, 0.7f, 0.8f);
    EXPECT_FLOAT_EQ(c.r, 0.5f);
    EXPECT_FLOAT_EQ(c.g, 0.6f);
    EXPECT_FLOAT_EQ(c.b, 0.7f);
    EXPECT_FLOAT_EQ(c.a, 0.8f);
}

TEST(ColorTest, Uint32Constructor)
{
    // 0xFF804020 は R=255, G=128, B=64, A=32 を表す
    Color c(0xFF804020u);
    EXPECT_NEAR(c.r, 1.0f, 1.0f / 255.0f);
    EXPECT_NEAR(c.g, 128.0f / 255.0f, 1.0f / 255.0f);
    EXPECT_NEAR(c.b, 64.0f / 255.0f, 1.0f / 255.0f);
    EXPECT_NEAR(c.a, 32.0f / 255.0f, 1.0f / 255.0f);
}

TEST(ColorTest, Uint8Constructor)
{
    Color c(static_cast<uint8_t>(255), static_cast<uint8_t>(128),
            static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    EXPECT_NEAR(c.r, 1.0f, 1.0f / 255.0f);
    EXPECT_NEAR(c.g, 128.0f / 255.0f, 1.0f / 255.0f);
    EXPECT_NEAR(c.b, 0.0f, 1.0f / 255.0f);
    EXPECT_NEAR(c.a, 1.0f, 1.0f / 255.0f);
}

TEST(ColorTest, ToRGBA)
{
    Color c(1.0f, 0.0f, 0.0f, 1.0f); // 赤
    uint32_t rgba = c.ToRGBA();
    EXPECT_EQ((rgba >> 24) & 0xFF, 255); // R
    EXPECT_EQ((rgba >> 16) & 0xFF, 0);   // G
    EXPECT_EQ((rgba >> 8) & 0xFF, 0);    // B
    EXPECT_EQ(rgba & 0xFF, 255);         // A
}

TEST(ColorTest, ToABGR)
{
    Color c(1.0f, 0.0f, 0.0f, 1.0f); // 赤
    uint32_t abgr = c.ToABGR();
    EXPECT_EQ((abgr >> 24) & 0xFF, 255); // A
    EXPECT_EQ((abgr >> 16) & 0xFF, 0);   // B
    EXPECT_EQ((abgr >> 8) & 0xFF, 0);    // G
    EXPECT_EQ(abgr & 0xFF, 255);         // R
}

TEST(ColorTest, HSVRoundTrip)
{
    Color original(0.8f, 0.4f, 0.2f, 1.0f);
    float h, s, v;
    original.ToHSV(h, s, v);
    Color restored = Color::FromHSV(h, s, v);

    EXPECT_NEAR(restored.r, original.r, 1e-3f);
    EXPECT_NEAR(restored.g, original.g, 1e-3f);
    EXPECT_NEAR(restored.b, original.b, 1e-3f);
}

TEST(ColorTest, Lerp)
{
    Color black = Color::Black();
    Color white = Color::White();
    Color mid = Color::Lerp(black, white, 0.5f);

    EXPECT_NEAR(mid.r, 0.5f, 1e-5f);
    EXPECT_NEAR(mid.g, 0.5f, 1e-5f);
    EXPECT_NEAR(mid.b, 0.5f, 1e-5f);
}

TEST(ColorTest, Multiply)
{
    Color c(0.5f, 0.5f, 0.5f, 1.0f);
    Color r = c * 2.0f;
    EXPECT_NEAR(r.r, 1.0f, 1e-5f);
    EXPECT_NEAR(r.g, 1.0f, 1e-5f);
}

TEST(ColorTest, NamedColors)
{
    Color red = Color::Red();
    EXPECT_FLOAT_EQ(red.r, 1.0f);
    EXPECT_FLOAT_EQ(red.g, 0.0f);
    EXPECT_FLOAT_EQ(red.b, 0.0f);

    Color trans = Color::Transparent();
    EXPECT_FLOAT_EQ(trans.a, 0.0f);
}
