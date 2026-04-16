/// @file test_PhysicsMaterial.cpp
/// @brief Phase 3: PhysicsMaterial テスト

#include <gtest/gtest.h>
#include "Physics/PhysicsMaterial.h"

using namespace gx;

// ============================================================================
// PhysicsMaterialParams
// ============================================================================

TEST(PhysicsMaterialTest, DefaultValues)
{
    PhysicsMaterialParams p;
    EXPECT_FLOAT_EQ(p.friction, 0.5f);
    EXPECT_FLOAT_EQ(p.restitution, 0.3f);
    EXPECT_FLOAT_EQ(p.density, 1.0f);
    EXPECT_FLOAT_EQ(p.linearDamping, 0.05f);
    EXPECT_FLOAT_EQ(p.angularDamping, 0.05f);
}

TEST(PhysicsMaterialTest, PresetDefault)
{
    auto p = PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Default);
    EXPECT_FLOAT_EQ(p.friction, 0.5f);
    EXPECT_FLOAT_EQ(p.restitution, 0.3f);
}

TEST(PhysicsMaterialTest, PresetMetal)
{
    auto p = PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Metal);
    EXPECT_FLOAT_EQ(p.friction, 0.3f);
    EXPECT_FLOAT_EQ(p.density, 7.8f);
}

TEST(PhysicsMaterialTest, PresetRubber)
{
    auto p = PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Rubber);
    EXPECT_FLOAT_EQ(p.friction, 0.9f);
    EXPECT_FLOAT_EQ(p.restitution, 0.8f);
}

TEST(PhysicsMaterialTest, PresetIce)
{
    auto p = PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Ice);
    EXPECT_FLOAT_EQ(p.friction, 0.05f);
}

TEST(PhysicsMaterialTest, PresetWood)
{
    auto p = PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Wood);
    EXPECT_FLOAT_EQ(p.friction, 0.6f);
    EXPECT_FLOAT_EQ(p.density, 0.6f);
}

TEST(PhysicsMaterialTest, PresetConcrete)
{
    auto p = PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Concrete);
    EXPECT_FLOAT_EQ(p.friction, 0.7f);
}

TEST(PhysicsMaterialTest, PresetGlass)
{
    auto p = PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Glass);
    EXPECT_FLOAT_EQ(p.restitution, 0.5f);
}

TEST(PhysicsMaterialTest, PresetMud)
{
    auto p = PhysicsMaterialParams::FromPreset(PhysicsMaterialPreset::Mud);
    EXPECT_FLOAT_EQ(p.friction, 0.8f);
    EXPECT_FLOAT_EQ(p.linearDamping, 0.3f);
}

// ============================================================================
// PhysicsMaterialLibrary
// ============================================================================

class PhysicsMaterialLibraryTest : public ::testing::Test
{
protected:
    void SetUp() override { PhysicsMaterialLibrary::Instance().Clear(); }
    void TearDown() override { PhysicsMaterialLibrary::Instance().Clear(); }
};

TEST_F(PhysicsMaterialLibraryTest, RegisterAndFind)
{
    PhysicsMaterialParams params;
    params.friction = 0.42f;

    uint32_t id = PhysicsMaterialLibrary::Instance().Register("Custom", params);

    const auto* found = PhysicsMaterialLibrary::Instance().Find("Custom");
    ASSERT_NE(found, nullptr);
    EXPECT_FLOAT_EQ(found->friction, 0.42f);

    const auto* foundById = PhysicsMaterialLibrary::Instance().FindById(id);
    ASSERT_NE(foundById, nullptr);
    EXPECT_FLOAT_EQ(foundById->friction, 0.42f);
}

TEST_F(PhysicsMaterialLibraryTest, FindNonExistent)
{
    EXPECT_EQ(PhysicsMaterialLibrary::Instance().Find("NonExistent"), nullptr);
    EXPECT_EQ(PhysicsMaterialLibrary::Instance().FindById(999), nullptr);
}

TEST_F(PhysicsMaterialLibraryTest, RegisterDefaults)
{
    PhysicsMaterialLibrary::Instance().RegisterDefaults();

    EXPECT_NE(PhysicsMaterialLibrary::Instance().Find("Default"), nullptr);
    EXPECT_NE(PhysicsMaterialLibrary::Instance().Find("Metal"), nullptr);
    EXPECT_NE(PhysicsMaterialLibrary::Instance().Find("Wood"), nullptr);
    EXPECT_NE(PhysicsMaterialLibrary::Instance().Find("Rubber"), nullptr);
    EXPECT_NE(PhysicsMaterialLibrary::Instance().Find("Ice"), nullptr);
    EXPECT_NE(PhysicsMaterialLibrary::Instance().Find("Concrete"), nullptr);
    EXPECT_NE(PhysicsMaterialLibrary::Instance().Find("Glass"), nullptr);
    EXPECT_NE(PhysicsMaterialLibrary::Instance().Find("Mud"), nullptr);

    EXPECT_EQ(PhysicsMaterialLibrary::Instance().GetCount(), 8u);
}

TEST_F(PhysicsMaterialLibraryTest, UpdateExisting)
{
    PhysicsMaterialParams p1;
    p1.friction = 0.1f;
    uint32_t id1 = PhysicsMaterialLibrary::Instance().Register("Mat", p1);

    PhysicsMaterialParams p2;
    p2.friction = 0.9f;
    uint32_t id2 = PhysicsMaterialLibrary::Instance().Register("Mat", p2);

    EXPECT_EQ(id1, id2); // 同じID
    EXPECT_FLOAT_EQ(PhysicsMaterialLibrary::Instance().Find("Mat")->friction, 0.9f);
    EXPECT_EQ(PhysicsMaterialLibrary::Instance().GetCount(), 1u);
}

TEST_F(PhysicsMaterialLibraryTest, Clear)
{
    PhysicsMaterialLibrary::Instance().RegisterDefaults();
    EXPECT_GT(PhysicsMaterialLibrary::Instance().GetCount(), 0u);

    PhysicsMaterialLibrary::Instance().Clear();
    EXPECT_EQ(PhysicsMaterialLibrary::Instance().GetCount(), 0u);
}
