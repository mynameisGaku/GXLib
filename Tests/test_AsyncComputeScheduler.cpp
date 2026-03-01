/// @file test_AsyncComputeScheduler.cpp
#include <gtest/gtest.h>
#include "Graphics/Device/AsyncComputeScheduler.h"
using namespace gx;

TEST(AsyncComputeSchedulerTest, Initialize) {
    AsyncComputeScheduler acs;
    EXPECT_TRUE(acs.Initialize(nullptr));
    EXPECT_EQ(acs.GetScheduledTaskCount(), 0u);
    EXPECT_EQ(acs.GetFrameIndex(), 0u);
}

TEST(AsyncComputeSchedulerTest, SetTaskAsync) {
    AsyncComputeScheduler acs;
    acs.Initialize(nullptr);
    acs.SetTaskAsync(AsyncComputeTask::ParticleSimulation, true);
    EXPECT_TRUE(acs.IsTaskAsync(AsyncComputeTask::ParticleSimulation));
    EXPECT_FALSE(acs.IsTaskAsync(AsyncComputeTask::LightCulling));
    EXPECT_EQ(acs.GetAsyncEnabledCount(), 1u);
}

TEST(AsyncComputeSchedulerTest, ScheduleTask) {
    AsyncComputeScheduler acs;
    acs.Initialize(nullptr);
    acs.BeginFrame();
    acs.ScheduleTask(AsyncComputeTask::LightCulling);
    EXPECT_EQ(acs.GetScheduledTaskCount(), 1u);
}

TEST(AsyncComputeSchedulerTest, FrameLifecycle) {
    AsyncComputeScheduler acs;
    acs.Initialize(nullptr);
    acs.BeginFrame();
    acs.ScheduleTask(AsyncComputeTask::SSAOGenerate);
    acs.EndFrame();
    EXPECT_EQ(acs.GetFrameIndex(), 1u);
}

TEST(AsyncComputeSchedulerTest, TaskNames) {
    EXPECT_EQ(AsyncComputeScheduler::GetTaskName(AsyncComputeTask::ParticleSimulation), "ParticleSimulation");
    EXPECT_EQ(AsyncComputeScheduler::GetTaskName(AsyncComputeTask::HiZGenerate), "HiZGenerate");
}

TEST(AsyncComputeSchedulerTest, DisableAllAndReset) {
    AsyncComputeScheduler acs;
    acs.Initialize(nullptr);
    acs.SetTaskAsync(AsyncComputeTask::ParticleSimulation, true);
    acs.SetTaskAsync(AsyncComputeTask::LightCulling, true);
    EXPECT_EQ(acs.GetAsyncEnabledCount(), 2u);
    acs.DisableAllAsync();
    EXPECT_EQ(acs.GetAsyncEnabledCount(), 0u);
    acs.Reset();
    EXPECT_EQ(acs.GetFrameIndex(), 0u);
}
