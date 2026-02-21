/// @file Samples/SceneShowcase/main.cpp
/// @brief シーングラフデモ
///
/// Scene/Entity/Componentシステムのデモ。
/// 親子階層のロボットアームとスクリプトコンポーネントによる回転を表示する。
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Core/Scene/Scene.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"

#include <string>
#include <cmath>

class SceneShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Scene Graph";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer  = ctx.renderer3D;
        auto& camera    = ctx.camera;
        auto& postFX    = ctx.postEffect;

        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // 床メッシュ
        m_floorMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(30.0f, 30.0f, 1, 1));
        m_floorTransform.SetPosition(0, 0, 0);
        m_floorMat.constants.albedoFactor = { 0.3f, 0.3f, 0.32f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        // ライト
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        renderer.SetLights(lights, 1, { 0.15f, 0.15f, 0.18f });

        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.2f, 0.25f, 0.35f }, { 0.4f, 0.45f, 0.5f });

        // カメラ
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(6.0f, 3.0f, -6.0f);
        camera.LookAt({ 0.0f, 1.0f, 0.0f });

        // GPUメッシュを作成（キューブ）
        m_cubeMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));
        m_sphereMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.5f, 16, 16));

        // シーン作成
        m_scene = std::make_unique<GX::Scene>("RobotArm Scene");

        // ベースプラットフォーム（回転台）
        auto* base = m_scene->CreateEntity("Base");
        base->GetTransform().SetPosition(0, 0.5f, 0);
        base->GetTransform().SetScale(2.0f, 0.5f, 2.0f);
        auto* baseScript = base->AddComponent<GX::ScriptComponent>();
        baseScript->onUpdate = [base](float dt)
        {
            auto rot = base->GetTransform().GetRotation();
            base->GetTransform().SetRotation(rot.x, rot.y + dt * 0.5f, rot.z);
        };

        // アーム1（ベースの子）
        auto* arm1 = m_scene->CreateEntity("Arm1");
        arm1->SetParent(base);
        arm1->GetTransform().SetPosition(0, 1.5f, 0);
        arm1->GetTransform().SetScale(0.5f, 2.0f, 0.5f);
        auto* arm1Script = arm1->AddComponent<GX::ScriptComponent>();
        arm1Script->onUpdate = [arm1](float dt)
        {
            auto rot = arm1->GetTransform().GetRotation();
            arm1->GetTransform().SetRotation(
                std::sin(rot.y * 2.0f) * 0.3f,
                rot.y + dt * 0.8f,
                rot.z);
        };

        // アーム2（アーム1の子）
        auto* arm2 = m_scene->CreateEntity("Arm2");
        arm2->SetParent(arm1);
        arm2->GetTransform().SetPosition(0, 1.5f, 0);
        arm2->GetTransform().SetScale(0.6f, 1.5f, 0.6f);
        auto* arm2Script = arm2->AddComponent<GX::ScriptComponent>();
        arm2Script->onUpdate = [arm2](float dt)
        {
            auto rot = arm2->GetTransform().GetRotation();
            arm2->GetTransform().SetRotation(rot.x, rot.y, std::sin(rot.y) * 0.5f);
        };

        // エンドエフェクター（球体、アーム2の子）
        auto* tip = m_scene->CreateEntity("Tip");
        tip->SetParent(arm2);
        tip->GetTransform().SetPosition(0, 1.0f, 0);
        tip->GetTransform().SetScale(1.2f);

        // 独立した回転キューブ群
        for (int i = 0; i < 4; ++i)
        {
            float angle = XM_2PI * i / 4.0f;
            float x = std::cos(angle) * 5.0f;
            float z = std::sin(angle) * 5.0f;

            auto* cube = m_scene->CreateEntity("Cube" + std::to_string(i));
            cube->GetTransform().SetPosition(x, 1.0f, z);
            float speed = 1.0f + i * 0.5f;
            auto* cubeScript = cube->AddComponent<GX::ScriptComponent>();
            cubeScript->onUpdate = [cube, speed](float dt)
            {
                auto rot = cube->GetTransform().GetRotation();
                cube->GetTransform().SetRotation(rot.x + dt * speed, rot.y + dt * speed * 0.7f, rot.z);
            };
        }
    }

    void Update(float dt) override
    {
        m_totalTime += dt;
        m_lastDt = dt;

        // シーン更新（スクリプト・アニメーション）
        m_scene->Update(dt);

        // WASD + マウスカメラ
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& kb = ctx.inputManager.GetKeyboard();
        auto& mouse = ctx.inputManager.GetMouse();

        // 右クリックでマウスルック
        if (mouse.IsButtonDown(1))
        {
            float mdx = static_cast<float>(mouse.GetDeltaX());
            float mdy = static_cast<float>(mouse.GetDeltaY());
            camera.SetYaw(camera.GetYaw() + mdx * 0.003f);
            camera.SetPitch(camera.GetPitch() - mdy * 0.003f);
        }

        // WASD移動
        float speed = 5.0f * dt;
        if (kb.IsKeyDown(VK_SHIFT)) speed *= 3.0f;
        float yaw = camera.GetYaw();
        float fwdX = std::sin(yaw), fwdZ = std::cos(yaw);
        float rightX = fwdZ, rightZ = -fwdX;
        auto pos = camera.GetPosition();
        if (kb.IsKeyDown('W')) { pos.x += fwdX * speed; pos.z += fwdZ * speed; }
        if (kb.IsKeyDown('S')) { pos.x -= fwdX * speed; pos.z -= fwdZ * speed; }
        if (kb.IsKeyDown('D')) { pos.x += rightX * speed; pos.z += rightZ * speed; }
        if (kb.IsKeyDown('A')) { pos.x -= rightX * speed; pos.z -= rightZ * speed; }
        if (kb.IsKeyDown('E')) pos.y += speed;
        if (kb.IsKeyDown('Q')) pos.y -= speed;
        camera.SetPosition(pos.x, pos.y, pos.z);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        ctx.FlushAll();

        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);

        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_totalTime);

        // 床
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_floorMesh, m_floorTransform);

        // シーン内エンティティの手動描画（GPUMeshを使用）
        GX::Material cubeMat;
        cubeMat.constants.albedoFactor = { 0.7f, 0.3f, 0.2f, 1.0f };
        cubeMat.constants.roughnessFactor = 0.4f;
        cubeMat.constants.metallicFactor = 0.8f;

        GX::Material sphereMat;
        sphereMat.constants.albedoFactor = { 0.2f, 0.6f, 0.9f, 1.0f };
        sphereMat.constants.roughnessFactor = 0.2f;
        sphereMat.constants.metallicFactor = 0.9f;

        for (const auto& entity : m_scene->GetEntities())
        {
            if (!entity->IsActive()) continue;

            // Transform3Dのワールド行列計算（親子階層考慮）
            XMMATRIX worldMat = entity->GetWorldMatrix();

            if (entity->GetName() == "Tip")
            {
                ctx.renderer3D.SetMaterial(sphereMat);
                ctx.renderer3D.DrawMesh(m_sphereMesh, worldMat);
            }
            else
            {
                ctx.renderer3D.SetMaterial(cubeMat);
                ctx.renderer3D.DrawMesh(m_cubeMesh, worldMat);
            }
        }

        ctx.renderer3D.End();

        ctx.postEffect.EndScene();

        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10,
                   FormatT(TEXT("FPS: {:.1f}  Entities: {}"), fps, m_scene->GetEntityCount()).c_str(),
                   GetColor(255, 255, 255));
        DrawString(10, 35, TEXT("Scene Graph: Robot Arm (parent-child hierarchy) + Rotating Cubes"),
                   GetColor(120, 180, 255));
        DrawString(10, 60, TEXT("WASD/QE: Move  RightClick+Drag: Look  Shift: Fast  ESC: Quit"),
                   GetColor(136, 136, 136));
    }

private:
    float m_totalTime = 0.0f;
    float m_lastDt    = 0.0f;

    GX::GPUMesh     m_floorMesh;
    GX::Transform3D m_floorTransform;
    GX::Material    m_floorMat;

    GX::GPUMesh     m_cubeMesh;
    GX::GPUMesh     m_sphereMesh;

    std::unique_ptr<GX::Scene> m_scene;
};

GX_EASY_APP(SceneShowcaseApp)
