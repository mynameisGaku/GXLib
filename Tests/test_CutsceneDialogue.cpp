/// @file test_CutsceneDialogue.cpp
/// @brief CutsceneSystemとDialogueSystemのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/CutsceneSystem.h"
#include "Core/DialogueSystem.h"

using namespace gx;

// ===========================================================================
// CutsceneSystemテスト
// ===========================================================================

TEST(CutsceneSystemTest, InitialState)
{
    CutsceneSystem cs;
    EXPECT_EQ(cs.GetState(), CutsceneState::Stopped);
    EXPECT_EQ(cs.GetActionCount(), 0);
    EXPECT_FLOAT_EQ(cs.GetCurrentTime(), 0.0f);
}

TEST(CutsceneSystemTest, AddAction_IncreasesCount)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.startTime = 0.0f;
    action.duration = 1.0f;
    cs.AddAction(action);
    EXPECT_EQ(cs.GetActionCount(), 1);
}

TEST(CutsceneSystemTest, Play_SetsStatePlaying)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.duration = 1.0f;
    cs.AddAction(action);
    cs.Play();
    EXPECT_EQ(cs.GetState(), CutsceneState::Playing);
}

TEST(CutsceneSystemTest, Pause_SetsStatePaused)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.duration = 1.0f;
    cs.AddAction(action);
    cs.Play();
    cs.Pause();
    EXPECT_EQ(cs.GetState(), CutsceneState::Paused);
}

TEST(CutsceneSystemTest, Resume_SetsStatePlaying)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.duration = 1.0f;
    cs.AddAction(action);
    cs.Play();
    cs.Pause();
    cs.Resume();
    EXPECT_EQ(cs.GetState(), CutsceneState::Playing);
}

TEST(CutsceneSystemTest, Stop_ResetsState)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.duration = 1.0f;
    cs.AddAction(action);
    cs.Play();
    cs.Update(0.5f);
    cs.Stop();
    EXPECT_EQ(cs.GetState(), CutsceneState::Stopped);
    EXPECT_FLOAT_EQ(cs.GetCurrentTime(), 0.0f);
}

TEST(CutsceneSystemTest, Update_AdvancesTime)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.duration = 2.0f;
    cs.AddAction(action);
    cs.Play();
    cs.Update(0.5f);
    EXPECT_NEAR(cs.GetCurrentTime(), 0.5f, 1e-5f);
}

TEST(CutsceneSystemTest, Update_DoesNotAdvanceWhenPaused)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.duration = 2.0f;
    cs.AddAction(action);
    cs.Play();
    cs.Update(0.5f);
    cs.Pause();
    cs.Update(1.0f);
    EXPECT_NEAR(cs.GetCurrentTime(), 0.5f, 1e-5f);
}

TEST(CutsceneSystemTest, GetTotalDuration)
{
    CutsceneSystem cs;
    CutsceneAction a1;
    a1.startTime = 0.0f;
    a1.duration = 1.0f;
    cs.AddAction(a1);

    CutsceneAction a2;
    a2.startTime = 1.0f;
    a2.duration = 2.0f;
    cs.AddAction(a2);

    EXPECT_NEAR(cs.GetTotalDuration(), 3.0f, 1e-5f);
}

TEST(CutsceneSystemTest, GetProgress_ReturnsCorrectValue)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.startTime = 0.0f;
    action.duration = 2.0f;
    cs.AddAction(action);
    cs.Play();
    cs.Update(1.0f);
    EXPECT_NEAR(cs.GetProgress(), 0.5f, 1e-5f);
}

TEST(CutsceneSystemTest, IsFinished_AfterAllActions)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.startTime = 0.0f;
    action.duration = 1.0f;
    cs.AddAction(action);
    cs.Play();
    cs.Update(1.5f);
    EXPECT_TRUE(cs.IsFinished());
}

TEST(CutsceneSystemTest, OnFinished_CallbackFired)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.duration = 0.5f;
    cs.AddAction(action);

    bool finished = false;
    cs.SetOnFinished([&]() { finished = true; });
    cs.Play();
    cs.Update(1.0f);
    EXPECT_TRUE(finished);
}

TEST(CutsceneSystemTest, PlaySound_CallbackFired)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::PlaySound;
    action.startTime = 0.0f;
    action.duration = 0.0f;
    action.soundName = "explosion";
    cs.AddAction(action);

    std::string playedSound;
    CutsceneCallbacks cb;
    cb.onPlaySound = [&](const std::string& s) { playedSound = s; };
    cs.SetCallbacks(cb);
    cs.Play();
    cs.Update(0.1f);
    EXPECT_EQ(playedSound, "explosion");
}

