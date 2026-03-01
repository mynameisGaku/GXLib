/// @file test_AudioMixerUI.cpp
#include <gtest/gtest.h>
#include "Editor/AudioMixerUI.h"
using namespace gx;

TEST(AudioMixerUITest, Initialize) {
    AudioMixerUI ui;
    ui.Initialize(4);
    EXPECT_EQ(ui.GetBusCount(), 4u);
}
TEST(AudioMixerUITest, InitializeMinOne) {
    AudioMixerUI ui;
    ui.Initialize(0);
    EXPECT_GE(ui.GetBusCount(), 1u);
}
TEST(AudioMixerUITest, DefaultBusNames) {
    AudioMixerUI ui;
    ui.Initialize(4);
    EXPECT_EQ(ui.GetBusName(0), "Master");
    EXPECT_EQ(ui.GetBusName(1), "BGM");
    EXPECT_EQ(ui.GetBusName(2), "SE");
    EXPECT_EQ(ui.GetBusName(3), "Voice");
}
TEST(AudioMixerUITest, DefaultBusNameCustom) {
    AudioMixerUI ui;
    ui.Initialize(6);
    EXPECT_EQ(ui.GetBusName(4), "Bus_4");
}
TEST(AudioMixerUITest, SetVolume) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetVolume(0, 0.5f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).volume, 0.5f);
}
TEST(AudioMixerUITest, SetVolumeClamp) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetVolume(0, 3.0f);
    EXPECT_LE(ui.GetChannelStrip(0).volume, 2.0f);
    ui.SetVolume(0, -1.0f);
    EXPECT_GE(ui.GetChannelStrip(0).volume, 0.0f);
}
TEST(AudioMixerUITest, SetPan) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetPan(1, -0.5f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(1).pan, -0.5f);
}
TEST(AudioMixerUITest, SetPanClamp) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetPan(0, 2.0f);
    EXPECT_LE(ui.GetChannelStrip(0).pan, 1.0f);
    ui.SetPan(0, -2.0f);
    EXPECT_GE(ui.GetChannelStrip(0).pan, -1.0f);
}
TEST(AudioMixerUITest, SetMuted) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetMuted(1, true);
    EXPECT_TRUE(ui.GetChannelStrip(1).muted);
}
TEST(AudioMixerUITest, SetSoloed) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetSoloed(2, true);
    EXPECT_TRUE(ui.GetChannelStrip(2).soloed);
}
TEST(AudioMixerUITest, SoloCount) {
    AudioMixerUI ui;
    ui.Initialize(4);
    EXPECT_EQ(ui.GetSoloCount(), 0u);
    ui.SetSoloed(0, true);
    ui.SetSoloed(2, true);
    EXPECT_EQ(ui.GetSoloCount(), 2u);
}
TEST(AudioMixerUITest, EffectiveMutedNoSolo) {
    AudioMixerUI ui;
    ui.Initialize(4);
    EXPECT_FALSE(ui.GetEffectiveMuted(0));
    ui.SetMuted(1, true);
    EXPECT_TRUE(ui.GetEffectiveMuted(1));
}
TEST(AudioMixerUITest, EffectiveMutedWithSolo) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetSoloed(0, true);
    // Bus 0 is soloed -> not effectively muted
    EXPECT_FALSE(ui.GetEffectiveMuted(0));
    // Bus 1 is NOT soloed -> effectively muted when any bus is soloed
    EXPECT_TRUE(ui.GetEffectiveMuted(1));
}
TEST(AudioMixerUITest, VULevel) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetVULevel(0, 0.8f, 0.6f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).vuLeft, 0.8f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).vuRight, 0.6f);
}
TEST(AudioMixerUITest, VUPeak) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetVULevel(0, 0.5f, 0.3f);
    ui.SetVULevel(0, 0.8f, 0.6f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).peakLeft, 0.8f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).peakRight, 0.6f);
}
TEST(AudioMixerUITest, VUDecay) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetVULevel(0, 1.0f, 1.0f);
    ui.UpdateVUMeters(0.1f); // decay = 12.0 * 0.1 = 1.2
    EXPECT_LT(ui.GetChannelStrip(0).vuLeft, 1.0f);
    EXPECT_GE(ui.GetChannelStrip(0).vuLeft, 0.0f);
}
TEST(AudioMixerUITest, VUDecayToZero) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetVULevel(0, 0.5f, 0.5f);
    ui.UpdateVUMeters(10.0f); // large dt -> should clamp to 0
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).vuLeft, 0.0f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).vuRight, 0.0f);
}
TEST(AudioMixerUITest, AddRouting) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.AddRouting(1, 0, 0.5f);
    EXPECT_EQ(ui.GetRoutingCount(), 1u);
}
TEST(AudioMixerUITest, RemoveRouting) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.AddRouting(1, 0);
    ui.AddRouting(2, 0);
    ui.RemoveRouting(1, 0);
    EXPECT_EQ(ui.GetRoutingCount(), 1u);
}
TEST(AudioMixerUITest, SetBusName) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetBusName(1, "Music");
    EXPECT_EQ(ui.GetBusName(1), "Music");
}
TEST(AudioMixerUITest, GetBusNameOutOfRange) {
    AudioMixerUI ui;
    ui.Initialize(2);
    EXPECT_EQ(ui.GetBusName(99), "");
}
TEST(AudioMixerUITest, ResetToDefault) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.SetVolume(0, 0.5f);
    ui.SetPan(1, -0.8f);
    ui.SetMuted(2, true);
    ui.SetSoloed(3, true);
    ui.AddRouting(1, 0);
    ui.ResetToDefault();
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).volume, 1.0f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(1).pan, 0.0f);
    EXPECT_FALSE(ui.GetChannelStrip(2).muted);
    EXPECT_FALSE(ui.GetChannelStrip(3).soloed);
    EXPECT_EQ(ui.GetRoutingCount(), 0u);
    EXPECT_EQ(ui.GetBusName(0), "Master");
}
TEST(AudioMixerUITest, DefaultVolume) {
    AudioMixerUI ui;
    ui.Initialize(4);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).volume, 1.0f);
    EXPECT_FLOAT_EQ(ui.GetChannelStrip(0).pan, 0.0f);
}
TEST(AudioMixerUITest, SetVolumeOutOfRange) {
    AudioMixerUI ui;
    ui.Initialize(2);
    ui.SetVolume(99, 0.5f); // should not crash
    EXPECT_EQ(ui.GetBusCount(), 2u);
}
TEST(AudioMixerUITest, MultipleRoutings) {
    AudioMixerUI ui;
    ui.Initialize(4);
    ui.AddRouting(1, 0);
    ui.AddRouting(2, 0);
    ui.AddRouting(3, 0);
    EXPECT_EQ(ui.GetRoutingCount(), 3u);
}
TEST(AudioMixerUITest, EffectiveMutedOutOfRange) {
    AudioMixerUI ui;
    ui.Initialize(2);
    EXPECT_TRUE(ui.GetEffectiveMuted(99));
}
