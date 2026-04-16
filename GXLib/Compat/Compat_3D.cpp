/// @file Compat_3D.cpp
/// @brief 簡易API 3D描画関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "Compat/CompatUtil.h"
#include "Core/Logger.h"

using Ctx = gx_internal::CompatContext;

// ----- internal: Layer 1 friendly model-handle validation (per ADR-0017) -----
#define GX_VALIDATE_MODEL_HANDLE(fn, h)                                           \
    do {                                                                          \
        if ((h) < 0 || (h) >= static_cast<int>(ctx.models.size())) {              \
            GX_LOG_ERROR(fn ": invalid model handle %d (out of range)", (h));     \
            return -1;                                                            \
        }                                                                         \
        if (!ctx.models[(h)].valid) {                                             \
            GX_LOG_ERROR(fn ": model handle %d is invalid (already deleted?)", (h));\
            return -1;                                                            \
        }                                                                         \
    } while (0)

namespace gx {

// ============================================================================
// カメラ
// ============================================================================
int SetCameraPositionAndTarget(VECTOR position, VECTOR target)
{
    auto& ctx = Ctx::Instance();
    ctx.camera.SetPosition(position);
    ctx.camera.SetTarget(target);
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
// ハンドルベースのモデル管理。フリーリストから再利用するか新規割り当て。
// スケルトン付きモデルの場合はAnimatorも初期化してバインドポーズを適用する。
int LoadModel(const char* filePath)
{
    auto& ctx = Ctx::Instance();
    auto model = ctx.modelLoader.LoadFromFile(
        gx_internal::ToWString(filePath),
        ctx.device,
        ctx.spriteBatch.GetTextureManager(),
        ctx.renderer3D.GetMaterialManager());
    if (!model) {
        GX_LOG_ERROR("MV1LoadModel: failed to load '%s' (file not found or unsupported format)", filePath ? filePath : "(null)");
        return -1;
    }

    int handle = ctx.AllocateModelHandle();
    if (handle < 0) {
        GX_LOG_ERROR("MV1LoadModel: failed to allocate model handle (handle pool exhausted)");
        return -1;
    }
    if (handle >= static_cast<int>(ctx.models.size()))
        ctx.models.resize(handle + 1);

    ctx.models[handle].model = std::move(model);
    ctx.models[handle].transform = gx::Transform3D();
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
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    ctx.models[handle].model.reset();
    ctx.models[handle].valid = false;
    ctx.modelFreeHandles.push_back(handle);
    return 0;
}

// スキンメッシュならDrawSkinnedModel、スタティックメッシュならDrawModelを使い分ける
int DrawModel(int handle)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);

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
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    ctx.models[handle].transform.SetPosition(position);
    return 0;
}

int SetModelScale(int handle, VECTOR scale)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    ctx.models[handle].transform.SetScale(scale);
    return 0;
}

int SetModelRotation(int handle, VECTOR rotation)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    ctx.models[handle].transform.SetRotation(rotation);
    return 0;
}

// ============================================================================
// モデルのマテリアル／シェーダー
// ============================================================================
int GetModelSubMeshCount(int handle)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    return static_cast<int>(ctx.models[handle].model->GetSubMeshCount());
}

int GetModelSubMeshMaterial(int handle, int subMeshIndex)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    const auto* sub = ctx.models[handle].model->GetSubMesh(static_cast<uint32_t>(subMeshIndex));
    if (!sub) {
        GX_LOG_ERROR("GetModelSubMeshMaterial: subMeshIndex %d not found in model handle %d", subMeshIndex, handle);
        return -1;
    }
    return sub->materialHandle;
}

int SetModelSubMeshMaterial(int handle, int subMeshIndex, int materialHandle)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    return ctx.models[handle].model->SetSubMeshMaterial(
        static_cast<uint32_t>(subMeshIndex), materialHandle) ? 0 : -1;
}

int SetModelSubMeshShader(int handle, int subMeshIndex, int shaderHandle)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    return ctx.models[handle].model->SetSubMeshShader(
        static_cast<uint32_t>(subMeshIndex), shaderHandle) ? 0 : -1;
}

int GetModelMaterialCount(int handle)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    return static_cast<int>(ctx.models[handle].model->GetMaterialHandles().size());
}

int GetModelMaterialHandle(int handle, int materialIndex)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    const auto& mats = ctx.models[handle].model->GetMaterialHandles();
    if (materialIndex < 0 || materialIndex >= static_cast<int>(mats.size())) {
        GX_LOG_ERROR("GetModelMaterialHandle: materialIndex %d out of range [0,%zu) for model handle %d", materialIndex, mats.size(), handle);
        return -1;
    }
    return mats[materialIndex];
}