TEST(CutsceneSystemTest, ShowDialogue_CallbackFired)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::ShowDialogue;
    action.startTime = 0.0f;
    action.speakerName = "Hero";
    action.dialogueText = "Hello!";
    cs.AddAction(action);

    std::string speaker, text;
    CutsceneCallbacks cb;
    cb.onShowDialogue = [&](const std::string& s, const std::string& t) {
        speaker = s;
        text = t;
    };
    cs.SetCallbacks(cb);
    cs.Play();
    cs.Update(0.1f);
    EXPECT_EQ(speaker, "Hero");
    EXPECT_EQ(text, "Hello!");
}

TEST(CutsceneSystemTest, Custom_CallbackFired)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Custom;
    action.startTime = 0.0f;
    int counter = 0;
    action.callback = [&]() { counter++; };
    cs.AddAction(action);
    cs.Play();
    cs.Update(0.1f);
    EXPECT_EQ(counter, 1);
}

TEST(CutsceneSystemTest, MoveTo_InterpolatesProgress)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::MoveTo;
    action.startTime = 0.0f;
    action.duration = 2.0f;
    action.targetPosition = { 10.0f, 20.0f, 30.0f };
    cs.AddAction(action);

    float lastProgress = -1.0f;
    XMFLOAT3 lastPos = {};
    CutsceneCallbacks cb;
    cb.onMoveTo = [&](const XMFLOAT3& p, float prog) {
        lastPos = p;
        lastProgress = prog;
    };
    cs.SetCallbacks(cb);
    cs.Play();
    cs.Update(1.0f); // 50% progress
    EXPECT_NEAR(lastProgress, 0.5f, 1e-5f);
    EXPECT_NEAR(lastPos.x, 5.0f, 1e-5f);
    EXPECT_NEAR(lastPos.y, 10.0f, 1e-5f);
}

TEST(CutsceneSystemTest, FadeTo_InterpolatesAlpha)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::FadeTo;
    action.startTime = 0.0f;
    action.duration = 1.0f;
    action.targetAlpha = 1.0f;
    cs.AddAction(action);

    float lastAlpha = -1.0f;
    CutsceneCallbacks cb;
    cb.onFadeTo = [&](float alpha) { lastAlpha = alpha; };
    cs.SetCallbacks(cb);
    cs.Play();
    cs.Update(0.5f);
    EXPECT_NEAR(lastAlpha, 0.5f, 1e-5f);
}

TEST(CutsceneSystemTest, Clear_RemovesAllActions)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::Wait;
    action.duration = 1.0f;
    cs.AddAction(action);
    cs.AddAction(action);
    cs.Clear();
    EXPECT_EQ(cs.GetActionCount(), 0);
    EXPECT_EQ(cs.GetState(), CutsceneState::Stopped);
}

TEST(CutsceneSystemTest, DelayedAction_StartsAtCorrectTime)
{
    CutsceneSystem cs;
    CutsceneAction action;
    action.type = CutsceneActionType::PlaySound;
    action.startTime = 1.0f;
    action.soundName = "delayed";
    cs.AddAction(action);

    std::string playedSound;
    CutsceneCallbacks cb;
    cb.onPlaySound = [&](const std::string& s) { playedSound = s; };
    cs.SetCallbacks(cb);
    cs.Play();

    cs.Update(0.5f);
    EXPECT_TRUE(playedSound.empty()); // まだ

    cs.Update(0.6f);
    EXPECT_EQ(playedSound, "delayed"); // 発火した
}

TEST(CutsceneSystemTest, MultipleActions_ExecuteInOrder)
{
    CutsceneSystem cs;
    std::vector<std::string> order;

    CutsceneAction a1;
    a1.type = CutsceneActionType::Custom;
    a1.startTime = 0.0f;
    a1.callback = [&]() { order.push_back("first"); };
    cs.AddAction(a1);

    CutsceneAction a2;
    a2.type = CutsceneActionType::Custom;
    a2.startTime = 0.5f;
    a2.callback = [&]() { order.push_back("second"); };
    cs.AddAction(a2);

    cs.Play();
    cs.Update(0.1f);
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], "first");

    cs.Update(0.5f);
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[1], "second");
}

