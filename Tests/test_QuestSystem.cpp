/// @file test_QuestSystem.cpp
/// @brief QuestSystem 単体テスト

#include <gtest/gtest.h>
#include "Core/QuestSystem.h"

using namespace gx;

// ヘルパー: 単純なクエスト定義を生成
static QuestDef MakeQuest(const gx::String& id,
                          gx::Vector<QuestObjective> objectives = {},
                          gx::Vector<gx::String> prereqs = {},
                          bool autoComplete = false)
{
    QuestDef def;
    def.questId      = id;
    def.title        = "Quest: " + id;
    def.description  = "Description for " + id;
    def.objectives   = std::move(objectives);
    def.prerequisites = std::move(prereqs);
    def.autoComplete = autoComplete;
    return def;
}

// ヘルパー: 目標を生成
static QuestObjective MakeObjective(const gx::String& id, int required = 1)
{
    QuestObjective obj;
    obj.id            = id;
    obj.description   = "Objective: " + id;
    obj.requiredCount = required;
    obj.currentCount  = 0;
    return obj;
}

TEST(QuestSystemTest, ConstructDestruct)
{
    QuestSystem qs;
    EXPECT_EQ(qs.GetQuestDefCount(), 0u);
    EXPECT_EQ(qs.GetActiveQuests().size(), 0u);
    EXPECT_EQ(qs.GetCompletedQuests().size(), 0u);
}

TEST(QuestSystemTest, RegisterQuest)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1"));
    qs.RegisterQuest(MakeQuest("q2"));
    EXPECT_EQ(qs.GetQuestDefCount(), 2u);
}

TEST(QuestSystemTest, GetQuestDef)
{
    QuestSystem qs;
    auto quest = MakeQuest("fetch_herbs", { MakeObjective("gather", 5) });
    quest.rewards.push_back({"exp", "", 100});
    qs.RegisterQuest(quest);

    const QuestDef* def = qs.GetQuestDef("fetch_herbs");
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->title, "Quest: fetch_herbs");
    EXPECT_EQ(def->objectives.size(), 1u);
    EXPECT_EQ(def->rewards.size(), 1u);

    EXPECT_EQ(qs.GetQuestDef("nonexistent"), nullptr);
}

TEST(QuestSystemTest, StartQuest)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1", { MakeObjective("kill", 3) }));

    EXPECT_TRUE(qs.StartQuest("q1"));
    EXPECT_EQ(qs.GetQuestState("q1"), QuestState::Active);

    // 未登録クエストは開始不可
    EXPECT_FALSE(qs.StartQuest("nonexistent"));
}

TEST(QuestSystemTest, StartWithPrereqs)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("intro"));
    qs.RegisterQuest(MakeQuest("chapter2", {}, {"intro"}));

    // 前提クエスト未完了 — 開始不可
    EXPECT_FALSE(qs.StartQuest("chapter2"));

    // 前提を完了させる
    qs.StartQuest("intro");
    qs.CompleteQuest("intro");
    EXPECT_EQ(qs.GetQuestState("intro"), QuestState::Completed);

    // 前提完了後 — 開始可能
    EXPECT_TRUE(qs.StartQuest("chapter2"));
    EXPECT_EQ(qs.GetQuestState("chapter2"), QuestState::Active);
}

TEST(QuestSystemTest, StartAlreadyActive)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1"));

    EXPECT_TRUE(qs.StartQuest("q1"));
    EXPECT_FALSE(qs.StartQuest("q1")); // 二重開始不可
}

TEST(QuestSystemTest, CompleteQuest)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1"));
    qs.StartQuest("q1");

    EXPECT_TRUE(qs.CompleteQuest("q1"));
    EXPECT_EQ(qs.GetQuestState("q1"), QuestState::Completed);

    // 完了済みを再完了は不可
    EXPECT_FALSE(qs.CompleteQuest("q1"));
}

TEST(QuestSystemTest, FailQuest)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1"));
    qs.StartQuest("q1");

    EXPECT_TRUE(qs.FailQuest("q1"));
    EXPECT_EQ(qs.GetQuestState("q1"), QuestState::Failed);

    // 失敗済みを再失敗は不可
    EXPECT_FALSE(qs.FailQuest("q1"));
}

TEST(QuestSystemTest, GetQuestState)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1"));

    // 未開始
    EXPECT_EQ(qs.GetQuestState("q1"), QuestState::Inactive);

    qs.StartQuest("q1");
    EXPECT_EQ(qs.GetQuestState("q1"), QuestState::Active);

    // 未登録
    EXPECT_EQ(qs.GetQuestState("unknown"), QuestState::Inactive);
}

TEST(QuestSystemTest, UpdateObjective)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1", { MakeObjective("kill", 5) }));
    qs.StartQuest("q1");

    EXPECT_TRUE(qs.UpdateObjective("q1", "kill", 2));
    EXPECT_EQ(qs.GetObjectiveProgress("q1", "kill"), 2);

    EXPECT_TRUE(qs.UpdateObjective("q1", "kill", 1));
    EXPECT_EQ(qs.GetObjectiveProgress("q1", "kill"), 3);

    // 未アクティブなクエストは更新不可
    QuestSystem qs2;
    qs2.RegisterQuest(MakeQuest("q2", { MakeObjective("obj") }));
    EXPECT_FALSE(qs2.UpdateObjective("q2", "obj"));
}

