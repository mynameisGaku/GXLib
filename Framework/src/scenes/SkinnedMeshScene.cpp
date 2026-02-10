/// @file SkinnedMeshScene.cpp
/// @brief FBX + Animator + スキンドMeshColliderのデモシーン。3Dモデルと物理の連携を試せる。
#include "SkinnedMeshScene.h"
#include "FrameworkApp.h"
#include "Core/Logger.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Quaternion.h"
#include <filesystem>

namespace GXFW
{

void SkinnedMeshScene::OnEnter(SceneContext& ctx)
{
    if (!ctx.renderer || !ctx.camera || !ctx.graphics || !ctx.physics)
        return;

    ctx.renderer->SetShadowEnabled(false);
    ctx.camera->SetPerspective(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 200.0f);
    ctx.camera->SetPosition(0.0f, 1.4f, -4.0f);

    GX::LightData light = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.4f },
                                                       { 1.0f, 0.98f, 0.95f }, 2.5f);
    ctx.renderer->SetLights(&light, 1, { 0.08f, 0.08f, 0.08f });

    if (!std::filesystem::exists(m_modelPath))
    {
        GX_LOG_WARN("GXFramework: Missing model at %ls", m_modelPath.c_str());
        return;
    }

    m_model = m_loader.LoadFromFile(m_modelPath,
                                    ctx.graphics->GetDevice(),
                                    ctx.renderer->GetTextureManager(),
                                    ctx.renderer->GetMaterialManager());
    if (!m_model)
    {
        GX_LOG_ERROR("GXFramework: Failed to load model");
        return;
    }

    m_transform.SetPosition(0.0f, 0.0f, 0.0f);
    m_transform.SetScale(1.0f, 1.0f, 1.0f);

    m_isSkinned = m_model->IsSkinned() && m_model->HasSkeleton();
    if (m_isSkinned)
    {
        m_animator.SetSkeleton(m_model->GetSkeleton());
        if (m_model->GetAnimationCount() > 0)
            m_animator.Play(&m_model->GetAnimations()[0], true);
        else
            m_animator.EvaluateBindPose();
    }

    // マテリアルの上書き（実行中にパラメータ変更）。
    // 見た目だけ変えて挙動はそのままにできるので、デバッグにも便利。
    auto& matMgr = ctx.renderer->GetMaterialManager();
    for (int handle : m_model->GetMaterialHandles())
    {
        if (auto* mat = matMgr.GetMaterial(handle))
        {
            mat->constants.metallicFactor = 0.1f;
            mat->constants.roughnessFactor = 0.6f;
        }
    }

    // 任意のテクスチャ上書き（置けば差し替え）。
    // ファイルが存在する場合だけ差し替えるので、未配置でも安全に動く。
    const std::filesystem::path overrideAlbedo = "Assets/Models/Override_Albedo.png";
    if (std::filesystem::exists(overrideAlbedo))
    {
        int tex = ctx.renderer->GetTextureManager().LoadTexture(overrideAlbedo.wstring());
        if (tex >= 0 && !m_model->GetMaterialHandles().empty())
        {
            matMgr.SetTexture(m_model->GetMaterialHandles()[0],
                              GX::MaterialTextureSlot::Albedo, tex);
        }
    }

    // 任意のシェーダー上書き（カスタムHLSLがあれば使用）。
    // ここを書き換えると、特定のメッシュだけ別の描画方式にできる。
    const std::filesystem::path customShader = "Assets/Shaders/CustomPBR.hlsl";
    if (std::filesystem::exists(customShader))
    {
        GX::ShaderProgramDesc desc;
        desc.vsPath = customShader.wstring();
        desc.psPath = customShader.wstring();
        desc.vsEntry = L"VSMain";
        desc.psEntry = L"PSMain";
        int shaderHandle = ctx.renderer->CreateMaterialShader(desc);
        if (shaderHandle >= 0 && m_model->GetSubMeshCount() > 0)
            m_model->SetSubMeshShader(0, shaderHandle);
    }

    // MeshCollider の作成（スキンドにも対応）。
    // スキンドは形が動くため、基本は凸形状として扱うのが安定。
    m_colliderDesc.type = m_isSkinned ? GX::MeshColliderType::Convex : GX::MeshColliderType::Static;
    m_colliderDesc.optimize = true;
    m_colliderDesc.weldTolerance = 0.0005f;
    m_colliderDesc.maxConvexVertices = 128;
    TryBuildCollider(ctx);

    GX_LOG_INFO("GXFramework: Scene ready (skinned=%s, anims=%u)",
                m_isSkinned ? "true" : "false",
                m_model->GetAnimationCount());
}

