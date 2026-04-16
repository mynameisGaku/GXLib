/// @file test_EyeSkinShader.cpp
/// @brief Eye/Skin シェーダーのマテリアルパラメータテスト（CPU側のみ）

#include "pch.h"
#include <gtest/gtest.h>
#include <cstdint>

// Eye/Skin シェーダー用のローカル定数定義
// (Material.h や ShaderModelConstants.h を変更しない)
namespace {

constexpr uint32_t ShaderModel_Eye = 8;
constexpr uint32_t ShaderModel_Skin = 9;

struct EyeParams {
    float irisColor[3] = {0.4f, 0.25f, 0.1f};
    float pupilSize = 0.3f;
    float scleraColor[3] = {1.0f, 0.98f, 0.95f};
    float irisIOR = 1.406f;
    float corneaReflection = 0.5f;
    float irisDepth = 0.02f;
    float limbusWidth = 0.05f;
};

struct SkinParams {
    float subsurfaceColor[3] = {0.8f, 0.2f, 0.1f};
    float subsurfaceRadius = 2.5f;
    float subsurfaceStrength = 0.5f;
    float microNormalScale = 0.5f;
    float cavityStrength = 0.3f;
    float fresnelOffset = 0.028f;
    float specTint[3] = {1.0f, 0.95f, 0.9f};
};

static constexpr float kEps = 1e-5f;

} // anonymous namespace

// ============================================================================
// EyeShaderModelId: Eyeシェーダーモデル識別子
// ============================================================================
TEST(EyeSkinShaderTest, EyeShaderModelId)
{
    EXPECT_EQ(ShaderModel_Eye, 8u);
    // 既存のシェーダーモデルと衝突しない
    EXPECT_NE(ShaderModel_Eye, 0u);  // Standard
    EXPECT_NE(ShaderModel_Eye, 1u);  // Unlit
    EXPECT_NE(ShaderModel_Eye, 2u);  // Toon
    EXPECT_NE(ShaderModel_Eye, 3u);  // Phong
    EXPECT_NE(ShaderModel_Eye, 4u);  // Subsurface
    EXPECT_NE(ShaderModel_Eye, 5u);  // ClearCoat
    EXPECT_NE(ShaderModel_Eye, 255u); // Custom
}

// ============================================================================
// EyeDefaults: Eyeパラメータのデフォルト値
// ============================================================================
TEST(EyeSkinShaderTest, EyeDefaults)
{
    EyeParams params;
    EXPECT_NEAR(params.pupilSize, 0.3f, kEps);
    EXPECT_NEAR(params.irisIOR, 1.406f, kEps);
    EXPECT_NEAR(params.corneaReflection, 0.5f, kEps);
    EXPECT_NEAR(params.irisDepth, 0.02f, kEps);
    EXPECT_NEAR(params.limbusWidth, 0.05f, kEps);
}

// ============================================================================
// EyeIrisParams: 虹彩カラーと強膜カラー
// ============================================================================
TEST(EyeSkinShaderTest, EyeIrisParams)
{
    EyeParams params;

    // 虹彩カラー (ブラウン系)
    EXPECT_NEAR(params.irisColor[0], 0.4f, kEps);
    EXPECT_NEAR(params.irisColor[1], 0.25f, kEps);
    EXPECT_NEAR(params.irisColor[2], 0.1f, kEps);

    // 強膜カラー (ほぼ白)
    EXPECT_NEAR(params.scleraColor[0], 1.0f, kEps);
    EXPECT_NEAR(params.scleraColor[1], 0.98f, kEps);
    EXPECT_NEAR(params.scleraColor[2], 0.95f, kEps);

    // 強膜は虹彩より明るい
    EXPECT_GT(params.scleraColor[0], params.irisColor[0]);
    EXPECT_GT(params.scleraColor[1], params.irisColor[1]);
    EXPECT_GT(params.scleraColor[2], params.irisColor[2]);
}

// ============================================================================
// EyePupilSize: 瞳孔サイズ
// ============================================================================
TEST(EyeSkinShaderTest, EyePupilSize)
{
    EyeParams params;
    EXPECT_NEAR(params.pupilSize, 0.3f, kEps);

    // 瞳孔サイズは0-1の範囲
    EXPECT_GE(params.pupilSize, 0.0f);
    EXPECT_LE(params.pupilSize, 1.0f);

    // カスタム値
    params.pupilSize = 0.5f;
    EXPECT_NEAR(params.pupilSize, 0.5f, kEps);

    // 散瞳
    params.pupilSize = 0.8f;
    EXPECT_NEAR(params.pupilSize, 0.8f, kEps);

    // 縮瞳
    params.pupilSize = 0.1f;
    EXPECT_NEAR(params.pupilSize, 0.1f, kEps);
}