TEST(QuestSystemTest, ObjectiveProgress)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1", { MakeObjective("gather", 10) }));
    qs.StartQuest("q1");

    EXPECT_EQ(qs.GetObjectiveProgress("q1", "gather"), 0);
    qs.UpdateObjective("q1", "gather", 7);
    EXPECT_EQ(qs.GetObjectiveProgress("q1", "gather"), 7);

    // 未登録目標
    EXPECT_EQ(qs.GetObjectiveProgress("q1", "nonexistent"), 0);
}

TEST(QuestSystemTest, ObjectiveComplete)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1", { MakeObjective("deliver", 3) }));
    qs.StartQuest("q1");

    EXPECT_FALSE(qs.IsObjectiveComplete("q1", "deliver"));
    qs.UpdateObjective("q1", "deliver", 3);
    EXPECT_TRUE(qs.IsObjectiveComplete("q1", "deliver"));

    // 超過しても完了
    qs.UpdateObjective("q1", "deliver", 2);
    EXPECT_TRUE(qs.IsObjectiveComplete("q1", "deliver"));
}

TEST(QuestSystemTest, AllObjectivesComplete)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1", {
        MakeObjective("a", 2),
        MakeObjective("b", 3)
    }));
    qs.StartQuest("q1");

    EXPECT_FALSE(qs.AreAllObjectivesComplete("q1"));

    qs.UpdateObjective("q1", "a", 2);
    EXPECT_FALSE(qs.AreAllObjectivesComplete("q1")); // b未完了

    qs.UpdateObjective("q1", "b", 3);
    EXPECT_TRUE(qs.AreAllObjectivesComplete("q1"));
}

TEST(QuestSystemTest, AutoComplete)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1", { MakeObjective("kill", 2) }, {}, true));
    qs.StartQuest("q1");

    qs.UpdateObjective("q1", "kill", 1);
    EXPECT_EQ(qs.GetQuestState("q1"), QuestState::Active);

    qs.UpdateObjective("q1", "kill", 1); // reaches 2/2
    EXPECT_EQ(qs.GetQuestState("q1"), QuestState::Completed);
}

TEST(QuestSystemTest, GetActiveQuests)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1"));
    qs.RegisterQuest(MakeQuest("q2"));
    qs.RegisterQuest(MakeQuest("q3"));

    qs.StartQuest("q1");
    qs.StartQuest("q2");
    qs.StartQuest("q3");
    qs.CompleteQuest("q2");

    auto active = qs.GetActiveQuests();
    EXPECT_EQ(active.size(), 2u);
    // q1 and q3 should be active
    EXPECT_NE(std::find(active.begin(), active.end(), "q1"), active.end());
    EXPECT_NE(std::find(active.begin(), active.end(), "q3"), active.end());
}

TEST(QuestSystemTest, GetCompletedQuests)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1"));
    qs.RegisterQuest(MakeQuest("q2"));

    qs.StartQuest("q1");
    qs.StartQuest("q2");
    qs.CompleteQuest("q1");

    auto completed = qs.GetCompletedQuests();
    EXPECT_EQ(completed.size(), 1u);
    EXPECT_EQ(completed[0], "q1");
}

TEST(QuestSystemTest, Reset)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1", { MakeObjective("obj", 5) }));
    qs.StartQuest("q1");
    qs.UpdateObjective("q1", "obj", 3);

    qs.Reset();

    EXPECT_EQ(qs.GetQuestState("q1"), QuestState::Inactive);
    EXPECT_EQ(qs.GetActiveQuests().size(), 0u);
    EXPECT_EQ(qs.GetObjectiveProgress("q1", "obj"), 0);

    // 定義は残っているので再開始可能
    EXPECT_EQ(qs.GetQuestDefCount(), 1u);
    EXPECT_TRUE(qs.StartQuest("q1"));
}

TEST(QuestSystemTest, OnStateChangedCallback)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1"));

    gx::Vector<QuestStateChangedEvent> events;
    qs.OnStateChanged = [&](const QuestStateChangedEvent& e) {
        events.push_back(e);
    };

    qs.StartQuest("q1");
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].questId, "q1");
    EXPECT_EQ(events[0].oldState, QuestState::Inactive);
    EXPECT_EQ(events[0].newState, QuestState::Active);

    qs.CompleteQuest("q1");
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[1].oldState, QuestState::Active);
    EXPECT_EQ(events[1].newState, QuestState::Completed);
}

TEST(QuestSystemTest, OnProgressCallback)
{
    QuestSystem qs;
    qs.RegisterQuest(MakeQuest("q1", { MakeObjective("gather", 10) }));
    qs.StartQuest("q1");

    gx::Vector<QuestProgressEvent> events;
    qs.OnProgress = [&](const QuestProgressEvent& e) {
        events.push_back(e);
    };

    qs.UpdateObjective("q1", "gather", 3);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].questId, "q1");
    EXPECT_EQ(events[0].objectiveId, "gather");
    EXPECT_EQ(events[0].oldCount, 0);
    EXPECT_EQ(events[0].newCount, 3);

    qs.UpdateObjective("q1", "gather", 5);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[1].oldCount, 3);
    EXPECT_EQ(events[1].newCount, 8);
}
