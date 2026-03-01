/// @file ShaderGraph.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/ShaderGraph.h"
#include "Core/Logger.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stack>
#include <unordered_set>

namespace gx
{

// ============================================================================
// コンストラクタ — Output ノードを自動追加
// ============================================================================

ShaderGraph::ShaderGraph()
{
    ShaderNode output;
    output.id   = m_nextNodeId++;
    output.type = ShaderNodeType::Output;
    output.name = "Output";
    CreateDefaultPins(output);
    m_nodes.push_back(std::move(output));
}

// ============================================================================
// ノード操作
// ============================================================================

uint32_t ShaderGraph::AddNode(ShaderNodeType type, const std::string& name,
                               float posX, float posY)
{
    // Output ノードは 1 つしか存在できない
    if (type == ShaderNodeType::Output)
        return 0; // 既存の Output を返す

    ShaderNode node;
    node.id   = m_nextNodeId++;
    node.type = type;
    node.name = name.empty() ? ("Node_" + std::to_string(node.id)) : name;
    node.posX = posX;
    node.posY = posY;
    CreateDefaultPins(node);

    uint32_t id = node.id;
    m_nodes.push_back(std::move(node));
    return id;
}

bool ShaderGraph::RemoveNode(uint32_t nodeId)
{
    // Output ノードは削除不可
    if (nodeId == 0)
        return false;

    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [nodeId](const ShaderNode& n) { return n.id == nodeId; });
    if (it == m_nodes.end())
        return false;

    m_nodes.erase(it);

    // 関連接続を除去
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [nodeId](const ShaderConnection& c) {
                return c.fromNode == nodeId || c.toNode == nodeId;
            }),
        m_connections.end());

    return true;
}

const ShaderNode* ShaderGraph::GetNode(uint32_t nodeId) const
{
    return FindNodeConst(nodeId);
}

ShaderNode* ShaderGraph::FindNode(uint32_t nodeId)
{
    for (auto& n : m_nodes)
        if (n.id == nodeId) return &n;
    return nullptr;
}

const ShaderNode* ShaderGraph::FindNodeConst(uint32_t nodeId) const
{
    for (const auto& n : m_nodes)
        if (n.id == nodeId) return &n;
    return nullptr;
}

// ============================================================================
// 接続
// ============================================================================

bool ShaderGraph::Connect(uint32_t fromNode, uint32_t fromPin,
                           uint32_t toNode, uint32_t toPin)
{
    if (!ValidateConnection(fromNode, fromPin, toNode, toPin))
        return false;

    // 既に同じ接続が存在する場合は追加しない
    for (const auto& c : m_connections)
    {
        if (c.fromNode == fromNode && c.fromPin == fromPin &&
            c.toNode == toNode && c.toPin == toPin)
            return true;
    }

    ShaderConnection conn;
    conn.fromNode = fromNode;
    conn.fromPin  = fromPin;
    conn.toNode   = toNode;
    conn.toPin    = toPin;
    m_connections.push_back(conn);
    return true;
}

bool ShaderGraph::Disconnect(uint32_t fromNode, uint32_t fromPin,
                              uint32_t toNode, uint32_t toPin)
{
    auto it = std::remove_if(m_connections.begin(), m_connections.end(),
        [&](const ShaderConnection& c) {
            return c.fromNode == fromNode && c.fromPin == fromPin &&
                   c.toNode == toNode && c.toPin == toPin;
        });
    if (it == m_connections.end())
        return false;
    m_connections.erase(it, m_connections.end());
    return true;
}

bool ShaderGraph::ValidateConnection(uint32_t fromNode, uint32_t fromPin,
                                      uint32_t toNode, uint32_t toPin) const
{
    // 自己接続は不可
    if (fromNode == toNode)
        return false;

    const ShaderNode* srcNode = FindNodeConst(fromNode);
    const ShaderNode* dstNode = FindNodeConst(toNode);
    if (!srcNode || !dstNode)
        return false;

    // ピンインデックスの範囲チェック
    // 出力ピンを検索
    const ShaderPin* srcPin = nullptr;
    for (const auto& p : srcNode->pins)
    {
        if (p.pinIndex == fromPin && p.isOutput)
        {
            srcPin = &p;
            break;
        }
    }

    const ShaderPin* dstPin = nullptr;
    for (const auto& p : dstNode->pins)
    {
        if (p.pinIndex == toPin && !p.isOutput)
        {
            dstPin = &p;
            break;
        }
    }

    if (!srcPin || !dstPin)
        return false;

    return AreTypesCompatible(srcPin->type, dstPin->type);
}