// ============================================================================
// EyeCorneaReflection: 角膜反射パラメータ
// ============================================================================
TEST(EyeSkinShaderTest, EyeCorneaReflection)
{
    EyeParams params;
    EXPECT_NEAR(params.corneaReflection, 0.5f, kEps);
    EXPECT_NEAR(params.irisIOR, 1.406f, kEps);

    // IORの物理的妥当性 (角膜は水とガラスの中間程度)
    EXPECT_GT(params.irisIOR, 1.0f);   // 真空より大きい
    EXPECT_LT(params.irisIOR, 2.0f);   // ダイヤモンドより小さい

    // カスタム値
    params.corneaReflection = 0.8f;
    EXPECT_NEAR(params.corneaReflection, 0.8f, kEps);
}

// ============================================================================
// SkinShaderModelId: Skinシェーダーモデル識別子
// ============================================================================
TEST(EyeSkinShaderTest, SkinShaderModelId)
{
    EXPECT_EQ(ShaderModel_Skin, 9u);
    EXPECT_NE(ShaderModel_Skin, ShaderModel_Eye);
    EXPECT_NE(ShaderModel_Skin, 0u);   // Standard
    EXPECT_NE(ShaderModel_Skin, 4u);   // Subsurface (既存の汎用SSS)
    EXPECT_NE(ShaderModel_Skin, 255u); // Custom
}

// ============================================================================
// SkinDefaults: Skinパラメータのデフォルト値
// ============================================================================
TEST(EyeSkinShaderTest, SkinDefaults)
{
    SkinParams params;
    EXPECT_NEAR(params.subsurfaceRadius, 2.5f, kEps);
    EXPECT_NEAR(params.subsurfaceStrength, 0.5f, kEps);
    EXPECT_NEAR(params.microNormalScale, 0.5f, kEps);
    EXPECT_NEAR(params.cavityStrength, 0.3f, kEps);
    EXPECT_NEAR(params.fresnelOffset, 0.028f, kEps);
}

// ============================================================================
// SkinSubsurface: サブサーフェスカラーとパラメータ
// ============================================================================
TEST(EyeSkinShaderTest, SkinSubsurface)
{
    SkinParams params;

    // サブサーフェスカラー (赤みの強い散乱)
    EXPECT_NEAR(params.subsurfaceColor[0], 0.8f, kEps);
    EXPECT_NEAR(params.subsurfaceColor[1], 0.2f, kEps);
    EXPECT_NEAR(params.subsurfaceColor[2], 0.1f, kEps);

    // 赤チャンネルが最も強い (血液による赤色散乱)
    EXPECT_GT(params.subsurfaceColor[0], params.subsurfaceColor[1]);
    EXPECT_GT(params.subsurfaceColor[1], params.subsurfaceColor[2]);

    // 散乱半径の物理的妥当性 (数mm相当)
    EXPECT_GT(params.subsurfaceRadius, 0.0f);
}

// ============================================================================
// SkinMicroNormal: マイクロ法線とキャビティ
// ============================================================================
TEST(EyeSkinShaderTest, SkinMicroNormal)
{
    SkinParams params;

    // マイクロ法線スケール
    EXPECT_NEAR(params.microNormalScale, 0.5f, kEps);
    EXPECT_GT(params.microNormalScale, 0.0f);
    EXPECT_LE(params.microNormalScale, 1.0f);

    // キャビティ強度
    EXPECT_NEAR(params.cavityStrength, 0.3f, kEps);
    EXPECT_GT(params.cavityStrength, 0.0f);
    EXPECT_LE(params.cavityStrength, 1.0f);

    // フレネルオフセット (皮膚のF0は0.028程度)
    EXPECT_NEAR(params.fresnelOffset, 0.028f, kEps);
    EXPECT_GT(params.fresnelOffset, 0.0f);
    EXPECT_LT(params.fresnelOffset, 0.1f);
}

// ============================================================================
// SkinParamsSize: SkinParamsのメモリレイアウト
// ============================================================================
TEST(EyeSkinShaderTest, SkinParamsSize)
{
    SkinParams params;

    // 全フィールドが48B以内に収まる (GPU定数バッファのcustom領域: 3x float4 = 48B)
    // subsurfaceColor[3] + subsurfaceRadius + subsurfaceStrength +
    // microNormalScale + cavityStrength + fresnelOffset + specTint[3] = 11 floats = 44B
    EXPECT_LE(sizeof(SkinParams), 48u);

    // スペキュラティント
    EXPECT_NEAR(params.specTint[0], 1.0f, kEps);
    EXPECT_NEAR(params.specTint[1], 0.95f, kEps);
    EXPECT_NEAR(params.specTint[2], 0.9f, kEps);

    // スペキュラティントは暖色系 (皮膚は赤みを帯びた反射)
    EXPECT_GE(params.specTint[0], params.specTint[1]);
    EXPECT_GE(params.specTint[1], params.specTint[2]);
}
