/// @file test_AdvancedFeatures.cpp
/// @brief Phase 2: ローカライズ、タイムライン、プロファイラー、SpriteAnimatorのテスト

#include <gtest/gtest.h>
#include "Core/Localization.h"
#include "Core/Timeline.h"
#include "Core/Profiler.h"
#include "Graphics/Rendering/SpriteAnimator.h"
#include <filesystem>
#include <fstream>

using namespace gx;

// ============================================================================
// Localization
// ============================================================================

class LocalizationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Localization::Instance().Clear();

        // テスト用言語ファイルを作成
        {
            std::ofstream f("test_lang_en.txt");
            f << "# English\n";
            f << "greeting=Hello\n";
            f << "farewell=Goodbye\n";
            f << "welcome=Welcome, {0}!\n";
        }
        {
            std::ofstream f("test_lang_ja.txt");
            f << "# Japanese\n";
            f << "greeting=こんにちは\n";
            f << "farewell=さようなら\n";
        }
    }
    void TearDown() override
    {
        Localization::Instance().Clear();
        std::filesystem::remove("test_lang_en.txt");
        std::filesystem::remove("test_lang_ja.txt");
    }
};

TEST_F(LocalizationTest, LoadAndGetString)
{
    auto& loc = Localization::Instance();
    EXPECT_TRUE(loc.LoadLanguage("en", "test_lang_en.txt"));
    loc.SetLanguage("en");

    EXPECT_EQ(loc.GetString("greeting"), "Hello");
    EXPECT_EQ(loc.GetString("farewell"), "Goodbye");
}

TEST_F(LocalizationTest, LanguageSwitch)
{
    auto& loc = Localization::Instance();
    loc.LoadLanguage("en", "test_lang_en.txt");
    loc.LoadLanguage("ja", "test_lang_ja.txt");

    loc.SetLanguage("en");
    EXPECT_EQ(loc.GetString("greeting"), "Hello");

    loc.SetLanguage("ja");
    EXPECT_EQ(loc.GetString("greeting"), "こんにちは");
}

TEST_F(LocalizationTest, FallbackLanguage)
{
    auto& loc = Localization::Instance();
    loc.LoadLanguage("en", "test_lang_en.txt");
    loc.LoadLanguage("ja", "test_lang_ja.txt");

    loc.SetLanguage("ja");
    loc.SetFallbackLanguage("en");

    // "welcome"は英語にのみ存在
    EXPECT_EQ(loc.GetString("welcome"), "Welcome, {0}!");
}

TEST_F(LocalizationTest, MissingKeyReturnsKey)
{
    auto& loc = Localization::Instance();
    loc.LoadLanguage("en", "test_lang_en.txt");
    loc.SetLanguage("en");

    EXPECT_EQ(loc.GetString("nonexistent_key"), "nonexistent_key");
}

TEST_F(LocalizationTest, GetAvailableLanguages)
{
    auto& loc = Localization::Instance();
    loc.LoadLanguage("en", "test_lang_en.txt");
    loc.LoadLanguage("ja", "test_lang_ja.txt");

    auto langs = loc.GetAvailableLanguages();
    EXPECT_EQ(langs.size(), 2u);
}

TEST_F(LocalizationTest, GXLOCMacro)
{
    auto& loc = Localization::Instance();
    loc.LoadLanguage("en", "test_lang_en.txt");
    loc.SetLanguage("en");

    EXPECT_EQ(GX_LOC("greeting"), "Hello");
}

// ============================================================================
// Timeline
// ============================================================================

TEST(TimelineTest, Float3TrackInterpolation)
{
    Vector3 result = { 0, 0, 0 };

    auto track = std::make_unique<Float3Track>();
    track->AddKeyframe(0.0f, { 0, 0, 0 });
    track->AddKeyframe(1.0f, { 10, 20, 30 });
    track->SetTarget([&](const Vector3& v) { result = v; });

    Timeline timeline;
    timeline.AddTrack(std::move(track));
    timeline.Play();

    timeline.Update(0.5f);

    EXPECT_NEAR(result.x, 5.0f, 0.1f);
    EXPECT_NEAR(result.y, 10.0f, 0.1f);
    EXPECT_NEAR(result.z, 15.0f, 0.1f);
}

TEST(TimelineTest, FloatTrackInterpolation)
{
    float result = 0.0f;

    auto track = std::make_unique<FloatTrack>();
    track->AddKeyframe(0.0f, 0.0f);
    track->AddKeyframe(2.0f, 100.0f);
    track->SetTarget([&](float v) { result = v; });

    Timeline timeline;
    timeline.AddTrack(std::move(track));
    timeline.Play();

    timeline.Update(1.0f);
    EXPECT_NEAR(result, 50.0f, 0.1f);

    timeline.Update(1.0f);
    EXPECT_NEAR(result, 100.0f, 0.1f);
}

