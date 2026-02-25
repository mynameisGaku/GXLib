/// @file Samples/TerrainShowcase/main.cpp
/// @brief Procedural terrain heightfield rendering demo.
///
/// Generates a terrain using FBM noise and renders it with PBR lighting.
/// Camera flies through the scene. Height query used for ground following.
///
/// Controls:
///   WASD / QE    - Camera movement
///   Right Click  - Toggle mouse capture for look
///   G            - Toggle ground-follow mode
///   ESC          - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/Terrain.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/PrimitiveBatch3D.h"

#include <Windows.h>

class TerrainShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Terrain";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 8; config.bgG = 10; config.bgB = 20;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera   = ctx.camera;
        auto& postFX   = ctx.postEffect;

        // Post effects
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // Lights
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.4f, -0.8f, 0.3f }, { 1.0f, 0.95f, 0.9f }, 4.0f);
        renderer.SetLights(lights, 1, { 0.12f, 0.12f, 0.15f });
        renderer.GetSkybox().SetSun({ 0.4f, -0.8f, 0.3f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.45f, 0.55f, 0.7f }, { 0.7f, 0.75f, 0.8f });

        // Camera
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 30.0f, -30.0f);
        camera.Rotate(0.4f, 0.0f);

        // Terrain
        m_terrain.CreateProcedural(ctx.device, 200.0f, 200.0f, 128, 128, 25.0f);

        // Terrain material
        m_terrainMat.constants.albedoFactor    = { 0.35f, 0.5f, 0.25f, 1.0f };
        m_terrainMat.constants.roughnessFactor = 0.85f;

        // Reference sphere mesh to show height query
        m_sphereMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.3f, 12, 6));
        m_sphereMat.constants.albedoFactor    = { 1.0f, 0.3f, 0.15f, 1.0f };
        m_sphereMat.constants.roughnessFactor = 0.3f;
        m_sphereMat.constants.metallicFactor  = 0.8f;
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse  = ctx.inputManager.GetMouse();
        auto& kb     = ctx.inputManager.GetKeyboard();

        m_totalTime += dt;
        m_lastDt = dt;

        // Mouse capture
        if (mouse.IsButtonTriggered(GX::MouseButton::Right))
        {
            m_mouseCaptured = !m_mouseCaptured;
            m_lastMX = mouse.GetX();
            m_lastMY = mouse.GetY();
            ShowCursor(m_mouseCaptured ? FALSE : TRUE);
        }
        if (m_mouseCaptured)
        {
            int mx = mouse.GetX(), my = mouse.GetY();
            camera.Rotate(static_cast<float>(my - m_lastMY) * 0.003f,
                          static_cast<float>(mx - m_lastMX) * 0.003f);
            m_lastMX = mx; m_lastMY = my;
        }

        // Camera movement
        float speed = 15.0f * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        // Ground follow toggle
        if (kb.IsKeyTriggered('G'))
            m_groundFollow = !m_groundFollow;

        // Ground follow: keep camera at terrain height + offset
        if (m_groundFollow)
        {
            XMFLOAT3 pos = camera.GetPosition();
            float groundH = m_terrain.GetHeight(pos.x, pos.z);
            camera.SetPosition(pos.x, groundH + 3.0f, pos.z);
        }

        // Update probe position (in front of camera on terrain)
        XMFLOAT3 camPos = camera.GetPosition();
        XMFLOAT3 camFwd = camera.GetForward();
        m_probeX = camPos.x + camFwd.x * 10.0f;
        m_probeZ = camPos.z + camFwd.z * 10.0f;
        m_probeY = m_terrain.GetHeight(m_probeX, m_probeZ);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t fi = ctx.frameIndex;

        ctx.FlushAll();
        ctx.postEffect.BeginScene(cmd, fi, ctx.renderer3D.GetDepthBuffer().GetDSVHandle(), ctx.camera);
        ctx.renderer3D.Begin(cmd, fi, ctx.camera, m_totalTime);

        // Terrain
        GX::Transform3D terrainT;
        ctx.renderer3D.SetMaterial(m_terrainMat);
        ctx.renderer3D.DrawTerrain(m_terrain, terrainT);

        // Height probe sphere
        GX::Transform3D probeT;
        probeT.SetPosition(m_probeX, m_probeY + 0.3f, m_probeZ);
        ctx.renderer3D.SetMaterial(m_sphereMat);
        ctx.renderer3D.DrawMesh(m_sphereMesh, probeT);

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        auto& db = ctx.renderer3D.GetDepthBuffer();
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(), db, ctx.camera, m_lastDt);
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10,
            FormatT(TEXT("FPS: {:.1f}  Terrain: {}x{} segs  GroundFollow: {}"),
                    fps, 128, 128, m_groundFollow ? TEXT("ON") : TEXT("OFF")).c_str(),
            GetColor(255, 255, 255));
        DrawString(10, 35,
            FormatT(TEXT("Probe: ({:.1f}, {:.1f}, {:.1f})  Height: {:.2f}"),
                    m_probeX, m_probeY, m_probeZ, m_probeY).c_str(),
            GetColor(100, 220, 255));
        DrawString(10, 60,
            TEXT("WASD/QE: Camera  RClick: Look  G: Ground-follow  ESC: Quit"),
            GetColor(136, 136, 136));
    }

private:
    GX::Terrain     m_terrain;
    GX::Material    m_terrainMat;
    GX::GPUMesh     m_sphereMesh;
    GX::Material    m_sphereMat;

    float m_totalTime = 0.0f, m_lastDt = 0.0f;
    bool  m_groundFollow = false;
    float m_probeX = 0.0f, m_probeY = 0.0f, m_probeZ = 0.0f;

    bool m_mouseCaptured = false;
    int  m_lastMX = 0, m_lastMY = 0;
};

GX_EASY_APP(TerrainShowcaseApp)
