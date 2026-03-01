/// @file DialogueGraph.cpp
/// @brief 対話グラフ実装

#include "pch_common.h"
#include "Core/DialogueGraph.h"
#include <algorithm>
#include <random>
#include <sstream>

namespace gx
{

// =============================================================================
// ノード管理
// =============================================================================

uint32_t DialogueGraph::AddNode(DialogueNodeType type, const std::string& speaker, const std::string& text)
{
    DialogueGraphNode node;
    node.id = m_nextNodeId++;
    node.type = type;
    node.speaker = speaker;
    node.text = text;
    node.setValue = false; // default variant value
    m_nodes.push_back(std::move(node));
    return node.id;
}

void DialogueGraph::RemoveNode(uint32_t nodeId)
{
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
                           [nodeId](const DialogueGraphNode& n) { return n.id == nodeId; });
    if (it != m_nodes.end())
        m_nodes.erase(it);
}

DialogueGraphNode* DialogueGraph::GetNode(uint32_t nodeId)
{
    for (auto& node : m_nodes)
    {
        if (node.id == nodeId)
            return &node;
    }
    return nullptr;
}

const DialogueGraphNode* DialogueGraph::GetNode(uint32_t nodeId) const
{
    for (const auto& node : m_nodes)
    {
        if (node.id == nodeId)
            return &node;
    }
    return nullptr;
}

// =============================================================================
// リンク管理
// =============================================================================

void DialogueGraph::AddLink(uint32_t fromNodeId, uint32_t toNodeId,
                             const std::string& label, const std::string& condition)
{
    DialogueGraphNode* from = GetNode(fromNodeId);
    if (!from) return;

    DialogueLink link;
    link.targetNodeId = toNodeId;
    link.label = label;
    link.condition = condition;
    from->links.push_back(std::move(link));
}

void DialogueGraph::RemoveLink(uint32_t fromNodeId, uint32_t toNodeId)
{
    DialogueGraphNode* from = GetNode(fromNodeId);
    if (!from) return;

    auto& links = from->links;
    links.erase(
        std::remove_if(links.begin(), links.end(),
                       [toNodeId](const DialogueLink& l) { return l.targetNodeId == toNodeId; }),
        links.end()
    );
}

// =============================================================================
// 変数システム
// =============================================================================

void DialogueGraph::SetVariable(const std::string& name, const DialogueValue& value)
{
    m_variables[name] = value;
}

DialogueValue DialogueGraph::GetVariable(const std::string& name) const
{
    auto it = m_variables.find(name);
    if (it != m_variables.end())
        return it->second;
    return DialogueValue{ false };
}

bool DialogueGraph::HasVariable(const std::string& name) const
{
    return m_variables.find(name) != m_variables.end();
}

void DialogueGraph::ClearVariables()
{
    m_variables.clear();
}

// =============================================================================
// グラフ走査
// =============================================================================

void DialogueGraph::Start(uint32_t startNodeId)
{
    m_finished = false;
    m_currentNodeId = startNodeId;
    EnterNode(startNodeId);
}

bool DialogueGraph::Advance()
{
    if (m_finished) return false;

    const DialogueGraphNode* current = GetCurrentNode();
    if (!current) { m_finished = true; return false; }

    // Choice nodes require SelectChoice, not Advance
    if (current->type == DialogueNodeType::Choice)
        return false;

    if (current->links.empty())
    {
        m_finished = true;
        return false;
    }

    // For Branch nodes: evaluate conditions and follow the first matching link
    if (current->type == DialogueNodeType::Branch)
    {
        for (const auto& link : current->links)
        {
            if (link.condition.empty() || EvaluateCondition(link.condition))
            {
                EnterNode(link.targetNodeId);
                return true;
            }
        }
        // No condition matched
        m_finished = true;
        return false;
    }

    // For Random nodes: pick a random link
    if (current->type == DialogueNodeType::Random)
    {
        if (!current->links.empty())
        {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<uint32_t> dist(
                0, static_cast<uint32_t>(current->links.size() - 1));
            uint32_t idx = dist(rng);
            EnterNode(current->links[idx].targetNodeId);
            return true;
        }
        m_finished = true;
        return false;
    }

    // For Jump nodes: follow first link unconditionally
    if (current->type == DialogueNodeType::Jump)
    {
        EnterNode(current->links[0].targetNodeId);
        return true;
    }

    // For Text nodes: follow first link
    EnterNode(current->links[0].targetNodeId);
    return true;
}

