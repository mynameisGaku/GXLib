/// @file test_SoundBank.cpp
/// @brief SoundBankとSoundCueのテスト（オーディオデバイス不要）

#include "pch.h"
#include <gtest/gtest.h>
#include "Audio/SoundBank.h"
#include <set>

using namespace gx;

// ===========================================================================
// SoundBankテスト（ユニットレベル、ほとんどAudioManager不要）
// ===========================================================================

TEST(SoundBankTest, InitialState)
{
    SoundBank bank;
    EXPECT_EQ(bank.GetCueCount(), 0);
}

TEST(SoundBankTest, CreateCue_ReturnsIndex)
{
    SoundBank bank;
    int idx = bank.CreateCue("footstep");
    EXPECT_EQ(idx, 0);
    EXPECT_EQ(bank.GetCueCount(), 1);
}

TEST(SoundBankTest, CreateCue_MultipleReturnsSequentialIndices)
{
    SoundBank bank;
    int a = bank.CreateCue("step");
    int b = bank.CreateCue("jump");
    int c = bank.CreateCue("land");
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 1);
    EXPECT_EQ(c, 2);
    EXPECT_EQ(bank.GetCueCount(), 3);
}

TEST(SoundBankTest, AddEntry_IncreasesEntryCount)
{
    SoundBank bank;
    int cue = bank.CreateCue("test");

    SoundCueEntry entry;
    entry.handle = 42;
    entry.volumeMin = 0.8f;
    entry.volumeMax = 1.0f;
    bank.AddEntry(cue, entry);

    EXPECT_EQ(bank.GetCue(cue).entries.size(), 1u);
}

TEST(SoundBankTest, AddEntry_InvalidIndex)
{
    SoundBank bank;
    SoundCueEntry entry;
    entry.handle = 1;
    bank.AddEntry(-1, entry); // クラッシュしないこと
    bank.AddEntry(100, entry); // クラッシュしないこと
    EXPECT_EQ(bank.GetCueCount(), 0);
}

TEST(SoundBankTest, SetCooldown)
{
    SoundBank bank;
    int cue = bank.CreateCue("test");
    bank.SetCooldown(cue, 0.5f);
    EXPECT_FLOAT_EQ(bank.GetCue(cue).cooldown, 0.5f);
}

TEST(SoundBankTest, SetMaxConcurrent)
{
    SoundBank bank;
    int cue = bank.CreateCue("test");
    bank.SetMaxConcurrent(cue, 2);
    EXPECT_EQ(bank.GetCue(cue).maxConcurrent, 2);
}

TEST(SoundBankTest, FindCue_Found)
{
    SoundBank bank;
    bank.CreateCue("alpha");
    bank.CreateCue("beta");
    bank.CreateCue("gamma");

    EXPECT_EQ(bank.FindCue("beta"), 1);
}

TEST(SoundBankTest, FindCue_NotFound)
{
    SoundBank bank;
    bank.CreateCue("alpha");
    EXPECT_EQ(bank.FindCue("delta"), -1);
}

TEST(SoundBankTest, Clear_RemovesAll)
{
    SoundBank bank;
    bank.CreateCue("a");
    bank.CreateCue("b");
    bank.Clear();
    EXPECT_EQ(bank.GetCueCount(), 0);
}

TEST(SoundBankTest, GetCue_ReturnsCorrectData)
{
    SoundBank bank;
    int idx = bank.CreateCue("explosion", SoundCueMode::Sequential);

    const SoundCue& cue = bank.GetCue(idx);
    EXPECT_EQ(cue.name, "explosion");
    EXPECT_EQ(cue.mode, SoundCueMode::Sequential);
}

TEST(SoundBankTest, CreateCue_DefaultModeIsRandom)
{
    SoundBank bank;
    int idx = bank.CreateCue("test");
    EXPECT_EQ(bank.GetCue(idx).mode, SoundCueMode::Random);
}

TEST(SoundBankTest, CreateCue_ShuffleMode)
{
    SoundBank bank;
    int idx = bank.CreateCue("test", SoundCueMode::Shuffle);
    EXPECT_EQ(bank.GetCue(idx).mode, SoundCueMode::Shuffle);
}

TEST(SoundBankTest, SetCooldown_InvalidIndex)
{
    SoundBank bank;
    bank.SetCooldown(-1, 1.0f); // クラッシュしないこと
    bank.SetCooldown(100, 1.0f); // クラッシュしないこと
}

TEST(SoundBankTest, SetMaxConcurrent_InvalidIndex)
{
    SoundBank bank;
    bank.SetMaxConcurrent(-1, 1); // クラッシュしないこと
    bank.SetMaxConcurrent(100, 1); // クラッシュしないこと
}

TEST(SoundBankTest, AddMultipleEntries)
{
    SoundBank bank;
    int cue = bank.CreateCue("variety");

    for (int i = 0; i < 5; ++i)
    {
        SoundCueEntry entry;
        entry.handle = i;
        entry.weight = 1.0f;
        bank.AddEntry(cue, entry);
    }

    EXPECT_EQ(bank.GetCue(cue).entries.size(), 5u);
}

TEST(SoundBankTest, Update_DoesNotCrash)
{
    SoundBank bank;
    bank.CreateCue("test");
    bank.Update(0.016f);
    // クラッシュしないことを確認
    EXPECT_EQ(bank.GetCueCount(), 1);
}

TEST(SoundBankTest, EntryVolumeRange)
{
    SoundBank bank;
    int cue = bank.CreateCue("test");

    SoundCueEntry entry;
    entry.handle = 0;
    entry.volumeMin = 0.5f;
    entry.volumeMax = 0.9f;
    entry.pitchMin = 0.8f;
    entry.pitchMax = 1.2f;
    bank.AddEntry(cue, entry);

    const auto& e = bank.GetCue(cue).entries[0];
    EXPECT_FLOAT_EQ(e.volumeMin, 0.5f);
    EXPECT_FLOAT_EQ(e.volumeMax, 0.9f);
    EXPECT_FLOAT_EQ(e.pitchMin, 0.8f);
    EXPECT_FLOAT_EQ(e.pitchMax, 1.2f);
}

TEST(SoundBankTest, DefaultCueState)
{
    SoundBank bank;
    int idx = bank.CreateCue("test");
    const SoundCue& cue = bank.GetCue(idx);
    EXPECT_EQ(cue.sequentialIndex, 0);
    EXPECT_EQ(cue.shuffleIndex, 0);
    EXPECT_EQ(cue.currentConcurrent, 0);
    EXPECT_LT(cue.lastPlayTime, 0.0f); // -1000で初期化
}

TEST(SoundBankTest, FindCue_EmptyBank)
{
    SoundBank bank;
    EXPECT_EQ(bank.FindCue("anything"), -1);
}
