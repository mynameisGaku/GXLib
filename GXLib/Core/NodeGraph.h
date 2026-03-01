#pragma once
/// @file NodeGraph.h
/// @brief ビジュアルスクリプティングランタイム（ノードグラフ実行エンジン）
///
/// NodeDef で入出力ピン・実行関数を定義し、
/// NodeGraph でノードの接続・実行を管理する。
/// フローピンによる実行順序制御とデータピンによる値伝搬をサポート。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <DirectXMath.h>
#include <variant>

namespace gx
{

/// @brief ノード間で伝搬する値の型
using NodeValue = std::variant<bool, int, float, std::string, DirectX::XMFLOAT3>;

/// @brief ピンの型
enum class PinType
{
    Flow,     ///< 実行フロー（制御の流れを示す）
    Bool,     ///< 真偽値
    Int,      ///< 整数値
    Float,    ///< 浮動小数点値
    String,   ///< 文字列
    Vector3   ///< 3Dベクトル (XMFLOAT3)
};

/// @brief ピン記述
struct PinDesc
{
    std::string name;  ///< ピン名
    PinType type;      ///< ピンの型
};

class NodeInstance;

/// @brief ノード定義（テンプレート）
class NodeDef
{
public:
    /// @brief コンストラクタ
    /// @param name ノード定義名
    explicit NodeDef(const std::string& name) : m_name(name) {}

    /// @brief 入力ピンを追加する
    /// @param pin ピン記述
    void AddInputPin(const PinDesc& pin) { m_inputPins.push_back(pin); }

    /// @brief 出力ピンを追加する
    /// @param pin ピン記述
    void AddOutputPin(const PinDesc& pin) { m_outputPins.push_back(pin); }

    /// @brief 実行関数を設定する
    /// @param func ノード実行時に呼ばれる関数
    void SetExecute(std::function<void(NodeInstance&)> func) { m_executeFunc = std::move(func); }

    /// @brief ノード定義名を取得する
    /// @return ノード定義名
    const std::string& GetName() const { return m_name; }

    /// @brief 入力ピン一覧を取得する
    /// @return 入力ピン記述の配列
    const std::vector<PinDesc>& GetInputPins() const { return m_inputPins; }

    /// @brief 出力ピン一覧を取得する
    /// @return 出力ピン記述の配列
    const std::vector<PinDesc>& GetOutputPins() const { return m_outputPins; }

    /// @brief 実行関数を取得する
    /// @return 実行関数への参照
    const std::function<void(NodeInstance&)>& GetExecuteFunc() const { return m_executeFunc; }

private:
    std::string m_name;                                ///< ノード定義名
    std::vector<PinDesc> m_inputPins;                  ///< 入力ピン定義リスト
    std::vector<PinDesc> m_outputPins;                 ///< 出力ピン定義リスト
    std::function<void(NodeInstance&)> m_executeFunc;  ///< ノード実行関数
};

/// @brief ノードグラフの実行中のノードインスタンス
class NodeInstance
{
public:
    NodeInstance() = default;

    /// @brief 入力ピンの値を取得する
    /// @param pinIndex ピンインデックス
    /// @return ピンの値（未設定の場合はデフォルト値 0）
    NodeValue GetInput(uint32_t pinIndex) const
    {
        auto it = m_inputs.find(pinIndex);
        if (it != m_inputs.end()) return it->second;
        return NodeValue{ 0 };
    }

    /// @brief 出力ピンに値を設定する
    /// @param pinIndex ピンインデックス
    /// @param value 設定する値
    void SetOutput(uint32_t pinIndex, const NodeValue& value)
    {
        m_outputs[pinIndex] = value;
    }

    /// @brief フロー出力ピンをトリガーする
    /// @param outputFlowPin フロー出力ピンインデックス
    void TriggerFlow(uint32_t outputFlowPin)
    {
        m_triggeredFlows.push_back(outputFlowPin);
    }

    // --- 内部用 ---

    /// @brief 入力ピンに値を設定する（内部用）
    /// @param pinIndex ピンインデックス
    /// @param value 設定する値
    void SetInput(uint32_t pinIndex, const NodeValue& value) { m_inputs[pinIndex] = value; }

