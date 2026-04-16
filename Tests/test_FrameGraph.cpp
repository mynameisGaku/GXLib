/// @file test_FrameGraph.cpp
/// @brief FrameGraph（レンダーグラフ）のテスト — リソース宣言、パス管理、コンパイル、実行

#include <gtest/gtest.h>
#include "Graphics/FrameGraph/FrameGraph.h"

using namespace gx;

// ============================================================================
// リソース宣言
// ============================================================================

TEST(FrameGraphTest, DeclareResource)
{
    FrameGraph fg;
    uint32_t r0 = fg.DeclareResource("Color", DXGI_FORMAT_R8G8B8A8_UNORM, 1920, 1080);
    uint32_t r1 = fg.DeclareResource("Depth", DXGI_FORMAT_D32_FLOAT, 1920, 1080);
    EXPECT_EQ(fg.GetResourceCount(), 2u);
    EXPECT_NE(r0, r1);
}

TEST(FrameGraphTest, DeclareBuffer)
{
    FrameGraph fg;
    uint32_t b = fg.DeclareBuffer("Constants", D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    EXPECT_EQ(fg.GetResourceCount(), 1u);
    EXPECT_EQ(b, 0u);
}

// ============================================================================
// パス管理
// ============================================================================

TEST(FrameGraphTest, AddPass)
{
    FrameGraph fg;
    uint32_t p0 = fg.AddPass("GBuffer", [](const PassContext&) {});
    uint32_t p1 = fg.AddPass("Lighting", [](const PassContext&) {});
    EXPECT_EQ(fg.GetPassCount(), 2u);
    EXPECT_NE(p0, p1);
}

TEST(FrameGraphTest, SetPassEnabled)
{
    FrameGraph fg;
    uint32_t r = fg.DeclareResource("RT", DXGI_FORMAT_R8G8B8A8_UNORM, 1920, 1080);

    int callCount = 0;
    uint32_t p = fg.AddPass("TestPass", [&](const PassContext&) { callCount++; });
    fg.PassWrites(p, r);

    fg.SetPassEnabled(p, false);
    EXPECT_TRUE(fg.Compile());

    // 無効化されたパスは、唯一のパスである場合、実行順序に含まれないはず
    // （実行順序は実装に依存するが、パスはスキップされるべき）
    const auto& order = fg.GetExecutionOrder();
    for (auto id : order) {
        EXPECT_NE(id, p);
    }
}

// ============================================================================
// コンパイル
// ============================================================================

TEST(FrameGraphTest, CompileSimpleChain)
{
    FrameGraph fg;
    uint32_t rt = fg.DeclareResource("Color", DXGI_FORMAT_R16G16B16A16_FLOAT, 1920, 1080,
                                      D3D12_RESOURCE_STATE_RENDER_TARGET);

    uint32_t passA = fg.AddPass("Writer", [](const PassContext&) {});
    fg.PassWrites(passA, rt, D3D12_RESOURCE_STATE_RENDER_TARGET);

    uint32_t passB = fg.AddPass("Reader", [](const PassContext&) {});
    fg.PassReads(passB, rt, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    EXPECT_TRUE(fg.Compile());
    EXPECT_TRUE(fg.IsCompiled());

    const auto& order = fg.GetExecutionOrder();
    EXPECT_EQ(order.size(), 2u);

    // WriterはReaderより前に来なければならない
    int writerIdx = -1, readerIdx = -1;
    for (int i = 0; i < static_cast<int>(order.size()); i++) {
        if (order[i] == passA) writerIdx = i;
        if (order[i] == passB) readerIdx = i;
    }
    EXPECT_GE(writerIdx, 0);
    EXPECT_GE(readerIdx, 0);
    EXPECT_LT(writerIdx, readerIdx);
}

TEST(FrameGraphTest, CompileBarrierGeneration)
{
    FrameGraph fg;
    uint32_t rt = fg.DeclareResource("Color", DXGI_FORMAT_R16G16B16A16_FLOAT, 1920, 1080,
                                      D3D12_RESOURCE_STATE_COMMON);

    uint32_t passA = fg.AddPass("Writer", [](const PassContext&) {});
    fg.PassWrites(passA, rt, D3D12_RESOURCE_STATE_RENDER_TARGET);

    uint32_t passB = fg.AddPass("Reader", [](const PassContext&) {});
    fg.PassReads(passB, rt, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    EXPECT_TRUE(fg.Compile());

    // バリアが存在することを確認 — ステート遷移に対して少なくとも1つのバリアが生成されるはず
    bool hasBarriers = false;
    for (uint32_t i = 0; i < static_cast<uint32_t>(fg.GetExecutionOrder().size()); i++) {
        const auto& barriers = fg.GetBarriersForPass(i);
        if (!barriers.empty()) {
            hasBarriers = true;
            break;
        }
    }
    EXPECT_TRUE(hasBarriers);
}

// ============================================================================
// リセットとクリア
// ============================================================================

TEST(FrameGraphTest, Reset)
{
    FrameGraph fg;
    uint32_t rt = fg.DeclareResource("Color", DXGI_FORMAT_R8G8B8A8_UNORM, 1920, 1080);
    fg.AddPass("Pass", [](const PassContext&) {});
    fg.Compile();
    EXPECT_TRUE(fg.IsCompiled());

    fg.Reset();
    EXPECT_FALSE(fg.IsCompiled());
    // リソースとパスはまだ残っているはず
    EXPECT_EQ(fg.GetResourceCount(), 1u);
    EXPECT_EQ(fg.GetPassCount(), 1u);
}

TEST(FrameGraphTest, Clear)
{
    FrameGraph fg;
    fg.DeclareResource("Color", DXGI_FORMAT_R8G8B8A8_UNORM, 1920, 1080);
    fg.AddPass("Pass", [](const PassContext&) {});
    fg.Compile();

    fg.Clear();
    EXPECT_EQ(fg.GetResourceCount(), 0u);
    EXPECT_EQ(fg.GetPassCount(), 0u);
    EXPECT_FALSE(fg.IsCompiled());
}

// ============================================================================
// 実行（GPU不要 — コールバック検証のみ）
// ============================================================================

TEST(FrameGraphTest, CompileProducesCorrectExecutionOrder)
{
    FrameGraph fg;
    uint32_t rt = fg.DeclareResource("Color", DXGI_FORMAT_R16G16B16A16_FLOAT, 1920, 1080,
                                      D3D12_RESOURCE_STATE_COMMON);

    uint32_t passA = fg.AddPass("Writer", [](const PassContext&) {});
    fg.PassWrites(passA, rt, D3D12_RESOURCE_STATE_RENDER_TARGET);

    uint32_t passB = fg.AddPass("Reader", [](const PassContext&) {});
    fg.PassReads(passB, rt, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    EXPECT_TRUE(fg.Compile());

    // 実行順序はWriterがReaderより前であるべき
    const auto& order = fg.GetExecutionOrder();
    EXPECT_EQ(order.size(), 2u);

    // 位置を検索
    int writerPos = -1, readerPos = -1;
    for (int i = 0; i < static_cast<int>(order.size()); i++) {
        if (order[i] == passA) writerPos = i;
        if (order[i] == passB) readerPos = i;
    }
    EXPECT_GE(writerPos, 0);
    EXPECT_GE(readerPos, 0);
    EXPECT_LT(writerPos, readerPos);

    // ステート遷移に対してバリアが存在するはず
    EXPECT_GT(fg.GetBarriersForPass(0).size() + fg.GetBarriersForPass(1).size(), 0u);
}
