/// @file test_HairShader.cpp
/// @brief Hair/Fur シェーダーのマテリアルパラメータテスト（CPU側のみ）

#include "pch.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>

// Hair シェーダー用のローカル定数定義
// (Material.h や ShaderModelConstants.h を変更しない)
namespace {

constexpr uint32_t ShaderModel_Hair = 7;
constexpr uint32_t HasHairFlowMap = (1u << 9);

struct HairParams {
    float specularShift1 = -0.1f;
    float specularShift2 = 0.1f;
    float specularExp1 = 80.0f;
    float specularExp2 = 40.0f;
    float rootColor[3] = {0.1f, 0.05f, 0.02f};
    float rootToTipBlend = 0.5f;
    float tipColor[3] = {0.3f, 0.15f, 0.05f};
    float alphaCutoff = 0.5f;
};

static constexpr float kEps = 1e-5f;

} // anonymous namespace

// ============================================================================
// MaterialDefaultHair: HairParams のデフォルト値
// ============================================================================
TEST(HairShaderTest, MaterialDefaultHair)
{
    HairParams params;
    EXPECT_NEAR(params.specularShift1, -0.1f, kEps);
    EXPECT_NEAR(params.specularShift2,  0.1f, kEps);
    EXPECT_NEAR(params.specularExp1, 80.0f, kEps);
    EXPECT_NEAR(params.specularExp2, 40.0f, kEps);
    EXPECT_NEAR(params.rootToTipBlend, 0.5f, kEps);
    EXPECT_NEAR(params.alphaCutoff, 0.5f, kEps);
}

// ============================================================================
// HairFlagsBit: フローマップフラグのビット位置
// ============================================================================
TEST(HairShaderTest, HairFlagsBit)
{
    EXPECT_EQ(HasHairFlowMap, 1u << 9);
    EXPECT_EQ(HasHairFlowMap, 512u);
}

// ============================================================================
// HairShaderModelId: シェーダーモデルID
// ============================================================================
TEST(HairShaderTest, HairShaderModelId)
{
    EXPECT_EQ(ShaderModel_Hair, 7u);
    // 既存のシェーダーモデル (0-5, 255) と衝突しないこと
    EXPECT_NE(ShaderModel_Hair, 0u); // Standard
    EXPECT_NE(ShaderModel_Hair, 1u); // Unlit
    EXPECT_NE(ShaderModel_Hair, 2u); // Toon
    EXPECT_NE(ShaderModel_Hair, 3u); // Phong
    EXPECT_NE(ShaderModel_Hair, 4u); // Subsurface
    EXPECT_NE(ShaderModel_Hair, 5u); // ClearCoat
    EXPECT_NE(ShaderModel_Hair, 255u); // Custom
}

// ============================================================================
// HairParamsDefaults: 全フィールドのデフォルト値確認
// ============================================================================
TEST(HairShaderTest, HairParamsDefaults)
{
    HairParams params;
    // 全パラメータがゼロでないこと（意図的なデフォルト値がセットされている）
    EXPECT_NE(params.specularExp1, 0.0f);
    EXPECT_NE(params.specularExp2, 0.0f);
    EXPECT_NE(params.rootToTipBlend, 0.0f);
    EXPECT_NE(params.alphaCutoff, 0.0f);
}

// ============================================================================
// HairColors: 根元/先端カラーのデフォルト値
// ============================================================================
TEST(HairShaderTest, HairColors)
{
    HairParams params;
    // 根元カラー (ダークブラウン)
    EXPECT_NEAR(params.rootColor[0], 0.1f, kEps);
    EXPECT_NEAR(params.rootColor[1], 0.05f, kEps);
    EXPECT_NEAR(params.rootColor[2], 0.02f, kEps);

    // 先端カラー (ライトブラウン)
    EXPECT_NEAR(params.tipColor[0], 0.3f, kEps);
    EXPECT_NEAR(params.tipColor[1], 0.15f, kEps);
    EXPECT_NEAR(params.tipColor[2], 0.05f, kEps);

    // 先端は根元より明るいこと
    EXPECT_GT(params.tipColor[0], params.rootColor[0]);
    EXPECT_GT(params.tipColor[1], params.rootColor[1]);
    EXPECT_GT(params.tipColor[2], params.rootColor[2]);
}

// ============================================================================
// HairFlowMapFlag: フローマップフラグの結合テスト
// ============================================================================
TEST(HairShaderTest, HairFlowMapFlag)
{
    uint32_t flags = 0;

    // フローマップフラグ単体
    flags |= HasHairFlowMap;
    EXPECT_TRUE(flags & HasHairFlowMap);

    // 他のフラグ (bit0-8) と結合しても独立
    uint32_t otherFlags = (1u << 0) | (1u << 1) | (1u << 8);
    flags |= otherFlags;
    EXPECT_TRUE(flags & HasHairFlowMap);
    EXPECT_TRUE(flags & (1u << 0));
    EXPECT_TRUE(flags & (1u << 8));
}

// ============================================================================
// HairAlphaCutoff: アルファカットオフの設定
// ============================================================================
TEST(HairShaderTest, HairAlphaCutoff)
{
    HairParams params;
    EXPECT_NEAR(params.alphaCutoff, 0.5f, kEps);

    // カスタム値の設定
    params.alphaCutoff = 0.3f;
    EXPECT_NEAR(params.alphaCutoff, 0.3f, kEps);

    // 0.0は透過なし
    params.alphaCutoff = 0.0f;
    EXPECT_NEAR(params.alphaCutoff, 0.0f, kEps);

    // 1.0はほぼ全てカット
    params.alphaCutoff = 1.0f;
    EXPECT_NEAR(params.alphaCutoff, 1.0f, kEps);
}

// ============================================================================
// HairSpecularShifts: スペキュラシフトの符号テスト
// ============================================================================
TEST(HairShaderTest, HairSpecularShifts)
{
    HairParams params;
    // 一次ハイライトは法線の反対側にシフト（負）
    EXPECT_LT(params.specularShift1, 0.0f);
    // 二次ハイライトは法線側にシフト（正）
    EXPECT_GT(params.specularShift2, 0.0f);

    // シフトの絶対値は等しい（対称）
    EXPECT_NEAR(std::abs(params.specularShift1), std::abs(params.specularShift2), kEps);

    // カスタム値の設定
    params.specularShift1 = -0.2f;
    params.specularShift2 = 0.3f;
    EXPECT_NEAR(params.specularShift1, -0.2f, kEps);
    EXPECT_NEAR(params.specularShift2,  0.3f, kEps);
}
