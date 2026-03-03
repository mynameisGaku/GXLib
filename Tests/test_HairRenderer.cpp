/// @file test_HairRenderer.cpp
/// @brief HairRenderer (Marschner反射モデル) のユニットテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/HairRenderer.h"

using namespace gx;

// ============================================================================
// HairControlPoint / HairStrand 構造体テスト
// ============================================================================

TEST(HairRenderer, ControlPoint_DefaultValues)
{
    HairControlPoint cp;
    cp.position = { 0.0f, 0.0f, 0.0f };
    cp.thickness = 1.0f;
    EXPECT_FLOAT_EQ(cp.position.x, 0.0f);
    EXPECT_FLOAT_EQ(cp.thickness, 1.0f);
}

TEST(HairRenderer, Strand_CanAddControlPoints)
{
    HairStrand strand;
    EXPECT_EQ(strand.controlPoints.size(), 0u);

    HairControlPoint cp;
    cp.position = { 0, 0, 0 };
    cp.thickness = 1.0f;
    strand.controlPoints.push_back(cp);
    EXPECT_EQ(strand.controlPoints.size(), 1u);

    cp.position = { 0, 1, 0 };
    cp.thickness = 0.8f;
    strand.controlPoints.push_back(cp);
    EXPECT_EQ(strand.controlPoints.size(), 2u);
}

// ============================================================================
// HairMaterial テスト
// ============================================================================

TEST(HairRenderer, Material_DefaultValues)
{
    HairMaterial mat;
    EXPECT_NEAR(mat.baseColor.x, 0.15f, 0.01f);
    EXPECT_NEAR(mat.baseColor.y, 0.08f, 0.01f);
    EXPECT_NEAR(mat.baseColor.z, 0.04f, 0.01f);
    EXPECT_FLOAT_EQ(mat.roughnessR, 0.1f);
    EXPECT_FLOAT_EQ(mat.roughnessTT, 0.2f);
    EXPECT_FLOAT_EQ(mat.roughnessTRT, 0.3f);
    EXPECT_FLOAT_EQ(mat.shift, -0.1f);
    EXPECT_FLOAT_EQ(mat.ior, 1.55f);
    EXPECT_FLOAT_EQ(mat.absorption, 0.2f);
    EXPECT_FLOAT_EQ(mat.specularMultiplier, 1.0f);
    EXPECT_FLOAT_EQ(mat.scatterStrength, 0.5f);
}

TEST(HairRenderer, Material_CustomValues)
{
    HairMaterial mat;
    mat.baseColor = { 0.8f, 0.7f, 0.2f };
    mat.roughnessR = 0.05f;
    mat.roughnessTT = 0.15f;
    mat.roughnessTRT = 0.25f;
    mat.shift = -0.2f;
    mat.ior = 1.45f;
    mat.absorption = 0.5f;
    mat.specularMultiplier = 2.0f;
    mat.scatterStrength = 0.8f;

    EXPECT_FLOAT_EQ(mat.baseColor.x, 0.8f);
    EXPECT_FLOAT_EQ(mat.roughnessR, 0.05f);
    EXPECT_FLOAT_EQ(mat.ior, 1.45f);
    EXPECT_FLOAT_EQ(mat.specularMultiplier, 2.0f);
}

// ============================================================================
// HairRenderer 基本テスト (GPU不要)
// ============================================================================

TEST(HairRenderer, DefaultConstruction)
{
    HairRenderer renderer;
    EXPECT_EQ(renderer.GetStrandCount(), 0u);
    EXPECT_EQ(renderer.GetVertexCount(), 0u);
    EXPECT_FALSE(renderer.IsReady());
}

TEST(HairRenderer, Initialize_NullDevice_ReturnsFalse)
{
    HairRenderer renderer;
    EXPECT_FALSE(renderer.Initialize(nullptr));
    EXPECT_FALSE(renderer.IsReady());
}