bool ShaderGraph::AreTypesCompatible(ShaderPinType from, ShaderPinType to)
{
    // 同じ型は常に互換
    if (from == to)
        return true;

    // Float は Float2/3/4 に自動スプラット可能
    if (from == ShaderPinType::Float &&
        (to == ShaderPinType::Float2 || to == ShaderPinType::Float3 || to == ShaderPinType::Float4))
        return true;

    // Float2/3/4 間は暗黙変換可 (HLSL のスウィズル / 拡張)
    if (from != ShaderPinType::Texture2D && from != ShaderPinType::Sampler &&
        to != ShaderPinType::Texture2D && to != ShaderPinType::Sampler)
        return true;

    return false;
}

// ============================================================================
// サイクル検出 (DFS)
// ============================================================================

bool ShaderGraph::HasCycle() const
{
    // 隣接リストを構築
    std::unordered_map<uint32_t, std::vector<uint32_t>> adj;
    for (const auto& n : m_nodes)
        adj[n.id]; // 空リストを保証

    for (const auto& c : m_connections)
        adj[c.fromNode].push_back(c.toNode);

    enum class Color { White, Gray, Black };
    std::unordered_map<uint32_t, Color> color;
    for (const auto& n : m_nodes)
        color[n.id] = Color::White;

    // DFS
    std::function<bool(uint32_t)> dfs = [&](uint32_t u) -> bool
    {
        color[u] = Color::Gray;
        for (uint32_t v : adj[u])
        {
            if (color[v] == Color::Gray)
                return true; // 後退辺 → サイクル
            if (color[v] == Color::White && dfs(v))
                return true;
        }
        color[u] = Color::Black;
        return false;
    };

    for (const auto& n : m_nodes)
    {
        if (color[n.id] == Color::White && dfs(n.id))
            return true;
    }
    return false;
}

// ============================================================================
// トポロジカルソート (Kahn's algorithm)
// ============================================================================

std::vector<uint32_t> ShaderGraph::TopologicalSort() const
{
    std::unordered_map<uint32_t, uint32_t> inDegree;
    std::unordered_map<uint32_t, std::vector<uint32_t>> adj;
    for (const auto& n : m_nodes)
    {
        inDegree[n.id] = 0;
        adj[n.id];
    }

    for (const auto& c : m_connections)
    {
        adj[c.fromNode].push_back(c.toNode);
        inDegree[c.toNode]++;
    }

    std::vector<uint32_t> queue;
    for (const auto& [id, deg] : inDegree)
    {
        if (deg == 0) queue.push_back(id);
    }

    std::vector<uint32_t> sorted;
    while (!queue.empty())
    {
        uint32_t u = queue.back();
        queue.pop_back();
        sorted.push_back(u);
        for (uint32_t v : adj[u])
        {
            if (--inDegree[v] == 0)
                queue.push_back(v);
        }
    }

    return sorted;
}

// ============================================================================
// コンパイル
// ============================================================================

CompileResult ShaderGraph::Compile() const
{
    CompileResult result;

    // 検証
    auto errors = Validate();
    if (!errors.empty())
    {
        result.errors = std::move(errors);
        result.success = false;
        return result;
    }

    // サイクルチェック
    if (HasCycle())
    {
        result.errors.push_back("Graph contains a cycle");
        result.success = false;
        return result;
    }

    auto sorted = TopologicalSort();

    std::ostringstream hlsl;
    hlsl << "// Auto-generated shader - Target: " << m_targetModel << "\n";
    hlsl << "// Node count: " << m_nodes.size() << "\n\n";

    // ノードごとにコード生成
    uint32_t index = 0;
    for (uint32_t nodeId : sorted)
    {
        const ShaderNode* node = FindNodeConst(nodeId);
        if (!node) continue;
        std::string code = GenerateHLSLForNode(*node, index);
        if (!code.empty())
            hlsl << code << "\n";
        index++;
    }

    result.hlslCode = hlsl.str();
    result.success  = true;
    return result;
}

