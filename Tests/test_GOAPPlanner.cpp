/// @file test_GOAPPlanner.cpp
/// @brief GOAPPlanner 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "AI/GOAPPlanner.h"

using namespace gx;

// ==================== WorldState ====================

TEST(GOAPPlannerTest, WorldStateSetGet)
{
    WorldState ws;
    ws.Set("hasWeapon", true);
    ws.Set("isAlive", true);
    ws.Set("hasAmmo", false);
    EXPECT_TRUE(ws.Get("hasWeapon"));
    EXPECT_TRUE(ws.Get("isAlive"));
    EXPECT_FALSE(ws.Get("hasAmmo"));
    EXPECT_FALSE(ws.Get("unknown")); // 未設定はfalse
}

TEST(GOAPPlannerTest, WorldStateHas)
{
    WorldState ws;
    EXPECT_FALSE(ws.Has("key"));
    ws.Set("key", true);
    EXPECT_TRUE(ws.Has("key"));
}

TEST(GOAPPlannerTest, WorldStateSatisfies)
{
    WorldState current;
    current.Set("hasWeapon", true);
    current.Set("isAlive", true);
    current.Set("hasAmmo", true);

    WorldState requirement;
    requirement.Set("hasWeapon", true);
    requirement.Set("isAlive", true);

    EXPECT_TRUE(current.Satisfies(requirement));

    // 要件に不一致がある場合
    WorldState unmet;
    unmet.Set("hasWeapon", false);
    EXPECT_FALSE(current.Satisfies(unmet));
}

TEST(GOAPPlannerTest, WorldStateDistance)
{
    WorldState current;
    current.Set("a", true);
    current.Set("b", false);
    current.Set("c", true);

    WorldState goal;
    goal.Set("a", true);
    goal.Set("b", true);  // 不一致
    goal.Set("c", false); // 不一致

    EXPECT_EQ(current.Distance(goal), 2);
}

// ==================== アクション管理 ====================

TEST(GOAPPlannerTest, AddAction)
{
    GOAPPlanner planner;
    GOAPAction action;
    action.name = "Attack";
    action.cost = 2.0f;
    planner.AddAction(action);
    EXPECT_EQ(planner.GetActionCount(), 1);
}

TEST(GOAPPlannerTest, RemoveAction)
{
    GOAPPlanner planner;
    GOAPAction a1, a2;
    a1.name = "Attack";
    a2.name = "Reload";
    planner.AddAction(a1);
    planner.AddAction(a2);
    EXPECT_EQ(planner.GetActionCount(), 2);

    EXPECT_TRUE(planner.RemoveAction("Attack"));
    EXPECT_EQ(planner.GetActionCount(), 1);
    EXPECT_FALSE(planner.RemoveAction("NonExistent"));
}

TEST(GOAPPlannerTest, ActionCount)
{
    GOAPPlanner planner;
    EXPECT_EQ(planner.GetActionCount(), 0);
    GOAPAction a;
    a.name = "a";
    planner.AddAction(a);
    a.name = "b";
    planner.AddAction(a);
    a.name = "c";
    planner.AddAction(a);
    EXPECT_EQ(planner.GetActionCount(), 3);
}

TEST(GOAPPlannerTest, GetAction)
{
    GOAPPlanner planner;
    GOAPAction a;
    a.name = "Heal";
    a.cost = 5.0f;
    planner.AddAction(a);

    const auto* found = planner.GetAction("Heal");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "Heal");
    EXPECT_NEAR(found->cost, 5.0f, 1e-5f);

    EXPECT_EQ(planner.GetAction("Missing"), nullptr);
}

// ==================== プラン ====================

TEST(GOAPPlannerTest, PlanEmpty)
{
    GOAPPlanner planner;
    WorldState current;
    GOAPGoal goal;
    goal.targetState.Set("win", true);

    auto plan = planner.MakePlan(current, goal);
    EXPECT_FALSE(plan.valid); // アクションなしではプランできない
}

TEST(GOAPPlannerTest, PlanSingleAction)
{
    GOAPPlanner planner;

    GOAPAction attack;
    attack.name = "Attack";
    attack.cost = 1.0f;
    attack.preconditions.Set("hasWeapon", true);
    attack.effects.Set("enemyDead", true);
    planner.AddAction(attack);

    WorldState current;
    current.Set("hasWeapon", true);

    GOAPGoal goal;
    goal.name = "KillEnemy";
    goal.targetState.Set("enemyDead", true);

    auto plan = planner.MakePlan(current, goal);
    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(plan.actions.size(), 1u);
    EXPECT_EQ(plan.actions[0]->name, "Attack");
}

