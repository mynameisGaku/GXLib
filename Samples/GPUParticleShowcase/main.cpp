/// @file Samples/GPUParticleShowcase/main.cpp
/// @brief GPU Compute Shaderパーティクルのデモ。10万粒子をリアルタイム描画。
///
/// 機能:
///   - スペースキー: 原点に1000粒子バースト放出
///   - 左クリック: マウス位置にレイを飛ばし、Y=0平面との交点に1000粒子バースト
///   - 連続放出: 毎フレーム100粒子を原点から放出
///   - WASD/QE: カメラ移動
///   - 右クリック: マウスキャプチャ（視点操作）
///   - 1/2/3: プリセット切り替え（火花/煙/噴水）
///
/// GPUParticleSystemのCompute Shader（Init/Emit/Update）で粒子を管理し、
/// ビルボードクワッドで描画する。
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/GPUParticleSystem.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"

#include <Windows.h>

/// @brief GPUパーティクルショーケースのメインクラス
class GPUParticleApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: GPU Particle Showcase";
        config.width = 1280;
        config.height = 720;
        config.bgR = 2;
        config.bgG = 2;
        config.bgB = 6;
        config.vsync = true;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera = ctx.camera;
        auto& postFX = ctx.postEffect;

        renderer.SetShadowEnabled(false);

        // ポストエフェクト: Bloom強めで光の粒子感を演出
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // 地面メッシュ（パーティクルの参考位置として）
        m_floorMesh = renderer.CreateGPUMesh(
            GX::MeshGenerator::CreatePlane(40.0f, 40.0f, 20, 20));
        m_floorTransform.SetPosition(0, 0, 0);
        m_floorMat.constants.albedoFactor = { 0.15f, 0.15f, 0.18f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.95f;

        // ライト
        GX::LightData lights[2];
        lights[0] = GX::Light::CreateDirectional(
            { 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 2.0f);
        lights[1] = GX::Light::CreatePoint(
            { 0.0f, 5.0f, 0.0f }, 30.0f, { 1.0f, 0.9f, 0.7f }, 5.0f);
        renderer.SetLights(lights, 2, { 0.03f, 0.03f, 0.05f });

        // スカイボックス（暗め）
        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 2.0f);
        renderer.GetSkybox().SetColors({ 0.02f, 0.02f, 0.05f }, { 0.1f, 0.1f, 0.15f });

        // カメラ
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 8.0f, -15.0f);
        camera.Rotate(0.4f, 0.0f);

        // GPUパーティクルシステム初期化
        m_particles.Initialize(ctx.device, ctx.commandQueue.GetQueue(), 100000);

        // 初期プリセット（火花）
        ApplyPreset(0);
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& kb = ctx.inputManager.GetKeyboard();
        auto& mouse = ctx.inputManager.GetMouse();

        m_totalTime += dt;
        m_lastDt = dt;

        // --- マウスキャプチャ ---
        if (mouse.IsButtonTriggered(GX::MouseButton::Right))
        {
            m_mouseCaptured = !m_mouseCaptured;
            if (m_mouseCaptured)
            {
                m_lastMX = mouse.GetX();
                m_lastMY = mouse.GetY();
                ShowCursor(FALSE);
            }
            else
            {
                ShowCursor(TRUE);
            }
        }

        if (m_mouseCaptured)
        {
            int mx = mouse.GetX();
            int my = mouse.GetY();
            camera.Rotate(
                static_cast<float>(my - m_lastMY) * m_mouseSens,
                static_cast<float>(mx - m_lastMX) * m_mouseSens);
            m_lastMX = mx;
            m_lastMY = my;
        }

        // --- WASD/QE カメラ移動 ---
        float speed = m_cameraSpeed * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        // --- プリセット切り替え ---
        if (kb.IsKeyTriggered('1')) ApplyPreset(0);
        if (kb.IsKeyTriggered('2')) ApplyPreset(1);
        if (kb.IsKeyTriggered('3')) ApplyPreset(2);

        // --- スペースキー: 原点バースト ---
        if (kb.IsKeyTriggered(VK_SPACE))
        {
            m_particles.SetEmitPosition({ 0.0f, 0.5f, 0.0f });
            m_particles.Emit(1000);
        }

        // --- 左クリック: マウスレイとY=0平面の交点にバースト ---
        if (mouse.IsButtonTriggered(GX::MouseButton::Left))
        {
            float mx = static_cast<float>(mouse.GetX());
            float my = static_cast<float>(mouse.GetY());
            float w = static_cast<float>(ctx.swapChain.GetWidth());
            float h = static_cast<float>(ctx.swapChain.GetHeight());

            // NDC座標
            float ndcX = (mx / w) * 2.0f - 1.0f;
            float ndcY = 1.0f - (my / h) * 2.0f;

            // レイ計算
            XMMATRIX invVP = XMMatrixInverse(nullptr, camera.GetViewProjectionMatrix());
            XMVECTOR nearPt = XMVector3TransformCoord(
                XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), invVP);
            XMVECTOR farPt = XMVector3TransformCoord(
                XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), invVP);
            XMVECTOR rayDir = XMVector3Normalize(farPt - nearPt);

            // Y=0平面との交差
            XMFLOAT3 origin, dir;
            XMStoreFloat3(&origin, nearPt);
            XMStoreFloat3(&dir, rayDir);

            if (fabsf(dir.y) > 0.001f)
            {
                float t = -origin.y / dir.y;
                if (t > 0.0f)
                {
                    XMFLOAT3 hitPos = {
                        origin.x + dir.x * t,
                        0.5f,
                        origin.z + dir.z * t
                    };
                    m_particles.SetEmitPosition(hitPos);
                    m_particles.Emit(1000);
                }
            }
        }

        // --- 連続放出（毎フレーム） ---
        m_particles.SetEmitPosition({ 0.0f, 0.5f, 0.0f });
        m_particles.Emit(m_continuousEmitRate);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        ctx.FlushAll();

        // HDR RTに3Dシーン描画
        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);
        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_totalTime);

        // 地面描画
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_floorMesh, m_floorTransform);

        ctx.renderer3D.End();

        // GPUパーティクル更新 + 描画（HDR RT上）
        m_particles.Update(cmd, m_lastDt, frameIndex);
        m_particles.Draw(cmd, ctx.camera, frameIndex);

        ctx.postEffect.EndScene();

        // Resolve: HDR → LDR → バックバッファ
        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // --- HUD ---
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10,
                   FormatT(TEXT("FPS: {:.1f}  |  Max Particles: {}"),
                           fps, m_particles.GetMaxParticles()).c_str(),
                   GetColor(255, 255, 255));
        DrawString(10, 35,
                   FormatT(TEXT("Preset: {}"), k_PresetNames[m_currentPreset]).c_str(),
                   GetColor(68, 204, 255));
        DrawString(10, 60,
                   TEXT("[Space] Burst  [LClick] Burst at cursor  [1/2/3] Preset"),
                   GetColor(136, 136, 136));
        DrawString(10, 85,
                   TEXT("WASD/QE Move  Shift Fast  RClick Mouse  ESC Quit"),
                   GetColor(136, 136, 136));
    }

    void Release() override
    {
        m_particles.Shutdown();
    }

