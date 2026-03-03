/// @file PropertyInspector.cpp
/// @brief リフレクションベースのプロパティインスペクター ImGui 実装
#include "pch_graphics.h"
#include "Editor/PropertyInspector.h"
#include "Core/Scene/Entity.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
#include "Core/Reflect/TypeInfo.h"
#include "Core/Reflect/TypeRegistry.h"
#include "Core/Reflect/PropertyMeta.h"
#include "Math/Vector3.h"

#include <imgui.h>

namespace gx
{

// ============================================================================
// コンポーネント型名テーブル
// ============================================================================
namespace
{

const char* GetComponentTypeName(ComponentType type)
{
    switch (type)
    {
    case ComponentType::Transform:          return "Transform";
    case ComponentType::MeshRenderer:       return "MeshRenderer";
    case ComponentType::SkinnedMeshRenderer:return "SkinnedMeshRenderer";
    case ComponentType::Camera:             return "Camera";
    case ComponentType::Light:              return "Light";
    case ComponentType::ParticleSystem:     return "ParticleSystem";
    case ComponentType::AudioSource:        return "AudioSource";
    case ComponentType::Terrain:            return "Terrain";
    case ComponentType::RigidBody:          return "RigidBody";
    case ComponentType::LOD:               return "LOD";
    case ComponentType::Script:            return "Script";
    case ComponentType::Custom:            return "Custom";
    default:                               return "Unknown";
    }
}

} // anonymous namespace

// ============================================================================
// メイン描画
// ============================================================================

void PropertyInspector::Draw(Entity* entity, Scene* scene)
{
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(320, 500), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Inspector", &m_visible))
    {
        ImGui::End();
        return;
    }

    if (!entity)
    {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    // エンティティ名の編集
    char nameBuf[256];
    snprintf(nameBuf, sizeof(nameBuf), "%s", entity->GetName().c_str());
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        entity->SetName(nameBuf);
    }

    // ID 表示
    ImGui::Text("ID: %u", entity->GetID());

    // アクティブ状態
    bool active = entity->IsActive();
    if (ImGui::Checkbox("Active", &active))
    {
        entity->SetActive(active);
    }

    ImGui::Separator();

    // ============================================================
    // Transform (ビルトイン -- 常に表示)
    // ============================================================
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& transform = entity->GetTransform();

        Vector3 pos = transform.GetPosition();
        float posArr[3] = { pos.x, pos.y, pos.z };
        if (ImGui::DragFloat3("Position", posArr, 0.1f))
        {
            transform.SetPosition(Vector3{ posArr[0], posArr[1], posArr[2] });
        }

        Vector3 rot = transform.GetRotation();
        float rotArr[3] = { rot.x, rot.y, rot.z };
        if (ImGui::DragFloat3("Rotation", rotArr, 1.0f))
        {
            transform.SetRotation(Vector3{ rotArr[0], rotArr[1], rotArr[2] });
        }

        Vector3 scl = transform.GetScale();
        float sclArr[3] = { scl.x, scl.y, scl.z };
        if (ImGui::DragFloat3("Scale", sclArr, 0.01f))
        {
            transform.SetScale(Vector3{ sclArr[0], sclArr[1], sclArr[2] });
        }
    }

    // ============================================================
    // 各コンポーネント
    // ============================================================
    const auto& components = entity->GetComponents();
    for (const auto& comp : components)
    {
        ComponentType type = comp->GetType();
        const char* typeName = GetComponentTypeName(type);

        ImGui::PushID(static_cast<int>(reinterpret_cast<uintptr_t>(comp.get())));

        bool headerOpen = ImGui::CollapsingHeader(typeName, ImGuiTreeNodeFlags_DefaultOpen);

        // コンポーネントの右クリックメニュー
        if (ImGui::BeginPopupContextItem())
        {
            bool enabled = comp->IsEnabled();
            if (ImGui::MenuItem(enabled ? "Disable" : "Enable"))
            {
                comp->SetEnabled(!enabled);
            }
            ImGui::EndPopup();
        }

        if (headerOpen)
        {
            // 有効/無効表示
            bool enabled = comp->IsEnabled();
            if (ImGui::Checkbox("Enabled", &enabled))
            {
                comp->SetEnabled(enabled);
            }

            // リフレクション情報があれば使う
            const TypeInfo* typeInfo = TypeRegistry::Instance().FindType(typeName);
            if (typeInfo)
            {
                DrawComponent(typeInfo, comp.get());
            }
            else
            {
                // リフレクション未登録の場合、型に応じた基本表示
                switch (type)
                {
                case ComponentType::AudioSource:
                {
                    auto* audio = static_cast<AudioSourceComponent*>(comp.get());
                    ImGui::DragInt("Sound Handle", &audio->soundHandle);
                    ImGui::Checkbox("Play on Start", &audio->playOnStart);
                    ImGui::Checkbox("Loop", &audio->loop);
                    break;
                }
                case ComponentType::Transform:
                {
                    auto* tc = static_cast<TransformComponent*>(comp.get());
                    float p[3] = { tc->position.x, tc->position.y, tc->position.z };
                    if (ImGui::DragFloat3("Comp Position", p, 0.1f))
                        tc->position = { p[0], p[1], p[2] };
                    float r[3] = { tc->rotation.x, tc->rotation.y, tc->rotation.z };
                    if (ImGui::DragFloat3("Comp Rotation", r, 1.0f))
                        tc->rotation = { r[0], r[1], r[2] };
                    float s[3] = { tc->scale.x, tc->scale.y, tc->scale.z };
                    if (ImGui::DragFloat3("Comp Scale", s, 0.01f))
                        tc->scale = { s[0], s[1], s[2] };
                    break;
                }
                default:
                    ImGui::TextDisabled("(No reflection data for %s)", typeName);
                    break;
                }
            }
        }

        ImGui::PopID();
    }