    /// @brief 全出力値を取得する（内部用）
    /// @return 出力ピンインデックスと値のマップ
    const std::unordered_map<uint32_t, NodeValue>& GetOutputs() const { return m_outputs; }

    /// @brief トリガーされたフローピン一覧を取得する（内部用）
    /// @return フローピンインデックスの配列
    const std::vector<uint32_t>& GetTriggeredFlows() const { return m_triggeredFlows; }

    /// @brief トリガー済みフローをクリアする（内部用）
    void ClearTriggeredFlows() { m_triggeredFlows.clear(); }

private:
    std::unordered_map<uint32_t, NodeValue> m_inputs;  ///< 入力ピン値マップ
    std::unordered_map<uint32_t, NodeValue> m_outputs; ///< 出力ピン値マップ
    std::vector<uint32_t> m_triggeredFlows;            ///< トリガーされたフローピンリスト
};

/// @brief ノードグラフ（接続・実行管理）
class NodeGraph
{
public:
    /// @brief ノードを追加する
    /// @param defName 使用するノード定義名
    /// @return ノードID (0xFFFFFFFF で失敗)
    uint32_t AddNode(const std::string& defName);

    /// @brief ノードを削除する
    /// @param nodeId ノードID
    void RemoveNode(uint32_t nodeId);

    /// @brief ピンを接続する
    /// @param srcNode 接続元ノードID
    /// @param srcPin 接続元ピンインデックス
    /// @param dstNode 接続先ノードID
    /// @param dstPin 接続先ピンインデックス
    void Connect(uint32_t srcNode, uint32_t srcPin, uint32_t dstNode, uint32_t dstPin);

    /// @brief ピンの接続を切断する
    /// @param srcNode 接続元ノードID
    /// @param srcPin 接続元ピンインデックス
    /// @param dstNode 接続先ノードID
    /// @param dstPin 接続先ピンインデックス
    void Disconnect(uint32_t srcNode, uint32_t srcPin, uint32_t dstNode, uint32_t dstPin);

    /// @brief ノードの入力ピンにデフォルト値を設定する
    /// @param nodeId ノードID
    /// @param pinIndex ピンインデックス
    /// @param value 設定する値
    void SetNodeInput(uint32_t nodeId, uint32_t pinIndex, const NodeValue& value);

    /// @brief グラフを指定ノードから実行する
    /// @param startNodeId 実行開始ノード
    void Execute(uint32_t startNodeId);

    /// @brief ノード数を取得する
    /// @return 現在のノード数
    size_t GetNodeCount() const { return m_nodes.size(); }

    /// @brief ノード定義を登録する（グローバル）
    /// @param def ノード定義（所有権を移譲）
    static void RegisterNodeDef(std::unique_ptr<NodeDef> def);

    /// @brief ノード定義を検索する
    /// @param name ノード定義名
    /// @return ノード定義へのポインタ（見つからない場合 nullptr）
    static const NodeDef* FindNodeDef(const std::string& name);

    /// @brief 全ノード定義をクリアする
    static void ClearNodeDefs();

private:
    /// @brief ノード間の接続情報
    struct Connection
    {
        uint32_t srcNode, srcPin;  ///< 接続元ノードID・ピン
        uint32_t dstNode, dstPin;  ///< 接続先ノードID・ピン
    };

    /// @brief グラフ内のノードデータ
    struct NodeData
    {
        std::string defName;       ///< 使用するノード定義名
        NodeInstance instance;     ///< ノードインスタンス
    };

    /// @brief 接続に基づいて入力値を伝搬する（内部用）
    /// @param nodeId 対象ノードID
    void PropagateInputs(uint32_t nodeId);

    std::unordered_map<uint32_t, NodeData> m_nodes; ///< ノードID→データマップ
    std::vector<Connection> m_connections;           ///< 接続リスト
    uint32_t m_nextNodeId = 0;                       ///< 次に割り当てるノードID

    /// @brief グローバルノード定義テーブルを取得する（内部用）
    /// @return ノード定義配列への参照
    static std::vector<std::unique_ptr<NodeDef>>& GetDefs();
};

} // namespace gx
/// @}
