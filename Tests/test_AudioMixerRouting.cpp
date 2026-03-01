/// @file test_AudioMixerRouting.cpp
/// @brief AudioMixerバスルーティング、ボリューム伝搬、ミュート動作のテスト
///
/// XAudio2初期化不要 -- AudioBusのデータ/ステートロジックのみを検証する

#include "pch.h"
#include <gtest/gtest.h>
#include "Audio/AudioBus.h"
#include "Audio/AudioMixer.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ============================================================================
// AudioBus Defaults (no Initialize() call -- data-only)
// ============================================================================

TEST(BusDefaults, DefaultVolume_IsOne)
{
    AudioBus bus;
    EXPECT_NEAR(bus.GetVolume(), 1.0f, kEps);
}

TEST(BusDefaults, DefaultMuted_IsFalse)
{
    AudioBus bus;
    EXPECT_FALSE(bus.IsMuted());
}

TEST(BusDefaults, DefaultEffectiveVolume_IsOne)
{
    AudioBus bus;
    EXPECT_NEAR(bus.GetEffectiveVolume(), 1.0f, kEps);
}

TEST(BusDefaults, DefaultName_IsEmpty)
{
    AudioBus bus;
    EXPECT_TRUE(bus.GetName().empty());
}

TEST(BusDefaults, DefaultSubmixVoice_IsNull)
{
    AudioBus bus;
    EXPECT_EQ(bus.GetSubmixVoice(), nullptr);
}

// ============================================================================
// Volume Propagation
// ============================================================================

TEST(VolumePropagation, SetVolume_StoresValue)
{
    AudioBus bus;
    bus.SetVolume(0.5f);
    EXPECT_NEAR(bus.GetVolume(), 0.5f, kEps);
}

TEST(VolumePropagation, SetVolume_Zero)
{
    AudioBus bus;
    bus.SetVolume(0.0f);
    EXPECT_NEAR(bus.GetVolume(), 0.0f, kEps);
}

TEST(VolumePropagation, SetVolume_Max)
{
    AudioBus bus;
    bus.SetVolume(1.0f);
    EXPECT_NEAR(bus.GetVolume(), 1.0f, kEps);
}

TEST(VolumePropagation, SetVolume_UpdatesEffective)
{
    AudioBus bus;
    bus.SetVolume(0.75f);
    EXPECT_NEAR(bus.GetEffectiveVolume(), 0.75f, kEps);
}

TEST(VolumePropagation, SetVolume_MultipleChanges)
{
    AudioBus bus;
    bus.SetVolume(0.3f);
    EXPECT_NEAR(bus.GetVolume(), 0.3f, kEps);
    bus.SetVolume(0.9f);
    EXPECT_NEAR(bus.GetVolume(), 0.9f, kEps);
}

// ============================================================================
// Mute State
// ============================================================================

TEST(MuteState, SetMuted_True)
{
    AudioBus bus;
    bus.SetMuted(true);
    EXPECT_TRUE(bus.IsMuted());
}

TEST(MuteState, SetMuted_EffectiveVolumeZero)
{
    AudioBus bus;
    bus.SetVolume(0.8f);
    bus.SetMuted(true);
    EXPECT_NEAR(bus.GetEffectiveVolume(), 0.0f, kEps);
}

TEST(MuteState, Unmute_RestoresVolume)
{
    AudioBus bus;
    bus.SetVolume(0.6f);
    bus.SetMuted(true);
    EXPECT_NEAR(bus.GetEffectiveVolume(), 0.0f, kEps);
    bus.SetMuted(false);
    EXPECT_NEAR(bus.GetEffectiveVolume(), 0.6f, kEps);
}

TEST(MuteState, Mute_DoesNotChangeStoredVolume)
{
    AudioBus bus;
    bus.SetVolume(0.4f);
    bus.SetMuted(true);
    EXPECT_NEAR(bus.GetVolume(), 0.4f, kEps);
}

