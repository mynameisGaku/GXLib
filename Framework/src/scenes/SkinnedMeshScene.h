#pragma once
/// @file SkinnedMeshScene.h
/// @brief FBX + Animator + スキンドMeshColliderのデモシーン。機能確認のために用意する。

#include "Scene.h"
#include "Graphics/3D/ModelLoader.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/Transform3D.h"
#include "Physics/MeshCollider.h"
#include "Physics/RigidBody3D.h"

namespace GXFW
{

class SkinnedMeshScene : public Scene
{
public:
    const char* GetName() const override { return "SkinnedMeshScene"; }

    void OnEnter(SceneContext& ctx) override;
    void OnExit(SceneContext& ctx) override;
    void Update(SceneContext& ctx, float dt) override;
    void Render(SceneContext& ctx) override;
    void RenderUI(SceneContext& ctx) override;

private:
    void TryBuildCollider(SceneContext& ctx);

    GX::ModelLoader m_loader;
    std::unique_ptr<GX::Model> m_model;
    GX::Animator m_animator;
    GX::Transform3D m_transform;

    GX::MeshCollider m_collider;
    GX::MeshColliderDesc m_colliderDesc;
    GX::RigidBody3D m_body;

    bool m_isSkinned = false;
    bool m_updateCollider = true;
    float m_colliderTimer = 0.0f;

    std::wstring m_modelPath = L"Assets/Models/Character.fbx";
};

} // namespace GXFW
