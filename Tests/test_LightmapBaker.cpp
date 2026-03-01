/// @file test_LightmapBaker.cpp
/// @brief LightmapBaker のテスト — 設定、メッシュ/ライト登録、ベイキング、アトラスパッキング

#include <gtest/gtest.h>
#include "Graphics/3D/LightmapBaker.h"

using namespace gx;

// ============================================================================
// ヘルパー: 単純な平面メッシュ（地面）
// ============================================================================

static void CreateGroundPlane(std::vector<Vector3>& positions,
                               std::vector<Vector3>& normals,
                               std::vector<Vector3>& uvs,
                               std::vector<uint32_t>& indices)
{
    positions = {
        {-5, 0, -5}, {5, 0, -5}, {5, 0, 5}, {-5, 0, 5}
    };
    normals = {
        {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}
    };
    uvs = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}
    };
    indices = {0, 1, 2, 0, 2, 3};
}

// ============================================================================
// 設定テスト
// ============================================================================

TEST(LightmapBakerTest, DefaultConfig)
{
    LightmapBaker baker;
    const auto& cfg = baker.GetConfig();
    EXPECT_EQ(cfg.resolution, 256u);
    EXPECT_EQ(cfg.samplesPerTexel, 64u);
    EXPECT_EQ(cfg.bounceCount, 2u);
    EXPECT_EQ(cfg.quality, LightmapQuality::Medium);
    EXPECT_FALSE(cfg.useGPU);
}

TEST(LightmapBakerTest, ConfigResolutionClamp)
{
    LightmapBaker baker;

    // 下限クランプ
    LightmapConfig cfg;
    cfg.resolution = 10;
    baker.SetConfig(cfg);
    EXPECT_GE(baker.GetConfig().resolution, 256u);

    // 上限クランプ
    cfg.resolution = 99999;
    baker.SetConfig(cfg);
    EXPECT_LE(baker.GetConfig().resolution, 4096u);

    // サンプル数クランプ
    cfg.samplesPerTexel = 1;
    baker.SetConfig(cfg);
    EXPECT_GE(baker.GetConfig().samplesPerTexel, 16u);

    cfg.samplesPerTexel = 10000;
    baker.SetConfig(cfg);
    EXPECT_LE(baker.GetConfig().samplesPerTexel, 1024u);

    // バウンス数クランプ
    cfg.bounceCount = 0;
    baker.SetConfig(cfg);
    EXPECT_GE(baker.GetConfig().bounceCount, 1u);

    cfg.bounceCount = 100;
    baker.SetConfig(cfg);
    EXPECT_LE(baker.GetConfig().bounceCount, 8u);
}

// ============================================================================
// メッシュ/ライト登録テスト
// ============================================================================

TEST(LightmapBakerTest, AddMeshInstance)
{
    LightmapBaker baker;
    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    EXPECT_EQ(baker.GetMeshInstanceCount(), 1u);
}

TEST(LightmapBakerTest, AddMultipleMeshes)
{
    LightmapBaker baker;
    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    baker.AddMeshInstance(1, pos, norm, uv, idx);
    baker.AddMeshInstance(2, pos, norm, uv, idx);
    EXPECT_EQ(baker.GetMeshInstanceCount(), 3u);
}

