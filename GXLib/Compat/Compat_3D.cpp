/// @file Compat_3D.cpp
/// @brief 簡易API 3D描画関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"

using Ctx = GX_Internal::CompatContext;

/// TCHAR文字列をstd::wstringに変換するヘルパー
static std::wstring ToWString(const TCHAR* str)
{
#ifdef UNICODE
    return std::wstring(str);
#else
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str, -1, result.data(), len);
    return result;
#endif
}

// ============================================================================
// カメラ
// ============================================================================
int SetCameraPositionAndTarget(VECTOR position, VECTOR target)
{
    auto& ctx = Ctx::Instance();
    ctx.camera.SetPosition({ position.x, position.y, position.z });
    ctx.camera.SetTarget({ target.x, target.y, target.z });
    return 0;
}

int SetCameraNearFar(float nearZ, float farZ)
{
    auto& ctx = Ctx::Instance();
    ctx.camera.SetPerspective(
        ctx.camera.GetFovY(),
        ctx.camera.GetAspect(),
        nearZ, farZ);
    return 0;
}

// ============================================================================
// モデル
// ============================================================================
int LoadModel(const TCHAR* filePath)
{
    auto& ctx = Ctx::Instance();
    auto model = ctx.modelLoader.LoadFromFile(
        ToWString(filePath),
        ctx.device,
        ctx.spriteBatch.GetTextureManager(),
        ctx.renderer3D.GetMaterialManager());
    if (!model) return -1;

    int handle = ctx.AllocateModelHandle();
    if (handle < 0) return -1;
    if (handle >= static_cast<int>(ctx.models.size()))
        ctx.models.resize(handle + 1);

    ctx.models[handle].model = std::move(model);
    ctx.models[handle].transform = GX::Transform3D();
    ctx.models[handle].valid = true;

    if (ctx.models[handle].model && ctx.models[handle].model->HasSkeleton())
    {
        ctx.models[handle].animator.SetSkeleton(ctx.models[handle].model->GetSkeleton());
        ctx.models[handle].animator.EvaluateBindPose();
    }
    return handle;
}

int DeleteModel(int handle)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    ctx.models[handle].model.reset();
    ctx.models[handle].valid = false;
    ctx.modelFreeHandles.push_back(handle);
    return 0;
}

int DrawModel(int handle)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;

    auto& entry = ctx.models[handle];
    if (entry.model->IsSkinned() && entry.model->HasSkeleton())
    {
        ctx.renderer3D.DrawSkinnedModel(*entry.model, entry.transform, entry.animator);
    }
    else
    {
        ctx.renderer3D.DrawModel(*entry.model, entry.transform);
    }
    return 0;
}

int SetModelPosition(int handle, VECTOR position)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    ctx.models[handle].transform.SetPosition({ position.x, position.y, position.z });
    return 0;
}

int SetModelScale(int handle, VECTOR scale)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    ctx.models[handle].transform.SetScale({ scale.x, scale.y, scale.z });
    return 0;
}

int SetModelRotation(int handle, VECTOR rotation)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    ctx.models[handle].transform.SetRotation({ rotation.x, rotation.y, rotation.z });
    return 0;
}

// ============================================================================
// モデルのマテリアル／シェーダー
// ============================================================================
int GetModelSubMeshCount(int handle)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    return static_cast<int>(ctx.models[handle].model->GetSubMeshCount());
}

int GetModelSubMeshMaterial(int handle, int subMeshIndex)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    const auto* sub = ctx.models[handle].model->GetSubMesh(static_cast<uint32_t>(subMeshIndex));
    if (!sub) return -1;
    return sub->materialHandle;
}

int SetModelSubMeshMaterial(int handle, int subMeshIndex, int materialHandle)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    return ctx.models[handle].model->SetSubMeshMaterial(
        static_cast<uint32_t>(subMeshIndex), materialHandle) ? 0 : -1;
}

int SetModelSubMeshShader(int handle, int subMeshIndex, int shaderHandle)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    return ctx.models[handle].model->SetSubMeshShader(
        static_cast<uint32_t>(subMeshIndex), shaderHandle) ? 0 : -1;
}

int GetModelMaterialCount(int handle)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    return static_cast<int>(ctx.models[handle].model->GetMaterialHandles().size());
}

