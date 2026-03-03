#pragma once
/// @file DialogueGraph.h
/// @brief 対話グラフ — ノードと分岐で構成された台詞フローを実行する
///
/// Text/Choice/Branch/Random/Jump の5種のノードをグラフ状に接続し、
/// 変数条件や乱数で遷移先を決定する。DialogueSystem より
/// 複雑な分岐が必要なADVやRPGの会話シーンで使う。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <functional>

namespace gx
{

/// @brief ダイアログノードタイプ
enum class DialogueNodeType
{
    Text,     ///< テキスト表示
    Choice,   ///< 選択肢
    Branch,   ///< 条件分岐
    Random,   ///< ランダム遷移
    Jump      ///< 別ノードへジャンプ
};

/// @brief ダイアログ変数値（bool, int, float, stringのいずれか）
using DialogueValue = std::variant<bool, int, float, gx::String>;

/// @brief 子ノードリンク（ノード間の接続）
struct DialogueLink
{
    uint32_t targetNodeId = UINT32_MAX; ///< 遷移先ノードID
    gx::String condition;  ///< 条件式 (空=無条件)
    gx::String label;      ///< 選択肢テキスト
};

/// @brief ダイアログノード（グラフ上の1つの要素）
struct DialogueGraphNode
{
    uint32_t id = UINT32_MAX;                           ///< ノードID（AddNode時に自動付番）
    DialogueNodeType type = DialogueNodeType::Text;     ///< ノードタイプ
    gx::String speaker;                                ///< 話者名
    gx::String text;                                   ///< テキスト内容
    gx::String emotion;                                ///< 感情タグ（演出用）
    gx::Vector<DialogueLink> links;                    ///< 子ノードへのリンク
    gx::String setVariable;                            ///< ノード通過時に設定する変数名
    DialogueValue setValue;                             ///< 設定する値
};

/// @brief ダイアロググラフ
class DialogueGraph
{
public:
    DialogueGraph() = default;
    ~DialogueGraph() = default;

    // --- ノード管理 ---

    /// @brief ノードを追加する
    /// @param type ノードタイプ
    /// @param speaker 話者名（オプション）
    /// @param text テキスト内容（オプション）
    /// @return 割り当てられたノードID
    uint32_t AddNode(DialogueNodeType type, const gx::String& speaker = "", const gx::String& text = "");

    /// @brief ノードを削除する
    /// @param nodeId 削除するノードID
    void RemoveNode(uint32_t nodeId);

    /// @brief ノードを取得する
    /// @param nodeId ノードID
    /// @return ノードへのポインタ（存在しない場合nullptr）
    DialogueGraphNode* GetNode(uint32_t nodeId);

    /// @brief ノードを取得する（const版）
    /// @param nodeId ノードID
    /// @return ノードへのconstポインタ（存在しない場合nullptr）
    const DialogueGraphNode* GetNode(uint32_t nodeId) const;

    /// @brief ノード数を取得する
    /// @return グラフ内のノード総数
    uint32_t GetNodeCount() const { return static_cast<uint32_t>(m_nodes.size()); }

    // --- リンク管理 ---

    /// @brief ノード間にリンクを追加する
    /// @param fromNodeId 遷移元ノードID
    /// @param toNodeId 遷移先ノードID
    /// @param label 選択肢テキスト（オプション）
    /// @param condition 条件式（オプション、空=無条件）
    void AddLink(uint32_t fromNodeId, uint32_t toNodeId, const gx::String& label = "", const gx::String& condition = "");

    /// @brief ノード間のリンクを削除する
    /// @param fromNodeId 遷移元ノードID
    /// @param toNodeId 遷移先ノードID
    void RemoveLink(uint32_t fromNodeId, uint32_t toNodeId);

    // --- 変数システム ---

    /// @brief 変数を設定する
    /// @param name 変数名
    /// @param value 設定する値
    void SetVariable(const gx::String& name, const DialogueValue& value);

