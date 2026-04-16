/// @file VisualScript.cpp
/// @brief ビジュアルスクリプティングシステム実装
#include "pch_common.h"
#include "Script/VisualScript.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace gx
{

void VisualScript::RegisterNodeType(const VSNodeDef& def)
{
    m_nodeTypes[def.typeName] = def;
}

const VSNodeDef* VisualScript::GetNodeType(const gx::String& typeName) const
{
    auto it = m_nodeTypes.find(typeName);
    if (it == m_nodeTypes.end()) return nullptr;
    return &it->second;
}

bool VisualScript::HasNodeType(const gx::String& typeName) const
{
    return m_nodeTypes.find(typeName) != m_nodeTypes.end();
}

uint32_t VisualScript::AddNode(const gx::String& typeName, float posX, float posY)
{
    if (!HasNodeType(typeName)) return 0xFFFFFFFF;

    uint32_t id = m_nextNodeId++;
    VSNodeInstance node;
    node.id = id;
    node.typeName = typeName;
    node.posX = posX;
    node.posY = posY;
    m_nodes[id] = std::move(node);
    return id;
}

bool VisualScript::RemoveNode(uint32_t nodeId)
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) return false;

    m_nodes.erase(it);

    // 関連する接続も削除
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [nodeId](const VSConnection& c) {
                return c.fromNode == nodeId || c.toNode == nodeId;
            }),
        m_connections.end());

    return true;
}

const VSNodeInstance* VisualScript::GetNode(uint32_t nodeId) const
{
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) return nullptr;
    return &it->second;
}

bool VisualScript::Connect(uint32_t fromNode, uint32_t fromPin, uint32_t toNode, uint32_t toPin)
{
    // ノードの存在確認
    if (m_nodes.find(fromNode) == m_nodes.end()) return false;
    if (m_nodes.find(toNode) == m_nodes.end()) return false;

    // 自己接続禁止
    if (fromNode == toNode) return false;

    // 重複チェック
    for (const auto& c : m_connections)
    {
        if (c.fromNode == fromNode && c.fromPin == fromPin &&
            c.toNode == toNode && c.toPin == toPin)
            return false;
    }

    VSConnection conn;
    conn.fromNode = fromNode;
    conn.fromPin = fromPin;
    conn.toNode = toNode;
    conn.toPin = toPin;
    m_connections.push_back(conn);
    return true;
}

bool VisualScript::Disconnect(uint32_t fromNode, uint32_t fromPin, uint32_t toNode, uint32_t toPin)
{
    auto it = std::remove_if(m_connections.begin(), m_connections.end(),
        [=](const VSConnection& c) {
            return c.fromNode == fromNode && c.fromPin == fromPin &&
                   c.toNode == toNode && c.toPin == toPin;
        });

    if (it == m_connections.end()) return false;

    m_connections.erase(it, m_connections.end());
    return true;
}

bool VisualScript::AddVariable(const gx::String& name, VSPinType type, const gx::String& defaultValue)
{
    if (name.empty()) return false;
    if (m_variables.find(name) != m_variables.end()) return false;

    VSVariable var;
    var.name = name;
    var.type = type;
    var.value = defaultValue;
    m_variables[name] = std::move(var);
    return true;
}

bool VisualScript::RemoveVariable(const gx::String& name)
{
    return m_variables.erase(name) > 0;
}

void VisualScript::SetVariable(const gx::String& name, const gx::String& value)
{
    auto it = m_variables.find(name);
    if (it != m_variables.end())
    {
        it->second.value = value;
    }
}

gx::String VisualScript::GetVariable(const gx::String& name) const
{
    auto it = m_variables.find(name);
    if (it != m_variables.end()) return it->second.value;
    return "";
}

bool VisualScript::HasVariable(const gx::String& name) const
{
    return m_variables.find(name) != m_variables.end();
}

VSPinType VisualScript::GetVariableType(const gx::String& name) const
{
    auto it = m_variables.find(name);
    if (it != m_variables.end()) return it->second.type;
    return VSPinType::Any;
}

