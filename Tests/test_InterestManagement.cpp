/// @file test_InterestManagement.cpp
#include <gtest/gtest.h>
#include "IO/Network/InterestManagement.h"
using namespace gx;

TEST(InterestManagementTest, DefaultState) {
    InterestManagement im;
    im.Initialize();
    EXPECT_EQ(im.GetEntityCount(), 0u);
}

TEST(InterestManagementTest, ConfigDefaults) {
    InterestManagement im;
    im.Initialize();
    EXPECT_FLOAT_EQ(im.GetConfig().aoiRadius, 100.0f);
    EXPECT_FLOAT_EQ(im.GetConfig().gridCellSize, 50.0f);
}

TEST(InterestManagementTest, RegisterEntity) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    EXPECT_EQ(im.GetEntityCount(), 1u);
}

TEST(InterestManagementTest, RemoveEntity) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    im.RemoveEntity(1);
    EXPECT_EQ(im.GetEntityCount(), 0u);
}

TEST(InterestManagementTest, QueryNearby) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    im.RegisterEntity(2, 10, 0, 10);
    im.RegisterEntity(3, 200, 0, 200); // out of AOI
    auto relevant = im.QueryRelevantEntities(1);
    EXPECT_EQ(relevant.size(), 1u);
    EXPECT_EQ(relevant[0], 2u);
}

TEST(InterestManagementTest, QuerySelf) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    auto relevant = im.QueryRelevantEntities(1);
    EXPECT_EQ(relevant.size(), 0u); // Self excluded
}

TEST(InterestManagementTest, QueryNone) {
    InterestManagement im;
    im.Initialize();
    auto relevant = im.QueryRelevantEntities(999);
    EXPECT_TRUE(relevant.empty());
}

TEST(InterestManagementTest, UpdatePosition) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    im.RegisterEntity(2, 200, 0, 200);
    EXPECT_EQ(im.QueryRelevantEntities(1).size(), 0u);
    im.UpdateEntityPosition(2, 10, 0, 10);
    EXPECT_EQ(im.QueryRelevantEntities(1).size(), 1u);
}

TEST(InterestManagementTest, PriorityNear) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    im.RegisterEntity(2, 5, 0, 0); // Very close
    EXPECT_EQ(im.GetPriority(1, 2), InterestPriority::Near);
}

TEST(InterestManagementTest, PriorityMid) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    im.RegisterEntity(2, 50, 0, 0); // 50/100 = 0.5 (mid range)
    EXPECT_EQ(im.GetPriority(1, 2), InterestPriority::Mid);
}

TEST(InterestManagementTest, PriorityFar) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    im.RegisterEntity(2, 80, 0, 0); // 80/100 = 0.8 (far)
    EXPECT_EQ(im.GetPriority(1, 2), InterestPriority::Far);
}

TEST(InterestManagementTest, ShouldUpdateNear) {
    InterestManagement im;
    im.Initialize();
    EXPECT_TRUE(im.ShouldUpdate(InterestPriority::Near, 0));
    EXPECT_TRUE(im.ShouldUpdate(InterestPriority::Near, 1));
}

TEST(InterestManagementTest, ShouldUpdateFar) {
    InterestManagement im;
    im.Initialize();
    EXPECT_TRUE(im.ShouldUpdate(InterestPriority::Far, 0));
    EXPECT_FALSE(im.ShouldUpdate(InterestPriority::Far, 1));
    EXPECT_TRUE(im.ShouldUpdate(InterestPriority::Far, 10));
}

TEST(InterestManagementTest, Distance) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    im.RegisterEntity(2, 3, 4, 0);
    EXPECT_NEAR(im.GetDistance(1, 2), 5.0f, 0.01f);
}

TEST(InterestManagementTest, CellId) {
    InterestManagement im;
    im.Initialize();
    int64_t a = im.GetCellId(0, 0);
    int64_t b = im.GetCellId(100, 100);
    EXPECT_NE(a, b);
}

TEST(InterestManagementTest, Clear) {
    InterestManagement im;
    im.Initialize();
    im.RegisterEntity(1, 0, 0, 0);
    im.RegisterEntity(2, 10, 0, 10);
    im.Clear();
    EXPECT_EQ(im.GetEntityCount(), 0u);
}