TEST(MuteState, DefaultMuted_EffectiveEqualsVolume)
{
    AudioBus bus;
    bus.SetVolume(0.7f);
    EXPECT_NEAR(bus.GetEffectiveVolume(), bus.GetVolume(), kEps);
}

// ============================================================================
// Bus Independence
// ============================================================================

TEST(BusIndependence, ChangingSE_DoesNotAffectBGM)
{
    AudioBus seBus;
    AudioBus bgmBus;
    seBus.SetVolume(0.3f);
    bgmBus.SetVolume(0.8f);

    EXPECT_NEAR(seBus.GetVolume(), 0.3f, kEps);
    EXPECT_NEAR(bgmBus.GetVolume(), 0.8f, kEps);
}

TEST(BusIndependence, MutingSE_DoesNotAffectBGM)
{
    AudioBus seBus;
    AudioBus bgmBus;
    bgmBus.SetVolume(0.9f);
    seBus.SetMuted(true);

    EXPECT_TRUE(seBus.IsMuted());
    EXPECT_FALSE(bgmBus.IsMuted());
    EXPECT_NEAR(bgmBus.GetEffectiveVolume(), 0.9f, kEps);
}

TEST(BusIndependence, MultipleBuses_IndependentVolumes)
{
    AudioBus master, bgm, se, voice;
    master.SetVolume(1.0f);
    bgm.SetVolume(0.7f);
    se.SetVolume(0.5f);
    voice.SetVolume(0.3f);

    EXPECT_NEAR(master.GetVolume(), 1.0f, kEps);
    EXPECT_NEAR(bgm.GetVolume(), 0.7f, kEps);
    EXPECT_NEAR(se.GetVolume(), 0.5f, kEps);
    EXPECT_NEAR(voice.GetVolume(), 0.3f, kEps);
}

// ============================================================================
// AudioMixer GetBusByName (data-only, no XAudio2 initialization)
// ============================================================================

TEST(GetBusByName, Master_ReturnsMasterBus)
{
    AudioMixer mixer;
    AudioBus* bus = mixer.GetBusByName("Master");
    EXPECT_NE(bus, nullptr);
}

TEST(GetBusByName, BGM_ReturnsBGMBus)
{
    AudioMixer mixer;
    AudioBus* bus = mixer.GetBusByName("BGM");
    EXPECT_NE(bus, nullptr);
}

TEST(GetBusByName, SE_ReturnsSEBus)
{
    AudioMixer mixer;
    AudioBus* bus = mixer.GetBusByName("SE");
    EXPECT_NE(bus, nullptr);
}

TEST(GetBusByName, Voice_ReturnsVoiceBus)
{
    AudioMixer mixer;
    AudioBus* bus = mixer.GetBusByName("Voice");
    EXPECT_NE(bus, nullptr);
}

TEST(GetBusByName, Unknown_ReturnsNullptr)
{
    AudioMixer mixer;
    AudioBus* bus = mixer.GetBusByName("NonExistent");
    EXPECT_EQ(bus, nullptr);
}

TEST(GetBusByName, MasterAndBGM_AreDifferent)
{
    AudioMixer mixer;
    AudioBus* master = mixer.GetBusByName("Master");
    AudioBus* bgm    = mixer.GetBusByName("BGM");
    EXPECT_NE(master, bgm);
}

// ============================================================================
// Custom Bus
// ============================================================================

TEST(CustomBus, CreateBus_ReturnsNonNull)
{
    AudioMixer mixer;
    AudioBus* custom = mixer.CreateBus("Custom");
    EXPECT_NE(custom, nullptr);
}

TEST(CustomBus, CreateBus_DefaultVolume)
{
    AudioMixer mixer;
    AudioBus* custom = mixer.CreateBus("Ambient");
    ASSERT_NE(custom, nullptr);
    EXPECT_NEAR(custom->GetVolume(), 1.0f, kEps);
}

TEST(CustomBus, CreateMultiple_Distinct)
{
    AudioMixer mixer;
    AudioBus* a = mixer.CreateBus("A");
    AudioBus* b = mixer.CreateBus("B");
    EXPECT_NE(a, b);
}
