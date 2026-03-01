#pragma once
/// @file BehaviorTree.h
/// @brief ビヘイビアツリー（AIの行動決定ツリー）
///
/// 敵AIの行動をツリー構造で記述するフレームワーク。
/// Selector（失敗したら次へ）・Sequence（順番に実行）・Parallel（同時実行）と
/// Action・Condition を組み合わせて「索敵→追跡→攻撃」などの判断を表現する。
/// @addtogroup grp_ai/// @{

#include "pch_common.h"
#include <variant>

namespace gx
{

/// @brief ビヘイビアツリーノードの戻り値
enum class BTStatus
{
    Success,  ///< 成功
    Failure,  ///< 失敗
    Running,  ///< 実行中（次フレームも継続）
};

/// @brief ブラックボード（AI間のデータ共有）
class Blackboard
{
public:
    using Value = std::variant<bool, int, float, std::string, XMFLOAT3>;

    /// @brief キーに値を設定する
    /// @param key キー文字列
    /// @param value 格納する値
    void Set(const std::string& key, const Value& value) { m_data[key] = value; }

    /// @brief キーから値を取得する
    /// @tparam T 取得する型
    /// @param key キー文字列
    /// @param defaultValue キーが存在しない場合のデフォルト値
    /// @return 格納された値、またはデフォルト値
    template<typename T>
    T Get(const std::string& key, const T& defaultValue = T{}) const
    {
        auto it = m_data.find(key);
        if (it == m_data.end()) return defaultValue;
        if (auto* v = std::get_if<T>(&it->second)) return *v;
        return defaultValue;
    }

    /// @brief キーが存在するか確認する
    /// @param key キー文字列
    /// @return キーが存在すればtrue
    bool Has(const std::string& key) const { return m_data.count(key) > 0; }

    /// @brief キーを削除する
    /// @param key 削除するキー文字列
    void Remove(const std::string& key) { m_data.erase(key); }

    /// @brief 全データをクリアする
    void Clear() { m_data.clear(); }

private:
    std::unordered_map<std::string, Value> m_data;  ///< キーバリューデータストア
};

/// @brief ビヘイビアツリーノード基底クラス
class BTNode
{
public:
    virtual ~BTNode() = default;

    /// @brief ノードを1フレーム分実行する
    /// @param dt デルタタイム（秒）
    /// @param bb 共有ブラックボード
    /// @return 実行結果ステータス
    virtual BTStatus Tick(float dt, Blackboard& bb) = 0;

    /// @brief ノードの状態をリセットする
    virtual void Reset() {}
};

// ============================================================================
// 複合ノード
// ============================================================================

/// @brief セレクター（OR）: 子ノードを順に試し、最初のSuccessを返す
class BTSelector : public BTNode
{
public:
    /// @brief 子ノードを追加する
    /// @param child 追加する子ノード
    void AddChild(std::shared_ptr<BTNode> child) { m_children.push_back(std::move(child)); }

    /// @copydoc BTNode::Tick
    BTStatus Tick(float dt, Blackboard& bb) override
    {
        for (size_t i = m_currentChild; i < m_children.size(); ++i)
        {
            BTStatus status = m_children[i]->Tick(dt, bb);
            if (status == BTStatus::Running)
            {
                m_currentChild = i;
                return BTStatus::Running;
            }
            if (status == BTStatus::Success)
            {
                m_currentChild = 0;
                return BTStatus::Success;
            }
        }
        m_currentChild = 0;
        return BTStatus::Failure;
    }

    void Reset() override
    {
        m_currentChild = 0;
        for (auto& child : m_children) child->Reset();
    }

private:
    std::vector<std::shared_ptr<BTNode>> m_children;  ///< 子ノードリスト
    size_t m_currentChild = 0;                         ///< 現在実行中の子ノードインデックス
};

/// @brief シーケンス（AND）: 子ノードを順に実行し、最初のFailureを返す
class BTSequence : public BTNode
{
public:
    /// @brief 子ノードを追加する
    /// @param child 追加する子ノード
    void AddChild(std::shared_ptr<BTNode> child) { m_children.push_back(std::move(child)); }

    /// @copydoc BTNode::Tick
    BTStatus Tick(float dt, Blackboard& bb) override
    {
        for (size_t i = m_currentChild; i < m_children.size(); ++i)
        {
            BTStatus status = m_children[i]->Tick(dt, bb);
            if (status == BTStatus::Running)
            {
                m_currentChild = i;
                return BTStatus::Running;
            }
            if (status == BTStatus::Failure)
            {
                m_currentChild = 0;
                return BTStatus::Failure;
            }
        }
        m_currentChild = 0;
        return BTStatus::Success;
    }

    void Reset() override
    {
        m_currentChild = 0;
        for (auto& child : m_children) child->Reset();
    }

private:
    std::vector<std::shared_ptr<BTNode>> m_children;  ///< 子ノードリスト
    size_t m_currentChild = 0;                         ///< 現在実行中の子ノードインデックス
};

/// @brief パラレルポリシー
enum class BTParallelPolicy
{
    RequireAll,  ///< 全成功でSuccess、1つでもFailureでFailure
    RequireOne,  ///< 1つ成功でSuccess
};

/// @brief パラレル: 全子ノードを同時実行
class BTParallel : public BTNode
{
public:
    /// @brief コンストラクタ
    /// @param policy 並列実行ポリシー
    explicit BTParallel(BTParallelPolicy policy = BTParallelPolicy::RequireAll)
        : m_policy(policy) {}

    /// @brief 子ノードを追加する
    /// @param child 追加する子ノード
    void AddChild(std::shared_ptr<BTNode> child) { m_children.push_back(std::move(child)); }