gx::Vector<gx::String> VisualScript::Validate() const
{
    gx::Vector<gx::String> errors;

    // ノードが空なら問題なし
    if (m_nodes.empty()) return errors;

    // 各ノードの型が登録済みか確認
    for (const auto& [id, node] : m_nodes)
    {
        if (m_nodeTypes.find(node.typeName) == m_nodeTypes.end())
        {
            errors.push_back(gx::String("Node " + std::to_string(id) + ": unknown type '" + std::string(node.typeName.c_str()) + "'"));
        }
    }

    // 接続先のノードが存在するか確認
    for (const auto& conn : m_connections)
    {
        if (m_nodes.find(conn.fromNode) == m_nodes.end())
            errors.push_back(gx::String("Connection: source node " + std::to_string(conn.fromNode) + " not found"));
        if (m_nodes.find(conn.toNode) == m_nodes.end())
            errors.push_back(gx::String("Connection: target node " + std::to_string(conn.toNode) + " not found"));
    }

    // 非イベントノードのExec入力が未接続かチェック
    for (const auto& [id, node] : m_nodes)
    {
        auto typeIt = m_nodeTypes.find(node.typeName);
        if (typeIt == m_nodeTypes.end()) continue;

        const auto& def = typeIt->second;
        if (def.isEvent || def.isPure) continue;

        // Exec入力ピンがあるか確認
        bool hasExecInput = false;
        for (const auto& pin : def.inputPins)
        {
            if (pin.type == VSPinType::Exec) { hasExecInput = true; break; }
        }

        if (!hasExecInput) continue;

        // 接続されているか確認
        bool connected = false;
        for (const auto& conn : m_connections)
        {
            if (conn.toNode == id)
            {
                connected = true;
                break;
            }
        }

        if (!connected)
        {
            errors.push_back(gx::String("Node " + std::to_string(id) + " (" + std::string(node.typeName.c_str()) + "): exec input not connected"));
        }
    }

    return errors;
}

bool VisualScript::HasCycle() const
{
    // DFSベースのサイクル検出
    // visited: 0=未訪問, 1=処理中, 2=完了
    gx::HashMap<uint32_t, int> visited;
    for (const auto& [id, _] : m_nodes)
    {
        visited[id] = 0;
    }

    for (const auto& [id, _] : m_nodes)
    {
        if (visited[id] == 0)
        {
            if (HasCycleDFS(id, visited)) return true;
        }
    }
    return false;
}

bool VisualScript::HasCycleDFS(uint32_t nodeId,
                                gx::HashMap<uint32_t, int>& visited) const
{
    visited[nodeId] = 1; // 処理中

    // このノードから出る全接続を辿る
    for (const auto& conn : m_connections)
    {
        if (conn.fromNode != nodeId) continue;

        uint32_t next = conn.toNode;
        if (visited[next] == 1) return true;  // サイクル検出
        if (visited[next] == 0)
        {
            if (HasCycleDFS(next, visited)) return true;
        }
    }

    visited[nodeId] = 2; // 完了
    return false;
}

