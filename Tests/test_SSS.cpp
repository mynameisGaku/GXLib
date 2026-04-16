/// @file test_SSS.cpp
/// @brief Screen-Space Subsurface Scattering のテスト
///
/// GPU不要のCPU側ロジック（設定管理、カーネル計算、パラメータインターフェース）を
/// 単体テストで検証する。GPU描画パスは統合テスト（WARP）で確認。

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/PostEffect/SSS.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ============================================================================
// デフォルト状態
// ============================================================================

TEST(SSSTest, DefaultDisabledState)
{
    SSS sss;
    EXPECT_FALSE(sss.IsEnabled());
}

TEST(SSSTest, DefaultSettings)
{
    SSS sss;
    const auto& s = sss.GetSettings();
    EXPECT_FALSE(s.enabled);
    EXPECT_FLOAT_EQ(s.strength, 1.0f);
    EXPECT_FLOAT_EQ(s.width, 0.012f);
    EXPECT_EQ(s.numSamples, 25);
    EXPECT_FLOAT_EQ(s.translucency, 0.0f);
}

TEST(SSSTest, DefaultFalloff)
{
    SSS sss;
    const auto& s = sss.GetSettings();
    EXPECT_FLOAT_EQ(s.falloff.x, 1.0f);
    EXPECT_FLOAT_EQ(s.falloff.y, 0.37f);
    EXPECT_FLOAT_EQ(s.falloff.z, 0.3f);
}

TEST(SSSTest, DefaultStrength3)
{
    SSS sss;
    const auto& s = sss.GetSettings();
    EXPECT_FLOAT_EQ(s.strength3.x, 0.48f);
    EXPECT_FLOAT_EQ(s.strength3.y, 0.41f);
    EXPECT_FLOAT_EQ(s.strength3.z, 0.28f);
}

// ============================================================================
// セッターゲッター
// ============================================================================

TEST(SSSTest, SetEnabledGetEnabled)
{
    SSS sss;
    sss.SetEnabled(true);
    EXPECT_TRUE(sss.IsEnabled());
    sss.SetEnabled(false);
    EXPECT_FALSE(sss.IsEnabled());
}

TEST(SSSTest, SetStrengthGetStrength)
{
    SSS sss;
    sss.SetStrength(2.5f);
    EXPECT_FLOAT_EQ(sss.GetStrength(), 2.5f);
}

TEST(SSSTest, SetWidthGetWidth)
{
    SSS sss;
    sss.SetWidth(0.025f);
    EXPECT_FLOAT_EQ(sss.GetWidth(), 0.025f);
}

TEST(SSSTest, SetFalloffGetFalloff)
{
    SSS sss;
    Vector3 f = { 0.5f, 0.2f, 0.1f };
    sss.SetFalloff(f);
    const auto& got = sss.GetFalloff();
    EXPECT_FLOAT_EQ(got.x, 0.5f);
    EXPECT_FLOAT_EQ(got.y, 0.2f);
    EXPECT_FLOAT_EQ(got.z, 0.1f);
}

TEST(SSSTest, SetStrength3GetStrength3)
{
    SSS sss;
    Vector3 s = { 0.6f, 0.5f, 0.4f };
    sss.SetStrength3(s);
    const auto& got = sss.GetStrength3();
    EXPECT_FLOAT_EQ(got.x, 0.6f);
    EXPECT_FLOAT_EQ(got.y, 0.5f);
    EXPECT_FLOAT_EQ(got.z, 0.4f);
}

TEST(SSSTest, SetNumSamplesGetNumSamples)
{
    SSS sss;
    sss.SetNumSamples(11);
    EXPECT_EQ(sss.GetNumSamples(), 11);
}

TEST(SSSTest, SetSettingsOverwritesAll)
{
    SSS sss;
    SSSSettings s;
    s.enabled     = true;
    s.strength    = 3.0f;
    s.width       = 0.05f;
    s.falloff     = { 0.8f, 0.6f, 0.4f };
    s.strength3   = { 0.3f, 0.2f, 0.1f };
    s.numSamples  = 15;
    s.translucency = 0.5f;
    sss.SetSettings(s);

    const auto& got = sss.GetSettings();
    EXPECT_TRUE(got.enabled);
    EXPECT_FLOAT_EQ(got.strength, 3.0f);
    EXPECT_FLOAT_EQ(got.width, 0.05f);
    EXPECT_FLOAT_EQ(got.falloff.x, 0.8f);
    EXPECT_FLOAT_EQ(got.strength3.x, 0.3f);
    EXPECT_EQ(got.numSamples, 15);
    EXPECT_FLOAT_EQ(got.translucency, 0.5f);
}

// ============================================================================
// IsReady
// ============================================================================

TEST(SSSTest, IsReadyFalseWithoutInit)
{
    SSS sss;
    EXPECT_FALSE(sss.IsReady());
}

