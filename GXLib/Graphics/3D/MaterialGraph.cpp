#include "pch_graphics.h"
/// @file MaterialGraph.cpp
/// @brief ノードベースマテリアルグラフ実装

#include "Graphics/3D/MaterialGraph.h"
#include <algorithm>
#include <stack>

namespace gx
{

// ============================================================================
// AddNode
// ============================================================================

int MaterialGraph::AddNode(MaterialNodeType type, const std::string& name)
{
    int id = m_nextId++;
    auto node = std::make_unique<MaterialNode>(id, type, name);
    InitializePins(*node);
    m_nodes[id] = std::move(node);
    return id;
}

// ============================================================================
// InitializePins
// ============================================================================

void MaterialGraph::InitializePins(MaterialNode& node) const
{
    node.inputs.clear();
    node.outputs.clear();

    auto makePin = [](const std::string& pinName, PinType pinType) -> MaterialNodePin
    {
        MaterialNodePin pin;
        pin.name = pinName;
        pin.type = pinType;
        return pin;
    };

    switch (node.type)
    {
    case MaterialNodeType::Float:
        node.outputs.push_back(makePin("Value", PinType::Float));
        break;

    case MaterialNodeType::Vector2:
        node.outputs.push_back(makePin("Value", PinType::Vec2));
        break;

    case MaterialNodeType::Vector3:
        node.outputs.push_back(makePin("Value", PinType::Vec3));
        break;

    case MaterialNodeType::Vector4:
        node.outputs.push_back(makePin("Value", PinType::Vec4));
        break;

    case MaterialNodeType::Texture:
        node.outputs.push_back(makePin("Color", PinType::Vec4));
        break;

    case MaterialNodeType::Add:
    case MaterialNodeType::Subtract:
    case MaterialNodeType::Multiply:
    case MaterialNodeType::Divide:
        node.inputs.push_back(makePin("A", PinType::Float));
        node.inputs.push_back(makePin("B", PinType::Float));
        node.outputs.push_back(makePin("Result", PinType::Float));
        break;

    case MaterialNodeType::Lerp:
        node.inputs.push_back(makePin("A", PinType::Float));
        node.inputs.push_back(makePin("B", PinType::Float));
        node.inputs.push_back(makePin("T", PinType::Float));
        node.outputs.push_back(makePin("Result", PinType::Float));
        break;

    case MaterialNodeType::Clamp:
        node.inputs.push_back(makePin("Value", PinType::Float));
        node.inputs.push_back(makePin("Min", PinType::Float));
        node.inputs.push_back(makePin("Max", PinType::Float));
        node.outputs.push_back(makePin("Result", PinType::Float));
        break;

    case MaterialNodeType::Dot:
        node.inputs.push_back(makePin("A", PinType::Vec3));
        node.inputs.push_back(makePin("B", PinType::Vec3));
        node.outputs.push_back(makePin("Result", PinType::Float));
        break;

    case MaterialNodeType::Normalize:
        node.inputs.push_back(makePin("Value", PinType::Vec3));
        node.outputs.push_back(makePin("Result", PinType::Vec3));
        break;

    case MaterialNodeType::Output:
        node.inputs.push_back(makePin("BaseColor", PinType::Vec3));
        node.inputs.push_back(makePin("Metallic", PinType::Float));
        node.inputs.push_back(makePin("Roughness", PinType::Float));
        node.inputs.push_back(makePin("Normal", PinType::Vec3));
        node.inputs.push_back(makePin("Emissive", PinType::Vec3));
        // Output has no output pins
        break;
    }
}

// ============================================================================
// RemoveNode
// ============================================================================

void MaterialGraph::RemoveNode(int id)
{
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return;

    // 他のノードから、このノードへの接続を切断
    for (auto& [nodeId, node] : m_nodes)
    {
        for (auto& pin : node->inputs)
        {
            if (pin.connected && pin.connectedNodeId == id)
            {
                pin.connected = false;
                pin.connectedNodeId = -1;
                pin.connectedPinIndex = -1;
            }
        }
    }

    if (m_outputNodeId == id)
    {
        m_outputNodeId = -1;
    }

    m_nodes.erase(it);
}

// ============================================================================
// Connect
// ============================================================================

bool MaterialGraph::Connect(int srcNodeId, int srcPinIndex, int dstNodeId, int dstPinIndex)
{
    auto srcIt = m_nodes.find(srcNodeId);
    auto dstIt = m_nodes.find(dstNodeId);
    if (srcIt == m_nodes.end() || dstIt == m_nodes.end()) return false;

    MaterialNode& srcNode = *srcIt->second;
    MaterialNode& dstNode = *dstIt->second;

    if (srcPinIndex < 0 || srcPinIndex >= static_cast<int>(srcNode.outputs.size())) return false;
    if (dstPinIndex < 0 || dstPinIndex >= static_cast<int>(dstNode.inputs.size())) return false;

    // 自己接続を防止
    if (srcNodeId == dstNodeId) return false;

    // 接続を設定
    dstNode.inputs[dstPinIndex].connected = true;
    dstNode.inputs[dstPinIndex].connectedNodeId = srcNodeId;
    dstNode.inputs[dstPinIndex].connectedPinIndex = srcPinIndex;

    return true;
}

// ============================================================================
// Disconnect
// ============================================================================

void MaterialGraph::Disconnect(int dstNodeId, int dstPinIndex)
{
    auto it = m_nodes.find(dstNodeId);
    if (it == m_nodes.end()) return;

    MaterialNode& node = *it->second;
    if (dstPinIndex < 0 || dstPinIndex >= static_cast<int>(node.inputs.size())) return;

    node.inputs[dstPinIndex].connected = false;
    node.inputs[dstPinIndex].connectedNodeId = -1;
    node.inputs[dstPinIndex].connectedPinIndex = -1;
}

// ============================================================================
// GetNode
// ============================================================================

MaterialNode* MaterialGraph::GetNode(int id)
{
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return nullptr;
    return it->second.get();
}

// ============================================================================
// HasCycle (DFS 3-color)
// ============================================================================

bool MaterialGraph::HasCycle() const
{
    // 0=white(未訪問), 1=gray(処理中), 2=black(完了)
    std::unordered_map<int, int> colorMap;
    for (const auto& [id, node] : m_nodes)
    {
        colorMap[id] = 0;
    }

    for (const auto& [id, node] : m_nodes)
    {
        if (colorMap[id] == 0)
        {
            if (DFSHasCycle(id, colorMap)) return true;
        }
    }
    return false;
}

bool MaterialGraph::DFSHasCycle(int nodeId,
                                 std::unordered_map<int, int>& colorMap) const
{
    colorMap[nodeId] = 1; // gray

    auto it = m_nodes.find(nodeId);
    if (it != m_nodes.end())
    {
        const MaterialNode& node = *it->second;
        // 入力ピンの接続先をたどる（逆方向にDFS）
        // → 実際にはグラフの方向はsrc→dstなので、
        //   各ノードの出力が接続されている先を探す
        for (const auto& [otherId, otherNode] : m_nodes)
        {
            for (const auto& pin : otherNode->inputs)
            {
                if (pin.connected && pin.connectedNodeId == nodeId)
                {
                    // nodeId → otherId の辺がある
                    if (colorMap[otherId] == 1) return true;  // gray → サイクル
                    if (colorMap[otherId] == 0)
                    {
                        if (DFSHasCycle(otherId, colorMap)) return true;
                    }
                }
            }
        }
    }

    colorMap[nodeId] = 2; // black
    return false;
}

// ============================================================================
// TopologicalSort
// ============================================================================

std::vector<int> MaterialGraph::TopologicalSort() const
{
    if (HasCycle()) return {};

    std::unordered_map<int, int> colorMap;
    for (const auto& [id, node] : m_nodes)
    {
        colorMap[id] = 0;
    }

    std::vector<int> result;
    for (const auto& [id, node] : m_nodes)
    {
        if (colorMap[id] == 0)
        {
            DFSTopSort(id, colorMap, result);
        }
    }

    std::reverse(result.begin(), result.end());
    return result;
}

void MaterialGraph::DFSTopSort(int nodeId,
                                std::unordered_map<int, int>& colorMap,
                                std::vector<int>& result) const
{
    colorMap[nodeId] = 1;

    // nodeIdの出力先をたどる
    for (const auto& [otherId, otherNode] : m_nodes)
    {
        for (const auto& pin : otherNode->inputs)
        {
            if (pin.connected && pin.connectedNodeId == nodeId)
            {
                if (colorMap[otherId] == 0)
                {
                    DFSTopSort(otherId, colorMap, result);
                }
            }
        }
    }

    colorMap[nodeId] = 2;
    result.push_back(nodeId);
}

// ============================================================================
// Compile
// ============================================================================

MaterialGraphResult MaterialGraph::Compile() const
{
    MaterialGraphResult result;

    if (m_nodes.empty())
    {
        result.success = true;
        return result;
    }

    if (HasCycle())
    {
        result.success = false;
        result.error = "Graph contains a cycle";
        return result;
    }

    std::vector<int> sorted = TopologicalSort();
    if (sorted.empty() && !m_nodes.empty())
    {
        result.success = false;
        result.error = "Topological sort failed";
        return result;
    }

    // 各ノードの出力値を評価
    std::unordered_map<int, std::array<float, 4>> nodeValues;

    for (int nodeId : sorted)
    {
        auto it = m_nodes.find(nodeId);
        if (it == m_nodes.end()) continue;

        const MaterialNode& node = *it->second;
        std::array<float, 4> value = { 0, 0, 0, 0 };

        switch (node.type)
        {
        case MaterialNodeType::Float:
            if (!node.outputs.empty())
            {
                value = { node.outputs[0].defaultValue.x, 0, 0, 0 };
            }
            break;

        case MaterialNodeType::Vector2:
            if (!node.outputs.empty())
            {
                value = { node.outputs[0].defaultValue.x,
                          node.outputs[0].defaultValue.y, 0, 0 };
            }
            break;

        case MaterialNodeType::Vector3:
            if (!node.outputs.empty())
            {
                value = { node.outputs[0].defaultValue.x,
                          node.outputs[0].defaultValue.y,
                          node.outputs[0].defaultValue.z, 0 };
            }
            break;

        case MaterialNodeType::Vector4:
            if (!node.outputs.empty())
            {
                value = { node.outputs[0].defaultValue.x,
                          node.outputs[0].defaultValue.y,
                          node.outputs[0].defaultValue.z,
                          node.outputs[0].defaultValue.w };
            }
            break;

        case MaterialNodeType::Texture:
            result.textureSlots.push_back(node.name);
            value = { 1, 1, 1, 1 }; // テクスチャのデフォルト値
            break;

        case MaterialNodeType::Add:
        {
            std::array<float, 4> a = { 0, 0, 0, 0 }, b = { 0, 0, 0, 0 };
            if (node.inputs.size() >= 2)
            {
                if (node.inputs[0].connected) a = nodeValues[node.inputs[0].connectedNodeId];
                else a = { node.inputs[0].defaultValue.x, node.inputs[0].defaultValue.y,
                           node.inputs[0].defaultValue.z, node.inputs[0].defaultValue.w };
                if (node.inputs[1].connected) b = nodeValues[node.inputs[1].connectedNodeId];
                else b = { node.inputs[1].defaultValue.x, node.inputs[1].defaultValue.y,
                           node.inputs[1].defaultValue.z, node.inputs[1].defaultValue.w };
            }
            value = { a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3] };
            break;
        }

        case MaterialNodeType::Subtract:
        {
            std::array<float, 4> a = { 0, 0, 0, 0 }, b = { 0, 0, 0, 0 };
            if (node.inputs.size() >= 2)
            {
                if (node.inputs[0].connected) a = nodeValues[node.inputs[0].connectedNodeId];
                if (node.inputs[1].connected) b = nodeValues[node.inputs[1].connectedNodeId];
            }
            value = { a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3] };
            break;
        }

        case MaterialNodeType::Multiply:
        {
            std::array<float, 4> a = { 0, 0, 0, 0 }, b = { 0, 0, 0, 0 };
            if (node.inputs.size() >= 2)
            {
                if (node.inputs[0].connected) a = nodeValues[node.inputs[0].connectedNodeId];
                if (node.inputs[1].connected) b = nodeValues[node.inputs[1].connectedNodeId];
            }
            value = { a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3] };
            break;
        }