TEST(HairRenderer, SetMaterial_GetMaterial)
{
    HairRenderer renderer;
    HairMaterial mat;
    mat.baseColor = { 1.0f, 0.5f, 0.2f };
    mat.roughnessR = 0.05f;
    mat.ior = 1.6f;

    renderer.SetMaterial(mat);
    const auto& retrieved = renderer.GetMaterial();
    EXPECT_FLOAT_EQ(retrieved.baseColor.x, 1.0f);
    EXPECT_FLOAT_EQ(retrieved.baseColor.y, 0.5f);
    EXPECT_FLOAT_EQ(retrieved.baseColor.z, 0.2f);
    EXPECT_FLOAT_EQ(retrieved.roughnessR, 0.05f);
    EXPECT_FLOAT_EQ(retrieved.ior, 1.6f);
}

TEST(HairRenderer, SetStrands_EmptyStrands)
{
    HairRenderer renderer;
    gx::Vector<HairStrand> strands;
    renderer.SetStrands(strands);
    EXPECT_EQ(renderer.GetStrandCount(), 0u);
    EXPECT_EQ(renderer.GetVertexCount(), 0u);
}

TEST(HairRenderer, SetStrands_SinglePointStrand_NoVertices)
{
    // 1制御点のストランドでは線分を作れないため、頂点数は0
    HairRenderer renderer;
    gx::Vector<HairStrand> strands(1);
    HairControlPoint cp;
    cp.position = { 0, 0, 0 };
    cp.thickness = 1.0f;
    strands[0].controlPoints.push_back(cp);

    renderer.SetStrands(strands);
    EXPECT_EQ(renderer.GetStrandCount(), 1u);
    EXPECT_EQ(renderer.GetVertexCount(), 0u);
}

TEST(HairRenderer, SetStrands_TwoPointStrand_TwoVertices)
{
    // 2制御点のストランドから1セグメント = 2頂点 (line list)
    // ただしGPUデバイスなしではバッファ作成されないため、
    // totalVerticesの計算値を検証
    HairRenderer renderer;
    gx::Vector<HairStrand> strands(1);

    HairControlPoint cp0;
    cp0.position = { 0, 0, 0 };
    cp0.thickness = 1.0f;
    strands[0].controlPoints.push_back(cp0);

    HairControlPoint cp1;
    cp1.position = { 0, 1, 0 };
    cp1.thickness = 0.5f;
    strands[0].controlPoints.push_back(cp1);

    renderer.SetStrands(strands);
    EXPECT_EQ(renderer.GetStrandCount(), 1u);
    // デバイスがないのでバッファは作成されないが、
    // 頂点数カウントは正しく計算される
    EXPECT_EQ(renderer.GetVertexCount(), 2u);
}

TEST(HairRenderer, SetStrands_MultipleStrands)
{
    HairRenderer renderer;
    gx::Vector<HairStrand> strands(3);

    // ストランド0: 3制御点 = 2セグメント = 4頂点
    for (int i = 0; i < 3; ++i)
    {
        HairControlPoint cp;
        cp.position = { static_cast<float>(i), static_cast<float>(i), 0 };
        cp.thickness = 1.0f - static_cast<float>(i) * 0.3f;
        strands[0].controlPoints.push_back(cp);
    }

    // ストランド1: 4制御点 = 3セグメント = 6頂点
    for (int i = 0; i < 4; ++i)
    {
        HairControlPoint cp;
        cp.position = { 2.0f + static_cast<float>(i), static_cast<float>(i), 0 };
        cp.thickness = 0.8f;
        strands[1].controlPoints.push_back(cp);
    }

    // ストランド2: 2制御点 = 1セグメント = 2頂点
    HairControlPoint cpA, cpB;
    cpA.position = { 5, 0, 0 }; cpA.thickness = 1.0f;
    cpB.position = { 5, 2, 0 }; cpB.thickness = 0.2f;
    strands[2].controlPoints.push_back(cpA);
    strands[2].controlPoints.push_back(cpB);

    renderer.SetStrands(strands);
    EXPECT_EQ(renderer.GetStrandCount(), 3u);
    // 4 + 6 + 2 = 12
    EXPECT_EQ(renderer.GetVertexCount(), 12u);
}