bool VisualScript::Execute(uint32_t entryNodeId, VSExecutionContext& context)
{
    auto nodeIt = m_nodes.find(entryNodeId);
    if (nodeIt == m_nodes.end())
    {
        context.lastError = gx::String("Entry node not found: " + std::to_string(entryNodeId));
        return false;
    }

    // 変数をコンテキストに同期
    for (const auto& [name, var] : m_variables)
    {
        if (context.variables.find(name) == context.variables.end())
        {
            context.variables[name] = var.value;
        }
    }

    // 実行フローを辿る
    constexpr uint32_t kMaxExecutions = 10000;
    uint32_t currentNodeId = entryNodeId;

    while (currentNodeId != 0xFFFFFFFF && context.executionCount < kMaxExecutions)
    {
        auto curIt = m_nodes.find(currentNodeId);
        if (curIt == m_nodes.end()) break;

        context.callStack.push_back(currentNodeId);
        context.executionCount++;

        // ノードハンドラを実行
        auto handlerIt = m_nodeHandlers.find(curIt->second.typeName);
        if (handlerIt != m_nodeHandlers.end())
        {
            if (!handlerIt->second(curIt->second, context))
            {
                context.lastError = gx::String("Handler failed at node " + std::to_string(currentNodeId));
                return false;
            }
        }

        // Exec出力ピン（ピンインデックス0）を辿って次のノードへ
        // ノードハンドラが context に "next_exec_pin" を設定した場合はそのピンを使う
        uint32_t execPin = 0;
        auto nextPinIt = context.variables.find("__next_exec_pin");
        if (nextPinIt != context.variables.end())
        {
            execPin = static_cast<uint32_t>(std::stoul(std::string(nextPinIt->second.c_str())));
            context.variables.erase(nextPinIt);
        }

        // 次のノードを探す
        auto nextNodes = GetExecOutputConnections(currentNodeId, execPin);
        if (nextNodes.empty())
        {
            break; // フロー終了
        }

        currentNodeId = nextNodes[0]; // 最初の接続先を辿る

        // Sequence ノード: 複数のExec出力を順番に実行
        // context に "__sequence_pins" がある場合は全出力を順番に実行
        auto seqIt = context.variables.find("__sequence_pins");
        if (seqIt != context.variables.end())
        {
            gx::String pinList = seqIt->second;
            context.variables.erase(seqIt);

            // pinListをパース ("0,1,2,...")
            std::istringstream ss(std::string(pinList.c_str()));
            gx::String token;
            while (gx::container::getline(ss, token, ','))
            {
                uint32_t pin = static_cast<uint32_t>(std::stoul(std::string(token.c_str())));
                auto targets = GetExecOutputConnections(context.callStack.back(), pin);
                for (uint32_t target : targets)
                {
                    VSExecutionContext subContext = context;
                    subContext.callStack.clear();
                    subContext.variables.erase("__sequence_pins");
                    Execute(target, subContext);
                    // 変数の変更をメインコンテキストにマージ
                    for (const auto& [k, v] : subContext.variables)
                    {
                        if (k.empty() || k[0] == '_') continue;
                        context.variables[k] = v;
                    }
                    context.executionCount += subContext.executionCount;
                }
            }
            break; // シーケンス処理が完了したのでメインループ終了
        }
    }

    return true;
}

gx::Vector<uint32_t> VisualScript::GetExecOutputConnections(uint32_t nodeId, uint32_t pinIndex) const
{
    gx::Vector<uint32_t> result;
    for (const auto& conn : m_connections)
    {
        if (conn.fromNode == nodeId && conn.fromPin == pinIndex)
        {
            result.push_back(conn.toNode);
        }
    }
    return result;
}

void VisualScript::ResolveInputs(uint32_t nodeId, VSExecutionContext& context)
{
    // データピンの入力を接続元から解決する
    for (const auto& conn : m_connections)
    {
        if (conn.toNode != nodeId) continue;

        auto srcIt = m_nodes.find(conn.fromNode);
        if (srcIt == m_nodes.end()) continue;

        // ソースノードのプロパティからデータを読み取り、ターゲットのプロパティに設定
        gx::String key = gx::String("output_" + std::to_string(conn.fromPin));
        auto propIt = srcIt->second.properties.find(key);
        if (propIt != srcIt->second.properties.end())
        {
            auto dstIt = m_nodes.find(nodeId);
            if (dstIt != m_nodes.end())
            {
                dstIt->second.properties[gx::String("input_" + std::to_string(conn.toPin))] = propIt->second;
            }
        }
    }
}

void VisualScript::SetNodeHandler(const gx::String& typeName, NodeHandler handler)
{
    m_nodeHandlers[typeName] = std::move(handler);
}

