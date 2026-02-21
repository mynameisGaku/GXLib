/// @file Samples/DXRShowcase/main.cpp
/// @brief DXR GI Cornell Box デモ。
///
/// 赤・緑の壁を持つ小部屋でカラーブリーディング（色滲み）を確認できる。
/// DXR GI(G) / 反射(Y) / SSR(R) の切り替えが可能。

#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Vertex3D.h"
#include "Graphics/RayTracing/RTReflections.h"
#include "Graphics/RayTracing/RTGI.h"
#include "Graphics/3D/CascadedShadowMap.h"

#include <Windows.h>

class DXRShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib: DXR GI - Cornell Box";
        config.width = 1280;
        config.height = 720;
        config.bgR = 2;
        config.bgG = 2;
        config.bgB = 4;
        config.vsync = true;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera = ctx.camera;
        auto& postFX = ctx.postEffect;

        renderer.SetShadowEnabled(true);

        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.SetExposure(0.7f);
        postFX.GetBloom().SetEnabled(true);
        postFX.GetBloom().SetIntensity(0.15f);
        postFX.GetBloom().SetThreshold(2.0f);
        postFX.GetSSAO().SetEnabled(true);
        postFX.GetSSAO().SetRadius(0.3f);
        postFX.GetSSAO().SetPower(2.5f);
        postFX.SetFXAAEnabled(true);
        postFX.SetVignetteEnabled(false);

        // === メッシュ作成 ===
        m_floorCeilMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(k_RoomW, k_RoomD, 1, 1));
        m_sideWallMesh  = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(k_RoomH, k_RoomD, 1, 1));
        m_backWallMesh  = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(k_RoomW, k_RoomH, 1, 1));
        m_tallBoxMesh   = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(0.65f, 1.8f, 0.65f));
        m_shortBoxMesh  = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(0.65f, 0.9f, 0.65f));
        m_sphereMesh    = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.4f, 48, 24));

        const float hw = k_RoomW / 2.0f;
        const float hh = k_RoomH / 2.0f;
        const float hd = k_RoomD / 2.0f;

        // === 壁面 (全てディフューズ: roughness=0.95, metallic=0) ===

        // 0: Floor (white)
        m_wallTransforms[0].SetPosition(0, 0, hd);
        m_wallMats[0].constants.albedoFactor = { 0.85f, 0.85f, 0.85f, 1.0f };

        // 1: Ceiling (white, 下向き)
        m_wallTransforms[1].SetPosition(0, k_RoomH, hd);
        m_wallTransforms[1].SetRotation(XM_PI, 0, 0);
        m_wallMats[1].constants.albedoFactor = { 0.85f, 0.85f, 0.85f, 1.0f };

        // 2: Left wall (RED, 法線+X = 室内向き)
        m_wallTransforms[2].SetPosition(-hw, hh, hd);
        m_wallTransforms[2].SetRotation(0, 0, -XM_PIDIV2);
        m_wallMats[2].constants.albedoFactor = { 0.85f, 0.08f, 0.08f, 1.0f };

        // 3: Right wall (GREEN, 法線-X = 室内向き)
        m_wallTransforms[3].SetPosition(hw, hh, hd);
        m_wallTransforms[3].SetRotation(0, 0, XM_PIDIV2);
        m_wallMats[3].constants.albedoFactor = { 0.08f, 0.85f, 0.08f, 1.0f };

        // 4: Back wall (white, 法線-Z = カメラ向き)
        m_wallTransforms[4].SetPosition(0, hh, k_RoomD);
        m_wallTransforms[4].SetRotation(-XM_PIDIV2, 0, 0);
        m_wallMats[4].constants.albedoFactor = { 0.85f, 0.85f, 0.85f, 1.0f };

        for (int i = 0; i < k_NumWalls; ++i)
        {
            m_wallMats[i].constants.roughnessFactor = 0.95f;
            m_wallMats[i].constants.metallicFactor  = 0.0f;
        }

        // === 室内オブジェクト (白ディフューズ → GI色が映り込む) ===

        // 0: Tall box (左寄り奥)
        m_objTransforms[0].SetPosition(-0.8f, 0.9f, 3.2f);
        m_objTransforms[0].SetRotation(0, XMConvertToRadians(15.0f), 0);

        // 1: Short box (右寄り手前)
        m_objTransforms[1].SetPosition(0.8f, 0.45f, 1.5f);
        m_objTransforms[1].SetRotation(0, XMConvertToRadians(-18.0f), 0);

        // 2: Sphere (中央手前)
        m_objTransforms[2].SetPosition(0.0f, 0.4f, 2.0f);

        for (int i = 0; i < k_NumObjects; ++i)
        {
            m_objMats[i].constants.albedoFactor     = { 0.85f, 0.85f, 0.85f, 1.0f };
            m_objMats[i].constants.roughnessFactor   = 0.95f;
            m_objMats[i].constants.metallicFactor    = 0.0f;
        }

        // === ライティング ===
        GX::LightData lights[2];
        lights[0] = GX::Light::CreateDirectional(
            { 0.1f, -0.8f, 0.3f }, { 1.0f, 0.98f, 0.95f }, 0.1f);
        lights[1] = GX::Light::CreatePoint(
            { 0.0f, k_RoomH - 0.15f, hd }, 8.0f, { 1.0f, 0.98f, 0.95f }, 6.0f);
        renderer.SetLights(lights, 2, { 0.01f, 0.01f, 0.012f });

        renderer.GetSkybox().SetSun({ 0.1f, -0.8f, 0.3f }, 0.5f);
        renderer.GetSkybox().SetColors({ 0.05f, 0.06f, 0.1f }, { 0.08f, 0.09f, 0.12f });

        // === カメラ (部屋の正面開口から覗く) ===
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4 * 1.1f, aspect, 0.1f, 100.0f);
        camera.SetPosition(0.0f, 1.75f, -0.3f);
        camera.Rotate(0.0f, 0.0f);

        InitDXR();
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& kb = ctx.inputManager.GetKeyboard();
        auto& mouse = ctx.inputManager.GetMouse();
        auto& postFX = ctx.postEffect;

        m_totalTime += dt;
        m_lastDt = dt;

        // マウスカメラ
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
            camera.Rotate((float)(my - m_lastMY) * 0.003f, (float)(mx - m_lastMX) * 0.003f);
            m_lastMX = mx;
            m_lastMY = my;
        }

        float speed = 3.0f * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        // Y: DXR Reflection toggle
        if (kb.IsKeyTriggered('Y') && m_rtReflections)
        {
            bool newRT = !m_rtReflections->IsEnabled();
            m_rtReflections->SetEnabled(newRT);
            if (newRT) postFX.GetSSR().SetEnabled(false);
        }

        // G: RTGI toggle
        if (kb.IsKeyTriggered('G') && m_rtGI)
            m_rtGI->SetEnabled(!m_rtGI->IsEnabled());

        // R: SSR toggle
        if (kb.IsKeyTriggered('R'))
        {
            bool newSSR = !postFX.GetSSR().IsEnabled();
            postFX.GetSSR().SetEnabled(newSSR);
            if (newSSR && m_rtReflections)
                m_rtReflections->SetEnabled(false);
        }

        // T: RTGI debug mode (0=normal, 1=GI only)
        if (kb.IsKeyTriggered('T') && m_rtGI)
            m_rtGI->SetDebugMode((m_rtGI->GetDebugMode() + 1) % 2);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        ctx.FlushAll();

        RegisterRTInstances();

        ctx.postEffect.SetCommandList4(ctx.commandList.Get4());

        // === CSM シャドウパス ===
        ctx.renderer3D.UpdateShadow(ctx.camera);
        for (uint32_t cascade = 0; cascade < GX::CascadedShadowMap::k_NumCascades; ++cascade)
        {
            ctx.renderer3D.BeginShadowPass(cmd, frameIndex, cascade);
            DrawSceneMeshes(ctx.renderer3D);
            ctx.renderer3D.EndShadowPass(cascade);
        }

        // === ポイントシャドウパス ===
        for (uint32_t face = 0; face < 6; ++face)
        {
            ctx.renderer3D.BeginPointShadowPass(cmd, frameIndex, face);
            DrawSceneMeshes(ctx.renderer3D);
            ctx.renderer3D.EndPointShadowPass(face);
        }

        // === HDR シーン描画 ===
        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);

        // Skybox
        {
            XMFLOAT4X4 viewF;
            XMStoreFloat4x4(&viewF, ctx.camera.GetViewMatrix());
            viewF._41 = 0; viewF._42 = 0; viewF._43 = 0;
            XMMATRIX viewRotOnly = XMLoadFloat4x4(&viewF);
            XMFLOAT4X4 vp;
            XMStoreFloat4x4(&vp, XMMatrixTranspose(viewRotOnly * ctx.camera.GetProjectionMatrix()));
            ctx.renderer3D.GetSkybox().Draw(cmd, frameIndex, vp);
        }

        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_totalTime);

        // Walls
        for (int i = 0; i < k_NumWalls; ++i)
        {
            ctx.renderer3D.SetMaterial(m_wallMats[i]);
            GX::GPUMesh& mesh = (i < 2) ? m_floorCeilMesh : (i < 4) ? m_sideWallMesh : m_backWallMesh;
            ctx.renderer3D.DrawMesh(mesh, m_wallTransforms[i]);
        }

        // Objects
        GX::GPUMesh* objMeshes[] = { &m_tallBoxMesh, &m_shortBoxMesh, &m_sphereMesh };
        for (int i = 0; i < k_NumObjects; ++i)
        {
            ctx.renderer3D.SetMaterial(m_objMats[i]);
            ctx.renderer3D.DrawMesh(*objMeshes[i], m_objTransforms[i]);
        }

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        DrawHUD();
    }

    void Release() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        ctx.postEffect.SetRTGI(nullptr);
        ctx.postEffect.SetRTReflections(nullptr);
        ctx.postEffect.SetCommandList4(nullptr);
        m_rtGI.reset();
        m_rtReflections.reset();

        if (m_mouseCaptured)
            ShowCursor(TRUE);
    }