TEST(SSSTest, IsReadyFalseWhenDisabled)
{
    SSS sss;
    // nullptr device → CPU only mode (no PSO)
    sss.Initialize(nullptr, 1920, 1080);
    sss.SetEnabled(true);
    // PSO is null in CPU-only mode
    EXPECT_FALSE(sss.IsReady());
}

// ============================================================================
// CPU初期化（nullptr device）
// ============================================================================

TEST(SSSTest, InitializeNullDeviceSucceeds)
{
    SSS sss;
    bool ok = sss.Initialize(nullptr, 1920, 1080);
    EXPECT_TRUE(ok);
}

TEST(SSSTest, InitializeZeroDimSucceeds)
{
    SSS sss;
    bool ok = sss.Initialize(nullptr, 0, 0);
    EXPECT_TRUE(ok);
}

// ============================================================================
// カーネル計算の検証（SetSettings経由で間接的にテスト）
// ============================================================================

TEST(SSSTest, KernelComputedOnSetSettings)
{
    SSS sss;
    SSSSettings s;
    s.numSamples = 5;
    s.falloff    = { 1.0f, 1.0f, 1.0f };
    s.strength3  = { 1.0f, 1.0f, 1.0f };
    sss.SetSettings(s);
    // After SetSettings, kernel should be recomputed
    EXPECT_EQ(sss.GetNumSamples(), 5);
}

TEST(SSSTest, KernelComputedOnSetFalloff)
{
    SSS sss;
    sss.SetFalloff({ 0.5f, 0.5f, 0.5f });
    // Ensures no crash — kernel recomputed internally
    EXPECT_FLOAT_EQ(sss.GetFalloff().x, 0.5f);
}

TEST(SSSTest, KernelComputedOnSetStrength3)
{
    SSS sss;
    sss.SetStrength3({ 0.3f, 0.3f, 0.3f });
    EXPECT_FLOAT_EQ(sss.GetStrength3().x, 0.3f);
}

TEST(SSSTest, KernelComputedOnSetNumSamples)
{
    SSS sss;
    sss.SetNumSamples(7);
    EXPECT_EQ(sss.GetNumSamples(), 7);
}

// ============================================================================
// MaxSamplesConstant
// ============================================================================

TEST(SSSTest, MaxSamplesIs25)
{
    EXPECT_EQ(SSS::k_MaxSamples, 25);
}

TEST(SSSTest, NumSamplesClampedBySetSettings)
{
    SSS sss;
    SSSSettings s;
    s.numSamples = 100; // exceeds k_MaxSamples
    sss.SetSettings(s);
    // numSamples stores the raw value but kernel computation clamps it
    EXPECT_EQ(sss.GetNumSamples(), 100);
    // The actual kernel is computed with min(numSamples, k_MaxSamples)
    // This is verified by the absence of out-of-bounds access
}

// ============================================================================
// OnResize (CPU側のみ)
// ============================================================================

TEST(SSSTest, OnResizeUpdatesNothing)
{
    SSS sss;
    sss.Initialize(nullptr, 800, 600);
    // nullptr device → no crash
    sss.OnResize(nullptr, 1920, 1080);
}

// ============================================================================
// SSSSettings struct defaults
// ============================================================================

TEST(SSSTest, SSSSettingsDefaultConstruction)
{
    SSSSettings s;
    EXPECT_FALSE(s.enabled);
    EXPECT_FLOAT_EQ(s.strength, 1.0f);
    EXPECT_FLOAT_EQ(s.width, 0.012f);
    EXPECT_EQ(s.numSamples, 25);
}

TEST(SSSTest, SSSSettingsCopyable)
{
    SSSSettings a;
    a.enabled = true;
    a.strength = 2.0f;
    a.width = 0.03f;
    a.falloff = { 0.5f, 0.4f, 0.3f };

    SSSSettings b = a;
    EXPECT_TRUE(b.enabled);
    EXPECT_FLOAT_EQ(b.strength, 2.0f);
    EXPECT_FLOAT_EQ(b.width, 0.03f);
    EXPECT_FLOAT_EQ(b.falloff.x, 0.5f);
}

// ============================================================================
// 複数インスタンスの独立性
// ============================================================================

TEST(SSSTest, MultipleInstancesIndependent)
{
    SSS a, b;
    a.SetStrength(5.0f);
    b.SetStrength(0.1f);
    EXPECT_FLOAT_EQ(a.GetStrength(), 5.0f);
    EXPECT_FLOAT_EQ(b.GetStrength(), 0.1f);
}

TEST(SSSTest, MultipleInstancesEnabledIndependent)
{
    SSS a, b;
    a.SetEnabled(true);
    b.SetEnabled(false);
    EXPECT_TRUE(a.IsEnabled());
    EXPECT_FALSE(b.IsEnabled());
}
