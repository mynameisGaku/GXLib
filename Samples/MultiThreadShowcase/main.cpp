/// @file Samples/MultiThreadShowcase/main.cpp
/// @brief Multi-thread rendering demo.
///
/// 500個のカラーキューブが波打つ3Dシーン。
/// WASD で移動、右クリックで視点操作。
///
/// Controls:
///   WASD / QE  - Camera movement
///   Shift      - Fast move
///   RClick     - Mouse look
///   ESC        - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Device/ParallelCommandRecorder.h"
#include "Graphics/Device/IndirectCommandBuffer.h"

#include <cmath>
#include <Windows.h>

static constexpr int k_ObjectCount = 500;
static constexpr int k_GridSize    = 23;

class MultiThreadShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Multi-Thread Rendering";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 10; config.bgG = 12; config.bgB = 20;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& r = ctx.renderer3D;
        auto& c = ctx.camera;
        auto& p = ctx.postEffect;

        // PostEffect / Camera / Light
        p.SetTonemapMode(GX::TonemapMode::ACES);
        p.SetExposure(1.0f);
        p.GetBloom().SetEnabled(true);
        p.GetBloom().SetIntensity(0.3f);
        p.GetSSAO().SetEnabled(true);
        p.SetFXAAEnabled(true);
        r.SetShadowEnabled(false);

        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f },
                                                  { 1.0f, 0.98f, 0.95f }, 3.0f);
        r.SetLights(lights, 1, { 0.08f, 0.08f, 0.08f });
        r.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
        r.GetSkybox().SetColors({ 0.5f, 0.55f, 0.6f }, { 0.75f, 0.75f, 0.75f });

        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        c.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        c.SetPosition(0.0f, 25.0f, -35.0f);
        c.Rotate(0.5f, 0.0f);

        // 地面
        m_floorMesh = r.CreateGPUMesh(GX::MeshGenerator::CreatePlane(80, 80, 4, 4));
        m_floorMat.constants.albedoFactor = { 0.3f, 0.3f, 0.32f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        // キューブメッシュ（全オブジェクト共有）
        m_cubeMesh = r.CreateGPUMesh(GX::MeshGenerator::CreateBox(0.8f, 0.8f, 0.8f));

        // オブジェクト配置 + 虹色マテリアル
        m_transforms.resize(k_ObjectCount);
        m_materials.resize(k_ObjectCount);
        for (int i = 0; i < k_ObjectCount; ++i)
        {
            int gx = (i % k_GridSize) - k_GridSize / 2;
            int gz = (i / k_GridSize) - k_GridSize / 2;
            m_transforms[i].SetPosition(gx * 2.0f, 0.5f, gz * 2.0f);
            m_transforms[i].SetScale(0.8f, 0.8f, 0.8f);

            float hue = static_cast<float>(i) / k_ObjectCount;
            float cr = (std::max)(0.0f, fabsf(hue * 6.0f - 3.0f) - 1.0f);
            float cg = (std::max)(0.0f, 2.0f - fabsf(hue * 6.0f - 2.0f));
            float cb = (std::max)(0.0f, 2.0f - fabsf(hue * 6.0f - 4.0f));
            m_materials[i].constants.albedoFactor = { cr, cg, cb, 1.0f };
            m_materials[i].constants.roughnessFactor = 0.4f;
            m_materials[i].constants.metallicFactor = 0.6f;
        }

        // ParallelCommandRecorder / IndirectCommandBuffer
        m_parallel.Initialize(ctx.graphicsDevice.GetDevice(), 4);
        m_indirect.Initialize(ctx.graphicsDevice.GetDevice(), k_ObjectCount);
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse = ctx.inputManager.GetMouse();

        m_time += dt;
        m_lastDt = dt;

        // マウスキャプチャ（右クリックトグル）
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
            int mx = mouse.GetX(), my = mouse.GetY();
            camera.Rotate((float)(my - m_lastMY) * 0.003f,
                          (float)(mx - m_lastMX) * 0.003f);
            m_lastMX = mx;
            m_lastMY = my;
        }

        // WASD/QE カメラ移動
        float speed = 8.0f * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        // キューブが上下に波打つアニメーション
        for (int i = 0; i < k_ObjectCount; ++i)
        {
            int gx = (i % k_GridSize) - k_GridSize / 2;
            int gz = (i / k_GridSize) - k_GridSize / 2;
            float y = 0.5f + sinf(m_time * 2.0f + gx * 0.3f + gz * 0.3f) * 1.5f;
            m_transforms[i].SetPosition(gx * 2.0f, y, gz * 2.0f);
            m_transforms[i].SetRotation(0.0f, m_time * 0.5f + i * 0.01f, 0.0f);
        }
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        ctx.FlushAll();

        // --- HDR 3D Scene ---
        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);
        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_time);

        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_floorMesh, m_floorT);

        for (int i = 0; i < k_ObjectCount; ++i)
        {
            ctx.renderer3D.SetMaterial(m_materials[i]);
            ctx.renderer3D.DrawMesh(m_cubeMesh, m_transforms[i]);
        }

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        // --- Post-processing ---
        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // --- HUD ---
        float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        const TString info = FormatT(TEXT("Objects: {}  FPS: {:.0f}  WASD=Move  RClick=Look"),
            k_ObjectCount, fps);
        DrawString(10, 10, info.c_str(), GetColor(255, 255, 255));
    }

private:
    GX::GPUMesh m_floorMesh, m_cubeMesh;
    GX::Transform3D m_floorT;
    GX::Material m_floorMat;

    std::vector<GX::Transform3D> m_transforms;
    std::vector<GX::Material>    m_materials;

    GX::ParallelCommandRecorder m_parallel;
    GX::IndirectCommandBuffer   m_indirect;

    float m_time = 0.0f;
    float m_lastDt = 0.0f;

    // Camera control
    bool m_mouseCaptured = false;
    int  m_lastMX = 0;
    int  m_lastMY = 0;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    MultiThreadShowcaseApp app;
    return GXEasy::Run(app, app.GetConfig());
}