TEST(CutsceneSystemTest, EmptyCutscene_IsFinished)
{
    CutsceneSystem cs;
    EXPECT_TRUE(cs.IsFinished());
}

// ===========================================================================
// DialogueSystemテスト
// ===========================================================================

TEST(DialogueSystemTest, InitialState)
{
    DialogueSystem ds;
    EXPECT_EQ(ds.GetState(), DialogueState::Inactive);
    EXPECT_FALSE(ds.IsActive());
    EXPECT_EQ(ds.GetSequenceCount(), 0);
}

TEST(DialogueSystemTest, RegisterSequence)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "greet";
    seq.lines.push_back({ "NPC", "Hello!" });
    ds.RegisterSequence(seq);
    EXPECT_EQ(ds.GetSequenceCount(), 1);
    EXPECT_TRUE(ds.HasSequence("greet"));
}

TEST(DialogueSystemTest, StartSequence_ValidId)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "intro";
    seq.lines.push_back({ "NPC", "Welcome!", 0.0f });
    ds.RegisterSequence(seq);

    EXPECT_TRUE(ds.StartSequence("intro"));
    EXPECT_TRUE(ds.IsActive());
}

TEST(DialogueSystemTest, StartSequence_InvalidId)
{
    DialogueSystem ds;
    EXPECT_FALSE(ds.StartSequence("nonexistent"));
    EXPECT_FALSE(ds.IsActive());
}

TEST(DialogueSystemTest, GetCurrentLine)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "Alice", "Hi there!", 0.0f });
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    const DialogueLine* line = ds.GetCurrentLine();
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(line->speaker, "Alice");
    EXPECT_EQ(line->text, "Hi there!");
}

TEST(DialogueSystemTest, AdvanceLine_GoesToNextLine)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "A", "Line 1", 0.0f });
    seq.lines.push_back({ "B", "Line 2", 0.0f });
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    EXPECT_EQ(ds.GetCurrentLineIndex(), 0);
    ds.AdvanceLine();
    EXPECT_EQ(ds.GetCurrentLineIndex(), 1);

    const DialogueLine* line = ds.GetCurrentLine();
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(line->speaker, "B");
}

TEST(DialogueSystemTest, AdvanceLine_EndsDialogueAtEnd)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "A", "Only line", 0.0f });
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    ds.AdvanceLine(); // 最後の行を超える
    EXPECT_FALSE(ds.IsActive());
    EXPECT_EQ(ds.GetState(), DialogueState::Inactive);
}

TEST(DialogueSystemTest, Choices_ShowAfterLastLine)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "NPC", "Question?", 0.0f });
    DialogueChoice c1;
    c1.text = "Yes";
    DialogueChoice c2;
    c2.text = "No";
    seq.choices.push_back(c1);
    seq.choices.push_back(c2);
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    ds.AdvanceLine(); // 最後の行を超える -> 選択肢を表示
    EXPECT_EQ(ds.GetState(), DialogueState::ShowingChoices);

    const auto& choices = ds.GetCurrentChoices();
    ASSERT_EQ(choices.size(), 2u);
    EXPECT_EQ(choices[0].text, "Yes");
    EXPECT_EQ(choices[1].text, "No");
}

TEST(DialogueSystemTest, SelectChoice_FiresCallback)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "NPC", "Pick one", 0.0f });

    int chosen = -1;
    DialogueChoice c1;
    c1.text = "Option A";
    c1.callback = [&]() { chosen = 0; };
    seq.choices.push_back(c1);
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    ds.AdvanceLine(); // 選択肢を表示
    ds.SelectChoice(0);
    EXPECT_EQ(chosen, 0);
}

TEST(DialogueSystemTest, SelectChoice_NavigatesToNextSequence)
{
    DialogueSystem ds;

    DialogueSequence seq1;
    seq1.id = "question";
    seq1.lines.push_back({ "NPC", "Where to?", 0.0f });
    DialogueChoice c;
    c.text = "Go to forest";
    c.nextSequenceId = "forest";
    seq1.choices.push_back(c);
    ds.RegisterSequence(seq1);

    DialogueSequence seq2;
    seq2.id = "forest";
    seq2.lines.push_back({ "NPC", "Welcome to the forest!", 0.0f });
    ds.RegisterSequence(seq2);

    ds.StartSequence("question");
    ds.AdvanceLine(); // 選択肢を表示
    ds.SelectChoice(0); // 森に移動

    EXPECT_TRUE(ds.IsActive());
    const DialogueLine* line = ds.GetCurrentLine();
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(line->text, "Welcome to the forest!");
}