        case MaterialNodeType::Divide:
        {
            std::array<float, 4> a = { 0, 0, 0, 0 }, b = { 1, 1, 1, 1 };
            if (node.inputs.size() >= 2)
            {
                if (node.inputs[0].connected) a = nodeValues[node.inputs[0].connectedNodeId];
                if (node.inputs[1].connected) b = nodeValues[node.inputs[1].connectedNodeId];
            }
            for (int c = 0; c < 4; ++c)
                value[c] = (std::abs(b[c]) > 1e-10f) ? a[c] / b[c] : 0.0f;
            break;
        }

        case MaterialNodeType::Lerp:
        {
            std::array<float, 4> a = { 0, 0, 0, 0 }, b = { 0, 0, 0, 0 };
            float t = 0.0f;
            if (node.inputs.size() >= 3)
            {
                if (node.inputs[0].connected) a = nodeValues[node.inputs[0].connectedNodeId];
                if (node.inputs[1].connected) b = nodeValues[node.inputs[1].connectedNodeId];
                if (node.inputs[2].connected) t = nodeValues[node.inputs[2].connectedNodeId][0];
            }
            for (int c = 0; c < 4; ++c)
                value[c] = a[c] + (b[c] - a[c]) * t;
            break;
        }

        case MaterialNodeType::Clamp:
        {
            std::array<float, 4> v = { 0, 0, 0, 0 };
            float minVal = 0.0f, maxVal = 1.0f;
            if (node.inputs.size() >= 3)
            {
                if (node.inputs[0].connected) v = nodeValues[node.inputs[0].connectedNodeId];
                if (node.inputs[1].connected) minVal = nodeValues[node.inputs[1].connectedNodeId][0];
                if (node.inputs[2].connected) maxVal = nodeValues[node.inputs[2].connectedNodeId][0];
            }
            for (int c = 0; c < 4; ++c)
                value[c] = std::max(minVal, std::min(maxVal, v[c]));
            break;
        }

