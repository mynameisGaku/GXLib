/// @file PropertyPanel.cpp
/// @brief Property panel implementation

#include "PropertyPanel.h"
#include <imgui.h>
#include <cstring>
#include <cmath>

#ifndef XM_PI
#define XM_PI 3.14159265358979323846f
#endif

static constexpr float k_DegToRad = XM_PI / 180.0f;
static constexpr float k_RadToDeg = 180.0f / XM_PI;

void PropertyPanel::Draw(SceneGraph& scene, GX::MaterialManager& matManager, GX::TextureManager& texManager,
                         ImGuizmo::OPERATION& gizmoOp, ImGuizmo::MODE& gizmoMode,
                         bool& useSnap, float& snapT, float& snapR, float& snapS)
{
    if (!ImGui::Begin("Properties"))
    {
        ImGui::End();
        return;
    }
    DrawContent(scene, matManager, texManager, gizmoOp, gizmoMode, useSnap, snapT, snapR, snapS);
    ImGui::End();
}

void PropertyPanel::DrawContent(SceneGraph& scene, GX::MaterialManager& matManager, GX::TextureManager& texManager,
                                ImGuizmo::OPERATION& gizmoOp, ImGuizmo::MODE& gizmoMode,
                                bool& useSnap, float& snapT, float& snapR, float& snapS)
{
    (void)texManager;

    if (scene.selectedEntity < 0)
    {
        ImGui::TextDisabled("No entity selected");
        return;
    }

    SceneEntity* entity = scene.GetEntity(scene.selectedEntity);
    if (!entity)
    {
        ImGui::TextDisabled("Invalid entity");
        return;
    }

    // --- Name ---
    strncpy(m_nameBuffer, entity->name.c_str(), sizeof(m_nameBuffer) - 1);
    m_nameBuffer[sizeof(m_nameBuffer) - 1] = '\0';
    if (ImGui::InputText("Name", m_nameBuffer, sizeof(m_nameBuffer)))
    {
        entity->name = m_nameBuffer;
    }

    ImGui::Separator();

    // --- Visible ---
    ImGui::Checkbox("Visible", &entity->visible);

    ImGui::Separator();

    // --- Gizmo Mode ---
    if (ImGui::CollapsingHeader("Gizmo", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Operation radio buttons
        if (ImGui::RadioButton("Translate (T)", gizmoOp == ImGuizmo::TRANSLATE))
            gizmoOp = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate (E)", gizmoOp == ImGuizmo::ROTATE))
            gizmoOp = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale (R)", gizmoOp == ImGuizmo::SCALE))
            gizmoOp = ImGuizmo::SCALE;

        // World / Local toggle
        if (gizmoOp != ImGuizmo::SCALE)
        {
            if (ImGui::RadioButton("World", gizmoMode == ImGuizmo::WORLD))
                gizmoMode = ImGuizmo::WORLD;
            ImGui::SameLine();
            if (ImGui::RadioButton("Local (L)", gizmoMode == ImGuizmo::LOCAL))
                gizmoMode = ImGuizmo::LOCAL;
        }

        // Snap controls
        ImGui::Checkbox("Snap", &useSnap);
        if (useSnap)
        {
            ImGui::Indent();
            if (gizmoOp == ImGuizmo::TRANSLATE)
                ImGui::DragFloat("Snap Value", &snapT, 0.05f, 0.01f, 100.0f, "%.2f");
            else if (gizmoOp == ImGuizmo::ROTATE)
                ImGui::DragFloat("Snap Angle", &snapR, 1.0f, 1.0f, 180.0f, "%.0f deg");
            else if (gizmoOp == ImGuizmo::SCALE)
                ImGui::DragFloat("Snap Scale", &snapS, 0.01f, 0.01f, 10.0f, "%.2f");
            ImGui::Unindent();
        }
    }

    ImGui::Separator();

    // --- Transform ---
    DrawTransformSection(*entity);

    ImGui::Separator();

    // --- Model Materials (direct editing) ---
    DrawModelMaterials(*entity, matManager);

    ImGui::Separator();

    // --- Rendering ---
    if (ImGui::CollapsingHeader("Rendering"))
    {
        ImGui::Checkbox("Show Bones", &entity->showBones);
        ImGui::Checkbox("Wireframe", &entity->showWireframe);
    }

    ImGui::Separator();

    // --- Material Override ---
    DrawMaterialOverrideSection(*entity);
}

void PropertyPanel::DrawTransformSection(SceneEntity& entity)
{
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Position
    XMFLOAT3 pos = entity.transform.GetPosition();
    if (ImGui::DragFloat3("Position", &pos.x, 0.05f))
    {
        entity.transform.SetPosition(pos);
    }

    // Rotation (display in degrees, store in radians)
    XMFLOAT3 rot = entity.transform.GetRotation();
    float rotDeg[3] = { rot.x * k_RadToDeg, rot.y * k_RadToDeg, rot.z * k_RadToDeg };
    if (ImGui::DragFloat3("Rotation", rotDeg, 0.5f, -360.0f, 360.0f))
    {
        entity.transform.SetRotation(rotDeg[0] * k_DegToRad, rotDeg[1] * k_DegToRad, rotDeg[2] * k_DegToRad);
    }

    // Scale
    XMFLOAT3 scl = entity.transform.GetScale();
    if (ImGui::DragFloat3("Scale", &scl.x, 0.01f, 0.001f, 100.0f))
    {
        entity.transform.SetScale(scl);
    }
}

void PropertyPanel::DrawMaterialOverrideSection(SceneEntity& entity)
{
    if (!ImGui::CollapsingHeader("Material Override", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ImGui::Checkbox("Override Material", &entity.useMaterialOverride);

    if (!entity.useMaterialOverride)
    {
        ImGui::TextDisabled("Material override is disabled.");
        return;
    }

    GX::Material& mat = entity.materialOverride;

    // Shader Model combo
    const char* shaderModelItems[] = { "Standard", "Unlit", "Toon", "Phong", "Subsurface", "ClearCoat" };
    int currentModel = static_cast<int>(mat.shaderModel);
    if (currentModel > 5) currentModel = 0;
    if (ImGui::Combo("Shader Model", &currentModel, shaderModelItems, 6))
    {
        gxfmt::ShaderModel newModel = static_cast<gxfmt::ShaderModel>(currentModel);
        if (newModel != mat.shaderModel)
        {
            mat.shaderModel = newModel;
            mat.shaderParams = gxfmt::DefaultShaderModelParams(newModel);
        }
    }

    ImGui::Separator();

    // --- Common parameters ---
    ImGui::Text("Common");
    ImGui::ColorEdit4("Base Color", mat.shaderParams.baseColor);
    ImGui::SliderFloat("Metallic", &mat.shaderParams.metallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &mat.shaderParams.roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("AO Strength", &mat.shaderParams.aoStrength, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Emissive");
    ImGui::ColorEdit3("Emissive Factor", mat.shaderParams.emissiveFactor);
    ImGui::SliderFloat("Emissive Strength", &mat.shaderParams.emissiveStrength, 0.0f, 10.0f);

    // Also update MaterialConstants to keep them in sync
    mat.constants.albedoFactor = { mat.shaderParams.baseColor[0], mat.shaderParams.baseColor[1],
                                   mat.shaderParams.baseColor[2], mat.shaderParams.baseColor[3] };
    mat.constants.metallicFactor  = mat.shaderParams.metallic;
    mat.constants.roughnessFactor = mat.shaderParams.roughness;
    mat.constants.aoStrength      = mat.shaderParams.aoStrength;
    mat.constants.emissiveFactor  = { mat.shaderParams.emissiveFactor[0],
                                      mat.shaderParams.emissiveFactor[1],
                                      mat.shaderParams.emissiveFactor[2] };
    mat.constants.emissiveStrength = mat.shaderParams.emissiveStrength;

    // --- Shader-model-specific parameters ---
    DrawShaderModelParams(mat.shaderParams, mat.shaderModel);
}

void PropertyPanel::DrawModelMaterials(SceneEntity& entity, GX::MaterialManager& matManager)
{
    if (!entity.model) return;
    if (!ImGui::CollapsingHeader("Model Materials", ImGuiTreeNodeFlags_DefaultOpen)) return;

    const auto& subMeshes = entity.model->GetMesh().GetSubMeshes();
    if (subMeshes.empty())
    {
        ImGui::TextDisabled("No materials");
        return;
    }

    for (size_t i = 0; i < subMeshes.size(); ++i)
    {
        int matHandle = subMeshes[i].materialHandle;
        if (matHandle < 0) continue;

        GX::Material* mat = matManager.GetMaterial(matHandle);
        if (!mat) continue;

        ImGui::PushID(static_cast<int>(i));

        // サブメッシュ可視性チェックボックス
        if (i < entity.submeshVisibility.size())
        {
            bool vis = entity.submeshVisibility[i];
            if (ImGui::Checkbox("##vis", &vis))
                entity.submeshVisibility[i] = vis;
            ImGui::SameLine();
        }

        char label[64];
        snprintf(label, sizeof(label), "Material %d", static_cast<int>(i));
        if (ImGui::TreeNode(label))
        {
            // Shader Model combo
            const char* items[] = { "Standard", "Unlit", "Toon", "Phong", "Subsurface", "ClearCoat" };
            int cur = static_cast<int>(mat->shaderModel);
            if (cur > 5) cur = 0;
            if (ImGui::Combo("Shader Model", &cur, items, 6))
            {
                auto newModel = static_cast<gxfmt::ShaderModel>(cur);
                if (newModel != mat->shaderModel)
                {
                    mat->shaderModel = newModel;
                    mat->shaderParams = gxfmt::DefaultShaderModelParams(newModel);
                }
            }

            // Common params
            ImGui::ColorEdit4("Base Color", mat->shaderParams.baseColor);
            ImGui::SliderFloat("Metallic", &mat->shaderParams.metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &mat->shaderParams.roughness, 0.0f, 1.0f);
            ImGui::SliderFloat("AO Strength", &mat->shaderParams.aoStrength, 0.0f, 1.0f);
            ImGui::ColorEdit3("Emissive", mat->shaderParams.emissiveFactor);
            ImGui::SliderFloat("Emissive Strength", &mat->shaderParams.emissiveStrength, 0.0f, 10.0f);

            // Sync MaterialConstants for backward compatibility
            mat->constants.albedoFactor = { mat->shaderParams.baseColor[0],
                mat->shaderParams.baseColor[1], mat->shaderParams.baseColor[2],
                mat->shaderParams.baseColor[3] };
            mat->constants.metallicFactor  = mat->shaderParams.metallic;
            mat->constants.roughnessFactor = mat->shaderParams.roughness;
            mat->constants.aoStrength      = mat->shaderParams.aoStrength;
            mat->constants.emissiveFactor  = { mat->shaderParams.emissiveFactor[0],
                mat->shaderParams.emissiveFactor[1], mat->shaderParams.emissiveFactor[2] };
            mat->constants.emissiveStrength = mat->shaderParams.emissiveStrength;

            // Shader-model specific params
            DrawShaderModelParams(mat->shaderParams, mat->shaderModel);

            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void PropertyPanel::DrawShaderModelParams(gxfmt::ShaderModelParams& params, gxfmt::ShaderModel model)
{
    switch (model)
    {
    case gxfmt::ShaderModel::Standard:
        // Standard PBR has no extra params beyond the common ones
        break;

    case gxfmt::ShaderModel::Unlit:
        // Unlit has no extra params
        ImGui::TextDisabled("Unlit: no additional parameters.");
        break;

    case gxfmt::ShaderModel::Toon:
    {
        ImGui::Separator();
        ImGui::Text("Shade");
        ImGui::ColorEdit4("1st Shade Color", params.shadeColor);
        ImGui::ColorEdit4("2nd Shade Color", params.shade2ndColor);
        ImGui::SliderFloat("Base Color Step", &params.baseColorStep, 0.0f, 1.0f);
        ImGui::SliderFloat("Base Shade Feather", &params.baseShadeFeather, 0.0f, 1.0f);
        ImGui::SliderFloat("Shade Color Step", &params.shadeColorStep, 0.0f, 1.0f);
        ImGui::SliderFloat("1st-2nd Shade Feather", &params.shade1st2ndFeather, 0.0f, 1.0f);
        ImGui::SliderFloat("Shadow Receive Level", &params.shadowReceiveLevel, 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("Outline");
        ImGui::SliderFloat("Outline Width", &params.outlineWidth, 0.0f, 5.0f, "%.3f");
        ImGui::ColorEdit3("Outline Color", params.outlineColor);
        ImGui::SliderFloat("Near Distance", &params.toonOutlineNearDist(), 0.0f, 10.0f);
        ImGui::SliderFloat("Far Distance", &params.toonOutlineFarDist(), 1.0f, 500.0f);
        ImGui::SliderFloat("Base Color Blend", &params.toonOutlineBlendBaseColor(), 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("Rim Light");
        ImGui::ColorEdit4("Rim Color", params.rimColor);
        ImGui::SliderFloat("Rim Power", &params.rimPower, 0.1f, 10.0f);
        ImGui::SliderFloat("Rim Intensity", &params.rimIntensity, 0.0f, 5.0f);
        ImGui::SliderFloat("Rim Inside Mask", &params.rimInsideMask, 0.0f, 1.0f);
        ImGui::SliderFloat("Light Dir Mask", &params.toonRimLightDirMask(), 0.0f, 1.0f);
        {
            bool featherOff = params.toonRimFeatherOff() > 0.5f;
            if (ImGui::Checkbox("Rim Feather Off", &featherOff))
                params.toonRimFeatherOff() = featherOff ? 1.0f : 0.0f;
        }
        ImGui::Separator();
        ImGui::Text("Specular");
        ImGui::ColorEdit3("High Color", params.highColor);
        ImGui::SliderFloat("High Color Power", &params.highColorPower, 1.0f, 128.0f, "%.1f");
        ImGui::SliderFloat("High Color Intensity", &params.highColorIntensity, 0.0f, 2.0f);
        ImGui::SliderFloat("High Color on Shadow", &params.toonHighColorOnShadow(), 0.0f, 1.0f);
        {
            bool blendAdd = params.toonHighColorBlendAdd() > 0.5f;
            if (ImGui::Checkbox("Additive Blend", &blendAdd))
                params.toonHighColorBlendAdd() = blendAdd ? 1.0f : 0.0f;
        }
        break;
    }

    case gxfmt::ShaderModel::Phong:
    {
        ImGui::Separator();
        ImGui::Text("Phong Parameters");
        ImGui::ColorEdit3("Specular Color", params.specularColor);
        ImGui::SliderFloat("Shininess", &params.shininess, 1.0f, 256.0f);
        break;
    }

    case gxfmt::ShaderModel::Subsurface:
    {
        ImGui::Separator();
        ImGui::Text("Subsurface Parameters");
        ImGui::ColorEdit3("Subsurface Color", params.subsurfaceColor);
        ImGui::SliderFloat("Subsurface Radius", &params.subsurfaceRadius, 0.0f, 5.0f);
        ImGui::SliderFloat("Subsurface Strength", &params.subsurfaceStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Thickness", &params.thickness, 0.0f, 1.0f);
        break;
    }

    case gxfmt::ShaderModel::ClearCoat:
    {
        ImGui::Separator();
        ImGui::Text("ClearCoat Parameters");
        ImGui::SliderFloat("ClearCoat Strength", &params.clearCoatStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("ClearCoat Roughness", &params.clearCoatRoughness, 0.0f, 1.0f);
        break;
    }

    default:
        break;
    }
}