    /// @copydoc BTNode::Tick
    BTStatus Tick(float dt, Blackboard& bb) override
    {
        int successCount = 0, failureCount = 0;
        for (auto& child : m_children)
        {
            BTStatus status = child->Tick(dt, bb);
            if (status == BTStatus::Success) successCount++;
            else if (status == BTStatus::Failure) failureCount++;
        }

        if (m_policy == BTParallelPolicy::RequireAll)
        {
            if (failureCount > 0) return BTStatus::Failure;
            if (successCount == static_cast<int>(m_children.size())) return BTStatus::Success;
        }
        else
        {
            if (successCount > 0) return BTStatus::Success;
            if (failureCount == static_cast<int>(m_children.size())) return BTStatus::Failure;
        }
        return BTStatus::Running;
    }

    void Reset() override
    {
        for (auto& child : m_children) child->Reset();
    }

private:
    std::vector<std::shared_ptr<BTNode>> m_children;  ///< 子ノードリスト
    BTParallelPolicy m_policy;                         ///< 並列実行ポリシー
};

// ============================================================================
// デコレータ
// ============================================================================

/// @brief デコレータ基底
class BTDecorator : public BTNode
{
public:
    /// @brief 装飾対象の子ノードを設定する
    /// @param child 子ノード
    void SetChild(std::shared_ptr<BTNode> child) { m_child = std::move(child); }
    void Reset() override { if (m_child) m_child->Reset(); }
protected:
    std::shared_ptr<BTNode> m_child;  ///< 装飾対象の子ノード
};

/// @brief インバータ: 子の結果を反転する
class BTInverter : public BTDecorator
{
public:
    BTStatus Tick(float dt, Blackboard& bb) override
    {
        if (!m_child) return BTStatus::Failure;
        BTStatus status = m_child->Tick(dt, bb);
        if (status == BTStatus::Success) return BTStatus::Failure;
        if (status == BTStatus::Failure) return BTStatus::Success;
        return BTStatus::Running;
    }
};

/// @brief リピーター: 子を指定回数繰り返す（0=無限）
class BTRepeater : public BTDecorator
{
public:
    /// @brief コンストラクタ
    /// @param count 最大繰り返し回数（0=無限）
    explicit BTRepeater(int count = 0) : m_maxCount(count) {}

    BTStatus Tick(float dt, Blackboard& bb) override
    {
        if (!m_child) return BTStatus::Failure;
        BTStatus status = m_child->Tick(dt, bb);
        if (status == BTStatus::Running) return BTStatus::Running;

        m_count++;
        if (m_maxCount > 0 && m_count >= m_maxCount)
        {
            m_count = 0;
            return status;
        }
        m_child->Reset();
        return BTStatus::Running;
    }

    void Reset() override
    {
        m_count = 0;
        BTDecorator::Reset();
    }

private:
    int m_maxCount = 0;  ///< 最大繰り返し回数（0=無限）
    int m_count    = 0;  ///< 現在の繰り返し回数
};

/// @brief サクシーダー: 子の結果に関わらずSuccessを返す
class BTSucceeder : public BTDecorator
{
public:
    BTStatus Tick(float dt, Blackboard& bb) override
    {
        if (m_child) m_child->Tick(dt, bb);
        return BTStatus::Success;
    }
};

// ============================================================================
// リーフノード
// ============================================================================

/// @brief アクションノード: 関数をラップしてリーフとして使う
class BTAction : public BTNode
{
public:
    /// @brief コンストラクタ
    /// @param func 実行するアクション関数
    explicit BTAction(std::function<BTStatus(float, Blackboard&)> func)
        : m_func(std::move(func)) {}

    BTStatus Tick(float dt, Blackboard& bb) override
    {
        return m_func ? m_func(dt, bb) : BTStatus::Failure;
    }

private:
    std::function<BTStatus(float, Blackboard&)> m_func;  ///< アクション実行関数
};

/// @brief コンディションノード: 条件判定
class BTCondition : public BTNode
{
public:
    /// @brief コンストラクタ
    /// @param func 条件判定関数
    explicit BTCondition(std::function<bool(const Blackboard&)> func)
        : m_func(std::move(func)) {}

    BTStatus Tick(float dt, Blackboard& bb) override
    {
        return (m_func && m_func(bb)) ? BTStatus::Success : BTStatus::Failure;
    }

private:
    std::function<bool(const Blackboard&)> m_func;  ///< 条件判定関数
};

// ============================================================================
// ビヘイビアツリー本体
// ============================================================================

/// @brief ビヘイビアツリー
class BehaviorTree
{
public:
    /// @brief ルートノードを設定する
    /// @param root ルートノード
    void SetRoot(std::shared_ptr<BTNode> root) { m_root = std::move(root); }

    /// @brief ツリーを1フレーム分実行する
    /// @param dt デルタタイム
    /// @return 実行結果
    BTStatus Tick(float dt)
    {
        if (!m_root) return BTStatus::Failure;
        return m_root->Tick(dt, m_blackboard);
    }

    /// @brief ブラックボードを取得する
    /// @return ブラックボードへの参照
    Blackboard& GetBlackboard() { return m_blackboard; }
    /// @brief ブラックボードを取得する（const版）
    /// @return ブラックボードへのconst参照
    const Blackboard& GetBlackboard() const { return m_blackboard; }

    /// @brief ツリーをリセットする
    void Reset()
    {
        if (m_root) m_root->Reset();
    }

private:
    std::shared_ptr<BTNode> m_root;  ///< ルートノード
    Blackboard m_blackboard;         ///< 共有ブラックボード
};

} // namespace gx
/// @}