private:
    GX::GPUParticleSystem m_particles;

    GX::GPUMesh     m_floorMesh;
    GX::Transform3D m_floorTransform;
    GX::Material    m_floorMat;

    float m_totalTime = 0.0f;
    float m_lastDt = 0.0f;

    float m_cameraSpeed = 5.0f;
    float m_mouseSens = 0.003f;
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0;
    int   m_lastMY = 0;

    int m_currentPreset = 0;
    uint32_t m_continuousEmitRate = 100;

    static constexpr const TChar* k_PresetNames[] = {
        TEXT("Sparks (Fire)"),
        TEXT("Smoke"),
        TEXT("Fountain")
    };

    /// @brief パーティクルプリセットを適用
    void ApplyPreset(int preset)
    {
        m_currentPreset = preset;

        switch (preset)
        {
        case 0: // 火花
            m_particles.SetGravity({ 0.0f, -9.8f, 0.0f });
            m_particles.SetDrag(0.02f);
            m_particles.SetVelocityRange({ -3.0f, 5.0f, -3.0f }, { 3.0f, 15.0f, 3.0f });
            m_particles.SetLifeRange(0.5f, 2.0f);
            m_particles.SetSizeRange(0.15f, 0.0f);
            m_particles.SetColorRange(
                { 1.0f, 0.8f, 0.2f, 1.0f },
                { 1.0f, 0.1f, 0.0f, 0.0f });
            m_continuousEmitRate = 200;
            break;

        case 1: // 煙
            m_particles.SetGravity({ 0.0f, 1.5f, 0.0f });  // 上昇
            m_particles.SetDrag(0.05f);
            m_particles.SetVelocityRange({ -1.0f, 2.0f, -1.0f }, { 1.0f, 5.0f, 1.0f });
            m_particles.SetLifeRange(2.0f, 5.0f);
            m_particles.SetSizeRange(0.1f, 0.8f);  // 広がる
            m_particles.SetColorRange(
                { 0.5f, 0.5f, 0.5f, 0.6f },
                { 0.3f, 0.3f, 0.3f, 0.0f });
            m_continuousEmitRate = 50;
            break;

        case 2: // 噴水
            m_particles.SetGravity({ 0.0f, -15.0f, 0.0f });
            m_particles.SetDrag(0.01f);
            m_particles.SetVelocityRange({ -2.0f, 15.0f, -2.0f }, { 2.0f, 25.0f, 2.0f });
            m_particles.SetLifeRange(1.0f, 3.0f);
            m_particles.SetSizeRange(0.1f, 0.05f);
            m_particles.SetColorRange(
                { 0.3f, 0.6f, 1.0f, 0.8f },
                { 0.1f, 0.3f, 0.8f, 0.0f });
            m_continuousEmitRate = 300;
            break;
        }
    }
};

GX_EASY_APP(GPUParticleApp)