        case MaterialNodeType::Dot:
        {
            std::array<float, 4> a = { 0, 0, 0, 0 }, b = { 0, 0, 0, 0 };
            if (node.inputs.size() >= 2)
            {
                if (node.inputs[0].connected) a = nodeValues[node.inputs[0].connectedNodeId];
                if (node.inputs[1].connected) b = nodeValues[node.inputs[1].connectedNodeId];
            }
            float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
            value = { dot, 0, 0, 0 };
            break;
        }

        case MaterialNodeType::Normalize:
        {
            std::array<float, 4> v = { 0, 0, 0, 0 };
            if (!node.inputs.empty() && node.inputs[0].connected)
                v = nodeValues[node.inputs[0].connectedNodeId];
            float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            if (len > 1e-10f)
                value = { v[0] / len, v[1] / len, v[2] / len, 0 };
            break;
        }

        case MaterialNodeType::Output:
            // 出力ノードは入力値を定数として収集
            for (size_t p = 0; p < node.inputs.size(); ++p)
            {
                std::array<float, 4> pinValue = { 0, 0, 0, 0 };
                if (node.inputs[p].connected)
                {
                    pinValue = nodeValues[node.inputs[p].connectedNodeId];
                }
                else
                {
                    pinValue = { node.inputs[p].defaultValue.x,
                                 node.inputs[p].defaultValue.y,
                                 node.inputs[p].defaultValue.z,
                                 node.inputs[p].defaultValue.w };
                }
                result.constants[node.inputs[p].name] = pinValue;
            }
            break;
        }

        nodeValues[nodeId] = value;
    }

    result.success = true;
    return result;
}

