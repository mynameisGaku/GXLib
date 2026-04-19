/// @file entity_bridge_test.cpp
/// @brief EntityBridge lifecycle tests (sprint-003 Task 5)
///
/// EntityBridge (`GXLib/ECS/EntityBridge.h`) uses process-global static
/// maps (`s_entityMap`, `s_reverseMap`) to bridge OOP Scene entities to
/// ECS World entities. This design imposes a single-World constraint
/// the ADR-0019 §10 documents:
///
///   > `EntityBridge::ClearMappings()` must be called when a Scene is
///   > destroyed; stale mappings cause dangling ECS entity references.
///
/// These tests exercise the documented lifecycle + verify the
/// cross-World collision behaviour the ADR flags.
#include <gtest/gtest.h>
#include "ECS/EntityBridge.h"
#include "ECS/World.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Entity.h"

namespace {

using gx::ecs::EntityBridge;
using gx::ecs::World;

class EntityBridgeTest : public ::testing::Test
{
protected:
    void SetUp() override { EntityBridge::ClearMappings(); }
    void TearDown() override { EntityBridge::ClearMappings(); }
};

TEST_F(EntityBridgeTest, ClearMappingsResetsCount)
{
    // Arrange
    gx::Scene scene("clear_test");
    auto* e = scene.CreateEntity("E");
    (void)e;
    EXPECT_EQ(EntityBridge::GetImportedCount(), 0u);

    // Act
    World world;
    EntityBridge::ImportEntity(world, *e);
    EXPECT_EQ(EntityBridge::GetImportedCount(), 1u);

    // Assert
    EntityBridge::ClearMappings();
    EXPECT_EQ(EntityBridge::GetImportedCount(), 0u);
}

TEST_F(EntityBridgeTest, ImportEntityCreatesECSEntity)
{
    // Arrange
    gx::Scene scene("import_test");
    auto* e = scene.CreateEntity("Player");
    e->GetTransform().SetPosition(1.0f, 2.0f, 3.0f);

    World world;

    // Act
    auto ecsId = EntityBridge::ImportEntity(world, *e);

    // Assert — ECS entity was created, mapping established
    EXPECT_TRUE(world.IsAlive(ecsId));
    EXPECT_EQ(EntityBridge::GetImportedCount(), 1u);
}

TEST_F(EntityBridgeTest, SyncSceneToWorldImportsAllEntities)
{
    // Arrange
    gx::Scene scene("sync_test");
    scene.CreateEntity("A");
    scene.CreateEntity("B");
    scene.CreateEntity("C");
    World world;

    // Act
    EntityBridge::SyncSceneToWorld(world, scene);

    // Assert — 3 mappings live
    EXPECT_EQ(EntityBridge::GetImportedCount(), 3u);
}

TEST_F(EntityBridgeTest, CrossWorldMappingCollisionDocumented)
{
    // This test exercises + PINS the single-World constraint documented
    // in ADR-0019 §10 / forbidden pattern `entity_bridge_stale_mapping`.
    //
    // Actual behaviour (verified 2026-04-19 against EntityBridge.cpp:21-23):
    // ImportEntity has an early-return when the scene-entity-ID is
    // already in `s_entityMap` — it returns the first-import's ECS ID
    // WITHOUT creating a new entity in the second World. Result: the
    // returned ID is silently invalid in the second World.
    //
    // This is arguably worse than ADR-0019's "stale mappings" phrasing
    // suggests — the bridge silently hands back a dead handle. Fix path
    // is tracked as TR-defer-world-scoped-entity-bridge in the
    // architecture-traceability deferred list (future redesign).
    //
    // The test pins current behaviour so any future redesign fails this
    // test loudly and must either: (a) clear mappings automatically per
    // World, or (b) update the contract.

    // Arrange
    gx::Scene scene("collision_test");
    auto* e = scene.CreateEntity("Shared");

    World worldA;
    World worldB;

    // Act — import into World A
    auto idA = EntityBridge::ImportEntity(worldA, *e);
    ASSERT_EQ(EntityBridge::GetImportedCount(), 1u);
    ASSERT_TRUE(worldA.IsAlive(idA));

    // Act — import SAME scene entity into World B without clearing
    auto idB = EntityBridge::ImportEntity(worldB, *e);

    // Assert — CURRENT BEHAVIOUR (pinned):
    // 1. The second import reuses the first mapping; idB == idA.
    EXPECT_EQ(idA, idB);

    // 2. `worldA` still has the entity (first import succeeded).
    EXPECT_TRUE(worldA.IsAlive(idA));

    // 3. `worldB` does NOT have that ID (no entity was actually created
    //    there — the bridge's early-return skipped World::CreateEntity).
    EXPECT_FALSE(worldB.IsAlive(idB));

    // 4. GetImportedCount remains 1 (single mapping, not two).
    EXPECT_EQ(EntityBridge::GetImportedCount(), 1u);

    // Correct usage: ClearMappings() between Worlds, OR use a dedicated
    // World per scene. Any future World-scoped EntityBridge redesign
    // (TR-defer-world-scoped-entity-bridge) is expected to break this
    // test — at which point, update the assertions to match the new
    // contract.
}

TEST_F(EntityBridgeTest, ExportEntityUpdatesOOPFromECS)
{
    // Arrange
    gx::Scene scene("export_test");
    auto* e = scene.CreateEntity("Exporter");
    e->GetTransform().SetPosition(0.0f, 0.0f, 0.0f);

    World world;
    auto ecsId = EntityBridge::ImportEntity(world, *e);

    // Act — mutate the ECS-side BridgePosition
    auto* pos = world.GetComponent<gx::ecs::BridgePosition>(ecsId);
    ASSERT_NE(pos, nullptr);
    pos->x = 10.0f;
    pos->y = 20.0f;
    pos->z = 30.0f;

    // Act — sync back to OOP
    EntityBridge::ExportEntity(world, ecsId, *e);

    // Assert — OOP Entity transform reflects ECS-side changes
    const auto& t = e->GetTransform().GetPosition();
    EXPECT_FLOAT_EQ(t.x, 10.0f);
    EXPECT_FLOAT_EQ(t.y, 20.0f);
    EXPECT_FLOAT_EQ(t.z, 30.0f);
}

} // namespace
