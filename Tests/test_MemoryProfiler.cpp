/// @file test_MemoryProfiler.cpp
/// @brief MemoryProfiler 単体テスト — アロケーション追跡・カテゴリ・リーク検出

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/MemoryProfiler.h"

using namespace gx;

// =============================================================================
// 基本テスト
// =============================================================================

TEST(MemoryProfilerTest, ConstructDestruct)
{
    MemoryProfiler profiler;
    EXPECT_FALSE(profiler.IsEnabled());
    EXPECT_EQ(profiler.GetTotalAllocated(), 0u);
    EXPECT_EQ(profiler.GetCurrentUsage(), 0u);
}

TEST(MemoryProfilerTest, InitializeShutdown)
{
    MemoryProfiler profiler;

    profiler.Initialize();
    EXPECT_TRUE(profiler.IsEnabled());

    profiler.Shutdown();
    EXPECT_FALSE(profiler.IsEnabled());
    EXPECT_EQ(profiler.GetTotalAllocated(), 0u);
}

// =============================================================================
// アロケーション記録テスト
// =============================================================================

TEST(MemoryProfilerTest, RecordAllocation)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int dummy1 = 0;
    int dummy2 = 0;
    profiler.RecordAllocation(&dummy1, 1024, "Textures");
    profiler.RecordAllocation(&dummy2, 512, "Audio");

    EXPECT_EQ(profiler.GetTotalAllocated(), 1536u);
    EXPECT_EQ(profiler.GetCurrentUsage(), 1536u);
    EXPECT_EQ(profiler.GetAllocationCount(), 2u);

    profiler.Shutdown();
}

TEST(MemoryProfilerTest, RecordFree)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int dummy = 0;
    profiler.RecordAllocation(&dummy, 1024, "Default");
    EXPECT_EQ(profiler.GetCurrentUsage(), 1024u);

    profiler.RecordFree(&dummy);
    EXPECT_EQ(profiler.GetCurrentUsage(), 0u);
    EXPECT_EQ(profiler.GetTotalFreed(), 1024u);

    profiler.Shutdown();
}

// =============================================================================
// スナップショットテスト
// =============================================================================

TEST(MemoryProfilerTest, CurrentSnapshot)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int dummy = 0;
    profiler.RecordAllocation(&dummy, 256, "Meshes");

    auto snapshot = profiler.GetCurrentSnapshot();
    EXPECT_EQ(snapshot.totalAllocated, 256u);
    EXPECT_EQ(snapshot.currentUsage, 256u);
    EXPECT_EQ(snapshot.allocationCount, 1u);

    profiler.Shutdown();
}

TEST(MemoryProfilerTest, PeakSnapshot)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int dummy1 = 0;
    int dummy2 = 0;
    // Allocate 1024 + 512 = 1536 peak
    profiler.RecordAllocation(&dummy1, 1024, "Default");
    profiler.RecordAllocation(&dummy2, 512, "Default");

    // Free one — usage drops to 512, but peak stays at 1536
    profiler.RecordFree(&dummy1);
    EXPECT_EQ(profiler.GetCurrentUsage(), 512u);
    EXPECT_EQ(profiler.GetPeakUsage(), 1536u);

    auto peakSnap = profiler.GetPeakSnapshot();
    EXPECT_EQ(peakSnap.peakUsage, 1536u);

    profiler.Shutdown();
}

// =============================================================================
// カテゴリテスト
// =============================================================================

TEST(MemoryProfilerTest, CategoryUsage)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int d1 = 0, d2 = 0, d3 = 0;
    profiler.RecordAllocation(&d1, 100, "Textures");
    profiler.RecordAllocation(&d2, 200, "Audio");
    profiler.RecordAllocation(&d3, 300, "Textures");

    EXPECT_EQ(profiler.GetCategoryUsage("Textures"), 400u);
    EXPECT_EQ(profiler.GetCategoryUsage("Audio"), 200u);
    EXPECT_EQ(profiler.GetCategoryUsage("Missing"), 0u);

    profiler.Shutdown();
}