bool VisualScript::Save(const gx::String& filePath) const
{
    std::ofstream file(filePath.c_str());
    if (!file.is_open()) return false;

    // ノード数
    file << "NODES " << m_nodes.size() << "\n";
    for (const auto& [id, node] : m_nodes)
    {
        file << id << " " << node.typeName << " " << node.posX << " " << node.posY << "\n";
        file << "PROPS " << node.properties.size() << "\n";
        for (const auto& [k, v] : node.properties)
        {
            file << k << "=" << v << "\n";
        }
    }

    // 接続数
    file << "CONNECTIONS " << m_connections.size() << "\n";
    for (const auto& conn : m_connections)
    {
        file << conn.fromNode << " " << conn.fromPin << " "
             << conn.toNode << " " << conn.toPin << "\n";
    }

    // 変数数
    file << "VARIABLES " << m_variables.size() << "\n";
    for (const auto& [name, var] : m_variables)
    {
        file << name << " " << static_cast<int>(var.type) << " " << var.value << "\n";
    }

    return true;
}

bool VisualScript::Load(const gx::String& filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) return false;

    Clear();

    gx::String line;
    gx::String token;

    // ノード読み込み
    if (!gx::container::getline(file, line)) return false;
    std::istringstream nodeLine(std::string(line.c_str()));
    nodeLine >> token; // "NODES"
    size_t nodeCount = 0;
    nodeLine >> nodeCount;

    for (size_t i = 0; i < nodeCount; i++)
    {
        if (!gx::container::getline(file, line)) return false;
        std::istringstream ns(std::string(line.c_str()));
        uint32_t id = 0;
        gx::String typeName;
        float px = 0.0f, py = 0.0f;
        ns >> id >> typeName >> px >> py;

        VSNodeInstance node;
        node.id = id;
        node.typeName = typeName;
        node.posX = px;
        node.posY = py;

        // プロパティ
        if (!gx::container::getline(file, line)) return false;
        std::istringstream ps(std::string(line.c_str()));
        ps >> token; // "PROPS"
        size_t propCount = 0;
        ps >> propCount;

        for (size_t j = 0; j < propCount; j++)
        {
            if (!gx::container::getline(file, line)) return false;
            auto eqPos = line.find('=');
            if (eqPos != gx::String::npos)
            {
                node.properties[line.substr(0, eqPos)] = line.substr(eqPos + 1);
            }
        }

        m_nodes[id] = std::move(node);
        if (id >= m_nextNodeId) m_nextNodeId = id + 1;
    }

    // 接続読み込み
    if (!gx::container::getline(file, line)) return false;
    std::istringstream connLine(std::string(line.c_str()));
    connLine >> token; // "CONNECTIONS"
    size_t connCount = 0;
    connLine >> connCount;

    for (size_t i = 0; i < connCount; i++)
    {
        if (!gx::container::getline(file, line)) return false;
        std::istringstream cs(std::string(line.c_str()));
        VSConnection conn;
        cs >> conn.fromNode >> conn.fromPin >> conn.toNode >> conn.toPin;
        m_connections.push_back(conn);
    }

    // 変数読み込み
    if (gx::container::getline(file, line))
    {
        std::istringstream varLine(std::string(line.c_str()));
        varLine >> token; // "VARIABLES"
        size_t varCount = 0;
        varLine >> varCount;

        for (size_t i = 0; i < varCount; i++)
        {
            if (!gx::container::getline(file, line)) return false;
            std::istringstream vs(std::string(line.c_str()));
            gx::String name;
            int typeInt = 0;
            gx::String value;
            vs >> name >> typeInt;
            gx::container::getline(vs, value);
            // trim leading space
            if (!value.empty() && value[0] == ' ') value = value.substr(1);

            VSVariable var;
            var.name = name;
            var.type = static_cast<VSPinType>(typeInt);
            var.value = value;
            m_variables[name] = std::move(var);
        }
    }

    return true;
}