TEST(GOAPPlannerTest, PlanMultipleActions)
{
    GOAPPlanner planner;

    GOAPAction getWeapon;
    getWeapon.name = "GetWeapon";
    getWeapon.cost = 1.0f;
    getWeapon.effects.Set("hasWeapon", true);
    planner.AddAction(getWeapon);

    GOAPAction attack;
    attack.name = "Attack";
    attack.cost = 1.0f;
    attack.preconditions.Set("hasWeapon", true);
    attack.effects.Set("enemyDead", true);
    planner.AddAction(attack);

    WorldState current; // 武器なし

    GOAPGoal goal;
    goal.targetState.Set("enemyDead", true);

    auto plan = planner.MakePlan(current, goal);
    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(plan.actions.size(), 2u);
    // GetWeapon → Attack の順
    EXPECT_EQ(plan.actions[0]->name, "GetWeapon");
    EXPECT_EQ(plan.actions[1]->name, "Attack");
}

TEST(GOAPPlannerTest, PlanOptimalCost)
{
    GOAPPlanner planner;

    // 高コストの直接攻撃
    GOAPAction directAttack;
    directAttack.name = "DirectAttack";
    directAttack.cost = 10.0f;
    directAttack.effects.Set("enemyDead", true);
    planner.AddAction(directAttack);

    // 低コストの武器取得 + 攻撃
    GOAPAction getWeapon;
    getWeapon.name = "GetWeapon";
    getWeapon.cost = 1.0f;
    getWeapon.effects.Set("hasWeapon", true);
    planner.AddAction(getWeapon);

    GOAPAction weaponAttack;
    weaponAttack.name = "WeaponAttack";
    weaponAttack.cost = 2.0f;
    weaponAttack.preconditions.Set("hasWeapon", true);
    weaponAttack.effects.Set("enemyDead", true);
    planner.AddAction(weaponAttack);

    WorldState current;
    GOAPGoal goal;
    goal.targetState.Set("enemyDead", true);

    auto plan = planner.MakePlan(current, goal);
    EXPECT_TRUE(plan.valid);
    // GetWeapon(1) + WeaponAttack(2) = 3 < DirectAttack(10)
    EXPECT_LT(plan.totalCost, 10.0f);
}

TEST(GOAPPlannerTest, PlanNoSolution)
{
    GOAPPlanner planner;

    GOAPAction attack;
    attack.name = "Attack";
    attack.preconditions.Set("hasNukes", true); // 満たせない条件
    attack.effects.Set("enemyDead", true);
    planner.AddAction(attack);

    WorldState current;
    GOAPGoal goal;
    goal.targetState.Set("enemyDead", true);

    auto plan = planner.MakePlan(current, goal);
    EXPECT_FALSE(plan.valid);
}

TEST(GOAPPlannerTest, PlanWithPreconditions)
{
    GOAPPlanner planner;

    GOAPAction reload;
    reload.name = "Reload";
    reload.cost = 1.0f;
    reload.preconditions.Set("hasAmmoBox", true);
    reload.effects.Set("hasAmmo", true);
    planner.AddAction(reload);

    GOAPAction getAmmo;
    getAmmo.name = "GetAmmoBox";
    getAmmo.cost = 2.0f;
    getAmmo.effects.Set("hasAmmoBox", true);
    planner.AddAction(getAmmo);

    GOAPAction shoot;
    shoot.name = "Shoot";
    shoot.cost = 1.0f;
    shoot.preconditions.Set("hasAmmo", true);
    shoot.effects.Set("enemyDead", true);
    planner.AddAction(shoot);

    WorldState current;
    GOAPGoal goal;
    goal.targetState.Set("enemyDead", true);

    auto plan = planner.MakePlan(current, goal);
    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(plan.actions.size(), 3u); // GetAmmoBox → Reload → Shoot
}

TEST(GOAPPlannerTest, PlannerStats)
{
    GOAPPlanner planner;

    GOAPAction a;
    a.name = "Do";
    a.effects.Set("done", true);
    planner.AddAction(a);

    WorldState current;
    GOAPGoal goal;
    goal.targetState.Set("done", true);

    planner.MakePlan(current, goal);

    auto stats = planner.GetLastStats();
    EXPECT_GT(stats.nodesExplored, 0);
    EXPECT_GE(stats.planLength, 1);
    EXPECT_GT(stats.planCost, 0.0f);
}

TEST(GOAPPlannerTest, MaxIterations)
{
    GOAPPlanner planner;
    EXPECT_EQ(planner.GetMaxIterations(), 1000);
    planner.SetMaxIterations(50);
    EXPECT_EQ(planner.GetMaxIterations(), 50);
}

