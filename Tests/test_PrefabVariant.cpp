/// @file test_PrefabVariant.cpp
/// @brief Prefab variant system unit tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/PrefabVariant.h"

using namespace gx;

// ============================================================================
// Prefab management
// ============================================================================

TEST(PrefabVariantTest, RegisterPrefab)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Enemy";
    data.entityData = { 0x01, 0x02, 0x03 };

    uint64_t id = sys.RegisterPrefab(data);
    EXPECT_GT(id, 0u);
}

TEST(PrefabVariantTest, UnregisterPrefab)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Enemy";
    uint64_t id = sys.RegisterPrefab(data);

    EXPECT_TRUE(sys.UnregisterPrefab(id));
    EXPECT_FALSE(sys.UnregisterPrefab(id));  // Already removed
}

TEST(PrefabVariantTest, GetPrefab)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Player";
    data.entityData = { 0xAA, 0xBB };
    uint64_t id = sys.RegisterPrefab(data);

    const PrefabData* result = sys.GetPrefab(id);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->name, "Player");
    EXPECT_EQ(result->entityData.size(), 2u);

    EXPECT_EQ(sys.GetPrefab(9999), nullptr);
}

TEST(PrefabVariantTest, PrefabCount)
{
    PrefabVariantSystem sys;
    EXPECT_EQ(sys.GetPrefabCount(), 0u);

    PrefabData d1; d1.name = "A";
    PrefabData d2; d2.name = "B";
    sys.RegisterPrefab(d1);
    sys.RegisterPrefab(d2);
    EXPECT_EQ(sys.GetPrefabCount(), 2u);
}

TEST(PrefabVariantTest, HasPrefab)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Test";
    uint64_t id = sys.RegisterPrefab(data);

    EXPECT_TRUE(sys.HasPrefab(id));
    EXPECT_FALSE(sys.HasPrefab(9999));
}

// ============================================================================
// Variant management
// ============================================================================

TEST(PrefabVariantTest, CreateVariant)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);

    uint64_t variantId = sys.CreateVariant(prefabId, "RedEnemy");
    EXPECT_GT(variantId, 0u);

    // Cannot create variant of non-existent prefab
    EXPECT_EQ(sys.CreateVariant(9999, "Bad"), 0u);
}

TEST(PrefabVariantTest, DeleteVariant)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    EXPECT_TRUE(sys.DeleteVariant(variantId));
    EXPECT_FALSE(sys.DeleteVariant(variantId));  // Already deleted
}

TEST(PrefabVariantTest, GetVariant)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "BlueFrog");

    const PrefabVariantData* result = sys.GetVariant(variantId);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->name, "BlueFrog");
    EXPECT_EQ(result->basePrefabId, prefabId);

    EXPECT_EQ(sys.GetVariant(9999), nullptr);
}

TEST(PrefabVariantTest, VariantCount)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);

    EXPECT_EQ(sys.GetVariantCount(), 0u);
    sys.CreateVariant(prefabId, "V1");
    sys.CreateVariant(prefabId, "V2");
    EXPECT_EQ(sys.GetVariantCount(), 2u);
}

TEST(PrefabVariantTest, HasVariant)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Var");

    EXPECT_TRUE(sys.HasVariant(variantId));
    EXPECT_FALSE(sys.HasVariant(9999));
}

TEST(PrefabVariantTest, GetVariantsOf)
{
    PrefabVariantSystem sys;
    PrefabData d1; d1.name = "Base1";
    PrefabData d2; d2.name = "Base2";
    uint64_t p1 = sys.RegisterPrefab(d1);
    uint64_t p2 = sys.RegisterPrefab(d2);

    sys.CreateVariant(p1, "V1A");
    sys.CreateVariant(p1, "V1B");
    sys.CreateVariant(p2, "V2A");

    auto variants1 = sys.GetVariantsOf(p1);
    auto variants2 = sys.GetVariantsOf(p2);
    EXPECT_EQ(variants1.size(), 2u);
    EXPECT_EQ(variants2.size(), 1u);
}

// ============================================================================
// Override management
// ============================================================================

TEST(PrefabVariantTest, AddPropertyOverride)
{
    PrefabVariantSystem sys;
    PrefabData data; data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    EXPECT_TRUE(sys.AddPropertyOverride(variantId, "Transform", "Position", "1,2,3"));
    EXPECT_FALSE(sys.AddPropertyOverride(9999, "Transform", "Position", "1,2,3"));
}

TEST(PrefabVariantTest, RemovePropertyOverride)
{
    PrefabVariantSystem sys;
    PrefabData data; data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    sys.AddPropertyOverride(variantId, "Transform", "Position", "1,2,3");
    EXPECT_TRUE(sys.RemovePropertyOverride(variantId, "Transform", "Position"));
    EXPECT_FALSE(sys.RemovePropertyOverride(variantId, "Transform", "Position"));  // Already removed
}

TEST(PrefabVariantTest, AddComponentOverride)
{
    PrefabVariantSystem sys;
    PrefabData data; data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    EXPECT_TRUE(sys.AddComponentOverride(variantId, "AudioSource", true));

    auto overrides = sys.GetOverrides(variantId);
    ASSERT_EQ(overrides.size(), 1u);
    EXPECT_EQ(overrides[0].componentName, "AudioSource");
    EXPECT_TRUE(overrides[0].isAdded);
}

TEST(PrefabVariantTest, IsOverridden)
{
    PrefabVariantSystem sys;
    PrefabData data; data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    sys.AddPropertyOverride(variantId, "Mesh", "Color", "red");
    EXPECT_TRUE(sys.IsOverridden(variantId, "Mesh", "Color"));
    EXPECT_FALSE(sys.IsOverridden(variantId, "Mesh", "Scale"));
    EXPECT_FALSE(sys.IsOverridden(9999, "Mesh", "Color"));
}

