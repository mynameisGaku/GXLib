/// @file MaterialEditor.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/MaterialEditor.h"
#include "Core/Logger.h"
#include <algorithm>

namespace gx
{

void MaterialEditor::SetMaterial(const std::string& materialName)
{
    m_materialName = materialName;
    m_dirty = false;
    GX_LOG_INFO("MaterialEditor: set material '%s'", materialName.c_str());
}

void MaterialEditor::ClearMaterial()
{
    m_materialName.clear();
    m_nodes.clear();
    m_connections.clear();
    m_textureSlots.clear();
    m_dirty = false;
}

void MaterialEditor::SetShaderModel(ShaderModelType model)
{
    m_shaderModel = model;
    NotifyChange("ShaderModel");
}

void MaterialEditor::SetBaseColor(float r, float g, float b)
{
    m_baseColor[0] = std::clamp(r, 0.0f, 1.0f);
    m_baseColor[1] = std::clamp(g, 0.0f, 1.0f);
    m_baseColor[2] = std::clamp(b, 0.0f, 1.0f);
    NotifyChange("BaseColor");
}

void MaterialEditor::GetBaseColor(float& r, float& g, float& b) const
{
    r = m_baseColor[0]; g = m_baseColor[1]; b = m_baseColor[2];
}

void MaterialEditor::SetMetallic(float value)
{
    m_metallic = std::clamp(value, 0.0f, 1.0f);
    NotifyChange("Metallic");
}

void MaterialEditor::SetRoughness(float value)
{
    m_roughness = std::clamp(value, 0.0f, 1.0f);
    NotifyChange("Roughness");
}

void MaterialEditor::SetEmissiveColor(float r, float g, float b)
{
    m_emissiveColor[0] = std::clamp(r, 0.0f, 1.0f);
    m_emissiveColor[1] = std::clamp(g, 0.0f, 1.0f);
    m_emissiveColor[2] = std::clamp(b, 0.0f, 1.0f);
    NotifyChange("EmissiveColor");
}

void MaterialEditor::GetEmissiveColor(float& r, float& g, float& b) const
{
    r = m_emissiveColor[0]; g = m_emissiveColor[1]; b = m_emissiveColor[2];
}

void MaterialEditor::SetEmissiveIntensity(float intensity)
{
    m_emissiveIntensity = std::max(0.0f, intensity);
    NotifyChange("EmissiveIntensity");
}

void MaterialEditor::SetTextureSlot(const std::string& slotName, const std::string& path)
{
    TextureSlotInfo info;
    info.name = slotName;
    info.path = path;
    info.slotIndex = static_cast<uint32_t>(m_textureSlots.size());

    auto it = m_textureSlots.find(slotName);
    if (it != m_textureSlots.end())
    {
        info.slotIndex = it->second.slotIndex;
    }

    m_textureSlots[slotName] = info;
    NotifyChange("Texture:" + slotName);
}

std::string MaterialEditor::GetTextureSlot(const std::string& slotName) const
{
    auto it = m_textureSlots.find(slotName);
    if (it != m_textureSlots.end())
        return it->second.path;
    return "";
}

std::vector<TextureSlotInfo> MaterialEditor::GetTextureSlots() const
{
    std::vector<TextureSlotInfo> slots;
    for (const auto& [name, info] : m_textureSlots)
        slots.push_back(info);
    return slots;
}

uint32_t MaterialEditor::AddNode(MENodeType type, const std::string& name)
{
    MENode node;
    node.id = m_nextNodeId++;
    node.type = type;
    node.name = name.empty() ? ("Node_" + std::to_string(node.id)) : name;
    m_nodes.push_back(node);
    NotifyChange("AddNode");
    return node.id;
}

void MaterialEditor::RemoveNode(uint32_t nodeId)
{
    m_nodes.erase(
        std::remove_if(m_nodes.begin(), m_nodes.end(),
            [nodeId](const MENode& n) { return n.id == nodeId; }),
        m_nodes.end());

    // Remove related connections
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [nodeId](const NodeConnection& c) {
                return c.srcNode == nodeId || c.dstNode == nodeId;
            }),
        m_connections.end());

    NotifyChange("RemoveNode");
}

