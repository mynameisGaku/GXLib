/// @file test_CompatAudio.cpp
/// @brief Compat audio stub resolution tests (StopSoundMem, CheckSoundMem)
///
/// Tests validate that the AudioManager StopSound/CheckSoundPlaying APIs
/// work correctly without requiring actual audio hardware.

#include "pch.h"
#include <gtest/gtest.h>
#include "Audio/AudioManager.h"

using namespace gx;

// ============================================================================
// AudioManager::StopSound (backs StopSoundMem)
// ============================================================================

TEST(CompatAudioTest, StopSoundInvalidHandle)
{
    AudioManager mgr;
    // Should not crash with invalid handles
    EXPECT_NO_THROW(mgr.StopSound(-1));
    EXPECT_NO_THROW(mgr.StopSound(999));
}

TEST(CompatAudioTest, StopSoundBasic)
{
    AudioManager mgr;
    // Without initialization, StopSound should handle gracefully
    EXPECT_NO_THROW(mgr.StopSound(0));
}

// ============================================================================
// AudioManager::CheckSoundPlaying (backs CheckSoundMem)
// ============================================================================

TEST(CompatAudioTest, CheckSoundMemInvalidHandle)
{
    AudioManager mgr;
    // Invalid handle should return -1 (error)
    EXPECT_EQ(mgr.CheckSoundPlaying(-1), -1);
    EXPECT_EQ(mgr.CheckSoundPlaying(999), -1);
}

TEST(CompatAudioTest, CheckSoundMemNotPlaying)
{
    AudioManager mgr;
    // Without any sound loaded, even valid-range handles should report error
    EXPECT_EQ(mgr.CheckSoundPlaying(0), -1);
}

TEST(CompatAudioTest, CheckSoundMemReturnValues)
{
    AudioManager mgr;
    // Verify the return value contract:
    // -1 = error (invalid handle)
    // 0 = not playing
    // 1 = playing
    int result = mgr.CheckSoundPlaying(-1);
    EXPECT_EQ(result, -1);

    // Valid handle range but no sound loaded
    result = mgr.CheckSoundPlaying(5000);
    EXPECT_EQ(result, -1);
}

// ============================================================================
// SoundPlayer handle tracking
// ============================================================================

TEST(CompatAudioTest, SoundPlayerStopByHandle)
{
    SoundPlayer player;
    // Should not crash when no device is set
    EXPECT_NO_THROW(player.StopByHandle(-1));
    EXPECT_NO_THROW(player.StopByHandle(0));
    EXPECT_NO_THROW(player.StopByHandle(100));
}

TEST(CompatAudioTest, SoundPlayerIsPlayingByHandle)
{
    SoundPlayer player;
    // Without any voices, nothing should be playing
    EXPECT_FALSE(player.IsPlayingByHandle(-1));
    EXPECT_FALSE(player.IsPlayingByHandle(0));
    EXPECT_FALSE(player.IsPlayingByHandle(100));
}