TEST(PrefabVariantTest, RevertOverride)
{
    PrefabVariantSystem sys;
    PrefabData data; data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    sys.AddPropertyOverride(variantId, "Light", "Intensity", "5.0");
    EXPECT_TRUE(sys.RevertOverride(variantId, "Light", "Intensity"));
    EXPECT_FALSE(sys.IsOverridden(variantId, "Light", "Intensity"));
}

TEST(PrefabVariantTest, RevertAllOverrides)
{
    PrefabVariantSystem sys;
    PrefabData data; data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    sys.AddPropertyOverride(variantId, "Transform", "Position", "1,2,3");
    sys.AddPropertyOverride(variantId, "Mesh", "Color", "blue");
    sys.AddComponentOverride(variantId, "Script", true);
    sys.RevertAllOverrides(variantId);

    EXPECT_EQ(sys.GetOverrideCount(variantId), 0u);
}

TEST(PrefabVariantTest, GetOverrides)
{
    PrefabVariantSystem sys;
    PrefabData data; data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    sys.AddPropertyOverride(variantId, "Transform", "Position", "1,2,3");
    sys.AddPropertyOverride(variantId, "Transform", "Scale", "2,2,2");

    auto overrides = sys.GetOverrides(variantId);
    EXPECT_GE(overrides.size(), 1u);

    // Empty for non-existent variant
    auto empty = sys.GetOverrides(9999);
    EXPECT_TRUE(empty.empty());
}

TEST(PrefabVariantTest, OverrideCount)
{
    PrefabVariantSystem sys;
    PrefabData data; data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    EXPECT_EQ(sys.GetOverrideCount(variantId), 0u);
    sys.AddPropertyOverride(variantId, "Transform", "Position", "1,2,3");
    EXPECT_EQ(sys.GetOverrideCount(variantId), 1u);
    sys.AddPropertyOverride(variantId, "Transform", "Scale", "2,2,2");
    EXPECT_EQ(sys.GetOverrideCount(variantId), 2u);
}

// ============================================================================
// Apply overrides
// ============================================================================

TEST(PrefabVariantTest, ApplyOverrides)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    data.entityData = { 0x10, 0x20, 0x30 };
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "Variant");

    sys.AddPropertyOverride(variantId, "Transform", "Position", "5,5,5");

    auto result = sys.ApplyOverrides(variantId, data.entityData);
    // Result should be larger than original (contains override data)
    EXPECT_GT(result.size(), data.entityData.size());
    // First bytes should match original data
    EXPECT_EQ(result[0], 0x10);
    EXPECT_EQ(result[1], 0x20);
    EXPECT_EQ(result[2], 0x30);
}

// ============================================================================
// Nested variants
// ============================================================================

TEST(PrefabVariantTest, NestedVariant)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t v1 = sys.CreateVariant(prefabId, "Variant1");
    uint64_t v2 = sys.CreateNestedVariant(v1, "NestedVariant");

    EXPECT_GT(v2, 0u);
    const PrefabVariantData* nested = sys.GetVariant(v2);
    ASSERT_NE(nested, nullptr);
    EXPECT_EQ(nested->basePrefabId, v1);

    // Cannot nest from non-existent variant
    EXPECT_EQ(sys.CreateNestedVariant(9999, "Bad"), 0u);
}

TEST(PrefabVariantTest, InheritanceChain)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t v1 = sys.CreateVariant(prefabId, "Level1");
    uint64_t v2 = sys.CreateNestedVariant(v1, "Level2");

    auto chain = sys.GetInheritanceChain(v2);
    ASSERT_EQ(chain.size(), 3u);
    EXPECT_EQ(chain[0], prefabId);  // Root prefab
    EXPECT_EQ(chain[1], v1);        // First variant
    EXPECT_EQ(chain[2], v2);        // Nested variant
}

// ============================================================================
// Serialization
// ============================================================================

TEST(PrefabVariantTest, SaveLoad)
{
    const gx::String testPath = "test_prefab_variant.gxpv";

    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "SaveTest";
    data.entityData = { 0xDE, 0xAD };
    uint64_t prefabId = sys.RegisterPrefab(data);
    uint64_t variantId = sys.CreateVariant(prefabId, "SavedVariant");
    sys.AddPropertyOverride(variantId, "Transform", "Pos", "10,20,30");

    EXPECT_TRUE(sys.Save(testPath));

    PrefabVariantSystem loaded;
    EXPECT_TRUE(loaded.Load(testPath));

    EXPECT_EQ(loaded.GetPrefabCount(), 1u);
    EXPECT_EQ(loaded.GetVariantCount(), 1u);

    const PrefabData* lp = loaded.GetPrefab(prefabId);
    ASSERT_NE(lp, nullptr);
    EXPECT_EQ(lp->name, "SaveTest");
    EXPECT_EQ(lp->entityData.size(), 2u);

    const PrefabVariantData* lv = loaded.GetVariant(variantId);
    ASSERT_NE(lv, nullptr);
    EXPECT_EQ(lv->name, "SavedVariant");
    EXPECT_TRUE(loaded.IsOverridden(variantId, "Transform", "Pos"));

    // Cleanup
    std::remove(testPath.c_str());
}

// ============================================================================
// Clear
// ============================================================================

TEST(PrefabVariantTest, ClearAll)
{
    PrefabVariantSystem sys;
    PrefabData data;
    data.name = "Base";
    uint64_t prefabId = sys.RegisterPrefab(data);
    sys.CreateVariant(prefabId, "V1");

    sys.Clear();
    EXPECT_EQ(sys.GetPrefabCount(), 0u);
    EXPECT_EQ(sys.GetVariantCount(), 0u);
}