TEST(DialogueSystemTest, AutoAdvance_WithDuration)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "A", "Auto line", 0.5f });
    seq.lines.push_back({ "B", "Next line", 0.0f });
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    EXPECT_EQ(ds.GetState(), DialogueState::ShowingLine);
    ds.Update(0.3f);
    EXPECT_EQ(ds.GetCurrentLineIndex(), 0); // まだ最初の行

    ds.Update(0.3f); // 合計0.6 > 0.5の継続時間
    EXPECT_EQ(ds.GetCurrentLineIndex(), 1); // 自動送り
}

TEST(DialogueSystemTest, ManualAdvance_WaitingForInput)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "A", "Manual", 0.0f }); // 継続時間0 = 手動送り
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    EXPECT_EQ(ds.GetState(), DialogueState::WaitingForInput);
    ds.Update(10.0f); // 自動送りされないはず
    EXPECT_EQ(ds.GetCurrentLineIndex(), 0);
}

TEST(DialogueSystemTest, EndDialogue_ResetsState)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "A", "Line", 0.0f });
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    ds.EndDialogue();
    EXPECT_FALSE(ds.IsActive());
    EXPECT_EQ(ds.GetState(), DialogueState::Inactive);
}

TEST(DialogueSystemTest, OnSequenceEnded_CallbackFired)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "A", "Bye", 0.0f });
    ds.RegisterSequence(seq);

    bool ended = false;
    DialogueCallbacks cb;
    cb.onSequenceEnded = [&]() { ended = true; };
    ds.SetCallbacks(cb);

    ds.StartSequence("test");
    ds.AdvanceLine(); // 対話終了
    EXPECT_TRUE(ended);
}

TEST(DialogueSystemTest, OnLineStarted_CallbackFired)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "Bob", "Hey!", 0.0f });
    ds.RegisterSequence(seq);

    std::string cbSpeaker, cbText;
    DialogueCallbacks cb;
    cb.onLineStarted = [&](const std::string& s, const std::string& t) {
        cbSpeaker = s;
        cbText = t;
    };
    ds.SetCallbacks(cb);

    ds.StartSequence("test");
    EXPECT_EQ(cbSpeaker, "Bob");
    EXPECT_EQ(cbText, "Hey!");
}

TEST(DialogueSystemTest, NextSequenceId_ChainsSequences)
{
    DialogueSystem ds;

    DialogueSequence seq1;
    seq1.id = "part1";
    seq1.lines.push_back({ "A", "Part 1", 0.0f });
    seq1.nextSequenceId = "part2";
    ds.RegisterSequence(seq1);

    DialogueSequence seq2;
    seq2.id = "part2";
    seq2.lines.push_back({ "B", "Part 2", 0.0f });
    ds.RegisterSequence(seq2);

    ds.StartSequence("part1");
    ds.AdvanceLine(); // part1の終了 -> part2に連鎖
    EXPECT_TRUE(ds.IsActive());

    const DialogueLine* line = ds.GetCurrentLine();
    ASSERT_NE(line, nullptr);
    EXPECT_EQ(line->text, "Part 2");
}

TEST(DialogueSystemTest, GetCurrentChoices_EmptyWhenNotShowingChoices)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "A", "Line", 0.0f });
    ds.RegisterSequence(seq);
    ds.StartSequence("test");

    const auto& choices = ds.GetCurrentChoices();
    EXPECT_TRUE(choices.empty());
}

TEST(DialogueSystemTest, HasSequence_ReturnsFalseForMissing)
{
    DialogueSystem ds;
    EXPECT_FALSE(ds.HasSequence("nonexistent"));
}

TEST(DialogueSystemTest, OnChoicesPresented_CallbackFired)
{
    DialogueSystem ds;
    DialogueSequence seq;
    seq.id = "test";
    seq.lines.push_back({ "NPC", "Choose", 0.0f });
    DialogueChoice c;
    c.text = "OK";
    seq.choices.push_back(c);
    ds.RegisterSequence(seq);

    bool choicesShown = false;
    DialogueCallbacks cb;
    cb.onChoicesPresented = [&](const std::vector<DialogueChoice>&) { choicesShown = true; };
    ds.SetCallbacks(cb);

    ds.StartSequence("test");
    ds.AdvanceLine();
    EXPECT_TRUE(choicesShown);
}
