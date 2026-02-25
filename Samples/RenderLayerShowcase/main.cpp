/// @file Samples/RenderLayerShowcase/main.cpp
/// @brief RenderLayer + LayerStack + LayerCompositor + MaskScreen demo.
///
/// Demonstrates the layer rendering system with 3 composited layers:
/// Background gradient, 3D scene, and 2D UI overlay. A circular mask
/// can be applied to the scene layer.
///
/// Controls:
///   1/2/3        - Toggle layer visibility (BG/Scene/UI)
///   M            - Toggle mask ON/OFF
///   Up/Down      - Adjust mask radius
///   ESC          - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/Layer/RenderLayer.h"
#include "Graphics/Layer/LayerStack.h"
#include "Graphics/Layer/LayerCompositor.h"
#include "Graphics/Layer/MaskScreen.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"

#include <Windows.h>

class RenderLayerShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Render Layers";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 0; config.bgG = 0; config.bgB = 0;
        config.autoClear = false;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera   = ctx.camera;
        auto& postFX   = ctx.postEffect;

        uint32_t w = ctx.screenWidth;
        uint32_t h = ctx.screenHeight;

        // Setup layers (owned by m_layerStack)
        m_bgLayer    = m_layerStack.CreateLayer(ctx.device, "Background", 0, w, h);
        m_sceneLayer = m_layerStack.CreateLayer(ctx.device, "Scene",      10, w, h);
        m_uiLayer    = m_layerStack.CreateLayer(ctx.device, "UI",         20, w, h);

        // Compositor
        m_layerCompositor.Initialize(ctx.device, w, h);

        // Mask
        m_mask.Create(ctx.device, w, h);
        m_maskX = w * 0.5f;
        m_maskY = h * 0.5f;
        m_maskRadius = 200.0f;

        // 3D setup
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);
        renderer.SetShadowEnabled(false);

        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        renderer.SetLights(lights, 1, { 0.1f, 0.1f, 0.12f });

        float aspect = static_cast<float>(w) / h;
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 100.0f);
        camera.SetPosition(0.0f, 3.0f, -6.0f);
        camera.LookAt({ 0.0f, 0.0f, 0.0f });

        // Meshes
        m_cubeMesh   = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));
        m_sphereMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.5f, 16, 8));
        m_planeMesh  = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(10.0f, 10.0f, 1, 1));

        m_floorMat.constants.albedoFactor    = { 0.4f, 0.4f, 0.42f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        m_cubeMat.constants.albedoFactor    = { 0.8f, 0.3f, 0.2f, 1.0f };
        m_cubeMat.constants.roughnessFactor = 0.4f;

        m_sphereMat.constants.albedoFactor    = { 0.2f, 0.5f, 0.9f, 1.0f };
        m_sphereMat.constants.roughnessFactor = 0.3f;
        m_sphereMat.constants.metallicFactor  = 0.7f;
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& kb  = ctx.inputManager.GetKeyboard();
        m_totalTime += dt;
        m_lastDt = dt;

        // Toggle layers
        if (kb.IsKeyTriggered('1') && m_bgLayer)
            m_bgLayer->SetVisible(!m_bgLayer->IsVisible());
        if (kb.IsKeyTriggered('2') && m_sceneLayer)
            m_sceneLayer->SetVisible(!m_sceneLayer->IsVisible());
        if (kb.IsKeyTriggered('3') && m_uiLayer)
            m_uiLayer->SetVisible(!m_uiLayer->IsVisible());

        // Mask toggle
        if (kb.IsKeyTriggered('M'))
            m_maskEnabled = !m_maskEnabled;

        // Mask controls
        if (CheckHitKey(KEY_INPUT_UP))   m_maskRadius = (std::min)(500.0f, m_maskRadius + 100.0f * dt);
        if (CheckHitKey(KEY_INPUT_DOWN)) m_maskRadius = (std::max)(30.0f,  m_maskRadius - 100.0f * dt);

        // Rotate objects
        m_cubeAngle += dt * 0.8f;
        m_sphereAngle += dt * 1.2f;
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t fi = ctx.frameIndex;
        uint32_t w = ctx.screenWidth;
        uint32_t h = ctx.screenHeight;

        ctx.FlushAll();

        // --- Layer 0: Background gradient ---
        if (m_bgLayer && m_bgLayer->IsVisible())
        {
            m_bgLayer->Begin(cmd);
            m_bgLayer->Clear(cmd, 0.0f, 0.0f, 0.0f, 1.0f);
            // Draw gradient using primitive batch
            for (int y = 0; y < static_cast<int>(h); y += 4)
            {
                float t = static_cast<float>(y) / h;
                int r = static_cast<int>(20 + t * 40);
                int g = static_cast<int>(10 + t * 30);
                int b = static_cast<int>(40 + t * 80);
                DrawBox(0, y, static_cast<int>(w), y + 4, GetColor(r, g, b), TRUE);
            }
            ctx.FlushAll();  // バッチをフラッシュしてからレイヤーを終了
            m_bgLayer->End();
        }

        // --- Layer 1: 3D Scene ---
        if (m_sceneLayer && m_sceneLayer->IsVisible())
        {
            m_sceneLayer->Begin(cmd);
            m_sceneLayer->Clear(cmd, 0.0f, 0.0f, 0.0f, 0.0f);

            ctx.postEffect.BeginScene(cmd, fi,
                ctx.renderer3D.GetDepthBuffer().GetDSVHandle(), ctx.camera);
            ctx.renderer3D.Begin(cmd, fi, ctx.camera, m_totalTime);

            // Floor
            GX::Transform3D floorT;
            ctx.renderer3D.SetMaterial(m_floorMat);
            ctx.renderer3D.DrawMesh(m_planeMesh, floorT);

            // Rotating cube
            GX::Transform3D cubeT;
            cubeT.SetPosition(-1.5f, 0.7f, 0.0f);
            cubeT.SetRotation(m_cubeAngle * 0.5f, m_cubeAngle, 0.0f);
            ctx.renderer3D.SetMaterial(m_cubeMat);
            ctx.renderer3D.DrawMesh(m_cubeMesh, cubeT);

            // Orbiting sphere
            float sx = std::cos(m_sphereAngle) * 2.0f;
            float sz = std::sin(m_sphereAngle) * 2.0f;
            GX::Transform3D sphereT;
            sphereT.SetPosition(sx, 0.7f, sz);
            ctx.renderer3D.SetMaterial(m_sphereMat);
            ctx.renderer3D.DrawMesh(m_sphereMesh, sphereT);

            ctx.renderer3D.End();
            ctx.postEffect.EndScene();

            auto& db = ctx.renderer3D.GetDepthBuffer();
            db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            ctx.postEffect.Resolve(m_sceneLayer->GetRenderTarget().GetRTVHandle(),
                                   db, ctx.camera, m_lastDt);
            db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

            m_sceneLayer->End();
        }

        // --- Apply mask to scene layer ---
        if (m_maskEnabled && m_sceneLayer)
        {
            m_mask.SetEnabled(true);
            m_mask.Clear(cmd, 0);
            m_mask.DrawCircle(cmd, fi, m_maskX, m_maskY, m_maskRadius);
            m_sceneLayer->SetMask(m_mask.GetAsLayer());
        }
        else if (m_sceneLayer)
        {
            m_mask.SetEnabled(false);
            m_sceneLayer->SetMask(nullptr);
        }

        // --- Layer 2: UI overlay ---
        if (m_uiLayer && m_uiLayer->IsVisible())
        {
            m_uiLayer->Begin(cmd);
            m_uiLayer->Clear(cmd, 0.0f, 0.0f, 0.0f, 0.0f);

            float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
            DrawString(10, 10,
                FormatT(TEXT("FPS: {:.1f}  Mask: {}  Radius: {:.0f}"),
                        fps, m_maskEnabled ? TEXT("ON") : TEXT("OFF"), m_maskRadius).c_str(),
                GetColor(255, 255, 255));
            DrawString(10, 35,
                FormatT(TEXT("BG: {}  Scene: {}  UI: {}"),
                    (m_bgLayer && m_bgLayer->IsVisible()) ? TEXT("ON") : TEXT("OFF"),
                    (m_sceneLayer && m_sceneLayer->IsVisible()) ? TEXT("ON") : TEXT("OFF"),
                    (m_uiLayer && m_uiLayer->IsVisible()) ? TEXT("ON") : TEXT("OFF")).c_str(),
                GetColor(100, 220, 255));
            DrawString(10, 60,
                TEXT("1/2/3: Toggle layers  M: Mask  Up/Down: Radius  ESC: Quit"),
                GetColor(136, 136, 136));

            ctx.FlushAll();  // バッチをフラッシュしてからレイヤーを終了
            m_uiLayer->End();
        }

        // --- Composite all layers to backbuffer ---
        m_layerCompositor.Composite(cmd, fi,
            ctx.swapChain.GetCurrentRTVHandle(), m_layerStack);
    }

private:
    GX::LayerStack     m_layerStack;
    GX::LayerCompositor m_layerCompositor;

    GX::RenderLayer* m_bgLayer    = nullptr;
    GX::RenderLayer* m_sceneLayer = nullptr;
    GX::RenderLayer* m_uiLayer    = nullptr;
    GX::MaskScreen   m_mask;

    bool  m_maskEnabled = false;
    float m_maskX = 640.0f, m_maskY = 360.0f;
    float m_maskRadius = 200.0f;

    GX::GPUMesh  m_cubeMesh, m_sphereMesh, m_planeMesh;
    GX::Material m_floorMat, m_cubeMat, m_sphereMat;

    float m_cubeAngle   = 0.0f;
    float m_sphereAngle = 0.0f;
    float m_totalTime   = 0.0f;
    float m_lastDt      = 0.0f;
};

GX_EASY_APP(RenderLayerShowcaseApp)