const MENode* MaterialEditor::GetNode(uint32_t nodeId) const
{
    for (const auto& n : m_nodes)
        if (n.id == nodeId) return &n;
    return nullptr;
}

void MaterialEditor::ConnectNodes(uint32_t srcNodeId, uint32_t srcPin,
                                   uint32_t dstNodeId, uint32_t dstPin)
{
    NodeConnection conn = { srcNodeId, srcPin, dstNodeId, dstPin };
    m_connections.push_back(conn);
    NotifyChange("Connect");
}

void MaterialEditor::DisconnectNodes(uint32_t srcNodeId, uint32_t srcPin,
                                      uint32_t dstNodeId, uint32_t dstPin)
{
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [&](const NodeConnection& c) {
                return c.srcNode == srcNodeId && c.srcPin == srcPin
                    && c.dstNode == dstNodeId && c.dstPin == dstPin;
            }),
        m_connections.end());
    NotifyChange("Disconnect");
}

std::vector<MaterialParam> MaterialEditor::GetEditableParams() const
{
    std::vector<MaterialParam> params;

    // Base params
    {
        MaterialParam p;
        p.name = "BaseColor"; p.category = "Base"; p.type = MaterialParam::Type::Float3;
        p.float3Value[0] = m_baseColor[0]; p.float3Value[1] = m_baseColor[1]; p.float3Value[2] = m_baseColor[2];
        params.push_back(p);
    }
    {
        MaterialParam p;
        p.name = "Metallic"; p.category = "PBR"; p.type = MaterialParam::Type::Float;
        p.floatValue = m_metallic;
        params.push_back(p);
    }
    {
        MaterialParam p;
        p.name = "Roughness"; p.category = "PBR"; p.type = MaterialParam::Type::Float;
        p.floatValue = m_roughness;
        params.push_back(p);
    }
    {
        MaterialParam p;
        p.name = "EmissiveColor"; p.category = "Emission"; p.type = MaterialParam::Type::Float3;
        p.float3Value[0] = m_emissiveColor[0]; p.float3Value[1] = m_emissiveColor[1]; p.float3Value[2] = m_emissiveColor[2];
        params.push_back(p);
    }
    {
        MaterialParam p;
        p.name = "EmissiveIntensity"; p.category = "Emission"; p.type = MaterialParam::Type::Float;
        p.floatValue = m_emissiveIntensity;
        params.push_back(p);
    }

    // Texture params
    for (const auto& [name, info] : m_textureSlots)
    {
        MaterialParam p;
        p.name = name; p.category = "Textures"; p.type = MaterialParam::Type::Texture;
        p.texturePath = info.path;
        params.push_back(p);
    }

    return params;
}

MaterialPreset MaterialEditor::SaveAsPreset(const std::string& name) const
{
    MaterialPreset preset;
    preset.name = name;
    preset.shaderModel = m_shaderModel;
    preset.params = GetEditableParams();
    return preset;
}

void MaterialEditor::LoadPreset(const MaterialPreset& preset)
{
    m_shaderModel = preset.shaderModel;
    for (const auto& p : preset.params)
    {
        if (p.name == "BaseColor") SetBaseColor(p.float3Value[0], p.float3Value[1], p.float3Value[2]);
        else if (p.name == "Metallic") SetMetallic(p.floatValue);
        else if (p.name == "Roughness") SetRoughness(p.floatValue);
        else if (p.name == "EmissiveColor") SetEmissiveColor(p.float3Value[0], p.float3Value[1], p.float3Value[2]);
        else if (p.name == "EmissiveIntensity") SetEmissiveIntensity(p.floatValue);
    }
}

std::vector<std::string> MaterialEditor::GetBuiltinPresetNames()
{
    return { "Default PBR", "Metal", "Plastic", "Glass", "Emissive", "Toon" };
}

void MaterialEditor::NotifyChange(const std::string& paramName)
{
    m_dirty = true;
    if (m_onChange) m_onChange(paramName);
}

} // namespace gx