int CreateMaterial()
{
    auto& ctx = Ctx::Instance();
    Material mat;
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
    if (!param) {
        GX_LOG_ERROR("SetMaterialParam: param is null (materialHandle=%d)", materialHandle);
        return -1;
    }
    auto& ctx = Ctx::Instance();
    Material* mat = ctx.renderer3D.GetMaterialManager().GetMaterial(materialHandle);
    if (!mat) {
        GX_LOG_ERROR("SetMaterialParam: invalid materialHandle %d (material not created or already deleted)", materialHandle);
        return -1;
    }

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
    MaterialTextureSlot slotEnum;
    switch (slot)
    {
    case GX_MATERIAL_TEX_ALBEDO:     slotEnum = MaterialTextureSlot::Albedo; break;
    case GX_MATERIAL_TEX_NORMAL:     slotEnum = MaterialTextureSlot::Normal; break;
    case GX_MATERIAL_TEX_METALROUGH: slotEnum = MaterialTextureSlot::MetalRoughness; break;
    case GX_MATERIAL_TEX_AO:         slotEnum = MaterialTextureSlot::AO; break;
    case GX_MATERIAL_TEX_EMISSIVE:   slotEnum = MaterialTextureSlot::Emissive; break;
    default:
        GX_LOG_ERROR("SetMaterialTexture: unknown slot %d (expected GX_MATERIAL_TEX_*)", slot);
        return -1;
    }
    return ctx.renderer3D.GetMaterialManager().SetTexture(materialHandle, slotEnum, textureHandle) ? 0 : -1;
}

int SetMaterialShader(int materialHandle, int shaderHandle)
{
    auto& ctx = Ctx::Instance();
    return ctx.renderer3D.GetMaterialManager().SetShaderHandle(materialHandle, shaderHandle) ? 0 : -1;
}

int SetMaterialShaderModel(int materialHandle, int shaderModelId)
{
    if (shaderModelId < 0 || shaderModelId > 255) {
        GX_LOG_ERROR("SetMaterialShaderModel: shaderModelId %d out of [0,255] (matH=%d)",
                     shaderModelId, materialHandle);
        return -1;
    }
    auto& ctx = Ctx::Instance();
    auto model = static_cast<gxfmt::ShaderModel>(shaderModelId);
    if (!ctx.renderer3D.GetMaterialManager().SetShaderModel(materialHandle, model)) {
        GX_LOG_ERROR("SetMaterialShaderModel: invalid materialHandle %d", materialHandle);
        return -1;
    }
    return 0;
}

int SetModelShaderModel(int modelHandle, int shaderModelId)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("SetModelShaderModel", modelHandle);
    if (shaderModelId < 0 || shaderModelId > 255) {
        GX_LOG_ERROR("SetModelShaderModel: shaderModelId %d out of [0,255] (modelH=%d)",
                     shaderModelId, modelHandle);
        return -1;
    }

    const auto& mats = ctx.models[modelHandle].model->GetMaterialHandles();
    auto model = static_cast<gxfmt::ShaderModel>(shaderModelId);
    int applied = 0;
    for (int matH : mats) {
        if (ctx.renderer3D.GetMaterialManager().SetShaderModel(matH, model))
            ++applied;
    }
    if (applied == 0) {
        GX_LOG_ERROR("SetModelShaderModel: no materials updated (modelH=%d, %zu materials)",
                     modelHandle, mats.size());
        return -1;
    }
    return applied;
}

int CreateMaterialShader(const char* vsPath, const char* psPath)
{
    if (!vsPath || !psPath) {
        GX_LOG_ERROR("CreateMaterialShader: vsPath or psPath is null (vsPath=%p psPath=%p)", (void*)vsPath, (void*)psPath);
        return -1;
    }
    auto& ctx = Ctx::Instance();
    ShaderProgramDesc desc;
    desc.vsPath = gx_internal::ToWString(vsPath);
    desc.psPath = gx_internal::ToWString(psPath);
    return ctx.renderer3D.CreateMaterialShader(desc);
}

// ============================================================================
// モデルアニメーション（Animator）
// ============================================================================
int GetModelAnimationCount(int handle)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    return static_cast<int>(ctx.models[handle].model->GetAnimationCount());
}

int PlayModelAnimation(int handle, int animIndex, int loop)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    const auto& anims = ctx.models[handle].model->GetAnimations();
    if (animIndex < 0 || animIndex >= static_cast<int>(anims.size())) {
        GX_LOG_ERROR("Compat_3D: animIndex %d out of range [0,%zu) for model handle %d", animIndex, anims.size(), handle);
        return -1;
    }
    ctx.models[handle].animator.Play(&anims[animIndex], loop != 0);
    return 0;
}

int CrossFadeModelAnimation(int handle, int animIndex, float duration, int loop)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    const auto& anims = ctx.models[handle].model->GetAnimations();
    if (animIndex < 0 || animIndex >= static_cast<int>(anims.size())) {
        GX_LOG_ERROR("Compat_3D: animIndex %d out of range [0,%zu) for model handle %d", animIndex, anims.size(), handle);
        return -1;
    }
    ctx.models[handle].animator.CrossFade(&anims[animIndex], duration, loop != 0);
    return 0;
}

int StopModelAnimation(int handle)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    ctx.models[handle].animator.Stop();
    return 0;
}

int UpdateModelAnimation(int handle, float deltaTime)
{
    auto& ctx = Ctx::Instance();
    GX_VALIDATE_MODEL_HANDLE("Compat_3D", handle);
    ctx.models[handle].animator.Update(deltaTime);
    return 0;
}

} // namespace gx