std::string ShaderGraph::GenerateHLSLForNode(const ShaderNode& node, uint32_t index) const
{
    std::string varName = "node_" + std::to_string(node.id);

    switch (node.type)
    {
    case ShaderNodeType::Output:
        return "// Output node";

    case ShaderNodeType::ConstFloat:
    {
        float val = 0.0f;
        if (!node.pins.empty()) val = node.pins[0].defaultValue[0];
        return "float " + varName + " = " + std::to_string(val) + ";";
    }

    case ShaderNodeType::ConstFloat2:
        return "float2 " + varName + " = float2(0, 0);";

    case ShaderNodeType::ConstFloat3:
    {
        float r = 0.0f, g = 0.0f, b = 0.0f;
        if (!node.pins.empty())
        {
            r = node.pins[0].defaultValue[0];
            g = node.pins[0].defaultValue[1];
            b = node.pins[0].defaultValue[2];
        }
        return "float3 " + varName + " = float3(" +
               std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ");";
    }

    case ShaderNodeType::ConstFloat4:
        return "float4 " + varName + " = float4(0, 0, 0, 0);";

    case ShaderNodeType::ConstColor:
        return "float4 " + varName + " = float4(1, 1, 1, 1);";

    case ShaderNodeType::Multiply:
        return "// " + varName + " = A * B";

    case ShaderNodeType::Add:
        return "// " + varName + " = A + B";

    case ShaderNodeType::Subtract:
        return "// " + varName + " = A - B";

    case ShaderNodeType::Divide:
        return "// " + varName + " = A / B";

    case ShaderNodeType::Dot:
        return "float " + varName + " = dot(A, B);";

    case ShaderNodeType::Cross:
        return "float3 " + varName + " = cross(A, B);";

    case ShaderNodeType::Normalize:
        return "float3 " + varName + " = normalize(In);";

    case ShaderNodeType::Lerp:
        return "// " + varName + " = lerp(A, B, T)";

    case ShaderNodeType::Clamp:
        return "// " + varName + " = clamp(In, Min, Max)";

    case ShaderNodeType::Saturate:
        return "// " + varName + " = saturate(In)";

    case ShaderNodeType::Power:
        return "// " + varName + " = pow(Base, Exp)";

    case ShaderNodeType::SquareRoot:
        return "// " + varName + " = sqrt(In)";

    case ShaderNodeType::Abs:
        return "// " + varName + " = abs(In)";

    case ShaderNodeType::Frac:
        return "// " + varName + " = frac(In)";

    case ShaderNodeType::Floor:
        return "// " + varName + " = floor(In)";

    case ShaderNodeType::Ceil:
        return "// " + varName + " = ceil(In)";

    case ShaderNodeType::Sin:
        return "float " + varName + " = sin(In);";

    case ShaderNodeType::Cos:
        return "float " + varName + " = cos(In);";

    case ShaderNodeType::TextureSample:
        return "float4 " + varName + " = tex.Sample(samp, uv);";

    case ShaderNodeType::Time:
        return "float " + varName + " = _Time;";

    case ShaderNodeType::UV:
        return "float2 " + varName + " = input.uv;";

    case ShaderNodeType::WorldPos:
        return "float3 " + varName + " = input.worldPos;";

    case ShaderNodeType::WorldNormal:
        return "float3 " + varName + " = input.worldNormal;";

    case ShaderNodeType::ViewDir:
        return "float3 " + varName + " = normalize(_CameraPos - input.worldPos);";

    case ShaderNodeType::Fresnel:
        return "float " + varName + " = pow(1.0 - saturate(dot(N, V)), Power);";

    case ShaderNodeType::NormalMap:
        return "float3 " + varName + " = UnpackNormal(tex.Sample(samp, uv));";

    case ShaderNodeType::Parallax:
        return "float2 " + varName + " = ParallaxMapping(uv, viewDir);";

    default:
        return "// Unknown node type";
    }
}

