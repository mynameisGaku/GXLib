/// @file Samples/TrailShowcase/main.cpp
/// @brief TrailRenderer demo - rainbow ribbon trail in 3D space.
///
/// An auto-moving point draws a Lissajous curve trail with rainbow colors.
/// Left mouse button drag adds a manual trail on the ground plane.
///
/// Controls:
///   LMB drag   - Draw manual trail on ground
///   WASD / QE  - Camera movement
///   RClick     - Toggle mouse capture for look
///   ESC        - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/TrailRenderer.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"

#include <cmath>
#include <Windows.h>

class TrailShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Trail Renderer";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        Setup3D();

        ctx.camera.SetPosition(0, 10, -12);
        ctx.camera.LookAt({ 0, 1, 0 });

        m_floor = ctx.renderer3D.CreateGPUMesh(
            GX::MeshGenerator::CreatePlane(20, 20, 10, 10));
        m_floorT.SetPosition(0, 0, 0);
        m_floorM.constants.albedoFactor = { 0.4f, 0.4f, 0.42f, 1 };
        m_floorM.constants.roughnessFactor = 0.85f;

        m_trail.Initialize(ctx.graphicsDevice.GetDevice(), 512);
        m_trail.lifetime = 1.5f;
        m_trail.fadeWithAge = true;
    }

    void Update(float dt) override
    {
        UpdateCamera(dt);
        m_trail.Update(dt);

        float t = m_totalTime;
        float ax = std::sin(t * 1.2f) * 4.0f;
        float az = std::sin(t * 0.8f) * 3.0f;
        float ay = 1.0f + std::sin(t * 2.0f) * 0.5f;

        float hue = std::fmod(t * 0.3f, 1.0f);
        GX::Color col = HSVtoColor(hue, 0.9f, 1.0f);
        m_trail.AddPoint({ ax, ay, az }, { 0, 1, 0 }, 0.4f, col);

        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& mouse = ctx.inputManager.GetMouse();
        if (mouse.IsButtonDown(GX::MouseButton::Left) && !m_captured)
        {
            XMFLOAT3 worldPos;
            if (ScreenToPlane(mouse.GetX(), mouse.GetY(), 0.0f, worldPos))
            {
                worldPos.y = 0.05f;
                float hue2 = std::fmod(t * 0.5f + 0.5f, 1.0f);
                m_trail.AddPoint(worldPos, { 0, 1, 0 }, 0.5f,
                                 HSVtoColor(hue2, 0.9f, 1.0f));
            }
        }
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        uint32_t fi = ctx.frameIndex;

        ctx.FlushAll();
        ctx.postEffect.BeginScene(cmd, fi,
            ctx.renderer3D.GetDepthBuffer().GetDSVHandle(), ctx.camera);
        DrawSkybox(cmd, fi);
        ctx.renderer3D.Begin(cmd, fi, ctx.camera, m_totalTime);

        ctx.renderer3D.SetMaterial(m_floorM);
        ctx.renderer3D.DrawMesh(m_floor, m_floorT);

        ctx.renderer3D.End();

        m_trail.Draw(cmd, ctx.camera, fi);

        ctx.postEffect.EndScene();

        auto& db = ctx.renderer3D.GetDepthBuffer();
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               db, ctx.camera, m_lastDt);
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        DrawString(10, 10,
            FormatT(TEXT("Trail Points: {}"), m_trail.GetPointCount()).c_str(),
            GetColor(68, 204, 255));
        DrawString(10, 35,
            TEXT("Auto: Lissajous curve  LMB drag: Manual trail"),
            GetColor(120, 180, 255));
        DrawString(10, 55, TEXT("RClick+WASD: Camera  ESC: Quit"),
                   GetColor(100, 100, 130));
    }

private:
    GX::TrailRenderer m_trail;
    GX::GPUMesh m_floor;
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

    bool ScreenToPlane(int sx, int sy, float planeY, XMFLOAT3& out)
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        float w = (float)ctx.screenWidth, h = (float)ctx.screenHeight;
        float nx = (2.0f * sx / w) - 1.0f;
        float ny = 1.0f - (2.0f * sy / h);
        XMMATRIX invVP = XMMatrixInverse(nullptr,
            ctx.camera.GetViewProjectionMatrix());
        XMVECTOR nearPt = XMVector3TransformCoord(
            XMVectorSet(nx, ny, 0, 1), invVP);
        XMVECTOR farPt = XMVector3TransformCoord(
            XMVectorSet(nx, ny, 1, 1), invVP);
        XMFLOAT3 o, d;
        XMStoreFloat3(&o, nearPt);
        XMStoreFloat3(&d, XMVectorSubtract(farPt, nearPt));
        if (std::abs(d.y) < 1e-6f) return false;
        float t = (planeY - o.y) / d.y;
        if (t < 0) return false;
        out = { o.x + d.x * t, planeY, o.z + d.z * t };
        return true;
    }

    static GX::Color HSVtoColor(float h, float s, float v)
    {
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
        float m = v - c;
        float r = 0, g = 0, b = 0;
        int hi = (int)(h * 6.0f) % 6;
        switch (hi) {
        case 0: r = c; g = x; break;
        case 1: r = x; g = c; break;
        case 2: g = c; b = x; break;
        case 3: g = x; b = c; break;
        case 4: r = x; b = c; break;
        case 5: r = c; b = x; break;
        }
        return GX::Color(r + m, g + m, b + m, 1.0f);
    }
};

GX_EASY_APP(TrailShowcaseApp)