void SkinnedMeshScene::OnExit(SceneContext& ctx)
{
    if (ctx.physics)
    {
        m_body.Destroy();
        m_collider.Release(*ctx.physics);
    }
    m_model.reset();
}

void SkinnedMeshScene::TryBuildCollider(SceneContext& ctx)
{
    if (!ctx.physics || !m_model)
        return;

    bool built = false;
    if (m_isSkinned)
        built = m_collider.BuildFromSkinnedModel(*ctx.physics, *m_model, m_animator, m_colliderDesc);
    else
        built = m_collider.BuildFromModel(*ctx.physics, *m_model, m_colliderDesc);

    if (!built || !m_collider.GetShape())
        return;

    GX::PhysicsBodySettings settings;
    settings.motionType = (m_colliderDesc.type == GX::MeshColliderType::Static)
        ? GX::MotionType3D::Static
        : GX::MotionType3D::Kinematic;
    settings.position = { m_transform.GetPosition().x, m_transform.GetPosition().y, m_transform.GetPosition().z };

    const auto& rot = m_transform.GetRotation();
    settings.rotation = GX::Quaternion::FromEuler(rot.x, rot.y, rot.z);
    settings.friction = 0.5f;
    settings.restitution = 0.1f;

    m_body.Create(ctx.physics, m_collider.GetShape(), settings);
}

void SkinnedMeshScene::Update(SceneContext& ctx, float dt)
{
    if (!m_model)
        return;

    auto& kb = ctx.input->GetKeyboard();
    if (kb.IsKeyTriggered(VK_F1))
        m_updateCollider = !m_updateCollider;

    // シンプルな回転。動きがあると更新確認がしやすい。
    auto rot = m_transform.GetRotation();
    rot.y += dt * 0.5f;
    m_transform.SetRotation(rot);

    if (m_isSkinned)
        m_animator.Update(dt);

    // 物理ボディの姿勢を同期。描画と物理を同じ位置に保つ。
    if (m_body.IsValid())
    {
        auto pos = m_transform.GetPosition();
        m_body.SetPosition({ pos.x, pos.y, pos.z });
        m_body.SetRotation(GX::Quaternion::FromEuler(rot.x, rot.y, rot.z));
    }

    // スキンドコライダーは低頻度で更新（高コスト）。
    // 毎フレーム更新すると重いので、間隔を空けて負荷を抑える。
    if (m_updateCollider && m_isSkinned && m_body.IsValid())
    {
        m_colliderTimer += dt;
        if (m_colliderTimer >= 0.2f)
        {
            m_colliderTimer = 0.0f;
            m_collider.UpdateFromSkinnedModel(*ctx.physics, m_body.GetID(),
                                              *m_model, m_animator, m_colliderDesc);
        }
    }
}

void SkinnedMeshScene::Render(SceneContext& ctx)
{
    if (!m_model)
        return;

    if (m_isSkinned)
        ctx.renderer->DrawSkinnedModel(*m_model, m_transform, m_animator);
    else
        ctx.renderer->DrawModel(*m_model, m_transform);
}

void SkinnedMeshScene::RenderUI(SceneContext& ctx)
{
    if (!ctx.textRenderer || ctx.defaultFont < 0)
        return;

    ctx.textRenderer->DrawString(ctx.defaultFont, 10.0f, 10.0f,
        L"[F1] Toggle skinned MeshCollider update", 0xFFFFFFFF);

    if (!m_model)
    {
        ctx.textRenderer->DrawString(ctx.defaultFont, 10.0f, 35.0f,
            L"Place FBX file at: Assets/Models/Character.fbx", 0xFFFFCC44);
        return;
    }

    std::wstring info = L"Model: ";
    info += m_modelPath;
    ctx.textRenderer->DrawString(ctx.defaultFont, 10.0f, 35.0f, info, 0xFF88FF88);
}

} // namespace GXFW