void DialogueGraph::SelectChoice(uint32_t choiceIndex)
{
    if (m_finished) return;

    const DialogueGraphNode* current = GetCurrentNode();
    if (!current || current->type != DialogueNodeType::Choice)
        return;

    if (choiceIndex < static_cast<uint32_t>(current->links.size()))
    {
        EnterNode(current->links[choiceIndex].targetNodeId);
    }
}

const DialogueGraphNode* DialogueGraph::GetCurrentNode() const
{
    return GetNode(m_currentNodeId);
}

bool DialogueGraph::IsWaitingForChoice() const
{
    if (m_finished) return false;
    const DialogueGraphNode* current = GetCurrentNode();
    return current && current->type == DialogueNodeType::Choice;
}

// =============================================================================
// 履歴
// =============================================================================

bool DialogueGraph::HasVisited(uint32_t nodeId) const
{
    return std::find(m_visited.begin(), m_visited.end(), nodeId) != m_visited.end();
}

void DialogueGraph::ClearHistory()
{
    m_visited.clear();
}

// =============================================================================
// シリアライズ
// =============================================================================

std::string DialogueGraph::Save() const
{
    // Simple text format: "nodeCount\n" followed by node data
    std::string result;
    result += std::to_string(m_nodes.size()) + "\n";
    for (const auto& node : m_nodes)
    {
        result += std::to_string(node.id) + "|";
        result += std::to_string(static_cast<int>(node.type)) + "|";
        result += node.speaker + "|";
        result += node.text + "|";
        result += node.emotion + "|";
        result += std::to_string(node.links.size());
        for (const auto& link : node.links)
        {
            result += "|" + std::to_string(link.targetNodeId);
            result += "|" + link.label;
            result += "|" + link.condition;
        }
        result += "\n";
    }
    // Save variables
    result += std::to_string(m_variables.size()) + "\n";
    for (const auto& [key, val] : m_variables)
    {
        result += key + "|";
        if (std::holds_alternative<bool>(val))
            result += "b|" + std::string(std::get<bool>(val) ? "1" : "0");
        else if (std::holds_alternative<int>(val))
            result += "i|" + std::to_string(std::get<int>(val));
        else if (std::holds_alternative<float>(val))
            result += "f|" + std::to_string(std::get<float>(val));
        else if (std::holds_alternative<std::string>(val))
            result += "s|" + std::get<std::string>(val);
        result += "\n";
    }
    return result;
}

bool DialogueGraph::Load(const std::string& data)
{
    if (data.empty()) return false;

    std::istringstream iss(data);
    std::string line;

    // Read node count
    if (!std::getline(iss, line)) return false;
    size_t nodeCount = 0;
    try { nodeCount = std::stoull(line); }
    catch (...) { return false; }

    m_nodes.clear();
    m_nextNodeId = 0;

    for (size_t i = 0; i < nodeCount; ++i)
    {
        if (!std::getline(iss, line)) return false;

        // Parse pipe-delimited fields
        std::vector<std::string> fields;
        size_t pos = 0;
        while (pos <= line.size())
        {
            size_t next = line.find('|', pos);
            if (next == std::string::npos)
            {
                fields.push_back(line.substr(pos));
                break;
            }
            fields.push_back(line.substr(pos, next - pos));
            pos = next + 1;
        }

        if (fields.size() < 6) return false;

        DialogueGraphNode node;
        try
        {
            node.id = static_cast<uint32_t>(std::stoul(fields[0]));
            node.type = static_cast<DialogueNodeType>(std::stoi(fields[1]));
            node.speaker = fields[2];
            node.text = fields[3];
            node.emotion = fields[4];
            size_t linkCount = std::stoull(fields[5]);

            size_t fieldIdx = 6;
            for (size_t l = 0; l < linkCount; ++l)
            {
                if (fieldIdx + 2 >= fields.size()) break;
                DialogueLink link;
                link.targetNodeId = static_cast<uint32_t>(std::stoul(fields[fieldIdx]));
                link.label = fields[fieldIdx + 1];
                link.condition = fields[fieldIdx + 2];
                node.links.push_back(std::move(link));
                fieldIdx += 3;
            }
        }
        catch (...) { return false; }

        if (node.id >= m_nextNodeId)
            m_nextNodeId = node.id + 1;

        m_nodes.push_back(std::move(node));
    }

    // Read variables
    if (std::getline(iss, line))
    {
        size_t varCount = 0;
        try { varCount = std::stoull(line); }
        catch (...) { return true; } // variables are optional

        m_variables.clear();
        for (size_t i = 0; i < varCount; ++i)
        {
            if (!std::getline(iss, line)) break;

            // Parse: key|type|value
            size_t first = line.find('|');
            if (first == std::string::npos) continue;
            size_t second = line.find('|', first + 1);
            if (second == std::string::npos) continue;

            std::string key = line.substr(0, first);
            std::string typeStr = line.substr(first + 1, second - first - 1);
            std::string valStr = line.substr(second + 1);

            try
            {
                if (typeStr == "b")
                    m_variables[key] = (valStr == "1");
                else if (typeStr == "i")
                    m_variables[key] = std::stoi(valStr);
                else if (typeStr == "f")
                    m_variables[key] = std::stof(valStr);
                else if (typeStr == "s")
                    m_variables[key] = valStr;
            }
            catch (...) { continue; }
        }
    }

    return true;
}

