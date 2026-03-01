/// @file NodeGraph.cpp
/// @brief ノードグラフ実行エンジン実装
#include "pch_common.h"
#include "Core/NodeGraph.h"

namespace gx
{

std::vector<std::unique_ptr<NodeDef>>& NodeGraph::GetDefs()
{
    static std::vector<std::unique_ptr<NodeDef>> defs;
    return defs;
}

void NodeGraph::RegisterNodeDef(std::unique_ptr<NodeDef> def)
{
    GetDefs().push_back(std::move(def));
}

const NodeDef* NodeGraph::FindNodeDef(const std::string& name)
{
    for (const auto& d : GetDefs())
    {
        if (d->GetName() == name) return d.get();
    }
    return nullptr;
}

void NodeGraph::ClearNodeDefs()
{
    GetDefs().clear();
}

uint32_t NodeGraph::AddNode(const std::string& defName)
{
    const NodeDef* def = FindNodeDef(defName);
    if (!def) return 0xFFFFFFFF;

    uint32_t id = m_nextNodeId++;
    NodeData data;
    data.defName = defName;
    m_nodes[id] = std::move(data);
    return id;
}

void NodeGraph::RemoveNode(uint32_t nodeId)
{
    m_nodes.erase(nodeId);

    // 関連する接続も削除
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [nodeId](const Connection& c) {
                return c.srcNode == nodeId || c.dstNode == nodeId;
            }),
        m_connections.end());
}

void NodeGraph::Connect(uint32_t srcNode, uint32_t srcPin, uint32_t dstNode, uint32_t dstPin)
{
    // 既存の重複接続をチェック
    for (const auto& c : m_connections)
    {
        if (c.srcNode == srcNode && c.srcPin == srcPin &&
            c.dstNode == dstNode && c.dstPin == dstPin)
            return;
    }
    m_connections.push_back({ srcNode, srcPin, dstNode, dstPin });
}

void NodeGraph::Disconnect(uint32_t srcNode, uint32_t srcPin, uint32_t dstNode, uint32_t dstPin)
{
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [=](const Connection& c) {
                return c.srcNode == srcNode && c.srcPin == srcPin &&
                       c.dstNode == dstNode && c.dstPin == dstPin;
            }),
        m_connections.end());
}

void NodeGraph::SetNodeInput(uint32_t nodeId, uint32_t pinIndex, const NodeValue& value)
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) return;
    it->second.instance.SetInput(pinIndex, value);
}

void NodeGraph::PropagateInputs(uint32_t nodeId)
{
    auto nodeIt = m_nodes.find(nodeId);
    if (nodeIt == m_nodes.end()) return;

    // 入力ピンに接続されたソースの出力を読み取る
    for (const auto& c : m_connections)
    {
        if (c.dstNode != nodeId) continue;

        auto srcIt = m_nodes.find(c.srcNode);
        if (srcIt == m_nodes.end()) continue;

        const auto& srcOutputs = srcIt->second.instance.GetOutputs();
        auto outIt = srcOutputs.find(c.srcPin);
        if (outIt != srcOutputs.end())
        {
            nodeIt->second.instance.SetInput(c.dstPin, outIt->second);
        }
    }
}

void NodeGraph::Execute(uint32_t startNodeId)
{
    // BFS的にフローピンを辿って実行
    std::vector<uint32_t> queue;
    queue.push_back(startNodeId);

    // 無限ループ防止
    constexpr size_t k_MaxExecutions = 1000;
    size_t execCount = 0;

    while (!queue.empty() && execCount < k_MaxExecutions)
    {
        uint32_t currentId = queue.front();
        queue.erase(queue.begin());
        ++execCount;

        auto nodeIt = m_nodes.find(currentId);
        if (nodeIt == m_nodes.end()) continue;

        const NodeDef* def = FindNodeDef(nodeIt->second.defName);
        if (!def || !def->GetExecuteFunc()) continue;

        // 入力を伝搬
        PropagateInputs(currentId);

        // ノードを実行
        nodeIt->second.instance.ClearTriggeredFlows();
        def->GetExecuteFunc()(nodeIt->second.instance);

        // フロー出力をたどる
        for (uint32_t flowPin : nodeIt->second.instance.GetTriggeredFlows())
        {
            for (const auto& c : m_connections)
            {
                if (c.srcNode == currentId && c.srcPin == flowPin)
                {
                    queue.push_back(c.dstNode);
                }
            }
        }
    }
}

} // namespace gx