std::string ShaderGraph::PinTypeToHLSL(ShaderPinType type)
{
    switch (type)
    {
    case ShaderPinType::Float:     return "float";
    case ShaderPinType::Float2:    return "float2";
    case ShaderPinType::Float3:    return "float3";
    case ShaderPinType::Float4:    return "float4";
    case ShaderPinType::Texture2D: return "Texture2D";
    case ShaderPinType::Sampler:   return "SamplerState";
    default:                       return "float";
    }
}

// ============================================================================
// 検証
// ============================================================================

std::vector<std::string> ShaderGraph::Validate() const
{
    std::vector<std::string> errors;

    // Output ノードが存在するか
    const ShaderNode* output = GetOutputNode();
    if (!output)
    {
        errors.push_back("No output node found");
        return errors;
    }

    // 各入力ピンに接続があるかチェック
    for (const auto& node : m_nodes)
    {
        for (const auto& pin : node.pins)
        {
            if (pin.isOutput) continue;

            // Output ノードの入力ピンのうち、BaseColor は必須
            if (node.type == ShaderNodeType::Output && pin.name == "BaseColor")
            {
                bool connected = false;
                for (const auto& c : m_connections)
                {
                    if (c.toNode == node.id && c.toPin == pin.pinIndex)
                    {
                        connected = true;
                        break;
                    }
                }
                if (!connected)
                {
                    errors.push_back("Output.BaseColor is not connected");
                }
            }
        }
    }

    return errors;
}

// ============================================================================
// シリアライズ
// ============================================================================

bool ShaderGraph::Save(const std::string& filePath) const
{
    std::ofstream file(filePath);
    if (!file.is_open())
        return false;

    file << "SHADERGRAPH\n";
    file << "target " << m_targetModel << "\n";
    file << "nodes " << m_nodes.size() << "\n";

    for (const auto& node : m_nodes)
    {
        file << "node " << node.id << " " << static_cast<uint32_t>(node.type)
             << " " << node.posX << " " << node.posY << " " << node.name << "\n";

        // プロパティ
        file << "props " << node.properties.size() << "\n";
        for (const auto& [key, val] : node.properties)
        {
            file << "prop " << key << " " << val << "\n";
        }
    }

    file << "connections " << m_connections.size() << "\n";
    for (const auto& c : m_connections)
    {
        file << "conn " << c.fromNode << " " << c.fromPin
             << " " << c.toNode << " " << c.toPin << "\n";
    }

    return true;
}

bool ShaderGraph::Load(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return false;

    std::string line;
    // Header
    if (!std::getline(file, line) || line != "SHADERGRAPH")
        return false;

    // Clear current state (keep generating new IDs)
    m_nodes.clear();
    m_connections.clear();
    m_nextNodeId = 0;

    // Target
    if (std::getline(file, line))
    {
        if (line.substr(0, 7) == "target ")
            m_targetModel = line.substr(7);
    }

    // Nodes
    uint32_t nodeCount = 0;
    if (std::getline(file, line))
    {
        if (line.substr(0, 6) == "nodes ")
            nodeCount = static_cast<uint32_t>(std::stoul(line.substr(6)));
    }

    for (uint32_t i = 0; i < nodeCount; ++i)
    {
        if (!std::getline(file, line)) break;

        ShaderNode node;
        std::istringstream iss(line);
        std::string token;
        iss >> token; // "node"
        uint32_t typeVal = 0;
        iss >> node.id >> typeVal >> node.posX >> node.posY;
        node.type = static_cast<ShaderNodeType>(typeVal);

        // 残りの文字列が名前
        std::getline(iss, node.name);
        if (!node.name.empty() && node.name[0] == ' ')
            node.name = node.name.substr(1);

        CreateDefaultPins(node);

        if (node.id >= m_nextNodeId)
            m_nextNodeId = node.id + 1;

        // プロパティ
        uint32_t propCount = 0;
        if (std::getline(file, line))
        {
            if (line.substr(0, 6) == "props ")
                propCount = static_cast<uint32_t>(std::stoul(line.substr(6)));
        }
        for (uint32_t p = 0; p < propCount; ++p)
        {
            if (!std::getline(file, line)) break;
            // "prop key value"
            std::istringstream pss(line);
            std::string propToken, key, value;
            pss >> propToken >> key;
            std::getline(pss, value);
            if (!value.empty() && value[0] == ' ')
                value = value.substr(1);
            node.properties[key] = value;
        }

        m_nodes.push_back(std::move(node));
    }

    // Connections
    uint32_t connCount = 0;
    if (std::getline(file, line))
    {
        if (line.substr(0, 12) == "connections ")
            connCount = static_cast<uint32_t>(std::stoul(line.substr(12)));
    }

    for (uint32_t i = 0; i < connCount; ++i)
    {
        if (!std::getline(file, line)) break;
        std::istringstream css(line);
        std::string token;
        ShaderConnection c;
        css >> token >> c.fromNode >> c.fromPin >> c.toNode >> c.toPin;
        m_connections.push_back(c);
    }

    return true;
}

