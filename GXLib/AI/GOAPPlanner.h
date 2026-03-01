#pragma once
/// @file GOAPPlanner.h
/// @brief GOAPプランナー（目標指向アクション計画）
///
/// 「ゴールを達成するために何をすべきか」をA*後方探索で自動計算するAI。
/// ワールドの状態フラグとアクションの前提条件・効果を定義すると、
/// 現在状態からゴールまでの最適なアクション列を出力する。
/// @addtogroup grp_ai/// @{

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace gx {

/// @brief ワールドステート (キーバリュー状態フラグの集合)
class WorldState
{
public:
    /// @brief 状態フラグを設定する
    void Set(const std::string& key, bool value) { m_state[key] = value; }

    /// @brief 状態フラグを取得する (未設定はfalse)
    bool Get(const std::string& key) const
    {
        auto it = m_state.find(key);
        return (it != m_state.end()) ? it->second : false;
    }

    /// @brief キーが設定されているか
    bool Has(const std::string& key) const { return m_state.count(key) > 0; }

    /// @brief other のすべてのキーバリューがこのステートに一致するか
    bool Satisfies(const WorldState& other) const
    {
        for (const auto& [key, value] : other.m_state)
        {
            auto it = m_state.find(key);
            if (it == m_state.end() || it->second != value)
                return false;
        }
        return true;
    }

    /// @brief 全キーバリューを取得する
    const std::unordered_map<std::string, bool>& GetAll() const { return m_state; }

    /// @brief other との不一致キー数 (ヒューリスティック用)
    int Distance(const WorldState& other) const
    {
        int dist = 0;
        for (const auto& [key, value] : other.m_state)
        {
            auto it = m_state.find(key);
            if (it == m_state.end() || it->second != value)
                ++dist;
        }
        return dist;
    }

private:
    std::unordered_map<std::string, bool> m_state;
};

/// @brief GOAPアクション定義
struct GOAPAction
{
    std::string name;                  ///< アクション名
    float       cost = 1.0f;           ///< コスト (正の値)
    WorldState  preconditions;         ///< 前提条件
    WorldState  effects;               ///< 適用効果
    std::function<bool(const WorldState&)> proceduralPrecondition; ///< 手続き的前提条件 (optional)
};

/// @brief GOAPゴール定義
struct GOAPGoal
{
    std::string name;                  ///< ゴール名
    WorldState  targetState;           ///< 達成条件
    float       priority = 1.0f;       ///< 優先度 (高いほど優先)
};

/// @brief 計画結果
struct Plan
{
    std::vector<const GOAPAction*> actions;  ///< アクション列 (実行順)
    float totalCost = 0.0f;                   ///< 合計コスト
    bool  valid     = false;                  ///< 有効な計画か
};

/// @brief プランナー統計情報
struct PlannerStats
{
    int   nodesExplored    = 0;  ///< 探索ノード数
    int   maxOpenListSize  = 0;  ///< オープンリスト最大サイズ
    int   planLength       = 0;  ///< 計画のアクション数
    float planCost         = 0.0f; ///< 計画のコスト
};

/// @brief GOAP プランナー
///
/// A*後方探索を使用してゴールから現在状態へ向かってアクション列を構築する。
/// ヒューリスティックは未充足条件数 (Distance)。
class GOAPPlanner
{
public:
    GOAPPlanner() = default;
    ~GOAPPlanner() = default;

    /// @brief アクションを登録する
    void AddAction(const GOAPAction& action);

    /// @brief アクションを名前で削除する
    bool RemoveAction(const std::string& name);

    /// @brief 登録アクション数
    int GetActionCount() const { return static_cast<int>(m_actions.size()); }

    /// @brief アクションを名前で取得する
    const GOAPAction* GetAction(const std::string& name) const;

    /// @brief アクションが登録されているか
    bool HasAction(const std::string& name) const;

    /// @brief 現在状態からゴールへの計画を作成する (A*後方探索)
    Plan MakePlan(const WorldState& currentState, const GOAPGoal& goal) const;

    /// @brief 最優先ゴールに対して計画を作成する
    Plan MakePlanForBestGoal(const WorldState& currentState, const std::vector<GOAPGoal>& goals) const;

    /// @brief 最後のMakePlanの統計情報
    PlannerStats GetLastStats() const { return m_lastStats; }

    /// @brief 探索の最大反復回数を設定する
    void SetMaxIterations(int n) { m_maxIterations = n; }

    /// @brief 探索の最大反復回数を取得する
    int GetMaxIterations() const { return m_maxIterations; }

    /// @brief 計画を検証する (前方シミュレーション)
    bool ValidatePlan(const Plan& plan, const WorldState& startState, const WorldState& goalState) const;

    /// @brief 全アクションを削除する
    void Clear() { m_actions.clear(); }

private:
    /// @brief A*探索ノード
    struct SearchNode
    {
        WorldState  state;          ///< この時点でのワールドステート要件
        float       g = 0.0f;      ///< 累積コスト
        float       h = 0.0f;      ///< ヒューリスティック
        float       f = 0.0f;      ///< g + h
        int         parentIndex = -1; ///< 親ノードインデックス
        const GOAPAction* action = nullptr; ///< このノードに到達したアクション
    };

    std::vector<GOAPAction> m_actions;
    mutable PlannerStats    m_lastStats = {};
    int                     m_maxIterations = 1000;
};

} // namespace gx
/// @}