// ============================================================================
// Clear
// ============================================================================

void MaterialGraph::Clear()
{
    m_nodes.clear();
    m_nextId = 0;
    m_outputNodeId = -1;
}

// ============================================================================
// SerializeToJSON
// ============================================================================

std::string MaterialGraph::SerializeToJSON() const
{
    std::ostringstream ss;
    ss << "{";

    // Output node
    ss << "\"outputNodeId\":" << m_outputNodeId << ",";
    ss << "\"nextId\":" << m_nextId << ",";

    // Nodes array
    ss << "\"nodes\":[";
    bool firstNode = true;
    for (const auto& [id, node] : m_nodes)
    {
        if (!firstNode) ss << ",";
        firstNode = false;

        ss << "{";
        ss << "\"id\":" << node->id << ",";
        ss << "\"type\":" << static_cast<int>(node->type) << ",";
        ss << "\"name\":\"" << node->name << "\",";
        ss << "\"posX\":" << node->position.x << ",";
        ss << "\"posY\":" << node->position.y << ",";

        // Inputs
        ss << "\"inputs\":[";
        for (size_t i = 0; i < node->inputs.size(); ++i)
        {
            if (i > 0) ss << ",";
            const auto& pin = node->inputs[i];
            ss << "{";
            ss << "\"name\":\"" << pin.name << "\",";
            ss << "\"type\":" << static_cast<int>(pin.type) << ",";
            ss << "\"connected\":" << (pin.connected ? "true" : "false") << ",";
            ss << "\"connectedNodeId\":" << pin.connectedNodeId << ",";
            ss << "\"connectedPinIndex\":" << pin.connectedPinIndex << ",";
            ss << "\"defaultValue\":[" << pin.defaultValue.x << ","
               << pin.defaultValue.y << "," << pin.defaultValue.z << ","
               << pin.defaultValue.w << "]";
            ss << "}";
        }
        ss << "],";

        // Outputs
        ss << "\"outputs\":[";
        for (size_t i = 0; i < node->outputs.size(); ++i)
        {
            if (i > 0) ss << ",";
            const auto& pin = node->outputs[i];
            ss << "{";
            ss << "\"name\":\"" << pin.name << "\",";
            ss << "\"type\":" << static_cast<int>(pin.type) << ",";
            ss << "\"defaultValue\":[" << pin.defaultValue.x << ","
               << pin.defaultValue.y << "," << pin.defaultValue.z << ","
               << pin.defaultValue.w << "]";
            ss << "}";
        }
        ss << "]";

        ss << "}";
    }
    ss << "]";

    ss << "}";
    return ss.str();
}