TEST(TimelineTest, EventTrack)
{
    int eventCount = 0;

    auto track = std::make_unique<EventTrack>();
    track->AddEvent(0.5f, [&]() { eventCount++; });
    track->AddEvent(1.0f, [&]() { eventCount += 10; });

    Timeline timeline;
    timeline.AddTrack(std::move(track));
    timeline.Play();

    timeline.Update(0.3f);
    EXPECT_EQ(eventCount, 0);

    timeline.Update(0.3f);
    EXPECT_EQ(eventCount, 1);

    timeline.Update(0.5f);
    EXPECT_EQ(eventCount, 11);
}

TEST(TimelineTest, PlayPauseStop)
{
    float result = 0.0f;

    auto track = std::make_unique<FloatTrack>();
    track->AddKeyframe(0.0f, 0.0f);
    track->AddKeyframe(10.0f, 100.0f);
    track->SetTarget([&](float v) { result = v; });

    Timeline timeline;
    timeline.AddTrack(std::move(track));

    timeline.Play();
    EXPECT_TRUE(timeline.IsPlaying());

    timeline.Update(5.0f);
    EXPECT_NEAR(result, 50.0f, 0.5f);

    timeline.Pause();
    EXPECT_FALSE(timeline.IsPlaying());
    timeline.Update(5.0f);
    EXPECT_NEAR(result, 50.0f, 0.5f);  // 進まないはず

    timeline.Play();
    timeline.Update(5.0f);
    EXPECT_NEAR(result, 100.0f, 0.5f);
}

TEST(TimelineTest, Loop)
{
    int loopCount = 0;
    auto track = std::make_unique<EventTrack>();
    track->AddEvent(0.5f, [&]() { loopCount++; });

    Timeline timeline;
    timeline.AddTrack(std::move(track));
    timeline.SetLoop(true);
    timeline.Play();

    timeline.Update(0.6f);  // 初回発火
    EXPECT_EQ(loopCount, 1);

    timeline.Update(0.5f);  // ループ + 2回目発火
    EXPECT_GE(loopCount, 2);
}

TEST(TimelineTest, Speed)
{
    float result = 0.0f;
    auto track = std::make_unique<FloatTrack>();
    track->AddKeyframe(0.0f, 0.0f);
    track->AddKeyframe(1.0f, 100.0f);
    track->SetTarget([&](float v) { result = v; });

    Timeline timeline;
    timeline.AddTrack(std::move(track));
    timeline.SetSpeed(2.0f);
    timeline.Play();

    timeline.Update(0.25f);  // 実効時間 = 0.5秒
    EXPECT_NEAR(result, 50.0f, 1.0f);
}

TEST(TimelineTest, Duration)
{
    auto t1 = std::make_unique<FloatTrack>();
    t1->AddKeyframe(0.0f, 0.0f);
    t1->AddKeyframe(5.0f, 100.0f);

    auto t2 = std::make_unique<EventTrack>();
    t2->AddEvent(3.0f, []() {});

    Timeline timeline;
    timeline.AddTrack(std::move(t1));
    timeline.AddTrack(std::move(t2));

    EXPECT_FLOAT_EQ(timeline.GetDuration(), 5.0f);
}

// ============================================================================
// Profiler
// ============================================================================

TEST(ProfilerTest, SectionTiming)
{
    auto& prof = Profiler::Instance();
    prof.SetEnabled(true);

    prof.BeginFrame();
    prof.BeginSection("TestSection");
    // 少しビジーウェイト
    volatile int x = 0;
    for (int i = 0; i < 10000; ++i) x += i;
    prof.EndSection("TestSection");
    prof.EndFrame();

    auto& results = prof.GetResults();
    ASSERT_GE(results.size(), 1u);
    EXPECT_EQ(results[0].name, "TestSection");
    EXPECT_GT(results[0].timeMs, 0.0f);
}

TEST(ProfilerTest, FrameTime)
{
    auto& prof = Profiler::Instance();
    prof.SetEnabled(true);

    prof.BeginFrame();
    volatile int x = 0;
    for (int i = 0; i < 10000; ++i) x += i;
    prof.EndFrame();

    EXPECT_GT(prof.GetFrameCPUTimeMs(), 0.0f);
}

TEST(ProfilerTest, MemoryUsage)
{
    size_t mem = Profiler::GetMemoryUsage();
    EXPECT_GT(mem, 0u);
}

TEST(ProfilerTest, ScopeProfiler)
{
    auto& prof = Profiler::Instance();
    prof.SetEnabled(true);
    prof.BeginFrame();
    {
        CPUProfileScope scope("ScopedSection");
        volatile int x = 0;
        for (int i = 0; i < 1000; ++i) x += i;
    }
    prof.EndFrame();

    auto& results = prof.GetResults();
    ASSERT_GE(results.size(), 1u);
    EXPECT_EQ(results[0].name, "ScopedSection");
}