// ============================================================================
// Clear / GetOutputNode / ピン操作
// ============================================================================

void ShaderGraph::Clear()
{
    m_nodes.clear();
    m_connections.clear();
    m_nextNodeId = 0;

    // Output ノードを再追加
    ShaderNode output;
    output.id   = m_nextNodeId++;
    output.type = ShaderNodeType::Output;
    output.name = "Output";
    CreateDefaultPins(output);
    m_nodes.push_back(std::move(output));
}

const ShaderNode* ShaderGraph::GetOutputNode() const
{
    for (const auto& n : m_nodes)
    {
        if (n.type == ShaderNodeType::Output)
            return &n;
    }
    return nullptr;
}

std::vector<ShaderPin> ShaderGraph::GetNodePins(uint32_t nodeId) const
{
    const ShaderNode* node = FindNodeConst(nodeId);
    if (!node) return {};
    return node->pins;
}

void ShaderGraph::SetNodeProperty(uint32_t nodeId, const std::string& key, const std::string& value)
{
    ShaderNode* node = FindNode(nodeId);
    if (node) node->properties[key] = value;
}

std::string ShaderGraph::GetNodeProperty(uint32_t nodeId, const std::string& key) const
{
    const ShaderNode* node = FindNodeConst(nodeId);
    if (!node) return "";
    auto it = node->properties.find(key);
    if (it != node->properties.end()) return it->second;
    return "";
}

// ============================================================================
// デフォルトピン生成
// ============================================================================

