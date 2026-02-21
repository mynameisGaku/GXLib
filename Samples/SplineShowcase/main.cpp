/// @file Samples/SplineShowcase/main.cpp
/// @brief Spline comparison demo - CatmullRom / Linear / CubicBezier.
///
/// 6 control points form a closed loop. A blue sphere moves along the curve
/// at constant speed using EvaluateByDistance(). Switch spline types with 1/2/3.
///
/// Controls:
///   1          - Linear
///   2          - CatmullRom
///   3          - CubicBezier
///   Space      - Pause/Resume
///   WASD / QE  - Camera movement
///   RClick     - Toggle mouse capture for look
///   ESC        - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Math/Spline.h"
#include "Math/Vector3.h"

#include <cmath>
#include <Windows.h>

class SplineShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Spline Showcase";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        Setup3D();

        ctx.camera.SetPosition(0, 12, -14);
        ctx.camera.LookAt({ 0, 0, 0 });

        auto& r = ctx.renderer3D;
        m_sphere = r.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.3f, 16, 8));
        m_moveSphere = r.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.5f, 16, 8));
        m_floor = r.CreateGPUMesh(GX::MeshGenerator::CreatePlane(30, 30, 1, 1));
        m_floorT.SetPosition(0, 0, 0);
        m_floorM.constants.albedoFactor = { 0.35f, 0.35f, 0.37f, 1 };
        m_floorM.constants.roughnessFactor = 0.9f;

        m_spline.Clear();
        const float radius = 6.0f;
        float heights[] = { 0.5f, 2.0f, 0.8f, 3.0f, 1.2f, 1.5f };
        for (int i = 0; i < 6; ++i)
        {
            float angle = XM_2PI * i / 6.0f;
            float r2 = radius + (i % 2 == 0 ? 1.0f : -0.5f);
            m_spline.AddPoint(GX::Vector3(
                std::cos(angle) * r2,
                heights[i],
                std::sin(angle) * r2
            ));
        }
        m_spline.SetClosed(true);
        m_spline.SetType(GX::SplineType::CatmullRom);

        m_distance = 0;
        m_paused = false;
    }

    void Update(float dt) override
    {
        UpdateCamera(dt);

        auto& kb = GX_Internal::CompatContext::Instance().inputManager.GetKeyboard();
        if (kb.IsKeyTriggered('1')) m_spline.SetType(GX::SplineType::Linear);
        if (kb.IsKeyTriggered('2')) m_spline.SetType(GX::SplineType::CatmullRom);
        if (kb.IsKeyTriggered('3')) m_spline.SetType(GX::SplineType::CubicBezier);
        if (kb.IsKeyTriggered(VK_SPACE)) m_paused = !m_paused;

        if (!m_paused)
        {
            float totalLen = m_spline.GetTotalLength();
            m_distance += 4.0f * dt;
            if (totalLen > 0)
                m_distance = std::fmod(m_distance, totalLen);
        }
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        uint32_t fi = ctx.frameIndex;

        Begin3DScene(cmd, fi);

        ctx.renderer3D.SetMaterial(m_floorM);
        ctx.renderer3D.DrawMesh(m_floor, m_floorT);

        GX::Material cpMat;
        cpMat.constants.albedoFactor = { 0.9f, 0.15f, 0.1f, 1 };
        cpMat.constants.roughnessFactor = 0.3f;
        for (int i = 0; i < m_spline.GetPointCount(); ++i)
        {
            GX::Vector3 pt = m_spline.GetPoint(i);
            GX::Transform3D t;
            t.SetPosition(pt.x, pt.y, pt.z);
            ctx.renderer3D.SetMaterial(cpMat);
            ctx.renderer3D.DrawMesh(m_sphere, t);
        }

        GX::Vector3 movePos = m_spline.EvaluateByDistance(m_distance);
        GX::Transform3D moveT;
        moveT.SetPosition(movePos.x, movePos.y, movePos.z);
        GX::Material moveMat;
        moveMat.constants.albedoFactor = { 0.1f, 0.4f, 0.95f, 1 };
        moveMat.constants.roughnessFactor = 0.2f;
        moveMat.constants.metallicFactor = 0.9f;
        ctx.renderer3D.SetMaterial(moveMat);
        ctx.renderer3D.DrawMesh(m_moveSphere, moveT);

        {
            auto& pb = ctx.renderer3D.GetPrimitiveBatch3D();
            XMFLOAT4X4 vp;
            XMStoreFloat4x4(&vp, XMMatrixTranspose(ctx.camera.GetViewProjectionMatrix()));
            pb.Begin(cmd, fi, vp);

            const int segments = 100;
            XMFLOAT4 lineColor = { 0.2f, 0.9f, 0.3f, 1.0f };
            for (int i = 0; i < segments; ++i)
            {
                float t0 = (float)i / segments;
                float t1 = (float)(i + 1) / segments;
                GX::Vector3 a = m_spline.Evaluate(t0);
                GX::Vector3 b = m_spline.Evaluate(t1);
                pb.DrawLine({ a.x, a.y, a.z }, { b.x, b.y, b.z }, lineColor);
            }

            pb.End();
        }

        End3DScene();

        const char* typeNames[] = { "Linear", "CatmullRom", "CubicBezier" };
        int typeIdx = static_cast<int>(m_spline.GetType());
        float totalLen = m_spline.GetTotalLength();

        DrawString(10, 10,
            FormatT(TEXT("Spline: {}  Length: {:.1f}"), typeNames[typeIdx], totalLen).c_str(),
            GetColor(68, 204, 255));
        DrawString(10, 35, TEXT("1: Linear  2: CatmullRom  3: CubicBezier  Space: Pause"),
                   GetColor(136, 136, 136));
        DrawString(10, 55, TEXT("RClick+WASD: Camera  ESC: Quit"),
                   GetColor(100, 100, 130));
    }

private:
    GX::Spline m_spline;
    GX::GPUMesh m_sphere, m_moveSphere, m_floor;
    GX::Transform3D m_floorT;
    GX::Material m_floorM;
    float m_distance = 0;
    bool m_paused = false;

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

GX_EASY_APP(SplineShowcaseApp)
