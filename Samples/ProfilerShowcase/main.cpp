/// @file Samples/ProfilerShowcase/main.cpp
/// @brief GPUProfiler hierarchy scope demo with tree HUD.
///
/// Draws 100 cubes + 4 spheres with nested GPU profiler scopes.
/// Results are displayed as a tree with colored duration bars.
///
/// Controls:
///   WASD / QE  - Camera movement
///   RClick     - Toggle mouse capture for look
///   ESC        - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Device/GPUProfiler.h"

#include <cmath>
#include <Windows.h>

class ProfilerShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: GPU Profiler";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        Setup3D();

        ctx.camera.SetPosition(0, 10, -15);
        ctx.camera.LookAt({ 0, 0, 0 });

        auto& r = ctx.renderer3D;
        m_floor = r.CreateGPUMesh(GX::MeshGenerator::CreatePlane(30, 30, 1, 1));
        m_cube = r.CreateGPUMesh(GX::MeshGenerator::CreateBox(0.6f, 0.6f, 0.6f));
        m_sphere = r.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.4f, 16, 8));
        m_floorT.SetPosition(0, 0, 0);
        m_floorM.constants.albedoFactor = { 0.35f, 0.35f, 0.38f, 1 };
        m_floorM.constants.roughnessFactor = 0.9f;

        auto& profiler = GX::GPUProfiler::Instance();
        profiler.Initialize(ctx.graphicsDevice.GetDevice(), ctx.commandQueue.GetQueue());
        profiler.SetEnabled(true);
    }

    void Update(float dt) override
    {
        UpdateCamera(dt);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        uint32_t fi = ctx.frameIndex;
        auto& profiler = GX::GPUProfiler::Instance();

        ctx.FlushAll();

        profiler.BeginFrame(cmd, fi);
        profiler.BeginScope(cmd, "Frame Total");

        ctx.postEffect.BeginScene(cmd, fi,
            ctx.renderer3D.GetDepthBuffer().GetDSVHandle(), ctx.camera);
        DrawSkybox(cmd, fi);
        ctx.renderer3D.Begin(cmd, fi, ctx.camera, m_totalTime);

        profiler.BeginScope(cmd, "Geometry");
        {
            profiler.BeginScope(cmd, "Floor");
            ctx.renderer3D.SetMaterial(m_floorM);
            ctx.renderer3D.DrawMesh(m_floor, m_floorT);
            profiler.EndScope(cmd);

            profiler.BeginScope(cmd, "Cubes (100)");
            GX::Material cubeMat;
            cubeMat.constants.roughnessFactor = 0.4f;
            for (int z = 0; z < 10; ++z)
            {
                for (int x = 0; x < 10; ++x)
                {
                    float fx = (x - 4.5f) * 1.5f;
                    float fz = (z - 4.5f) * 1.5f;
                    float h = 0.3f + std::sin(m_totalTime * 2.0f + fx + fz) * 0.3f;
                    float cr = (float)x / 9.0f;
                    float cb = (float)z / 9.0f;
                    cubeMat.constants.albedoFactor = { cr, 0.3f, cb, 1 };

                    GX::Transform3D t;
                    t.SetPosition(fx, h + 0.3f, fz);
                    ctx.renderer3D.SetMaterial(cubeMat);
                    ctx.renderer3D.DrawMesh(m_cube, t);
                }
            }
            profiler.EndScope(cmd);

            profiler.BeginScope(cmd, "Spheres (4)");
            GX::Material sphereMat;
            sphereMat.constants.albedoFactor = { 1, 0.8f, 0.3f, 1 };
            sphereMat.constants.metallicFactor = 1;
            sphereMat.constants.roughnessFactor = 0.15f;
            float corners[][2] = { {-5, -5}, {5, -5}, {-5, 5}, {5, 5} };
            for (auto& c2 : corners)
            {
                GX::Transform3D t;
                t.SetPosition(c2[0], 0.4f, c2[1]);
                ctx.renderer3D.SetMaterial(sphereMat);
                ctx.renderer3D.DrawMesh(m_sphere, t);
            }
            profiler.EndScope(cmd);
        }
        profiler.EndScope(cmd);  // Geometry

        ctx.renderer3D.End();

        profiler.BeginScope(cmd, "PostEffects");
        ctx.postEffect.EndScene();
        auto& db = ctx.renderer3D.GetDepthBuffer();
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(), db, ctx.camera, m_lastDt);
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        profiler.EndScope(cmd);  // PostEffects

        profiler.EndScope(cmd);  // Frame Total
        profiler.EndFrame(cmd);

        // 2D HUD: profiler tree
        float fps = (m_lastDt > 0) ? (1.0f / m_lastDt) : 0;
        DrawString(10, 10,
            FormatT(TEXT("FPS: {:.1f}  GPU Frame: {:.2f} ms"), fps, profiler.GetFrameGPUTimeMs()).c_str(),
            GetColor(255, 255, 255));

        const auto& results = profiler.GetResults();
        int py = 45;
        DrawString(10, py, TEXT("=== GPU Profiler Scopes ==="), GetColor(180, 180, 255));
        py += 24;

        for (const auto& r : results)
        {
            int indent = 20 + r.depth * 20;

            int barWidth = (int)(r.durationMs / 2.0f * 300.0f);
            barWidth = (std::min)(barWidth, 300);
            barWidth = (std::max)(barWidth, 2);

            unsigned int barCol;
            if (r.durationMs < 0.5f)
                barCol = GetColor(60, 200, 80);
            else if (r.durationMs < 1.5f)
                barCol = GetColor(220, 200, 50);
            else
                barCol = GetColor(220, 60, 40);

            DrawBox(indent, py + 2, indent + barWidth, py + 14, barCol, TRUE);

            DrawString(indent + barWidth + 8, py,
                FormatT(TEXT("{} {:.3f}ms"), r.name, r.durationMs).c_str(),
                GetColor(220, 220, 220));

            py += 18;
        }

        DrawString(10, 680, TEXT("GPU scopes measured 1 frame behind (readback)  RClick+WASD: Camera  ESC: Quit"),
                   GetColor(100, 100, 130));
    }