// ============================================================================
// SpriteAnimator
// ============================================================================

TEST(SpriteAnimatorTest, AddClipAndUpdate)
{
    SpriteAnimator anim;

    SpriteAnimClip idle;
    idle.name = "Idle";
    idle.startFrame = 0;
    idle.endFrame = 2;
    idle.fps = 10.0f;
    idle.loop = true;

    anim.AddClip(idle);
    anim.Play("Idle");
    EXPECT_EQ(anim.GetCurrentClipName(), "Idle");

    anim.Update(0.05f);
    EXPECT_EQ(anim.GetCurrentFrame(), 0u);

    anim.Update(0.1f);
    EXPECT_EQ(anim.GetCurrentFrame(), 1u);
}

TEST(SpriteAnimatorTest, Transition)
{
    SpriteAnimator anim;

    SpriteAnimClip idle;
    idle.name = "Idle";
    idle.startFrame = 0;
    idle.endFrame = 0;
    idle.fps = 10.0f;
    idle.loop = false;
    anim.AddClip(idle);

    SpriteAnimClip run;
    run.name = "Run";
    run.startFrame = 0;
    run.endFrame = 0;
    run.fps = 10.0f;
    run.loop = true;
    anim.AddClip(run);

    SpriteAnimTransition trans;
    trans.fromClip = "Idle";
    trans.toClip = "Run";
    trans.exitTime = 1.0f;
    trans.blendDuration = 0.0f;
    anim.AddTransition(trans);

    anim.Play("Idle");
    EXPECT_EQ(anim.GetCurrentClipName(), "Idle");

    // Update enough to trigger the exit-time transition
    anim.Update(0.2f);
    EXPECT_EQ(anim.GetCurrentClipName(), "Run");
}

TEST(SpriteAnimatorTest, ManualPlaySwitch)
{
    SpriteAnimator anim;

    SpriteAnimClip idle;
    idle.name = "Idle";
    idle.startFrame = 0;
    idle.endFrame = 0;
    idle.fps = 10.0f;
    anim.AddClip(idle);

    SpriteAnimClip attack;
    attack.name = "Attack";
    attack.startFrame = 0;
    attack.endFrame = 3;
    attack.fps = 10.0f;
    attack.loop = false;
    anim.AddClip(attack);

    anim.Play("Idle");
    anim.Update(0.016f);
    EXPECT_EQ(anim.GetCurrentClipName(), "Idle");

    anim.Play("Attack");
    anim.Update(0.016f);
    EXPECT_EQ(anim.GetCurrentClipName(), "Attack");

    // Switch back to idle
    anim.Play("Idle");
    anim.Update(0.016f);
    EXPECT_EQ(anim.GetCurrentClipName(), "Idle");
}

TEST(SpriteAnimatorTest, FrameEvent)
{
    SpriteAnimator anim;
    int eventFired = 0;

    SpriteAnimClip clip;
    clip.name = "Test";
    clip.startFrame = 0;
    clip.endFrame = 3;
    clip.fps = 10.0f;
    clip.loop = false;
    clip.events.push_back({ 1, "hit", [&]() { eventFired++; } });
    anim.AddClip(clip);

    anim.Play("Test");
    anim.Update(0.15f);  // Should pass frame 1 and fire event
    EXPECT_GE(eventFired, 1);
}

TEST(SpriteAnimatorTest, GetCurrentFrame)
{
    SpriteAnimator anim;

    SpriteAnimClip clip;
    clip.name = "Test";
    clip.startFrame = 5;
    clip.endFrame = 10;
    clip.fps = 10.0f;
    clip.loop = true;
    anim.AddClip(clip);

    anim.Play("Test");
    EXPECT_EQ(anim.GetCurrentFrame(), 5u);

    anim.Update(0.1f);
    EXPECT_EQ(anim.GetCurrentFrame(), 6u);
}

TEST(SpriteAnimatorTest, PlaySwitchesClip)
{
    SpriteAnimator anim;

    SpriteAnimClip a;
    a.name = "A";
    a.startFrame = 0;
    a.endFrame = 1;
    a.fps = 10.0f;
    anim.AddClip(a);

    SpriteAnimClip b;
    b.name = "B";
    b.startFrame = 2;
    b.endFrame = 3;
    b.fps = 10.0f;
    anim.AddClip(b);

    anim.Play("A");
    EXPECT_EQ(anim.GetCurrentClipName(), "A");
    anim.Play("B");
    EXPECT_EQ(anim.GetCurrentClipName(), "B");
}