// ============================================================================
// DeserializeFromJSON (simple parser)
// ============================================================================

bool MaterialGraph::DeserializeFromJSON(const std::string& json)
{
    Clear();

    if (json.empty() || json[0] != '{') return false;

    // 簡易パーサー: outputNodeId, nextId, nodes配列を抽出
    // "outputNodeId":N を検索
    auto findInt = [&](const std::string& key) -> int
    {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return -1;
        pos += search.size();
        return std::atoi(json.c_str() + pos);
    };

    m_outputNodeId = findInt("outputNodeId");
    m_nextId = findInt("nextId");

    // ノード配列を解析
    size_t nodesPos = json.find("\"nodes\":[");
    if (nodesPos == std::string::npos) return true; // ノードなし

    nodesPos += 9; // "nodes":[ の後

    // 各ノードオブジェクトを解析
    int braceDepth = 0;
    size_t nodeStart = std::string::npos;

    for (size_t i = nodesPos; i < json.size(); ++i)
    {
        if (json[i] == '{')
        {
            if (braceDepth == 0) nodeStart = i;
            braceDepth++;
        }
        else if (json[i] == '}')
        {
            braceDepth--;
            if (braceDepth == 0 && nodeStart != std::string::npos)
            {
                std::string nodeJson = json.substr(nodeStart, i - nodeStart + 1);

                // ノードの基本情報を抽出
                auto findIntIn = [&](const std::string& src, const std::string& key) -> int
                {
                    std::string search = "\"" + key + "\":";
                    size_t pos = src.find(search);
                    if (pos == std::string::npos) return 0;
                    pos += search.size();
                    return std::atoi(src.c_str() + pos);
                };

                auto findStringIn = [&](const std::string& src, const std::string& key) -> std::string
                {
                    std::string search = "\"" + key + "\":\"";
                    size_t pos = src.find(search);
                    if (pos == std::string::npos) return "";
                    pos += search.size();
                    size_t endPos = src.find('"', pos);
                    if (endPos == std::string::npos) return "";
                    return src.substr(pos, endPos - pos);
                };

                int nodeId = findIntIn(nodeJson, "id");
                int nodeType = findIntIn(nodeJson, "type");
                std::string nodeName = findStringIn(nodeJson, "name");

                auto node = std::make_unique<MaterialNode>(nodeId, static_cast<MaterialNodeType>(nodeType), nodeName);
                InitializePins(*node);

                // 接続情報を復元: inputs配列内のconnected/connectedNodeId/connectedPinIndex
                size_t inputsPos = nodeJson.find("\"inputs\":[");
                if (inputsPos != std::string::npos)
                {
                    inputsPos += 10;
                    int pinIdx = 0;
                    int pinBrace = 0;
                    int bracketDepth = 0;
                    size_t pinStart = std::string::npos;

                    for (size_t j = inputsPos; j < nodeJson.size(); ++j)
                    {
                        char ch = nodeJson[j];
                        if (ch == '[') bracketDepth++;
                        else if (ch == ']')
                        {
                            if (bracketDepth == 0 && pinBrace == 0) break; // end of inputs array
                            bracketDepth--;
                        }
                        else if (ch == '{')
                        {
                            if (pinBrace == 0) pinStart = j;
                            pinBrace++;
                        }
                        else if (ch == '}')
                        {
                            pinBrace--;
                            if (pinBrace == 0 && pinStart != std::string::npos)
                            {
                                std::string pinJson = nodeJson.substr(pinStart, j - pinStart + 1);

                                if (pinIdx < static_cast<int>(node->inputs.size()))
                                {
                                    bool isConnected = pinJson.find("\"connected\":true") != std::string::npos;
                                    if (isConnected)
                                    {
                                        node->inputs[pinIdx].connected = true;
                                        node->inputs[pinIdx].connectedNodeId = findIntIn(pinJson, "connectedNodeId");
                                        node->inputs[pinIdx].connectedPinIndex = findIntIn(pinJson, "connectedPinIndex");
                                    }
                                }
                                pinIdx++;
                                pinStart = std::string::npos;
                            }
                        }
                    }
                }

                m_nodes[nodeId] = std::move(node);
                nodeStart = std::string::npos;
            }
        }
        else if (json[i] == ']' && braceDepth == 0)
        {
            break; // ノード配列終了
        }
    }

    return true;
}

} // namespace gx