TEST(HairRenderer, SetStrands_MixedValidInvalid)
{
    HairRenderer renderer;
    gx::Vector<HairStrand> strands(3);

    // ストランド0: 空（無効）
    // ストランド1: 1制御点（無効 — 線分作れない）
    HairControlPoint cp;
    cp.position = { 0, 0, 0 };
    cp.thickness = 1.0f;
    strands[1].controlPoints.push_back(cp);

    // ストランド2: 2制御点（有効）
    cp.position = { 0, 0, 0 };
    strands[2].controlPoints.push_back(cp);
    cp.position = { 1, 1, 0 };
    strands[2].controlPoints.push_back(cp);

    renderer.SetStrands(strands);
    EXPECT_EQ(renderer.GetStrandCount(), 3u);
    EXPECT_EQ(renderer.GetVertexCount(), 2u);
}

TEST(HairRenderer, Render_NullCmdList_NoCrash)
{
    HairRenderer renderer;
    Matrix4x4 vp;
    Vector3 camPos = { 0, 0, -5 };
    Vector3 lightDir = { 0, -1, 0 };
    Vector3 lightColor = { 1, 1, 1 };

    // null cmdListで呼んでもクラッシュしないこと
    renderer.Render(nullptr, 0, vp, camPos, lightDir, lightColor);
    SUCCEED();
}

TEST(HairRenderer, Render_NotReady_NoCrash)
{
    HairRenderer renderer;
    // IsReady() == falseの状態でRenderを呼んでもクラッシュしないこと
    Matrix4x4 vp;
    renderer.Render(nullptr, 0, vp, { 0, 0, 0 }, { 0, -1, 0 }, { 1, 1, 1 });
    SUCCEED();
}

TEST(HairRenderer, Shutdown_NoCrash)
{
    HairRenderer renderer;
    // 未初期化のShutdownは安全であること
    renderer.Shutdown();
    EXPECT_FALSE(renderer.IsReady());
    EXPECT_EQ(renderer.GetStrandCount(), 0u);
    EXPECT_EQ(renderer.GetVertexCount(), 0u);
}

TEST(HairRenderer, SetStrands_LongStrand)
{
    // 長いストランド (100制御点) = 99セグメント = 198頂点
    HairRenderer renderer;
    gx::Vector<HairStrand> strands(1);

    for (int i = 0; i < 100; ++i)
    {
        HairControlPoint cp;
        cp.position = { 0.0f, static_cast<float>(i) * 0.1f, 0.0f };
        cp.thickness = 1.0f - static_cast<float>(i) / 100.0f;
        strands[0].controlPoints.push_back(cp);
    }

    renderer.SetStrands(strands);
    EXPECT_EQ(renderer.GetStrandCount(), 1u);
    EXPECT_EQ(renderer.GetVertexCount(), 198u);
}

TEST(HairRenderer, MaterialPersistsAfterSetStrands)
{
    HairRenderer renderer;
    HairMaterial mat;
    mat.baseColor = { 1.0f, 0.0f, 0.0f };
    mat.roughnessR = 0.02f;
    renderer.SetMaterial(mat);

    gx::Vector<HairStrand> strands(1);
    HairControlPoint cp;
    cp.position = { 0, 0, 0 }; cp.thickness = 1.0f;
    strands[0].controlPoints.push_back(cp);
    cp.position = { 0, 1, 0 };
    strands[0].controlPoints.push_back(cp);
    renderer.SetStrands(strands);

    // マテリアルはSetStrandsで変更されないこと
    const auto& retrieved = renderer.GetMaterial();
    EXPECT_FLOAT_EQ(retrieved.baseColor.x, 1.0f);
    EXPECT_FLOAT_EQ(retrieved.roughnessR, 0.02f);
}