void VisualScript::Clear()
{
    m_nodes.clear();
    m_connections.clear();
    m_variables.clear();
    m_nextNodeId = 1;
    // ノード型定義とハンドラは保持する
}

gx::Vector<uint32_t> VisualScript::GetEntryNodes() const
{
    gx::Vector<uint32_t> result;

    for (const auto& [id, node] : m_nodes)
    {
        auto typeIt = m_nodeTypes.find(node.typeName);
        if (typeIt == m_nodeTypes.end()) continue;

        const auto& def = typeIt->second;
        if (!def.isEvent) continue;

        // Exec入力ピンがないノードがエントリーポイント
        bool hasExecInput = false;
        for (const auto& pin : def.inputPins)
        {
            if (pin.type == VSPinType::Exec) { hasExecInput = true; break; }
        }

        if (!hasExecInput)
        {
            result.push_back(id);
        }
    }

    return result;
}

void VisualScript::RegisterBuiltinTypes()
{
    // --- Event ノード ---
    {
        VSNodeDef def;
        def.typeName = "OnBeginPlay";
        def.category = VSNodeCategory::Event;
        def.displayName = "On Begin Play";
        def.isEvent = true;
        def.outputPins = { {"Exec", VSPinType::Exec, 0, 0, true} };
        RegisterNodeType(def);
    }
    {
        VSNodeDef def;
        def.typeName = "OnTick";
        def.category = VSNodeCategory::Event;
        def.displayName = "On Tick";
        def.isEvent = true;
        def.outputPins = {
            {"Exec", VSPinType::Exec, 0, 0, true},
            {"DeltaTime", VSPinType::Float, 0, 1, true}
        };
        RegisterNodeType(def);
    }
    {
        VSNodeDef def;
        def.typeName = "OnOverlap";
        def.category = VSNodeCategory::Event;
        def.displayName = "On Overlap";
        def.isEvent = true;
        def.outputPins = {
            {"Exec", VSPinType::Exec, 0, 0, true},
            {"OtherEntity", VSPinType::Entity, 0, 1, true}
        };
        RegisterNodeType(def);
    }

    // --- Flow ノード ---
    {
        VSNodeDef def;
        def.typeName = "Branch";
        def.category = VSNodeCategory::Flow;
        def.displayName = "Branch";
        def.inputPins = {
            {"Exec", VSPinType::Exec, 0, 0, false},
            {"Condition", VSPinType::Bool, 0, 1, false}
        };
        def.outputPins = {
            {"True", VSPinType::Exec, 0, 0, true},
            {"False", VSPinType::Exec, 0, 1, true}
        };
        RegisterNodeType(def);
    }
    {
        VSNodeDef def;
        def.typeName = "Sequence";
        def.category = VSNodeCategory::Flow;
        def.displayName = "Sequence";
        def.inputPins = { {"Exec", VSPinType::Exec, 0, 0, false} };
        def.outputPins = {
            {"Then 0", VSPinType::Exec, 0, 0, true},
            {"Then 1", VSPinType::Exec, 0, 1, true},
            {"Then 2", VSPinType::Exec, 0, 2, true}
        };
        RegisterNodeType(def);
    }
    {
        VSNodeDef def;
        def.typeName = "ForLoop";
        def.category = VSNodeCategory::Flow;
        def.displayName = "For Loop";
        def.inputPins = {
            {"Exec", VSPinType::Exec, 0, 0, false},
            {"Start", VSPinType::Int, 0, 1, false},
            {"End", VSPinType::Int, 0, 2, false}
        };
        def.outputPins = {
            {"LoopBody", VSPinType::Exec, 0, 0, true},
            {"Completed", VSPinType::Exec, 0, 1, true},
            {"Index", VSPinType::Int, 0, 2, true}
        };
        RegisterNodeType(def);
    }

    // --- Action ノード ---
    {
        VSNodeDef def;
        def.typeName = "Print";
        def.category = VSNodeCategory::Flow;
        def.displayName = "Print String";
        def.inputPins = {
            {"Exec", VSPinType::Exec, 0, 0, false},
            {"String", VSPinType::String, 0, 1, false}
        };
        def.outputPins = { {"Exec", VSPinType::Exec, 0, 0, true} };
        RegisterNodeType(def);
    }

    // --- Math ノード (pure) ---
    auto registerMathBinary = [this](const gx::String& name, const gx::String& display) {
        VSNodeDef def;
        def.typeName = name;
        def.category = VSNodeCategory::Math;
        def.displayName = display;
        def.isPure = true;
        def.inputPins = {
            {"A", VSPinType::Float, 0, 0, false},
            {"B", VSPinType::Float, 0, 1, false}
        };
        def.outputPins = { {"Result", VSPinType::Float, 0, 0, true} };
        RegisterNodeType(def);
    };

    registerMathBinary("Add", "Add");
    registerMathBinary("Subtract", "Subtract");
    registerMathBinary("Multiply", "Multiply");
    registerMathBinary("Divide", "Divide");

    // --- Logic ノード (pure) ---
    auto registerComparison = [this](const gx::String& name, const gx::String& display) {
        VSNodeDef def;
        def.typeName = name;
        def.category = VSNodeCategory::Logic;
        def.displayName = display;
        def.isPure = true;
        def.inputPins = {
            {"A", VSPinType::Float, 0, 0, false},
            {"B", VSPinType::Float, 0, 1, false}
        };
        def.outputPins = { {"Result", VSPinType::Bool, 0, 0, true} };
        RegisterNodeType(def);
    };

    registerComparison("Equal", "Equal");
    registerComparison("NotEqual", "Not Equal");
    registerComparison("GreaterThan", "Greater Than");
    registerComparison("LessThan", "Less Than");

    {
        VSNodeDef def;
        def.typeName = "And";
        def.category = VSNodeCategory::Logic;
        def.displayName = "AND";
        def.isPure = true;
        def.inputPins = {
            {"A", VSPinType::Bool, 0, 0, false},
            {"B", VSPinType::Bool, 0, 1, false}
        };
        def.outputPins = { {"Result", VSPinType::Bool, 0, 0, true} };
        RegisterNodeType(def);
    }
    {
        VSNodeDef def;
        def.typeName = "Or";
        def.category = VSNodeCategory::Logic;
        def.displayName = "OR";
        def.isPure = true;
        def.inputPins = {
            {"A", VSPinType::Bool, 0, 0, false},
            {"B", VSPinType::Bool, 0, 1, false}
        };
        def.outputPins = { {"Result", VSPinType::Bool, 0, 0, true} };
        RegisterNodeType(def);
    }
    {
        VSNodeDef def;
        def.typeName = "Not";
        def.category = VSNodeCategory::Logic;
        def.displayName = "NOT";
        def.isPure = true;
        def.inputPins = { {"A", VSPinType::Bool, 0, 0, false} };
        def.outputPins = { {"Result", VSPinType::Bool, 0, 0, true} };
        RegisterNodeType(def);
    }

    // --- Variable ノード ---
    {
        VSNodeDef def;
        def.typeName = "GetVariable";
        def.category = VSNodeCategory::Variable;
        def.displayName = "Get Variable";
        def.isPure = true;
        def.inputPins = { {"Name", VSPinType::String, 0, 0, false} };
        def.outputPins = { {"Value", VSPinType::Any, 0, 0, true} };
        RegisterNodeType(def);
    }
    {
        VSNodeDef def;
        def.typeName = "SetVariable";
        def.category = VSNodeCategory::Variable;
        def.displayName = "Set Variable";
        def.inputPins = {
            {"Exec", VSPinType::Exec, 0, 0, false},
            {"Name", VSPinType::String, 0, 1, false},
            {"Value", VSPinType::Any, 0, 2, false}
        };
        def.outputPins = { {"Exec", VSPinType::Exec, 0, 0, true} };
        RegisterNodeType(def);
    }
}

} // namespace gx
