/// @file weather_map_test.cpp
/// @brief WeatherMap tests — init, cell access, region batch, JSON round-trip (ADR-0021 Phase B).
#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/WeatherMap.h"

#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;

namespace {

class WeatherMapTest : public ::testing::Test
{
protected:
    fs::path tmpDir;

    void SetUp() override
    {
        char tmpBuf[MAX_PATH];
        GetTempPathA(MAX_PATH, tmpBuf);
        tmpDir = fs::path(tmpBuf) / (std::string("gxlib_weathermap_") +
                 ::testing::UnitTest::GetInstance()->current_test_info()->name());
        std::error_code ec;
        fs::remove_all(tmpDir, ec);
        fs::create_directories(tmpDir);
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(tmpDir, ec);
    }
};

TEST_F(WeatherMapTest, InitializeSmallSizeSucceeds)
{
    gx::WeatherMap map;
    EXPECT_TRUE(map.Initialize(64, 64, nullptr));
    EXPECT_TRUE(map.IsInitialized());
    EXPECT_EQ(map.GetWidth(), 64u);
    EXPECT_EQ(map.GetHeight(), 64u);
    EXPECT_EQ(map.GetPixels().size(), 64u * 64u * 4u);
}

TEST_F(WeatherMapTest, InitializeZeroSizeFails)
{
    gx::WeatherMap map;
    EXPECT_FALSE(map.Initialize(0, 64, nullptr));
    EXPECT_FALSE(map.Initialize(64, 0, nullptr));
    EXPECT_FALSE(map.IsInitialized());
}

TEST_F(WeatherMapTest, DefaultInitReturnsClearCells)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(16, 16, nullptr));
    auto c = map.Sample(8, 8);
    EXPECT_EQ(c.coverage, 0);
    EXPECT_EQ(c.precipitation, 0);
    EXPECT_EQ(c.cloudType, 0);
    EXPECT_EQ(c.reserved, 255);
}

TEST_F(WeatherMapTest, SetCellIsReadableBack)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(16, 16, nullptr));
    gx::WeatherCell cell{ 200, 100, 50, 255 };
    map.SetCell(5, 7, cell);

    auto back = map.Sample(5, 7);
    EXPECT_EQ(back.coverage, 200);
    EXPECT_EQ(back.precipitation, 100);
    EXPECT_EQ(back.cloudType, 50);

    // Neighbor unchanged
    auto neighbor = map.Sample(5, 6);
    EXPECT_EQ(neighbor.coverage, 0);
}

TEST_F(WeatherMapTest, SetCellOutOfBoundsIgnored)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(8, 8, nullptr));
    // Should not crash
    map.SetCell(100, 100, gx::WeatherCell{ 255, 255, 255, 255 });
    // Sample at valid location should still be default
    EXPECT_EQ(map.Sample(4, 4).coverage, 0);
}

TEST_F(WeatherMapTest, FillSetsAllCells)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(8, 8, nullptr));
    gx::WeatherCell cell{ 50, 100, 150, 255 };
    map.Fill(cell);

    for (uint32_t y = 0; y < 8; ++y) {
        for (uint32_t x = 0; x < 8; ++x) {
            auto c = map.Sample(x, y);
            EXPECT_EQ(c.coverage, 50);
            EXPECT_EQ(c.precipitation, 100);
            EXPECT_EQ(c.cloudType, 150);
        }
    }
}

TEST_F(WeatherMapTest, SetRegionBatchSetsRectangle)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(10, 10, nullptr));
    gx::WeatherRegion region{ 2, 3, 4, 5 };  // x=2..6, y=3..8
    map.SetRegion(region, gx::WeatherCell{ 77, 0, 0, 255 });

    // Inside region
    EXPECT_EQ(map.Sample(2, 3).coverage, 77);
    EXPECT_EQ(map.Sample(5, 7).coverage, 77);
    // Outside region
    EXPECT_EQ(map.Sample(0, 0).coverage, 0);
    EXPECT_EQ(map.Sample(6, 3).coverage, 0);  // x=6 is end-exclusive
    EXPECT_EQ(map.Sample(2, 8).coverage, 0);  // y=8 is end-exclusive
}

TEST_F(WeatherMapTest, SetRegionClampsToBounds)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(8, 8, nullptr));
    // Region partially outside
    gx::WeatherRegion region{ 6, 6, 10, 10 };
    map.SetRegion(region, gx::WeatherCell{ 99, 0, 0, 255 });

    EXPECT_EQ(map.Sample(7, 7).coverage, 99);
    EXPECT_EQ(map.Sample(6, 6).coverage, 99);
}