    ImGui::Separator();

    // ============================================================
    // Add Component ボタン
    // ============================================================
    float buttonWidth = ImGui::GetContentRegionAvail().x;
    if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0)))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (ImGui::MenuItem("Transform") && !entity->HasComponent<TransformComponent>())
        {
            entity->AddComponent<TransformComponent>();
        }
        if (ImGui::MenuItem("Particle System") && !entity->HasComponent<ParticleSystemComponent>())
        {
            entity->AddComponent<ParticleSystemComponent>();
        }
        if (ImGui::MenuItem("Audio Source") && !entity->HasComponent<AudioSourceComponent>())
        {
            entity->AddComponent<AudioSourceComponent>();
        }
        if (ImGui::MenuItem("Script") && !entity->HasComponent<ScriptComponent>())
        {
            entity->AddComponent<ScriptComponent>();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

// ============================================================================
// リフレクションベースのコンポーネント描画
// ============================================================================

void PropertyInspector::DrawComponent(const TypeInfo* typeInfo, void* componentData)
{
    if (!typeInfo || !componentData) return;

    const auto& properties = typeInfo->GetProperties();
    for (const auto& prop : properties)
    {
        DrawProperty(prop, componentData);
    }
}

// ============================================================================
// 単一プロパティ描画
// ============================================================================

void PropertyInspector::DrawProperty(const PropertyMeta& prop, void* base)
{
    if (!base) return;

    const char* label = prop.displayName.empty() ? prop.name.c_str() : prop.displayName.c_str();
    void* ptr = static_cast<uint8_t*>(base) + prop.offset;

    if (prop.readOnly)
    {
        ImGui::BeginDisabled();
    }

    switch (prop.type)
    {
    case PropertyType::Bool:
    {
        bool* val = static_cast<bool*>(ptr);
        ImGui::Checkbox(label, val);
        break;
    }
    case PropertyType::Int:
    {
        int* val = static_cast<int*>(ptr);
        if (prop.hasRange)
            ImGui::SliderInt(label, val, static_cast<int>(prop.minValue), static_cast<int>(prop.maxValue));
        else
            ImGui::DragInt(label, val);
        break;
    }
    case PropertyType::Float:
    {
        float* val = static_cast<float*>(ptr);
        if (prop.hasRange)
            ImGui::SliderFloat(label, val, prop.minValue, prop.maxValue, "%.3f");
        else
            ImGui::DragFloat(label, val, 0.01f);
        break;
    }
    case PropertyType::String:
    {
        // String は getter/setter で処理
        if (prop.getter && prop.setter)
        {
            gx::String strVal;
            prop.getter(base, &strVal);
            char buf[512];
            snprintf(buf, sizeof(buf), "%s", strVal.c_str());
            if (ImGui::InputText(label, buf, sizeof(buf)))
            {
                gx::String newVal(buf);
                prop.setter(base, &newVal);
            }
        }
        else
        {
            ImGui::TextDisabled("%s: (string - no accessor)", label);
        }
        break;
    }
    case PropertyType::Vec3:
    {
        float* val = static_cast<float*>(ptr);
        ImGui::DragFloat3(label, val, 0.1f);
        break;
    }
    case PropertyType::Vec4:
    {
        float* val = static_cast<float*>(ptr);
        ImGui::DragFloat4(label, val, 0.1f);
        break;
    }
    case PropertyType::Color:
    {
        float* val = static_cast<float*>(ptr);
        ImGui::ColorEdit4(label, val);
        break;
    }
    case PropertyType::Quaternion:
    {
        float* val = static_cast<float*>(ptr);
        ImGui::DragFloat4(label, val, 0.01f);
        break;
    }
    case PropertyType::Enum:
    {
        int* val = static_cast<int*>(ptr);
        ImGui::DragInt(label, val);
        break;
    }
    case PropertyType::Custom:
    {
        ImGui::TextDisabled("%s: (custom type)", label);
        break;
    }
    }

    if (prop.readOnly)
    {
        ImGui::EndDisabled();
    }
}

} // namespace gx