    /// @brief 変数の値を取得する
    /// @param name 変数名
    /// @return 変数の値（未設定の場合はデフォルト値）
    DialogueValue GetVariable(const gx::String& name) const;

    /// @brief 変数が存在するか確認する
    /// @param name 変数名
    /// @return 存在すればtrue
    bool HasVariable(const gx::String& name) const;

    /// @brief すべての変数をクリアする
    void ClearVariables();

    // --- グラフ走査 ---

    /// @brief グラフの走査を開始する
    /// @param startNodeId 開始ノードID（デフォルト: 0）
    void Start(uint32_t startNodeId = 0);

    /// @brief 次のノードに進む
    /// @return 進行できた場合true（終端に達した場合false）
    bool Advance();

    /// @brief 選択肢を選択して次のノードに進む
    /// @param choiceIndex 選択肢のインデックス（0始まり）
    void SelectChoice(uint32_t choiceIndex);

    /// @brief 現在のノードを取得する
    /// @return 現在のノードへのconstポインタ（終了済みの場合nullptr）
    const DialogueGraphNode* GetCurrentNode() const;

    /// @brief グラフ走査が終了したか確認する
    /// @return 終了済みならtrue
    bool IsFinished() const { return m_finished; }

    /// @brief 選択肢の入力待ちか確認する
    /// @return Choiceノードで待機中ならtrue
    bool IsWaitingForChoice() const;

    // --- 履歴 ---

    /// @brief 訪問済みノードIDの一覧を取得する
    /// @return 訪問順のノードID配列への参照
    const gx::Vector<uint32_t>& GetVisitedNodes() const { return m_visited; }

    /// @brief 指定ノードを訪問済みか確認する
    /// @param nodeId ノードID
    /// @return 訪問済みならtrue
    bool HasVisited(uint32_t nodeId) const;

    /// @brief 訪問履歴をクリアする
    void ClearHistory();

    // --- シリアライズ ---

    /// @brief グラフ全体を文字列にシリアライズする
    /// @return シリアライズされた文字列データ
    gx::String Save() const;

    /// @brief 文字列からグラフをデシリアライズする
    /// @param data Save()で保存された文字列データ
    /// @return 成功した場合true
    bool Load(const gx::String& data);

    // --- 開始ノード ---

    /// @brief 開始ノードを設定する
    /// @param nodeId 開始ノードID
    void SetStartNode(uint32_t nodeId) { m_startNodeId = nodeId; }

    /// @brief 開始ノードIDを取得する
    /// @return 開始ノードID
    uint32_t GetStartNode() const { return m_startNodeId; }

    /// @brief ノード進入時に呼ばれるコールバック型
    using NodeCallback = std::function<void(const DialogueGraphNode&)>;

    /// @brief ノード進入時のコールバックを設定する
    /// @param cb コールバック関数
    void SetOnNodeEnter(NodeCallback cb) { m_onNodeEnter = std::move(cb); }

private:
    /// @brief 条件式を評価する
    /// @param condition 条件式文字列
    /// @return 条件が成立すればtrue
    bool EvaluateCondition(const gx::String& condition) const;

    /// @brief 指定ノードに進入し、変数設定・コールバック発火を行う
    /// @param nodeId 進入するノードID
    void EnterNode(uint32_t nodeId);

    gx::Vector<DialogueGraphNode> m_nodes;                     ///< 全ノード
    uint32_t m_nextNodeId = 0;                                  ///< 次に割り当てるノードID
    uint32_t m_startNodeId = 0;                                 ///< 開始ノードID
    uint32_t m_currentNodeId = UINT32_MAX;                      ///< 現在のノードID
    bool m_finished = true;                                     ///< 走査終了フラグ

    gx::HashMap<gx::String, DialogueValue> m_variables; ///< 変数テーブル
    gx::Vector<uint32_t> m_visited;                            ///< 訪問済みノード履歴

    NodeCallback m_onNodeEnter;                                 ///< ノード進入コールバック
};

} // namespace gx
/// @}
