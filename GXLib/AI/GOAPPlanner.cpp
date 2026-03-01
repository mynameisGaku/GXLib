/// @file GOAPPlanner.cpp
/// @brief GOAPPlanner の実装
#include "pch_common.h"

#include "AI/GOAPPlanner.h"
#include <queue>
#include <algorithm>

namespace gx {

// ============================================================================
// AddAction
// ============================================================================
void GOAPPlanner::AddAction(const GOAPAction& action)
{
    // 同名がある場合は上書き
    for (auto& a : m_actions)
    {
        if (a.name == action.name)
        {
            a = action;
            return;
        }
    }
    m_actions.push_back(action);
}

// ============================================================================
// RemoveAction
// ============================================================================
bool GOAPPlanner::RemoveAction(const std::string& name)
{
    for (auto it = m_actions.begin(); it != m_actions.end(); ++it)
    {
        if (it->name == name)
        {
            m_actions.erase(it);
            return true;
        }
    }
    return false;
}

// ============================================================================
// GetAction / HasAction
// ============================================================================
const GOAPAction* GOAPPlanner::GetAction(const std::string& name) const
{
    for (const auto& a : m_actions)
    {
        if (a.name == name)
            return &a;
    }
    return nullptr;
}

bool GOAPPlanner::HasAction(const std::string& name) const
{
    return GetAction(name) != nullptr;
}

// ============================================================================
// MakePlan — A*後方探索
//
// ゴールの要件から出発し、各アクションのeffectsで要件を満たしつつ
// preconditionsを新たな要件として追加していく。
// 現在状態が全要件を満たした時点で計画が完成する。
// ============================================================================
Plan GOAPPlanner::MakePlan(const WorldState& currentState, const GOAPGoal& goal) const
{
    m_lastStats = {};
    Plan result;
    result.valid = false;

    if (m_actions.empty()) return result;

    // 初期ノード: ゴールの要件
    std::vector<SearchNode> allNodes;
    allNodes.reserve(256);

    SearchNode startNode;
    startNode.state = goal.targetState;
    startNode.g     = 0.0f;
    startNode.h     = static_cast<float>(currentState.Distance(goal.targetState));
    startNode.f     = startNode.g + startNode.h;
    startNode.parentIndex = -1;
    startNode.action      = nullptr;
    allNodes.push_back(startNode);

    // オープンリスト (インデックス, fコスト)
    struct OpenEntry
    {
        int   index;
        float f;
        bool operator>(const OpenEntry& other) const { return f > other.f; }
    };

    std::priority_queue<OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>> openList;
    openList.push({ 0, startNode.f });

    int iterations = 0;

    while (!openList.empty() && iterations < m_maxIterations)
    {
        ++iterations;
        m_lastStats.nodesExplored = iterations;

        int openSize = static_cast<int>(openList.size());
        if (openSize > m_lastStats.maxOpenListSize)
            m_lastStats.maxOpenListSize = openSize;

        OpenEntry current = openList.top();
        openList.pop();

        const SearchNode& currentNode = allNodes[static_cast<size_t>(current.index)];

        // ゴールテスト: 現在の状態が全要件を満たすか
        if (currentState.Satisfies(currentNode.state))
        {
            // 計画を逆順で構築
            std::vector<const GOAPAction*> planActions;
            float totalCost = currentNode.g;
            int nodeIdx = current.index;
            while (nodeIdx >= 0)
            {
                const SearchNode& node = allNodes[static_cast<size_t>(nodeIdx)];
                if (node.action)
                    planActions.push_back(node.action);
                nodeIdx = node.parentIndex;
            }
            // planActionsは逆順（ゴール→スタート）なのでそのまま（後方探索なので正しい順序）
            // 実際には: 最後に追加されたアクションが最初に実行される

            result.actions   = planActions;
            result.totalCost = totalCost;
            result.valid     = true;

            m_lastStats.planLength = static_cast<int>(planActions.size());
            m_lastStats.planCost   = totalCost;
            return result;
        }

        // 各アクションで展開
        for (const auto& action : m_actions)
        {
            // アクションのeffectsが現在の要件のいずれかを満たすか？
            bool satisfiesAny = false;
            for (const auto& [key, value] : currentNode.state.GetAll())
            {
                // この要件が現在状態で既に満たされていればスキップ
                if (currentState.Has(key) && currentState.Get(key) == value)
                    continue;

                // アクションのeffectsがこの要件を満たすか
                if (action.effects.Has(key) && action.effects.Get(key) == value)
                {
                    satisfiesAny = true;
                    break;
                }
            }

            if (!satisfiesAny) continue;

            // 手続き的前提条件チェック
            if (action.proceduralPrecondition && !action.proceduralPrecondition(currentState))
                continue;

            // 新しいノードを作成
            // 要件: 現在のstate + アクションのpreconditions - アクションのeffects
            SearchNode newNode;
            newNode.state = currentNode.state;

            // effectsで充足される要件を除去
            WorldState newState;
            for (const auto& [key, value] : currentNode.state.GetAll())
            {
                if (action.effects.Has(key) && action.effects.Get(key) == value)
                    continue; // この要件はアクションが満たす
                newState.Set(key, value);
            }

            // アクションのpreconditionsを新たな要件として追加
            for (const auto& [key, value] : action.preconditions.GetAll())
            {
                newState.Set(key, value);
            }

            newNode.state       = newState;
            newNode.g           = currentNode.g + action.cost;
            newNode.h           = static_cast<float>(currentState.Distance(newState));
            newNode.f           = newNode.g + newNode.h;
            newNode.parentIndex = current.index;
            newNode.action      = &action;

            int newIndex = static_cast<int>(allNodes.size());
            allNodes.push_back(newNode);
            openList.push({ newIndex, newNode.f });
        }
    }

    // 計画が見つからなかった
    return result;
}

// ============================================================================
// MakePlanForBestGoal — 最優先ゴールに対して計画を作成
// ============================================================================
Plan GOAPPlanner::MakePlanForBestGoal(const WorldState& currentState,
                                       const std::vector<GOAPGoal>& goals) const
{
    // 優先度の高い順にソート
    std::vector<const GOAPGoal*> sorted;
    sorted.reserve(goals.size());
    for (const auto& g : goals)
        sorted.push_back(&g);
    std::sort(sorted.begin(), sorted.end(),
              [](const GOAPGoal* a, const GOAPGoal* b) { return a->priority > b->priority; });

    // 優先度順に計画を試行
    for (const auto* g : sorted)
    {
        Plan plan = MakePlan(currentState, *g);
        if (plan.valid)
            return plan;
    }

    return {}; // 有効な計画なし
}

// ============================================================================
// ValidatePlan — 前方シミュレーションで計画を検証
// ============================================================================
bool GOAPPlanner::ValidatePlan(const Plan& plan, const WorldState& startState,
                                const WorldState& goalState) const
{
    if (!plan.valid || plan.actions.empty()) return false;

    WorldState simState = startState;

    for (const auto* action : plan.actions)
    {
        if (!action) return false;

        // 前提条件チェック
        if (!simState.Satisfies(action->preconditions))
            return false;

        // 手続き的前提条件
        if (action->proceduralPrecondition && !action->proceduralPrecondition(simState))
            return false;

        // 効果を適用
        for (const auto& [key, value] : action->effects.GetAll())
            simState.Set(key, value);
    }

    // ゴール状態を満たしているか
    return simState.Satisfies(goalState);
}

} // namespace gx
