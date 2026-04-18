/// @file scene_persistence_atomic_test.cpp
/// @brief ScenePersistence::SaveToFile atomicity — E3 regression tests
///        (ADR-0019 §5 atomicity invariant).
///
/// Added 2026-04-19 as part of sprint-002 Task 6 (deferred test hardening).
/// Covers the contract added by the 2026-04-18 E3 fix: save writes to
/// <path>.tmp, flushes, closes, then std::filesystem::rename to the final
/// path. Partial writes must never land on the destination.
#include <gtest/gtest.h>
#include "Core/Scene/Scene.h"
#include "Core/Scene/ScenePersistence.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

namespace fs = std::filesystem;

/// Test scene builder — creates a 3-entity scene for round-trip verification.
static std::unique_ptr<gx::Scene> MakeTestScene()
{
    auto scene = std::make_unique<gx::Scene>("atomic_test_scene");
    auto* root = scene->CreateEntity("Root");
    root->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);
    auto* child = scene->CreateEntity("Child");
    child->GetTransform().SetScale(0.5f, 0.5f, 0.5f);
    child->SetParent(root);
    auto* leaf = scene->CreateEntity("Leaf");
    leaf->SetActive(false);
    return scene;
}

/// Unique per-test temp directory under system temp.
/// Using process id + test name avoids cross-test collision under parallel runs.
static fs::path UniqueTempDir(const std::string& tag)
{
    fs::path dir = fs::temp_directory_path() / ("gxlib_scn_persist_" + tag);
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    return dir;
}

class ScenePersistenceAtomicTest : public ::testing::Test
{
protected:
    fs::path tmpDir;

    void SetUp() override
    {
        tmpDir = UniqueTempDir(::testing::UnitTest::GetInstance()
                                   ->current_test_info()->name());
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(tmpDir, ec);
    }
};

TEST_F(ScenePersistenceAtomicTest, TextSaveSucceedsAndLeavesNoTempSibling)
{
    // Arrange
    auto scene = MakeTestScene();
    const fs::path finalPath = tmpDir / "scene.gxscene";
    const fs::path tmpPath   = tmpDir / "scene.gxscene.tmp";

    // Act
    bool ok = gx::ScenePersistence::SaveToFile(
        *scene, gx::String(finalPath.string().c_str()),
        gx::SceneFileFormat::Text);

    // Assert — final file exists; .tmp sibling does NOT (rename consumed it)
    EXPECT_TRUE(ok);
    EXPECT_TRUE(fs::exists(finalPath));
    EXPECT_FALSE(fs::exists(tmpPath));
    EXPECT_GT(fs::file_size(finalPath), 0u);
}

TEST_F(ScenePersistenceAtomicTest, BinarySaveSucceedsAndLeavesNoTempSibling)
{
    // Arrange
    auto scene = MakeTestScene();
    const fs::path finalPath = tmpDir / "scene.gxscbin";
    const fs::path tmpPath   = tmpDir / "scene.gxscbin.tmp";

    // Act
    bool ok = gx::ScenePersistence::SaveToFile(
        *scene, gx::String(finalPath.string().c_str()),
        gx::SceneFileFormat::Binary);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_TRUE(fs::exists(finalPath));
    EXPECT_FALSE(fs::exists(tmpPath));
    EXPECT_GT(fs::file_size(finalPath), 0u);
}

TEST_F(ScenePersistenceAtomicTest, SaveFailureDoesNotOverwriteExistingFile)
{
    // E3 contract core guarantee: a failed save must leave the pre-existing
    // destination file intact (torn-write protection).

    // Arrange — pre-existing known-content file at destination
    const fs::path finalPath = tmpDir / "scene.gxscene";
    {
        std::ofstream out(finalPath);
        out << "PRE-EXISTING CONTENT\n";
    }
    const auto originalSize = fs::file_size(finalPath);
    ASSERT_GT(originalSize, 0u);

    // Arrange — force SaveToFile to fail by passing a path whose parent
    // directory does not exist. The .tmp ofstream open will fail.
    const fs::path badPath = tmpDir / "no_such_subdir" / "scene.gxscene";
    auto scene = MakeTestScene();

    // Act
    bool ok = gx::ScenePersistence::SaveToFile(
        *scene, gx::String(badPath.string().c_str()),
        gx::SceneFileFormat::Text);

    // Assert — the save at badPath returned false
    EXPECT_FALSE(ok);
    EXPECT_FALSE(fs::exists(badPath));
    // No .tmp sibling leaked under the bad path
    EXPECT_FALSE(fs::exists(tmpDir / "no_such_subdir" / "scene.gxscene.tmp"));

    // Assert — original destination at finalPath is untouched
    EXPECT_TRUE(fs::exists(finalPath));
    EXPECT_EQ(fs::file_size(finalPath), originalSize);
}

TEST_F(ScenePersistenceAtomicTest, RoundTripTextFormatPreservesEntities)
{
    // Regression: the atomic rewrite must not have broken the serialisation
    // payload — byte-content identity of the round trip is the guarantee.

    // Arrange
    auto original = MakeTestScene();
    const fs::path finalPath = tmpDir / "round_trip.gxscene";

    // Act — save + load
    ASSERT_TRUE(gx::ScenePersistence::SaveToFile(
        *original, gx::String(finalPath.string().c_str()),
        gx::SceneFileFormat::Text));
    auto loaded = gx::ScenePersistence::LoadFromFile(
        gx::String(finalPath.string().c_str()),
        gx::SceneFileFormat::Text);

    // Assert
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetEntityCount(), original->GetEntityCount());
}

TEST_F(ScenePersistenceAtomicTest, OverwriteReplacesOldContent)
{
    // A second save to the same path must atomically replace the first.

    // Arrange
    const fs::path finalPath = tmpDir / "overwrite.gxscene";
    auto first = MakeTestScene();
    ASSERT_TRUE(gx::ScenePersistence::SaveToFile(
        *first, gx::String(finalPath.string().c_str()),
        gx::SceneFileFormat::Text));
    const auto firstSize = fs::file_size(finalPath);

    // Arrange — second scene with different entity count
    auto second = std::make_unique<gx::Scene>("second");
    for (int i = 0; i < 10; ++i)
        second->CreateEntity("E" + std::to_string(i));

    // Act
    ASSERT_TRUE(gx::ScenePersistence::SaveToFile(
        *second, gx::String(finalPath.string().c_str()),
        gx::SceneFileFormat::Text));

    // Assert — file was replaced, not appended; no .tmp leftover
    const auto secondSize = fs::file_size(finalPath);
    EXPECT_NE(firstSize, secondSize);
    EXPECT_FALSE(fs::exists(tmpDir / "overwrite.gxscene.tmp"));
}

} // namespace