TEST(GOAPPlannerTest, ValidatePlanSuccess)
{
    GOAPPlanner planner;

    GOAPAction getWeapon;
    getWeapon.name = "GetWeapon";
    getWeapon.cost = 1.0f;
    getWeapon.effects.Set("hasWeapon", true);
    planner.AddAction(getWeapon);

    GOAPAction attack;
    attack.name = "Attack";
    attack.cost = 1.0f;
    attack.preconditions.Set("hasWeapon", true);
    attack.effects.Set("enemyDead", true);
    planner.AddAction(attack);

    WorldState current;
    GOAPGoal goal;
    goal.targetState.Set("enemyDead", true);

    auto plan = planner.MakePlan(current, goal);
    EXPECT_TRUE(plan.valid);

    WorldState goalState;
    goalState.Set("enemyDead", true);
    EXPECT_TRUE(planner.ValidatePlan(plan, current, goalState));
}

TEST(GOAPPlannerTest, ValidatePlanFailure)
{
    GOAPPlanner planner;

    // 不正な計画を手動で構築
    GOAPAction attack;
    attack.name = "Attack";
    attack.preconditions.Set("hasWeapon", true);
    attack.effects.Set("enemyDead", true);
    planner.AddAction(attack);

    Plan fakePlan;
    fakePlan.valid   = true;
    fakePlan.actions = { planner.GetAction("Attack") };

    WorldState current; // hasWeapon=false
    WorldState goalState;
    goalState.Set("enemyDead", true);

    // 前提条件が満たされないため失敗
    EXPECT_FALSE(planner.ValidatePlan(fakePlan, current, goalState));
}

TEST(GOAPPlannerTest, BestGoalSelection)
{
    GOAPPlanner planner;

    GOAPAction heal;
    heal.name = "Heal";
    heal.cost = 1.0f;
    heal.effects.Set("healthy", true);
    planner.AddAction(heal);

    GOAPAction fight;
    fight.name = "Fight";
    fight.cost = 1.0f;
    fight.effects.Set("enemyDead", true);
    planner.AddAction(fight);

    WorldState current;

    std::vector<GOAPGoal> goals;
    GOAPGoal goalHealth;
    goalHealth.name = "StayHealthy";
    goalHealth.targetState.Set("healthy", true);
    goalHealth.priority = 10.0f; // 高優先度
    goals.push_back(goalHealth);

    GOAPGoal goalKill;
    goalKill.name = "KillEnemy";
    goalKill.targetState.Set("enemyDead", true);
    goalKill.priority = 1.0f; // 低優先度
    goals.push_back(goalKill);

    auto plan = planner.MakePlanForBestGoal(current, goals);
    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(plan.actions.size(), 1u);
    EXPECT_EQ(plan.actions[0]->name, "Heal"); // 高優先度のゴールが選ばれる
}

TEST(GOAPPlannerTest, ProceduralPrecondition)
{
    GOAPPlanner planner;

    GOAPAction action;
    action.name = "ConditionalAction";
    action.cost = 1.0f;
    action.effects.Set("done", true);
    action.proceduralPrecondition = [](const WorldState& ws) {
        return ws.Get("ready"); // "ready" が true の場合のみ実行可能
    };
    planner.AddAction(action);

    // ready=falseの場合はプランできない
    WorldState notReady;
    GOAPGoal goal;
    goal.targetState.Set("done", true);

    auto plan1 = planner.MakePlan(notReady, goal);
    EXPECT_FALSE(plan1.valid);

    // ready=trueの場合はプランできる
    WorldState ready;
    ready.Set("ready", true);
    auto plan2 = planner.MakePlan(ready, goal);
    EXPECT_TRUE(plan2.valid);
}

TEST(GOAPPlannerTest, ClearActions)
{
    GOAPPlanner planner;
    GOAPAction a;
    a.name = "a";
    planner.AddAction(a);
    a.name = "b";
    planner.AddAction(a);
    EXPECT_EQ(planner.GetActionCount(), 2);
    planner.Clear();
    EXPECT_EQ(planner.GetActionCount(), 0);
}

TEST(GOAPPlannerTest, PlanCostCalculation)
{
    GOAPPlanner planner;

    GOAPAction step1;
    step1.name = "Step1";
    step1.cost = 3.0f;
    step1.effects.Set("phase1", true);
    planner.AddAction(step1);

    GOAPAction step2;
    step2.name = "Step2";
    step2.cost = 7.0f;
    step2.preconditions.Set("phase1", true);
    step2.effects.Set("done", true);
    planner.AddAction(step2);

    WorldState current;
    GOAPGoal goal;
    goal.targetState.Set("done", true);

    auto plan = planner.MakePlan(current, goal);
    EXPECT_TRUE(plan.valid);
    EXPECT_NEAR(plan.totalCost, 10.0f, 1e-3f); // 3 + 7
}