// =============================================================================
// 内部
// =============================================================================

bool DialogueGraph::EvaluateCondition(const std::string& condition) const
{
    if (condition.empty()) return true;

    // Simple condition format: "varName == value" or "varName != value"
    // Parse operator
    std::string op;
    size_t opPos = std::string::npos;

    size_t neqPos = condition.find("!=");
    size_t eqPos = condition.find("==");

    if (neqPos != std::string::npos)
    {
        op = "!=";
        opPos = neqPos;
    }
    else if (eqPos != std::string::npos)
    {
        op = "==";
        opPos = eqPos;
    }
    else
    {
        // No operator: check if variable exists and is truthy
        auto it = m_variables.find(condition);
        if (it == m_variables.end()) return false;
        if (std::holds_alternative<bool>(it->second))
            return std::get<bool>(it->second);
        if (std::holds_alternative<int>(it->second))
            return std::get<int>(it->second) != 0;
        return true;
    }

    // Extract variable name and value
    std::string varName = condition.substr(0, opPos);
    std::string valStr = condition.substr(opPos + op.size());

    // Trim whitespace
    auto trim = [](std::string& s) {
        while (!s.empty() && s.front() == ' ') s.erase(s.begin());
        while (!s.empty() && s.back() == ' ') s.pop_back();
    };
    trim(varName);
    trim(valStr);

    auto it = m_variables.find(varName);
    if (it == m_variables.end())
        return op == "!="; // variable doesn't exist, so != anything is true

    const auto& var = it->second;
    bool equals = false;

    if (std::holds_alternative<bool>(var))
    {
        bool expected = (valStr == "true" || valStr == "1");
        equals = (std::get<bool>(var) == expected);
    }
    else if (std::holds_alternative<int>(var))
    {
        try { equals = (std::get<int>(var) == std::stoi(valStr)); }
        catch (...) { equals = false; }
    }
    else if (std::holds_alternative<float>(var))
    {
        try { equals = (std::get<float>(var) == std::stof(valStr)); }
        catch (...) { equals = false; }
    }
    else if (std::holds_alternative<std::string>(var))
    {
        equals = (std::get<std::string>(var) == valStr);
    }

    return op == "==" ? equals : !equals;
}

void DialogueGraph::EnterNode(uint32_t nodeId)
{
    const DialogueGraphNode* node = GetNode(nodeId);
    if (!node)
    {
        m_finished = true;
        return;
    }

    m_currentNodeId = nodeId;
    m_visited.push_back(nodeId);

    // Apply variable setting if specified
    if (!node->setVariable.empty())
        m_variables[node->setVariable] = node->setValue;

    // Fire callback
    if (m_onNodeEnter)
        m_onNodeEnter(*node);
}

} // namespace gx