private:
    GX::GPUMesh m_floor, m_cube, m_sphere;
    GX::Transform3D m_floorT;
    GX::Material m_floorM;

    // Camera control state
    float m_totalTime = 0, m_lastDt = 0;
    bool m_captured = false;
    int m_lastMX = 0, m_lastMY = 0;

    void Setup3D()
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& r = ctx.renderer3D; auto& c = ctx.camera; auto& p = ctx.postEffect;

        p.SetTonemapMode(GX::TonemapMode::ACES);
        p.SetExposure(1.0f);
        p.GetBloom().SetEnabled(true);
        p.GetBloom().SetIntensity(0.3f);
        p.GetBloom().SetThreshold(1.5f);
        p.GetSSAO().SetEnabled(true);
        p.SetFXAAEnabled(true);
        r.SetShadowEnabled(false);

        GX::LightData lights[3];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        lights[1] = GX::Light::CreatePoint({ -3, 3, -3 }, 15, { 1, 0.95f, 0.9f }, 3);
        lights[2] = GX::Light::CreateSpot({ 3, 5, -2 }, { -0.3f, -1, 0.2f }, 20, 30, { 1, 0.8f, 0.4f }, 10);
        r.SetLights(lights, 3, { 0.05f, 0.05f, 0.05f });

        r.SetFog(GX::FogMode::Linear, { 0.7f, 0.7f, 0.7f }, 30, 100);
        r.GetSkybox().SetSun({ 0.3f, -1, 0.5f }, 5);
        r.GetSkybox().SetColors({ 0.5f, 0.55f, 0.6f }, { 0.75f, 0.75f, 0.75f });

        float aspect = (float)ctx.swapChain.GetWidth() / ctx.swapChain.GetHeight();
        c.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
    }

    void DrawSkybox(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex)
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        XMFLOAT4X4 viewF;
        XMStoreFloat4x4(&viewF, ctx.camera.GetViewMatrix());
        viewF._41 = 0; viewF._42 = 0; viewF._43 = 0;
        XMMATRIX viewRotOnly = XMLoadFloat4x4(&viewF);
        XMFLOAT4X4 vp;
        XMStoreFloat4x4(&vp, XMMatrixTranspose(viewRotOnly * ctx.camera.GetProjectionMatrix()));
        ctx.renderer3D.GetSkybox().Draw(cmd, frameIndex, vp);
    }

    void UpdateCamera(float dt)
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse = ctx.inputManager.GetMouse();
        m_totalTime += dt; m_lastDt = dt;

        if (mouse.IsButtonTriggered(GX::MouseButton::Right))
        {
            m_captured = !m_captured;
            if (m_captured) { m_lastMX = mouse.GetX(); m_lastMY = mouse.GetY(); ShowCursor(FALSE); }
            else ShowCursor(TRUE);
        }
        if (m_captured)
        {
            int mx = mouse.GetX(), my = mouse.GetY();
            camera.Rotate((float)(my - m_lastMY) * 0.003f, (float)(mx - m_lastMX) * 0.003f);
            m_lastMX = mx; m_lastMY = my;
        }
        float s = 5.0f * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) s *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(s);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-s);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(s);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-s);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(s);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-s);
    }
};

GX_EASY_APP(ProfilerShowcaseApp)