TEST(LightmapBakerTest, AddLight)
{
    LightmapBaker baker;
    baker.AddLight(LightmapLightType::Point, {0, 5, 0}, {0, -1, 0},
                   Color{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 10.0f);
    EXPECT_EQ(baker.GetLightCount(), 1u);
}

TEST(LightmapBakerTest, AddMultipleLights)
{
    LightmapBaker baker;
    baker.AddLight(LightmapLightType::Point, {0, 5, 0}, {0, -1, 0},
                   Color{1.0f, 0.0f, 0.0f, 1.0f}, 1.0f, 10.0f);
    baker.AddLight(LightmapLightType::Directional, {0, 0, 0}, {0, -1, 0},
                   Color{1.0f, 1.0f, 1.0f, 1.0f}, 0.5f, 0.0f);
    baker.AddLight(LightmapLightType::Spot, {0, 3, 0}, {0, -1, 0},
                   Color{0.0f, 0.0f, 1.0f, 1.0f}, 2.0f, 5.0f);
    EXPECT_EQ(baker.GetLightCount(), 3u);
}

// ============================================================================
// クリアテスト
// ============================================================================

TEST(LightmapBakerTest, ClearState)
{
    LightmapBaker baker;
    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    baker.AddLight(LightmapLightType::Point, {0, 5, 0}, {0, -1, 0},
                   Color{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 10.0f);

    baker.Clear();

    EXPECT_EQ(baker.GetMeshInstanceCount(), 0u);
    EXPECT_EQ(baker.GetLightCount(), 0u);
    EXPECT_TRUE(baker.GetAtlas().charts.empty());
    EXPECT_FLOAT_EQ(baker.GetProgress(), 0.0f);
}

// ============================================================================
// ベイキングテスト
// ============================================================================

TEST(LightmapBakerTest, BakeEmptyScene)
{
    LightmapBaker baker;
    // クラッシュしないことを確認
    baker.Bake();
    EXPECT_FLOAT_EQ(baker.GetProgress(), 1.0f);
    EXPECT_TRUE(baker.GetAtlas().charts.empty());
}

TEST(LightmapBakerTest, BakeDirectOnly)
{
    LightmapBaker baker;

    LightmapConfig cfg;
    cfg.resolution = 256;
    cfg.samplesPerTexel = 16;
    cfg.bounceCount = 1;
    baker.SetConfig(cfg);

    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    baker.AddLight(LightmapLightType::Point, {0, 5, 0}, {0, -1, 0},
                   Color{1.0f, 1.0f, 1.0f, 1.0f}, 2.0f, 20.0f);

    baker.BakeDirectOnly();

    EXPECT_FLOAT_EQ(baker.GetProgress(), 1.0f);
    EXPECT_EQ(baker.GetAtlas().charts.size(), 1u);

    // いくつかのテクセルが照らされているか確認
    const auto& chart = baker.GetAtlas().charts[0];
    bool hasValidTexel = false;
    bool hasLitTexel = false;
    for (const auto& texel : chart.texelData)
    {
        if (texel.valid)
        {
            hasValidTexel = true;
            if (texel.color.r > 0.01f || texel.color.g > 0.01f || texel.color.b > 0.01f)
                hasLitTexel = true;
        }
    }
    EXPECT_TRUE(hasValidTexel);
    EXPECT_TRUE(hasLitTexel);
}

TEST(LightmapBakerTest, BakeSingleLight)
{
    LightmapBaker baker;

    LightmapConfig cfg;
    cfg.resolution = 256;
    cfg.samplesPerTexel = 16;
    cfg.bounceCount = 1;
    baker.SetConfig(cfg);

    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    baker.AddLight(LightmapLightType::Directional, {0, 0, 0}, {0, -1, 0},
                   Color{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.0f);

    baker.BakeDirectOnly();

    EXPECT_EQ(baker.GetAtlas().charts.size(), 1u);

    const auto& chart = baker.GetAtlas().charts[0];
    // ディレクショナルライトが上から当たるので、有効なテクセルは全て照らされるはず
    for (const auto& texel : chart.texelData)
    {
        if (texel.valid)
        {
            // アンビエント + 直接照明で何らかの明るさがあるはず
            EXPECT_GT(texel.color.r + texel.color.g + texel.color.b, 0.0f);
        }
    }
}

TEST(LightmapBakerTest, BakeMultipleBounces)
{
    LightmapBaker baker;

    LightmapConfig cfg;
    cfg.resolution = 256;
    cfg.samplesPerTexel = 16;
    cfg.bounceCount = 3;
    baker.SetConfig(cfg);

    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    baker.AddLight(LightmapLightType::Point, {0, 3, 0}, {0, -1, 0},
                   Color{1.0f, 1.0f, 1.0f, 1.0f}, 2.0f, 15.0f);

    baker.Bake();

    EXPECT_FLOAT_EQ(baker.GetProgress(), 1.0f);
    EXPECT_EQ(baker.GetAtlas().charts.size(), 1u);
}

// ============================================================================
// アトラスパッキングテスト
// ============================================================================

TEST(LightmapBakerTest, PackAtlasEmpty)
{
    LightmapBaker baker;
    PackResult result = baker.PackAtlas();

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.totalTexels, 0u);
    EXPECT_EQ(result.usedTexels, 0u);
}

TEST(LightmapBakerTest, PackAtlasSingleChart)
{
    LightmapBaker baker;

    LightmapConfig cfg;
    cfg.resolution = 256;
    cfg.samplesPerTexel = 16;
    cfg.bounceCount = 1;
    baker.SetConfig(cfg);

    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    baker.BakeDirectOnly();

    PackResult result = baker.PackAtlas();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(baker.GetAtlas().packed);
    EXPECT_GT(baker.GetAtlas().totalWidth, 0u);
    EXPECT_GT(baker.GetAtlas().totalHeight, 0u);
}

TEST(LightmapBakerTest, PackAtlasMultiple)
{
    LightmapBaker baker;

    LightmapConfig cfg;
    cfg.resolution = 256;
    cfg.samplesPerTexel = 16;
    cfg.bounceCount = 1;
    baker.SetConfig(cfg);

    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    baker.AddMeshInstance(1, pos, norm, uv, idx);
    baker.AddMeshInstance(2, pos, norm, uv, idx);

    baker.BakeDirectOnly();

    PackResult result = baker.PackAtlas();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(baker.GetAtlas().charts.size(), 3u);
}

TEST(LightmapBakerTest, PackEfficiency)
{
    LightmapBaker baker;

    LightmapConfig cfg;
    cfg.resolution = 256;
    cfg.samplesPerTexel = 16;
    cfg.bounceCount = 1;
    baker.SetConfig(cfg);

    std::vector<Vector3> pos, norm, uv;
    std::vector<uint32_t> idx;
    CreateGroundPlane(pos, norm, uv, idx);

    baker.AddMeshInstance(0, pos, norm, uv, idx);
    baker.BakeDirectOnly();

    PackResult result = baker.PackAtlas();
    EXPECT_TRUE(result.success);
    // 効率は0-1の範囲
    EXPECT_GE(result.packEfficiency, 0.0f);
    EXPECT_LE(result.packEfficiency, 1.0f);
    EXPECT_GT(result.usedTexels, 0u);
    EXPECT_GT(result.totalTexels, 0u);
}

// ============================================================================
// 進捗/キャンセルテスト
// ============================================================================

TEST(LightmapBakerTest, ProgressInitialZero)
{
    LightmapBaker baker;
    EXPECT_FLOAT_EQ(baker.GetProgress(), 0.0f);
}

TEST(LightmapBakerTest, CancelBake)
{
    LightmapBaker baker;
    // Cancel前は false
    baker.Cancel();
    // キャンセルフラグが設定されていればベイクは途中で止まる
    // ここではAPIが正しく動作することを確認
    baker.Bake();
    // 空シーンでは即座に完了するのでprogressは1.0
    EXPECT_FLOAT_EQ(baker.GetProgress(), 1.0f);
}

// ============================================================================
// 列挙型テスト
// ============================================================================

TEST(LightmapBakerTest, LightTypeEnums)
{
    EXPECT_NE(static_cast<int>(LightmapLightType::Point),
              static_cast<int>(LightmapLightType::Directional));
    EXPECT_NE(static_cast<int>(LightmapLightType::Directional),
              static_cast<int>(LightmapLightType::Spot));
    EXPECT_NE(static_cast<int>(LightmapLightType::Point),
              static_cast<int>(LightmapLightType::Spot));
}

TEST(LightmapBakerTest, QualityEnums)
{
    EXPECT_NE(static_cast<int>(LightmapQuality::Draft),
              static_cast<int>(LightmapQuality::Medium));
    EXPECT_NE(static_cast<int>(LightmapQuality::Medium),
              static_cast<int>(LightmapQuality::High));
    EXPECT_NE(static_cast<int>(LightmapQuality::High),
              static_cast<int>(LightmapQuality::Ultra));
}

TEST(LightmapBakerTest, AtlasMaxSize)
{
    LightmapBaker baker;

    LightmapConfig cfg;
    cfg.maxAtlasSize = 512;
    baker.SetConfig(cfg);
    EXPECT_EQ(baker.GetConfig().maxAtlasSize, 512u);

    // 最低値テスト
    cfg.maxAtlasSize = 10;
    baker.SetConfig(cfg);
    EXPECT_GE(baker.GetConfig().maxAtlasSize, 256u);
}
