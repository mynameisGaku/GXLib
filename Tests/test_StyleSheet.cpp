/// @file test_StyleSheet.cpp
/// @brief StyleSheet CSS parsing and StyleLength/StyleColor tests

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/StyleSheet.h"
#include "GUI/Style.h"

using namespace gx::GUI;

TEST(StyleTest, StyleLength_Px)
{
    StyleLength len = StyleLength::Px(100.0f);
    EXPECT_NEAR(len.value, 100.0f, 1e-5f);
    EXPECT_EQ(len.unit, SizeUnit::Px);
    EXPECT_FALSE(len.IsAuto());
}

TEST(StyleTest, StyleLength_Pct)
{
    StyleLength len = StyleLength::Pct(50.0f);
    EXPECT_NEAR(len.value, 50.0f, 1e-5f);
    EXPECT_EQ(len.unit, SizeUnit::Percent);
}

TEST(StyleTest, StyleLength_Auto)
{
    StyleLength len = StyleLength::Auto();
    EXPECT_TRUE(len.IsAuto());
}

TEST(StyleTest, StyleLength_Resolve_Px)
{
    StyleLength len = StyleLength::Px(100.0f);
    float resolved = len.Resolve(500.0f);
    EXPECT_NEAR(resolved, 100.0f, 1e-5f);
}

TEST(StyleTest, StyleLength_Resolve_Pct)
{
    StyleLength len = StyleLength::Pct(50.0f);
    float resolved = len.Resolve(200.0f);
    EXPECT_NEAR(resolved, 100.0f, 1e-5f);
}

TEST(StyleTest, StyleColor_FromHex_RGB)
{
    StyleColor c = StyleColor::FromHex("#FF0000");
    EXPECT_NEAR(c.r, 1.0f, 0.01f);
    EXPECT_NEAR(c.g, 0.0f, 0.01f);
    EXPECT_NEAR(c.b, 0.0f, 0.01f);
    EXPECT_NEAR(c.a, 1.0f, 0.01f);
}

TEST(StyleTest, StyleColor_FromHex_RGBA)
{
    StyleColor c = StyleColor::FromHex("#FF000080");
    EXPECT_NEAR(c.r, 1.0f, 0.01f);
    EXPECT_NEAR(c.g, 0.0f, 0.01f);
    EXPECT_NEAR(c.b, 0.0f, 0.01f);
    EXPECT_NEAR(c.a, 128.0f / 255.0f, 0.01f);
}

TEST(StyleTest, StyleColor_IsTransparent)
{
    StyleColor transparent(0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(transparent.IsTransparent());

    StyleColor opaque(1.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_FALSE(opaque.IsTransparent());
}

TEST(StyleTest, StyleEdges_AllSame)
{
    StyleEdges edges(10.0f);
    EXPECT_NEAR(edges.top, 10.0f, 1e-5f);
    EXPECT_NEAR(edges.right, 10.0f, 1e-5f);
    EXPECT_NEAR(edges.bottom, 10.0f, 1e-5f);
    EXPECT_NEAR(edges.left, 10.0f, 1e-5f);
}

TEST(StyleTest, StyleEdges_Totals)
{
    StyleEdges edges(5.0f, 10.0f, 15.0f, 20.0f);
    EXPECT_NEAR(edges.HorizontalTotal(), 30.0f, 1e-5f);  // right + left = 10 + 20
    EXPECT_NEAR(edges.VerticalTotal(), 20.0f, 1e-5f);     // top + bottom = 5 + 15
}

TEST(StyleTest, StyleSelector_Parse_Type)
{
    StyleSelector sel = StyleSelector::Parse("Button");
    EXPECT_GT(sel.Specificity(), 0);
}

TEST(StyleTest, StyleSelector_Parse_Class)
{
    StyleSelector sel = StyleSelector::Parse(".highlight");
    EXPECT_EQ(sel.Specificity(), 10);
}

TEST(StyleTest, StyleSelector_Parse_ID)
{
    StyleSelector sel = StyleSelector::Parse("#main");
    EXPECT_EQ(sel.Specificity(), 100);
}

TEST(StyleTest, StyleSheet_LoadFromString)
{
    StyleSheet sheet;
    const char* css = R"(
        Button {
            background-color: #333333;
            color: #FFFFFF;
            font-size: 16px;
            padding: 8px 12px;
        }
        .highlight {
            color: #FFCC00;
        }
        #main {
            width: 100%;
            height: 50px;
        }
    )";
    bool ok = sheet.LoadFromString(css);
    EXPECT_TRUE(ok);
    EXPECT_GT(sheet.GetRuleCount(), 0u);
}