void ShaderGraph::CreateDefaultPins(ShaderNode& node) const
{
    node.pins.clear();
    uint32_t inIdx  = 0;
    uint32_t outIdx = 0;

    auto addInput  = [&](const std::string& name, ShaderPinType type) {
        ShaderPin pin;
        pin.name     = name;
        pin.type     = type;
        pin.nodeId   = node.id;
        pin.pinIndex = inIdx++;
        pin.isOutput = false;
        node.pins.push_back(pin);
    };

    auto addOutput = [&](const std::string& name, ShaderPinType type) {
        ShaderPin pin;
        pin.name     = name;
        pin.type     = type;
        pin.nodeId   = node.id;
        pin.pinIndex = outIdx++;
        pin.isOutput = true;
        node.pins.push_back(pin);
    };

    switch (node.type)
    {
    case ShaderNodeType::Output:
        addInput("BaseColor",  ShaderPinType::Float3);
        addInput("Metallic",   ShaderPinType::Float);
        addInput("Roughness",  ShaderPinType::Float);
        addInput("Normal",     ShaderPinType::Float3);
        addInput("Emissive",   ShaderPinType::Float3);
        addInput("Opacity",    ShaderPinType::Float);
        break;

    case ShaderNodeType::TextureSample:
        addInput("UV",      ShaderPinType::Float2);
        addInput("Texture", ShaderPinType::Texture2D);
        addInput("Sampler", ShaderPinType::Sampler);
        addOutput("RGBA",  ShaderPinType::Float4);
        addOutput("RGB",   ShaderPinType::Float3);
        addOutput("R",     ShaderPinType::Float);
        addOutput("G",     ShaderPinType::Float);
        addOutput("B",     ShaderPinType::Float);
        addOutput("A",     ShaderPinType::Float);
        break;

    case ShaderNodeType::ConstFloat:
        addOutput("Value", ShaderPinType::Float);
        break;

    case ShaderNodeType::ConstFloat2:
        addOutput("Value", ShaderPinType::Float2);
        break;

    case ShaderNodeType::ConstFloat3:
        addOutput("Value", ShaderPinType::Float3);
        break;

    case ShaderNodeType::ConstFloat4:
        addOutput("Value", ShaderPinType::Float4);
        break;

    case ShaderNodeType::ConstColor:
        addOutput("Color", ShaderPinType::Float4);
        break;

    case ShaderNodeType::Multiply:
    case ShaderNodeType::Add:
    case ShaderNodeType::Subtract:
    case ShaderNodeType::Divide:
        addInput("A", ShaderPinType::Float);
        addInput("B", ShaderPinType::Float);
        addOutput("Result", ShaderPinType::Float);
        break;

    case ShaderNodeType::Dot:
        addInput("A", ShaderPinType::Float3);
        addInput("B", ShaderPinType::Float3);
        addOutput("Result", ShaderPinType::Float);
        break;

    case ShaderNodeType::Cross:
        addInput("A", ShaderPinType::Float3);
        addInput("B", ShaderPinType::Float3);
        addOutput("Result", ShaderPinType::Float3);
        break;

    case ShaderNodeType::Normalize:
        addInput("In", ShaderPinType::Float3);
        addOutput("Result", ShaderPinType::Float3);
        break;

    case ShaderNodeType::Lerp:
        addInput("A", ShaderPinType::Float);
        addInput("B", ShaderPinType::Float);
        addInput("T", ShaderPinType::Float);
        addOutput("Result", ShaderPinType::Float);
        break;

    case ShaderNodeType::Clamp:
        addInput("In",  ShaderPinType::Float);
        addInput("Min", ShaderPinType::Float);
        addInput("Max", ShaderPinType::Float);
        addOutput("Result", ShaderPinType::Float);
        break;

    case ShaderNodeType::Saturate:
    case ShaderNodeType::Abs:
    case ShaderNodeType::Frac:
    case ShaderNodeType::Floor:
    case ShaderNodeType::Ceil:
    case ShaderNodeType::SquareRoot:
        addInput("In", ShaderPinType::Float);
        addOutput("Result", ShaderPinType::Float);
        break;

    case ShaderNodeType::Power:
        addInput("Base", ShaderPinType::Float);
        addInput("Exp",  ShaderPinType::Float);
        addOutput("Result", ShaderPinType::Float);
        break;

    case ShaderNodeType::Sin:
    case ShaderNodeType::Cos:
        addInput("In", ShaderPinType::Float);
        addOutput("Result", ShaderPinType::Float);
        break;

    case ShaderNodeType::Time:
        addOutput("Time", ShaderPinType::Float);
        break;

    case ShaderNodeType::UV:
        addOutput("UV", ShaderPinType::Float2);
        break;

    case ShaderNodeType::WorldPos:
        addOutput("Position", ShaderPinType::Float3);
        break;

    case ShaderNodeType::WorldNormal:
        addOutput("Normal", ShaderPinType::Float3);
        break;

    case ShaderNodeType::ViewDir:
        addOutput("Direction", ShaderPinType::Float3);
        break;

    case ShaderNodeType::Fresnel:
        addInput("Normal",  ShaderPinType::Float3);
        addInput("ViewDir", ShaderPinType::Float3);
        addInput("Power",   ShaderPinType::Float);
        addOutput("Result", ShaderPinType::Float);
        break;

    case ShaderNodeType::NormalMap:
        addInput("Texture", ShaderPinType::Texture2D);
        addInput("UV",      ShaderPinType::Float2);
        addOutput("Normal", ShaderPinType::Float3);
        break;

    case ShaderNodeType::Parallax:
        addInput("HeightMap", ShaderPinType::Texture2D);
        addInput("UV",        ShaderPinType::Float2);
        addInput("ViewDir",   ShaderPinType::Float3);
        addInput("Scale",     ShaderPinType::Float);
        addOutput("UV",       ShaderPinType::Float2);
        break;

    default:
        break;
    }
}

} // namespace gx
