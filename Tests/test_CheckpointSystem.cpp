/// @file test_CheckpointSystem.cpp
/// @brief CheckpointSystemのテスト

#include <gtest/gtest.h>
#include "Core/CheckpointSystem.h"

using namespace gx;

TEST(CheckpointSystemTest, DefaultState)
{
    CheckpointSystem cs;
    EXPECT_EQ(cs.GetCheckpointCount(), 0u);
    EXPECT_EQ(cs.GetMaxCheckpoints(), 20u);
    EXPECT_FALSE(cs.IsAutoOnSceneChange());
}

TEST(CheckpointSystemTest, CreateCheckpoint)
{
    CheckpointSystem cs;
    uint32_t id = cs.CreateCheckpoint("Start", "Level1");
    EXPECT_GT(id, 0u);
    EXPECT_EQ(cs.GetCheckpointCount(), 1u);
}

TEST(CheckpointSystemTest, GetCheckpoint)
{
    CheckpointSystem cs;
    uint32_t id = cs.CreateCheckpoint("Boss Room", "Level3");
    const CheckpointData* cp = cs.GetCheckpoint(id);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->name, "Boss Room");
    EXPECT_EQ(cp->locationName, "Level3");
    EXPECT_GT(cp->timestamp, 0u);
}

TEST(CheckpointSystemTest, GetCheckpointMissing)
{
    CheckpointSystem cs;
    EXPECT_EQ(cs.GetCheckpoint(999), nullptr);
}

TEST(CheckpointSystemTest, RemoveCheckpoint)
{
    CheckpointSystem cs;
    uint32_t id = cs.CreateCheckpoint("A");
    EXPECT_TRUE(cs.RemoveCheckpoint(id));
    EXPECT_EQ(cs.GetCheckpointCount(), 0u);
    EXPECT_FALSE(cs.RemoveCheckpoint(id)); // already removed
}

TEST(CheckpointSystemTest, MaxCheckpointEnforcement)
{
    CheckpointSystem cs(3);
    cs.CreateCheckpoint("CP1");
    cs.CreateCheckpoint("CP2");
    cs.CreateCheckpoint("CP3");
    cs.CreateCheckpoint("CP4"); // should evict CP1
    EXPECT_EQ(cs.GetCheckpointCount(), 3u);
    // Oldest (CP1) should be gone
    auto all = cs.GetAllCheckpoints();
    EXPECT_EQ(all[0].name, "CP2");
}

TEST(CheckpointSystemTest, Metadata)
{
    CheckpointSystem cs;
    uint32_t id = cs.CreateCheckpoint("Save");
    cs.SetMetadata(id, "health", "100");
    cs.SetMetadata(id, "score", "5000");
    EXPECT_EQ(cs.GetMetadata(id, "health"), "100");
    EXPECT_EQ(cs.GetMetadata(id, "score"), "5000");
    EXPECT_EQ(cs.GetMetadata(id, "missing"), "");
}

TEST(CheckpointSystemTest, QuickSave)
{
    CheckpointSystem cs;
    uint32_t id = cs.QuickSave("World1");
    EXPECT_GT(id, 0u);
    const CheckpointData* cp = cs.QuickLoad();
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->name, "QuickSave");
    EXPECT_EQ(cp->locationName, "World1");
}

TEST(CheckpointSystemTest, QuickLoadEmpty)
{
    CheckpointSystem cs;
    EXPECT_EQ(cs.QuickLoad(), nullptr);
}

TEST(CheckpointSystemTest, QuickSaveReplace)
{
    CheckpointSystem cs;
    cs.QuickSave("Area1");
    cs.QuickSave("Area2");
    // Only the latest quick save should remain as quick save
    const CheckpointData* cp = cs.QuickLoad();
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->locationName, "Area2");
}

TEST(CheckpointSystemTest, AutoOnSceneChange)
{
    CheckpointSystem cs;
    cs.SetAutoOnSceneChange(true);
    EXPECT_TRUE(cs.IsAutoOnSceneChange());
    cs.NotifySceneChange("Forest");
    EXPECT_EQ(cs.GetCheckpointCount(), 1u);
    auto all = cs.GetAllCheckpoints();
    EXPECT_EQ(all[0].name, "Auto_Forest");
    EXPECT_EQ(all[0].locationName, "Forest");
}

TEST(CheckpointSystemTest, AutoDisabledNoCreate)
{
    CheckpointSystem cs;
    cs.SetAutoOnSceneChange(false);
    cs.NotifySceneChange("Desert");
    EXPECT_EQ(cs.GetCheckpointCount(), 0u);
}

TEST(CheckpointSystemTest, ClearAll)
{
    CheckpointSystem cs;
    cs.CreateCheckpoint("A");
    cs.CreateCheckpoint("B");
    cs.QuickSave("C");
    cs.ClearAll();
    EXPECT_EQ(cs.GetCheckpointCount(), 0u);
    EXPECT_EQ(cs.QuickLoad(), nullptr);
}

TEST(CheckpointSystemTest, Callback)
{
    CheckpointSystem cs;
    std::string lastCreated;
    cs.SetOnCheckpointCreated([&](const CheckpointData& cp) {
        lastCreated = cp.name;
    });
    cs.CreateCheckpoint("TestCB");
    EXPECT_EQ(lastCreated, "TestCB");
}