int GetModelMaterialHandle(int handle, int materialIndex)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    const auto& mats = ctx.models[handle].model->GetMaterialHandles();
    if (materialIndex < 0 || materialIndex >= static_cast<int>(mats.size())) return -1;
    return mats[materialIndex];
}

int CreateMaterial()
{
    auto& ctx = Ctx::Instance();
    GX::Material mat;
    return ctx.renderer3D.GetMaterialManager().CreateMaterial(mat);
}

int DeleteMaterial(int materialHandle)
{
    auto& ctx = Ctx::Instance();
    ctx.renderer3D.GetMaterialManager().ReleaseMaterial(materialHandle);
    return 0;
}

int SetMaterialParam(int materialHandle, const GX_MATERIAL_PARAM* param)
{
    if (!param) return -1;
    auto& ctx = Ctx::Instance();
    GX::Material* mat = ctx.renderer3D.GetMaterialManager().GetMaterial(materialHandle);
    if (!mat) return -1;

    mat->constants.albedoFactor = { param->albedoR, param->albedoG, param->albedoB, param->albedoA };
    mat->constants.metallicFactor = param->metallic;
    mat->constants.roughnessFactor = param->roughness;
    mat->constants.aoStrength = param->aoStrength;
    mat->constants.emissiveStrength = param->emissiveStrength;
    mat->constants.emissiveFactor = { param->emissiveR, param->emissiveG, param->emissiveB };
    return 0;
}

int SetMaterialTexture(int materialHandle, int slot, int textureHandle)
{
    auto& ctx = Ctx::Instance();
    GX::MaterialTextureSlot slotEnum;
    switch (slot)
    {
    case GX_MATERIAL_TEX_ALBEDO:     slotEnum = GX::MaterialTextureSlot::Albedo; break;
    case GX_MATERIAL_TEX_NORMAL:     slotEnum = GX::MaterialTextureSlot::Normal; break;
    case GX_MATERIAL_TEX_METALROUGH: slotEnum = GX::MaterialTextureSlot::MetalRoughness; break;
    case GX_MATERIAL_TEX_AO:         slotEnum = GX::MaterialTextureSlot::AO; break;
    case GX_MATERIAL_TEX_EMISSIVE:   slotEnum = GX::MaterialTextureSlot::Emissive; break;
    default: return -1;
    }
    return ctx.renderer3D.GetMaterialManager().SetTexture(materialHandle, slotEnum, textureHandle) ? 0 : -1;
}

int SetMaterialShader(int materialHandle, int shaderHandle)
{
    auto& ctx = Ctx::Instance();
    return ctx.renderer3D.GetMaterialManager().SetShaderHandle(materialHandle, shaderHandle) ? 0 : -1;
}

int CreateMaterialShader(const TCHAR* vsPath, const TCHAR* psPath)
{
    if (!vsPath || !psPath) return -1;
    auto& ctx = Ctx::Instance();
    GX::ShaderProgramDesc desc;
    desc.vsPath = ToWString(vsPath);
    desc.psPath = ToWString(psPath);
    return ctx.renderer3D.CreateMaterialShader(desc);
}

// ============================================================================
// モデルアニメーション（Animator）
// ============================================================================
int GetModelAnimationCount(int handle)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    return static_cast<int>(ctx.models[handle].model->GetAnimationCount());
}

int PlayModelAnimation(int handle, int animIndex, int loop)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    const auto& anims = ctx.models[handle].model->GetAnimations();
    if (animIndex < 0 || animIndex >= static_cast<int>(anims.size())) return -1;
    ctx.models[handle].animator.Play(&anims[animIndex], loop != 0);
    return 0;
}

int CrossFadeModelAnimation(int handle, int animIndex, float duration, int loop)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    const auto& anims = ctx.models[handle].model->GetAnimations();
    if (animIndex < 0 || animIndex >= static_cast<int>(anims.size())) return -1;
    ctx.models[handle].animator.CrossFade(&anims[animIndex], duration, loop != 0);
    return 0;
}

int StopModelAnimation(int handle)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    ctx.models[handle].animator.Stop();
    return 0;
}

int UpdateModelAnimation(int handle, float deltaTime)
{
    auto& ctx = Ctx::Instance();
    if (handle < 0 || handle >= static_cast<int>(ctx.models.size())) return -1;
    if (!ctx.models[handle].valid) return -1;
    ctx.models[handle].animator.Update(deltaTime);
    return 0;
}