TEST(MemoryProfilerTest, GetCategories)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int d1 = 0, d2 = 0;
    profiler.RecordAllocation(&d1, 100, "Meshes");
    profiler.RecordAllocation(&d2, 200, "Audio");

    auto categories = profiler.GetCategories();
    ASSERT_EQ(categories.size(), 2u);
    // GetCategories returns sorted
    EXPECT_EQ(categories[0], "Audio");
    EXPECT_EQ(categories[1], "Meshes");

    profiler.Shutdown();
}

// =============================================================================
// リーク検出テスト
// =============================================================================

TEST(MemoryProfilerTest, LeakedAllocations)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int d1 = 0, d2 = 0;
    profiler.RecordAllocation(&d1, 100, "A");
    profiler.RecordAllocation(&d2, 200, "B");
    profiler.RecordFree(&d1);

    auto leaks = profiler.GetLeakedAllocations();
    ASSERT_EQ(leaks.size(), 1u);
    EXPECT_EQ(leaks[0].address, &d2);
    EXPECT_EQ(leaks[0].size, 200u);

    profiler.Shutdown();
}

TEST(MemoryProfilerTest, HasLeaks)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    EXPECT_FALSE(profiler.HasLeaks());

    int dummy = 0;
    profiler.RecordAllocation(&dummy, 64, "Default");
    EXPECT_TRUE(profiler.HasLeaks());

    profiler.RecordFree(&dummy);
    EXPECT_FALSE(profiler.HasLeaks());

    profiler.Shutdown();
}

// =============================================================================
// 統計テスト
// =============================================================================

TEST(MemoryProfilerTest, TotalStats)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int d1 = 0, d2 = 0;
    profiler.RecordAllocation(&d1, 1000, "Default");
    profiler.RecordAllocation(&d2, 2000, "Default");
    profiler.RecordFree(&d1);

    EXPECT_EQ(profiler.GetTotalAllocated(), 3000u);
    EXPECT_EQ(profiler.GetTotalFreed(), 1000u);
    EXPECT_EQ(profiler.GetCurrentUsage(), 2000u);
    EXPECT_EQ(profiler.GetPeakUsage(), 3000u);
    EXPECT_EQ(profiler.GetAllocationCount(), 2u);

    profiler.Shutdown();
}

// =============================================================================
// リセットテスト
// =============================================================================

TEST(MemoryProfilerTest, Reset)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    int dummy = 0;
    profiler.RecordAllocation(&dummy, 512, "Default");

    profiler.Reset();
    EXPECT_EQ(profiler.GetTotalAllocated(), 0u);
    EXPECT_EQ(profiler.GetCurrentUsage(), 0u);
    EXPECT_FALSE(profiler.HasLeaks());

    profiler.Shutdown();
}

// =============================================================================
// 有効/無効テスト
// =============================================================================

TEST(MemoryProfilerTest, EnableDisable)
{
    MemoryProfiler profiler;
    // Not initialized — disabled by default
    EXPECT_FALSE(profiler.IsEnabled());

    int dummy = 0;
    profiler.RecordAllocation(&dummy, 256, "Default");
    // Should be ignored when disabled
    EXPECT_EQ(profiler.GetTotalAllocated(), 0u);

    profiler.SetEnabled(true);
    profiler.RecordAllocation(&dummy, 256, "Default");
    EXPECT_EQ(profiler.GetTotalAllocated(), 256u);
}

// =============================================================================
// フレーム管理テスト
// =============================================================================

TEST(MemoryProfilerTest, FrameBeginEnd)
{
    MemoryProfiler profiler;
    profiler.Initialize();

    profiler.BeginFrame(1);
    int d1 = 0;
    profiler.RecordAllocation(&d1, 128, "Frame");
    profiler.EndFrame();

    auto snapshot = profiler.GetCurrentSnapshot();
    EXPECT_EQ(snapshot.frameNumber, 1u);

    profiler.BeginFrame(2);
    int d2 = 0;
    profiler.RecordAllocation(&d2, 64, "Frame");
    profiler.EndFrame();

    snapshot = profiler.GetCurrentSnapshot();
    EXPECT_EQ(snapshot.frameNumber, 2u);
    EXPECT_EQ(snapshot.currentUsage, 192u);

    profiler.Shutdown();
}
