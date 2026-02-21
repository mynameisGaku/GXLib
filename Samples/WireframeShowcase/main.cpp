/// @file Samples/WireframeShowcase/main.cpp
/// @brief PrimitiveBatch3D wireframe primitives showcase.
///
/// Displays 5 wireframe primitives: Cone, Capsule, Frustum, Circle, Axis.
/// Each primitive is animated with rotation or oscillation.
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
#include "Graphics/3D/PrimitiveBatch3D.h"

#include <cmath>
#include <Windows.h>

class WireframeShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Wireframe Primitives";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        Setup3D();

        ctx.camera.SetPosition(0, 6, -14);
        ctx.camera.LookAt({ 0, 1.5f, 0 });

        m_floor = ctx.renderer3D.CreateGPUMesh(GX::MeshGenerator::CreatePlane(30, 30, 1, 1));
        m_floorT.SetPosition(0, 0, 0);
        m_floorM.constants.albedoFactor = { 0.3f, 0.3f, 0.32f, 1 };
        m_floorM.constants.roughnessFactor = 0.9f;
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

        Begin3DScene(cmd, fi);

        ctx.renderer3D.SetMaterial(m_floorM);
        ctx.renderer3D.DrawMesh(m_floor, m_floorT);

        auto& pb = ctx.renderer3D.GetPrimitiveBatch3D();
        XMFLOAT4X4 vp;
        XMStoreFloat4x4(&vp, XMMatrixTranspose(ctx.camera.GetViewProjectionMatrix()));
        pb.Begin(cmd, fi, vp);

        float rot = m_totalTime * 0.8f;

        // (1) Cone
        {
            float cy = std::cos(rot), sy = std::sin(rot);
            XMFLOAT3 dir = { sy * 0.3f, 1.0f, cy * 0.3f };
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            dir = { dir.x / len, dir.y / len, dir.z / len };
            pb.DrawWireCone({ -6, 0, 0 }, dir, 3.0f, 1.0f, { 1.0f, 0.3f, 0.2f, 1.0f });
        }

        // (2) Capsule
        {
            float wave = std::sin(rot) * 0.5f;
            pb.DrawWireCapsule(
                { -3, 0.8f + wave, 0 }, { -3, 3.0f - wave, 0 },
                0.6f, { 0.2f, 0.9f, 0.3f, 1.0f });
        }

        // (3) Frustum
        {
            XMMATRIX view = XMMatrixLookAtLH(
                XMVectorSet(0, 2, -2, 1), XMVectorSet(0, 1, 2, 1), XMVectorSet(0, 1, 0, 0));
            XMMATRIX proj = XMMatrixPerspectiveFovLH(
                XM_PIDIV4 + std::sin(rot) * 0.2f, 1.3f, 0.5f, 6.0f);
            XMMATRIX invFrustum = XMMatrixInverse(nullptr, view * proj);
            XMFLOAT4X4 invF;
            XMStoreFloat4x4(&invF, invFrustum);
            pb.DrawWireFrustum(invF, { 0.2f, 0.4f, 1.0f, 1.0f });
        }

        // (4) Circle
        {
            float tilt = std::sin(rot * 0.7f) * 0.5f;
            XMFLOAT3 normal = { std::sin(tilt), std::cos(tilt), 0 };
            pb.DrawWireCircle({ 3, 1.5f, 0 }, normal, 1.5f, { 1.0f, 0.9f, 0.2f, 1.0f });
        }

        // (5) Axis
        pb.DrawAxis({ 6, 0.01f, 0 }, 2.0f);

        // Grid
        pb.DrawGrid(20, 20, { 0.2f, 0.2f, 0.2f, 0.3f });

        pb.End();

        End3DScene();

        DrawString(10, 10, TEXT("PrimitiveBatch3D - 5 Wireframe Primitives"), GetColor(68, 204, 255));
        int lx = 80;
        DrawString(lx,       670, TEXT("Cone"),    GetColor(255, 80, 60));   lx += 180;
        DrawString(lx,       670, TEXT("Capsule"), GetColor(60, 230, 80));   lx += 180;
        DrawString(lx,       670, TEXT("Frustum"), GetColor(60, 100, 255));  lx += 180;
        DrawString(lx,       670, TEXT("Circle"),  GetColor(255, 230, 60));  lx += 180;
        DrawString(lx,       670, TEXT("Axis"),    GetColor(200, 200, 200));
        DrawString(10, 695, TEXT("RClick+WASD: Camera  ESC: Quit"), GetColor(100, 100, 130));
    }

private:
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

    void Begin3DScene(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex)
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        ctx.FlushAll();
        ctx.postEffect.BeginScene(cmd, frameIndex,
            ctx.renderer3D.GetDepthBuffer().GetDSVHandle(), ctx.camera);
        DrawSkybox(cmd, frameIndex);
        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_totalTime);
    }

    void End3DScene()
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        ctx.renderer3D.End();
        ctx.postEffect.EndScene();
        auto& db = ctx.renderer3D.GetDepthBuffer();
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(), db, ctx.camera, m_lastDt);
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);
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

GX_EASY_APP(WireframeShowcaseApp)
