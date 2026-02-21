/// @file Samples/ParticleShowcase/main.cpp
/// @brief パーティクルシステムデモ
///
/// 3種のエミッター（火花=加算, 煙=アルファ, 噴水=重力落下）を同時表示する。
/// 1-3キーで個別トグル、Spaceでバースト発射。
///
/// 使用API:
///   - ParticleSystem3D::Initialize() / AddEmitter() / Update() / Draw()
///   - ParticleEmitter::SetActive() / Burst()
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/ParticleSystem3D.h"

#include <cmath>
#include <Windows.h>

class ParticleShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Particle System";
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

        renderer.SetShadowEnabled(false);

        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // 床メッシュ
        m_floorMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(30.0f, 30.0f, 1, 1));
        m_floorTransform.SetPosition(0, 0, 0);
        m_floorMat.constants.albedoFactor    = { 0.3f, 0.3f, 0.32f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        // ライト
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        renderer.SetLights(lights, 1, { 0.1f, 0.1f, 0.12f });

        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.2f, 0.25f, 0.35f }, { 0.4f, 0.45f, 0.5f });

        // カメラ
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, -0.1f, -15.0f);
        camera.Rotate(0.2f, 0.0f);

        // パーティクルシステム初期化
        m_particleSystem.Initialize(ctx.device, renderer.GetTextureManager());

        // エミッター1: 火花（加算ブレンド、上方向コーン発射）
        {
            GX::ParticleEmitterConfig cfg;
            cfg.emissionRate = 100.0f;
            cfg.maxParticles = 2000;
            cfg.lifeMin = 0.5f;  cfg.lifeMax = 1.5f;
            cfg.sizeMin = 0.05f; cfg.sizeMax = 0.15f;
            cfg.speedMin = 3.0f; cfg.speedMax = 8.0f;
            cfg.colorStart = { 1.0f, 0.8f, 0.2f, 1.0f };
            cfg.colorEnd   = { 1.0f, 0.2f, 0.0f, 0.0f };
            cfg.shape = GX::ParticleShape::Cone;
            cfg.coneAngle = 25.0f;
            cfg.gravity = { 0, -5.0f, 0 };
            cfg.blend = GX::ParticleBlend::Additive;
            m_sparkIdx = m_particleSystem.AddEmitter(cfg);
            m_particleSystem.GetEmitter(m_sparkIdx).SetPosition({ -5.0f, 0.5f, 0.0f });
            m_particleSystem.GetEmitter(m_sparkIdx).SetDirection({ 0, 1, 0 });
        }

        // エミッター2: 煙（アルファブレンド、ゆっくり上昇）
        {
            GX::ParticleEmitterConfig cfg;
            cfg.emissionRate = 20.0f;
            cfg.maxParticles = 500;
            cfg.lifeMin = 2.0f;  cfg.lifeMax = 4.0f;
            cfg.sizeMin = 0.3f;  cfg.sizeMax = 0.6f;
            cfg.speedMin = 0.5f; cfg.speedMax = 1.5f;
            cfg.colorStart = { 0.6f, 0.6f, 0.6f, 0.5f };
            cfg.colorEnd   = { 0.4f, 0.4f, 0.4f, 0.0f };
            cfg.sizeOverLife = 1.5f;
            cfg.shape = GX::ParticleShape::Sphere;
            cfg.shapeRadius = 0.3f;
            cfg.gravity = { 0, 0.5f, 0 };
            cfg.drag = 0.5f;
            cfg.blend = GX::ParticleBlend::Alpha;
            m_smokeIdx = m_particleSystem.AddEmitter(cfg);
            m_particleSystem.GetEmitter(m_smokeIdx).SetPosition({ 0.0f, 0.2f, 0.0f });
            m_particleSystem.GetEmitter(m_smokeIdx).SetDirection({ 0, 1, 0 });
        }

        // エミッター3: 噴水（アルファブレンド、放物線落下）
        {
            GX::ParticleEmitterConfig cfg;
            cfg.emissionRate = 80.0f;
            cfg.maxParticles = 3000;
            cfg.lifeMin = 1.5f;  cfg.lifeMax = 3.0f;
            cfg.sizeMin = 0.08f; cfg.sizeMax = 0.12f;
            cfg.speedMin = 6.0f; cfg.speedMax = 10.0f;
            cfg.colorStart = { 0.3f, 0.5f, 1.0f, 0.8f };
            cfg.colorEnd   = { 0.1f, 0.3f, 0.8f, 0.0f };
            cfg.shape = GX::ParticleShape::Cone;
            cfg.coneAngle = 15.0f;
            cfg.gravity = { 0, -9.8f, 0 };
            cfg.blend = GX::ParticleBlend::Alpha;
            m_fountainIdx = m_particleSystem.AddEmitter(cfg);
            m_particleSystem.GetEmitter(m_fountainIdx).SetPosition({ 5.0f, 0.2f, 0.0f });
            m_particleSystem.GetEmitter(m_fountainIdx).SetDirection({ 0, 1, 0 });
        }
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& kb     = ctx.inputManager.GetKeyboard();

        m_totalTime += dt;
        m_lastDt = dt;

        // 1-3キーでエミッタートグル
        if (kb.IsKeyTriggered('1'))
        {
            auto& e = m_particleSystem.GetEmitter(m_sparkIdx);
            e.SetActive(!e.IsActive());
        }
        if (kb.IsKeyTriggered('2'))
        {
            auto& e = m_particleSystem.GetEmitter(m_smokeIdx);
            e.SetActive(!e.IsActive());
        }
        if (kb.IsKeyTriggered('3'))
        {
            auto& e = m_particleSystem.GetEmitter(m_fountainIdx);
            e.SetActive(!e.IsActive());
        }

        // Spaceでバースト
        if (kb.IsKeyTriggered(VK_SPACE))
        {
            m_particleSystem.GetEmitter(m_sparkIdx).Burst(200);
            m_particleSystem.GetEmitter(m_fountainIdx).Burst(100);
        }

        // パーティクル更新
        m_particleSystem.Update(dt);

        // カメラ自動回転（エミッター全体が画面中央に来るよう調整）
        float camAngle = m_totalTime * 0.15f;
        float camDist  = 14.0f;
        camera.SetPosition(std::cos(camAngle) * camDist, 3.5f, std::sin(camAngle) * camDist);
        camera.LookAt({ 0.0f, 2.5f, 0.0f });
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

        ctx.renderer3D.End();

        // パーティクル描画（3Dシーン内、End()後にPSO切り替え）
        m_particleSystem.Draw(cmd, ctx.camera, frameIndex);

        ctx.postEffect.EndScene();

        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        uint32_t totalParticles = m_particleSystem.GetTotalParticleCount();
        DrawString(10, 10,
                   FormatT(TEXT("FPS: {:.1f}  Particles: {}"), fps, totalParticles).c_str(),
                   GetColor(255, 255, 255));

        auto& sparkE   = m_particleSystem.GetEmitter(m_sparkIdx);
        auto& smokeE   = m_particleSystem.GetEmitter(m_smokeIdx);
        auto& fountainE = m_particleSystem.GetEmitter(m_fountainIdx);

        const TChar* sparkState   = sparkE.IsActive()   ? TEXT("ON") : TEXT("OFF");
        const TChar* smokeState   = smokeE.IsActive()   ? TEXT("ON") : TEXT("OFF");
        const TChar* fountainState = fountainE.IsActive() ? TEXT("ON") : TEXT("OFF");
        DrawString(10, 35,
                   FormatT(TEXT("[1] Sparks: {}  [2] Smoke: {}  [3] Fountain: {}"),
                           sparkState, smokeState, fountainState).c_str(),
                   GetColor(120, 180, 255));
        DrawString(10, 60, TEXT("Space: Burst  1-3: Toggle emitters  ESC: Quit"),
                   GetColor(136, 136, 136));
    }

private:
    float m_totalTime = 0.0f;
    float m_lastDt    = 0.0f;

    GX::GPUMesh     m_floorMesh;
    GX::Transform3D m_floorTransform;
    GX::Material    m_floorMat;

    GX::ParticleSystem3D m_particleSystem;
    int m_sparkIdx    = -1;
    int m_smokeIdx    = -1;
    int m_fountainIdx = -1;
};

GX_EASY_APP(ParticleShowcaseApp)