TEST_F(WeatherMapTest, SampleWorldWrapsAround)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(8, 8, nullptr));
    map.SetWorldExtent(10000.0f);  // 10km square
    map.Fill(gx::WeatherCell{ 128, 0, 64, 255 });

    // Sample far outside bounds — should wrap (not crash, not return default)
    auto c1 = map.SampleWorld(50000.0f, 50000.0f);
    EXPECT_EQ(c1.coverage, 128);

    auto c2 = map.SampleWorld(-100000.0f, -100000.0f);
    EXPECT_EQ(c2.coverage, 128);
}

TEST_F(WeatherMapTest, DirtyFlagTracksMutation)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(8, 8, nullptr));
    // Initialize sets dirty (first-time)
    EXPECT_TRUE(map.IsDirty());

    // Without TextureManager, UploadToGPU fails but should not crash
    EXPECT_FALSE(map.UploadToGPU());
    // Still dirty (upload didn't succeed)
    EXPECT_TRUE(map.IsDirty());

    map.SetCell(0, 0, gx::WeatherCell{ 1, 2, 3, 255 });
    EXPECT_TRUE(map.IsDirty());
}

TEST_F(WeatherMapTest, JSONRoundTripPreservesFilledMap)
{
    gx::WeatherMap original;
    ASSERT_TRUE(original.Initialize(16, 16, nullptr));
    original.SetWorldExtent(12345.0f);
    original.Fill(gx::WeatherCell{ 50, 60, 70, 255 });
    // A handful of differing regions
    original.SetRegion({ 3, 3, 5, 5 }, gx::WeatherCell{ 200, 100, 30, 255 });
    original.SetCell(10, 10, gx::WeatherCell{ 220, 80, 160, 255 });

    const auto path = gx::String((tmpDir / "round_trip.json").string().c_str());
    ASSERT_TRUE(original.SaveToJSON(path));

    // File exists, no .tmp sibling leftover
    EXPECT_TRUE(fs::exists(tmpDir / "round_trip.json"));
    EXPECT_FALSE(fs::exists(tmpDir / "round_trip.json.tmp"));

    gx::WeatherMap loaded;
    ASSERT_TRUE(loaded.LoadFromJSON(path));
    EXPECT_EQ(loaded.GetWidth(), 16u);
    EXPECT_EQ(loaded.GetHeight(), 16u);
    EXPECT_FLOAT_EQ(loaded.GetWorldExtent(), 12345.0f);

    // Region-set cells match
    EXPECT_EQ(loaded.Sample(4, 4).coverage, 200);
    EXPECT_EQ(loaded.Sample(10, 10).coverage, 220);
    EXPECT_EQ(loaded.Sample(10, 10).cloudType, 160);

    // Fill background
    EXPECT_EQ(loaded.Sample(15, 15).coverage, 50);
}

TEST_F(WeatherMapTest, SaveToJSONFailureDoesNotOverwriteDestination)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(8, 8, nullptr));
    map.Fill(gx::WeatherCell{ 42, 0, 0, 255 });

    // Write a known-good file first
    const fs::path good = tmpDir / "good.json";
    std::ofstream(good) << "PRE-EXISTING";

    // Bad target: parent dir doesn't exist
    const fs::path badPath = tmpDir / "nosubdir" / "bad.json";
    const auto badStr = gx::String(badPath.string().c_str());

    EXPECT_FALSE(map.SaveToJSON(badStr));
    EXPECT_FALSE(fs::exists(badPath));
    EXPECT_FALSE(fs::exists(tmpDir / "nosubdir" / "bad.json.tmp"));

    // Good file untouched (atomic invariant guarantees destination untouched on failure)
    ASSERT_TRUE(fs::exists(good));
    std::ifstream f(good);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "PRE-EXISTING");
}

TEST_F(WeatherMapTest, ShutdownClearsState)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(8, 8, nullptr));
    map.Fill(gx::WeatherCell{ 10, 20, 30, 255 });

    map.Shutdown();
    EXPECT_FALSE(map.IsInitialized());
    EXPECT_EQ(map.GetWidth(), 0u);
    EXPECT_EQ(map.GetHeight(), 0u);
}

TEST_F(WeatherMapTest, ReinitializeAfterShutdown)
{
    gx::WeatherMap map;
    ASSERT_TRUE(map.Initialize(8, 8, nullptr));
    map.Shutdown();
    ASSERT_TRUE(map.Initialize(16, 16, nullptr));
    EXPECT_EQ(map.GetWidth(), 16u);
}

} // namespace