private:
    // ----------------------------------------------------------
    // DXR 初期化
    // ----------------------------------------------------------
    void InitDXR()
    {
        auto& ctx = GX_Internal::CompatContext::Instance();

        if (!ctx.graphicsDevice.SupportsRaytracing())
        {
            m_dxrSupported = false;
            ctx.postEffect.GetSSR().SetEnabled(true);
            return;
        }

        m_dxrSupported = true;
        auto* device5 = ctx.graphicsDevice.GetDevice5();
        uint32_t w = ctx.swapChain.GetWidth();
        uint32_t h = ctx.swapChain.GetHeight();

        // --- RTReflections ---
        m_rtReflections = std::make_unique<GX::RTReflections>();
        if (!m_rtReflections->Initialize(device5, w, h))
        {
            m_rtReflections.reset();
            m_dxrSupported = false;
            ctx.postEffect.GetSSR().SetEnabled(true);
            return;
        }

        // BLAS 構築
        ctx.commandList.Reset(0, nullptr);
        auto* cmdList4 = ctx.commandList.Get4();

        constexpr uint32_t stride = sizeof(GX::Vertex3D_PBR);

        auto vtxCount = [](GX::GPUMesh& m) -> uint32_t {
            return static_cast<uint32_t>(m.vertexBuffer.GetResource()->GetDesc().Width / stride);
        };
        auto build = [&](GX::GPUMesh& m) {
            return m_rtReflections->BuildBLAS(cmdList4,
                m.vertexBuffer.GetResource(), vtxCount(m), stride,
                m.indexBuffer.GetResource(), m.indexCount);
        };

        m_blasFloorCeil = build(m_floorCeilMesh);
        m_blasSideWall  = build(m_sideWallMesh);
        m_blasBackWall  = build(m_backWallMesh);
        m_blasTallBox   = build(m_tallBoxMesh);
        m_blasShortBox  = build(m_shortBoxMesh);
        m_blasSphere    = build(m_sphereMesh);

        ctx.commandList.Close();
        ID3D12CommandList* cls[] = { ctx.commandList.Get() };
        ctx.commandQueue.ExecuteCommandLists(cls, 1);
        ctx.commandQueue.Flush();

        m_rtReflections->CreateGeometrySRVs();
        ctx.postEffect.SetRTReflections(m_rtReflections.get());

        // ライト (レンダラーと同一)
        const float hd = k_RoomD / 2.0f;
        GX::LightData rtLights[2];
        rtLights[0] = GX::Light::CreateDirectional({ 0.1f, -0.8f, 0.3f }, { 1.0f, 0.98f, 0.95f }, 0.1f);
        rtLights[1] = GX::Light::CreatePoint({ 0.0f, k_RoomH - 0.15f, hd }, 8.0f, { 1.0f, 0.98f, 0.95f }, 6.0f);
        m_rtReflections->SetLights(rtLights, 2, { 0.01f, 0.01f, 0.012f });
        m_rtReflections->SetSkyColors({ 0.05f, 0.06f, 0.1f }, { 0.08f, 0.09f, 0.12f });

        // ディフューズシーンなので反射はデフォルトOFF
        m_rtReflections->SetEnabled(false);
        m_rtReflections->SetIntensity(0.7f);

        // --- RTGI ---
        m_rtGI = std::make_unique<GX::RTGI>();
        if (m_rtGI->Initialize(device5, w, h))
        {
            ctx.commandList.Reset(0, nullptr);
            auto* cl4 = ctx.commandList.Get4();

            auto buildGI = [&](GX::GPUMesh& m) {
                return m_rtGI->BuildBLAS(cl4,
                    m.vertexBuffer.GetResource(), vtxCount(m), stride,
                    m.indexBuffer.GetResource(), m.indexCount);
            };
            buildGI(m_floorCeilMesh);
            buildGI(m_sideWallMesh);
            buildGI(m_backWallMesh);
            buildGI(m_tallBoxMesh);
            buildGI(m_shortBoxMesh);
            buildGI(m_sphereMesh);

            ctx.commandList.Close();
            ID3D12CommandList* cl2[] = { ctx.commandList.Get() };
            ctx.commandQueue.ExecuteCommandLists(cl2, 1);
            ctx.commandQueue.Flush();

            m_rtGI->CreateGeometrySRVs();
            m_rtGI->SetLights(rtLights, 2, { 0.01f, 0.01f, 0.012f });
            m_rtGI->SetSkyColors({ 0.05f, 0.06f, 0.1f }, { 0.08f, 0.09f, 0.12f });

            // Cornell Box ではGIをデフォルトON
            m_rtGI->SetEnabled(true);
            m_rtGI->SetIntensity(1.0f);
            m_rtGI->SetMaxDistance(15.0f);

            ctx.postEffect.SetRTGI(m_rtGI.get());
        }
        else
        {
            m_rtGI.reset();
        }
    }

    // ----------------------------------------------------------
    // RT インスタンス登録 (毎フレーム)
    // ----------------------------------------------------------
    void RegisterRTInstances()
    {
        auto toF3 = [](const XMFLOAT4& c) { return XMFLOAT3{ c.x, c.y, c.z }; };

        // BLAS→壁/オブジェクト対応
        int wallBLAS[] = { m_blasFloorCeil, m_blasFloorCeil, m_blasSideWall, m_blasSideWall, m_blasBackWall };
        int objBLAS[]  = { m_blasTallBox, m_blasShortBox, m_blasSphere };

        auto registerAll = [&](auto* rt) {
            if (!rt || !rt->IsEnabled()) return;
            rt->BeginFrame();

            for (int i = 0; i < k_NumWalls; ++i)
                rt->AddInstance(wallBLAS[i], m_wallTransforms[i].GetWorldMatrix(),
                                toF3(m_wallMats[i].constants.albedoFactor),
                                m_wallMats[i].constants.metallicFactor,
                                m_wallMats[i].constants.roughnessFactor);

            for (int i = 0; i < k_NumObjects; ++i)
                rt->AddInstance(objBLAS[i], m_objTransforms[i].GetWorldMatrix(),
                                toF3(m_objMats[i].constants.albedoFactor),
                                m_objMats[i].constants.metallicFactor,
                                m_objMats[i].constants.roughnessFactor);
        };

        registerAll(m_rtReflections.get());
        registerAll(m_rtGI.get());
    }

    // ----------------------------------------------------------
    // シーンメッシュ描画 (シャドウ兼用)
    // ----------------------------------------------------------
    void DrawSceneMeshes(GX::Renderer3D& renderer)
    {
        GX::GPUMesh* wallMeshes[] = { &m_floorCeilMesh, &m_floorCeilMesh, &m_sideWallMesh, &m_sideWallMesh, &m_backWallMesh };
        for (int i = 0; i < k_NumWalls; ++i)
            renderer.DrawMesh(*wallMeshes[i], m_wallTransforms[i]);

        GX::GPUMesh* objMeshes[] = { &m_tallBoxMesh, &m_shortBoxMesh, &m_sphereMesh };
        for (int i = 0; i < k_NumObjects; ++i)
            renderer.DrawMesh(*objMeshes[i], m_objTransforms[i]);
    }

    // ----------------------------------------------------------
    // HUD
    // ----------------------------------------------------------
    void DrawHUD()
    {
        auto& ctx = GX_Internal::CompatContext::Instance();

        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        int y = 10;

        const TString fpsText = FormatT(TEXT("FPS: {:.1f}"), fps);
        DrawString(10, y, fpsText.c_str(), GetColor(255, 255, 255));
        y += 25;

        // GI
        bool giOn = m_rtGI && m_rtGI->IsEnabled();
        const TString giText = FormatT(TEXT("GI: {}"), giOn ? TEXT("ON") : TEXT("OFF"));
        DrawString(10, y, giText.c_str(), giOn ? GetColor(255, 200, 100) : GetColor(136, 136, 136));
        y += 25;

        // Reflection
        bool rtOn = m_rtReflections && m_rtReflections->IsEnabled();
        bool ssrOn = ctx.postEffect.GetSSR().IsEnabled();
        const TChar* mode = TEXT("OFF");
        unsigned int modeCol = GetColor(136, 136, 136);
        if (rtOn) { mode = TEXT("DXR"); modeCol = GetColor(100, 255, 100); }
        else if (ssrOn) { mode = TEXT("SSR"); modeCol = GetColor(100, 200, 255); }
        const TString reflText = FormatT(TEXT("Reflection: {}"), mode);
        DrawString(10, y, reflText.c_str(), modeCol);
        y += 25;

        if (!m_dxrSupported)
        {
            DrawString(10, y, TEXT("DXR not supported"), GetColor(255, 100, 100));
            y += 25;
        }

        // GI Debug
        if (m_rtGI && m_rtGI->GetDebugMode() > 0)
        {
            DrawString(10, y, TEXT("DEBUG: GI Only"), GetColor(255, 255, 0));
            y += 25;
        }

        y += 10;
        DrawString(10, y, TEXT("[G] GI  [Y] DXR Refl  [R] SSR  [T] GI Debug"), GetColor(200, 200, 200));
        y += 20;
        DrawString(10, y, TEXT("WASD Move  QE Up/Down  RClick Mouse"),
                   GetColor(136, 136, 136));
    }

    // === 定数 ===
    static constexpr float k_RoomW = 5.0f;
    static constexpr float k_RoomH = 3.5f;
    static constexpr float k_RoomD = 5.0f;
    static constexpr int k_NumWalls   = 5;  // floor, ceil, left, right, back
    static constexpr int k_NumObjects = 3;  // tall box, short box, sphere

    // === タイマー ===
    float m_totalTime = 0.0f;
    float m_lastDt = 0.0f;

    // === カメラ操作 ===
    bool m_mouseCaptured = false;
    int  m_lastMX = 0;
    int  m_lastMY = 0;

    // === メッシュ (6 unique shapes) ===
    GX::GPUMesh m_floorCeilMesh;
    GX::GPUMesh m_sideWallMesh;
    GX::GPUMesh m_backWallMesh;
    GX::GPUMesh m_tallBoxMesh;
    GX::GPUMesh m_shortBoxMesh;
    GX::GPUMesh m_sphereMesh;

    // === 壁面 ===
    GX::Transform3D m_wallTransforms[k_NumWalls];
    GX::Material    m_wallMats[k_NumWalls];

    // === 室内オブジェクト ===
    GX::Transform3D m_objTransforms[k_NumObjects];
    GX::Material    m_objMats[k_NumObjects];

    // === DXR ===
    bool m_dxrSupported = false;
    std::unique_ptr<GX::RTReflections> m_rtReflections;
    std::unique_ptr<GX::RTGI> m_rtGI;
    int m_blasFloorCeil = -1;
    int m_blasSideWall  = -1;
    int m_blasBackWall  = -1;
    int m_blasTallBox   = -1;
    int m_blasShortBox  = -1;
    int m_blasSphere    = -1;
};

GX_EASY_APP(DXRShowcaseApp)
