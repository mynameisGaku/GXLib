/// @file PropertyInspector.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"

#include "Editor/PropertyInspector.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// ヘルパー: ComponentTypeを文字列に変換
// ============================================================================
namespace
{
    const char* ComponentTypeToString(ComponentType type)
    {
        switch (type)
        {
        case ComponentType::Transform:           return "TransformComponent";
        case ComponentType::MeshRenderer:        return "MeshRenderer";
        case ComponentType::SkinnedMeshRenderer: return "SkinnedMeshRenderer";
        case ComponentType::Camera:              return "Camera";
        case ComponentType::Light:               return "Light";
        case ComponentType::ParticleSystem:      return "ParticleSystem";
        case ComponentType::AudioSource:         return "AudioSource";
        case ComponentType::Terrain:             return "Terrain";
        case ComponentType::RigidBody:           return "RigidBody";
        case ComponentType::LOD:                 return "LOD";
        case ComponentType::Script:              return "Script";
        case ComponentType::Custom:              return "Custom";
        default:                                 return "Unknown";
        }
    }

    std::string FloatToString(float v)
    {
        // 末尾のノイズを避ける固定精度表現を使用
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.6f", static_cast<double>(v));
        return buf;
    }

    bool TryParseFloat(const std::string& str, float& out)
    {
        try
        {
            out = std::stof(str);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
} // anonymous namespace

// ============================================================================
// SetTarget / ClearTarget
// ============================================================================

void PropertyInspector::SetTarget(Entity* entity)
{
    m_target = entity;
}

void PropertyInspector::ClearTarget()
{
    m_target = nullptr;
}

// ============================================================================
// GetProperties
// ============================================================================

std::vector<PropertyInspector::ComponentProperties> PropertyInspector::GetProperties() const
{
    std::vector<ComponentProperties> result;

    if (!m_target)
        return result;

    // 1. エンティティ基本プロパティ（常に存在）
    {
        ComponentProperties entityProps;
        entityProps.componentName = "Entity";
        entityProps.properties.push_back({ "Name", m_target->GetName(), false });
        entityProps.properties.push_back({ "ID", std::to_string(m_target->GetID()), true });
        result.push_back(std::move(entityProps));
    }

    // 2. トランスフォーム（エンティティに常に存在）
    {
        ComponentProperties transformProps;
        transformProps.componentName = "Transform";

        const Transform3D& t = m_target->GetTransform();
        const XMFLOAT3& pos = t.GetPosition();
        const XMFLOAT3& rot = t.GetRotation();
        const XMFLOAT3& scl = t.GetScale();

        transformProps.properties.push_back({ "Position.X", FloatToString(pos.x), false });
        transformProps.properties.push_back({ "Position.Y", FloatToString(pos.y), false });
        transformProps.properties.push_back({ "Position.Z", FloatToString(pos.z), false });
        transformProps.properties.push_back({ "Rotation.X", FloatToString(rot.x), false });
        transformProps.properties.push_back({ "Rotation.Y", FloatToString(rot.y), false });
        transformProps.properties.push_back({ "Rotation.Z", FloatToString(rot.z), false });
        transformProps.properties.push_back({ "Scale.X", FloatToString(scl.x), false });
        transformProps.properties.push_back({ "Scale.Y", FloatToString(scl.y), false });
        transformProps.properties.push_back({ "Scale.Z", FloatToString(scl.z), false });

        result.push_back(std::move(transformProps));
    }

    // 3. 実際のコンポーネントを走査
    const auto& components = m_target->GetComponents();
    for (const auto& comp : components)
    {
        ComponentProperties compProps;
        compProps.componentName = ComponentTypeToString(comp->GetType());

        // 共通プロパティを追加: Enabled
        compProps.properties.push_back({
            "Enabled",
            comp->IsEnabled() ? "true" : "false",
            false
        });

        // 型固有プロパティを読み取り専用情報として追加
        compProps.properties.push_back({
            "Type",
            ComponentTypeToString(comp->GetType()),
            true
        });

        result.push_back(std::move(compProps));
    }

    return result;
}

// ============================================================================
// SetProperty
// ============================================================================

bool PropertyInspector::SetProperty(const std::string& componentName,
                                     const std::string& propertyName,
                                     const std::string& value)
{
    if (!m_target)
        return false;

    bool handled = false;

    // --- エンティティプロパティ ---
    if (componentName == "Entity")
    {
        if (propertyName == "Name")
        {
            m_target->SetName(value);
            handled = true;
        }
        // IDは読み取り専用; 設定不可
    }
    // --- トランスフォームプロパティ ---
    else if (componentName == "Transform")
    {
        float fval = 0.0f;
        if (!TryParseFloat(value, fval))
        {
            Logger::Warn("PropertyInspector: failed to parse float from '%s'", value.c_str());
            return false;
        }

        Transform3D& t = m_target->GetTransform();
        XMFLOAT3 pos = t.GetPosition();
        XMFLOAT3 rot = t.GetRotation();
        XMFLOAT3 scl = t.GetScale();

        if (propertyName == "Position.X") { pos.x = fval; t.SetPosition(pos); handled = true; }
        else if (propertyName == "Position.Y") { pos.y = fval; t.SetPosition(pos); handled = true; }
        else if (propertyName == "Position.Z") { pos.z = fval; t.SetPosition(pos); handled = true; }
        else if (propertyName == "Rotation.X") { rot.x = fval; t.SetRotation(rot); handled = true; }
        else if (propertyName == "Rotation.Y") { rot.y = fval; t.SetRotation(rot); handled = true; }
        else if (propertyName == "Rotation.Z") { rot.z = fval; t.SetRotation(rot); handled = true; }
        else if (propertyName == "Scale.X") { scl.x = fval; t.SetScale(scl); handled = true; }
        else if (propertyName == "Scale.Y") { scl.y = fval; t.SetScale(scl); handled = true; }
        else if (propertyName == "Scale.Z") { scl.z = fval; t.SetScale(scl); handled = true; }
    }

    // プロパティが処理された場合にコールバックを発火
    if (handled && m_onPropertyChanged)
    {
        PropertyChange change;
        change.componentName = componentName;
        change.propertyName  = propertyName;
        change.newValue      = value;
        m_onPropertyChanged(change);
    }

    return handled;
}

} // namespace gx
