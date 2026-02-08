/// @file main.cpp
/// @brief GXLib Phase 4f テストアプリケーション — HDR + SSAO + Bloom + FXAA + Vignette + ColorGrading
///
/// HDR浮動小数点RTに3D描画 → SSAO → Bloom → ColorGrading → トーンマッピング → FXAA → Vignette → LDRバックバッファ

#include "pch.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Device/CommandQueue.h"
#include "Graphics/Device/CommandList.h"
#include "Graphics/Device/SwapChain.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Transform3D.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Fog.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Input/InputManager.h"

// ============================================================================
// グローバル変数
// ============================================================================
static GX::Application       g_app;
static GX::GraphicsDevice    g_device;
static GX::CommandQueue      g_commandQueue;
static GX::CommandList       g_commandList;
static GX::SwapChain         g_swapChain;

static GX::SpriteBatch       g_spriteBatch;
static GX::FontManager       g_fontManager;
static GX::TextRenderer      g_textRenderer;
static GX::InputManager      g_inputManager;

// 3D
static GX::Renderer3D        g_renderer3D;
static GX::Camera3D          g_camera;

// ポストエフェクト
static GX::PostEffectPipeline g_postEffect;

// メッシュ
static GX::GPUMesh           g_sphereMesh;
static GX::GPUMesh           g_planeMesh;
static GX::GPUMesh           g_cubeMesh;
static GX::GPUMesh           g_cylinderMesh;
static GX::GPUMesh           g_tallBoxMesh;
static GX::GPUMesh           g_wallMesh;

// 球体（少数、AO確認用）
static constexpr int k_NumSpheres = 3;
static GX::Transform3D g_sphereTransforms[k_NumSpheres];
static GX::Material    g_sphereMaterials[k_NumSpheres];

// 地面
static GX::Transform3D g_planeTransform;
static GX::Material    g_planeMaterial;

// 箱の集まり（地面に置いた箱群 → 接地部AO確認）
static constexpr int k_NumBoxes = 6;
static GX::Transform3D g_boxTransforms[k_NumBoxes];
static GX::Material    g_boxMaterials[k_NumBoxes];

// 柱（円柱 → 地面との接合部AO確認）
static constexpr int k_NumPillars = 4;
static GX::Transform3D g_pillarTransforms[k_NumPillars];
static GX::Material    g_pillarMaterial;

// 壁（L字コーナー → 凹角AO確認）
static constexpr int k_NumWalls = 2;
static GX::Transform3D g_wallTransforms[k_NumWalls];
static GX::Material    g_wallMaterial;

// 段差（階段状 → 角のAO確認）
static constexpr int k_NumSteps = 4;
static GX::Transform3D g_stepTransforms[k_NumSteps];
static GX::Material    g_stepMaterial;

// 回転キューブ
static GX::Transform3D g_cubeTransform;
static GX::Material    g_cubeMaterial;

static uint64_t g_frameFenceValues[GX::SwapChain::k_BufferCount] = {};
static uint32_t g_frameIndex = 0;
static float    g_totalTime  = 0.0f;
static int g_fontHandle = -1;

// カメラ制御
static float g_cameraSpeed      = 5.0f;
static float g_mouseSensitivity = 0.003f;
static bool  g_mouseCaptured    = false;
static int   g_lastMouseX = 0;
static int   g_lastMouseY = 0;

// ============================================================================
// シーン描画（シャドウパスとメインパスで共通）
// ============================================================================

void DrawScene()
{
    // 地面
    g_renderer3D.SetMaterial(g_planeMaterial);
    g_renderer3D.DrawMesh(g_planeMesh, g_planeTransform);

    // 球体
    for (int i = 0; i < k_NumSpheres; ++i)
    {
        g_renderer3D.SetMaterial(g_sphereMaterials[i]);
        g_renderer3D.DrawMesh(g_sphereMesh, g_sphereTransforms[i]);
    }

    // 箱の集まり
    for (int i = 0; i < k_NumBoxes; ++i)
    {
        g_renderer3D.SetMaterial(g_boxMaterials[i]);
        g_renderer3D.DrawMesh(g_cubeMesh, g_boxTransforms[i]);
    }

    // 柱
    g_renderer3D.SetMaterial(g_pillarMaterial);
    for (int i = 0; i < k_NumPillars; ++i)
        g_renderer3D.DrawMesh(g_cylinderMesh, g_pillarTransforms[i]);

    // 壁（L字コーナー）
    g_renderer3D.SetMaterial(g_wallMaterial);
    for (int i = 0; i < k_NumWalls; ++i)
        g_renderer3D.DrawMesh(g_wallMesh, g_wallTransforms[i]);

    // 段差
    g_renderer3D.SetMaterial(g_stepMaterial);
    for (int i = 0; i < k_NumSteps; ++i)
        g_renderer3D.DrawMesh(g_tallBoxMesh, g_stepTransforms[i]);

    // 回転キューブ
    g_renderer3D.SetMaterial(g_cubeMaterial);
    g_renderer3D.DrawMesh(g_cubeMesh, g_cubeTransform);
}

// ============================================================================
// 初期化
// ============================================================================

bool InitializeGraphics()
{
    auto* device = g_device.GetDevice();

    if (!g_commandQueue.Initialize(device))
        return false;
    if (!g_commandList.Initialize(device))
        return false;

    GX::SwapChainDesc scDesc;
    scDesc.hwnd   = g_app.GetWindow().GetHWND();
    scDesc.width  = g_app.GetWindow().GetWidth();
    scDesc.height = g_app.GetWindow().GetHeight();

    if (!g_swapChain.Initialize(g_device.GetFactory(), device, g_commandQueue.GetQueue(), scDesc))
        return false;

    return true;
}

bool InitializeRenderers()
{
    auto* device = g_device.GetDevice();
    auto* queue  = g_commandQueue.GetQueue();
    uint32_t w = g_app.GetWindow().GetWidth();
    uint32_t h = g_app.GetWindow().GetHeight();

    if (!g_spriteBatch.Initialize(device, queue, w, h))
        return false;
    if (!g_fontManager.Initialize(device, &g_spriteBatch.GetTextureManager()))
        return false;
    g_textRenderer.Initialize(&g_spriteBatch, &g_fontManager);

    if (!g_renderer3D.Initialize(device, queue, w, h))
        return false;

    // ポストエフェクトパイプライン
    if (!g_postEffect.Initialize(device, w, h))
        return false;

    return true;
}

bool InitializeScene()
{
    g_fontHandle = g_fontManager.CreateFont(L"Meiryo", 20);
    if (g_fontHandle < 0)
        g_fontHandle = g_fontManager.CreateFont(L"MS Gothic", 20);

    // メッシュ生成
    auto sphereData   = GX::MeshGenerator::CreateSphere(0.5f, 32, 16);
    auto planeData    = GX::MeshGenerator::CreatePlane(40.0f, 40.0f, 40, 40);
    auto boxData      = GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f);
    auto cylinderData = GX::MeshGenerator::CreateCylinder(0.3f, 0.3f, 3.0f, 16, 1);
    auto tallBoxData  = GX::MeshGenerator::CreateBox(2.0f, 0.5f, 3.0f);
    auto wallData     = GX::MeshGenerator::CreateBox(0.3f, 3.0f, 6.0f);

    g_sphereMesh   = g_renderer3D.CreateGPUMesh(sphereData);
    g_planeMesh    = g_renderer3D.CreateGPUMesh(planeData);
    g_cubeMesh     = g_renderer3D.CreateGPUMesh(boxData);
    g_cylinderMesh = g_renderer3D.CreateGPUMesh(cylinderData);
    g_tallBoxMesh  = g_renderer3D.CreateGPUMesh(tallBoxData);
    g_wallMesh     = g_renderer3D.CreateGPUMesh(wallData);

    // === 球体（地面に接するように配置 → 接地部AO） ===
    g_sphereTransforms[0].SetPosition(0.0f, 0.5f, 0.0f);
    g_sphereMaterials[0].constants.albedoFactor = { 0.8f, 0.2f, 0.2f, 1.0f };
    g_sphereMaterials[0].constants.metallicFactor  = 0.0f;
    g_sphereMaterials[0].constants.roughnessFactor = 0.5f;

    g_sphereTransforms[1].SetPosition(1.5f, 0.5f, 0.0f);
    g_sphereMaterials[1].constants.albedoFactor = { 0.2f, 0.8f, 0.2f, 1.0f };
    g_sphereMaterials[1].constants.metallicFactor  = 0.5f;
    g_sphereMaterials[1].constants.roughnessFactor = 0.3f;

    // 箱の角に置いた球 → 箱と球の間にAO
    g_sphereTransforms[2].SetPosition(-3.0f, 1.5f, 2.0f);
    g_sphereMaterials[2].constants.albedoFactor = { 0.2f, 0.2f, 0.8f, 1.0f };
    g_sphereMaterials[2].constants.metallicFactor  = 0.0f;
    g_sphereMaterials[2].constants.roughnessFactor = 0.8f;

    // === 地面 ===
    g_planeTransform.SetPosition(0.0f, 0.0f, 0.0f);
    g_planeMaterial.constants.albedoFactor = { 0.5f, 0.5f, 0.52f, 1.0f };
    g_planeMaterial.constants.metallicFactor  = 0.0f;
    g_planeMaterial.constants.roughnessFactor = 0.9f;

    // === 箱の集まり（密集配置 → 箱間のAO） ===
    {
        XMFLOAT4 boxColor = { 0.7f, 0.65f, 0.55f, 1.0f };
        // 地面に並べた箱
        float bx = -3.0f, bz = 0.0f;
        g_boxTransforms[0].SetPosition(bx, 0.5f, bz);
        g_boxTransforms[1].SetPosition(bx + 1.05f, 0.5f, bz);
        g_boxTransforms[2].SetPosition(bx + 0.5f, 0.5f, bz + 1.05f);
        // 積み上げた箱
        g_boxTransforms[3].SetPosition(bx, 1.5f, bz);
        g_boxTransforms[3].SetRotation(0.0f, 0.3f, 0.0f);
        // 大きさ違い
        g_boxTransforms[4].SetPosition(bx + 2.5f, 0.75f, bz);
        g_boxTransforms[4].SetScale(1.5f, 1.5f, 1.5f);
        // 傾いた箱
        g_boxTransforms[5].SetPosition(bx + 1.0f, 0.5f, bz - 1.5f);
        g_boxTransforms[5].SetRotation(0.0f, 0.78f, 0.0f);
        for (int i = 0; i < k_NumBoxes; ++i)
        {
            g_boxMaterials[i].constants.albedoFactor = boxColor;
            g_boxMaterials[i].constants.metallicFactor  = 0.0f;
            g_boxMaterials[i].constants.roughnessFactor = 0.7f;
        }
    }

    // === 柱（地面との接合部 → 根元AO） ===
    g_pillarTransforms[0].SetPosition(4.0f, 1.5f, 3.0f);
    g_pillarTransforms[1].SetPosition(6.0f, 1.5f, 3.0f);
    g_pillarTransforms[2].SetPosition(4.0f, 1.5f, 5.0f);
    g_pillarTransforms[3].SetPosition(6.0f, 1.5f, 5.0f);
    g_pillarMaterial.constants.albedoFactor = { 0.6f, 0.6f, 0.6f, 1.0f };
    g_pillarMaterial.constants.metallicFactor  = 0.0f;
    g_pillarMaterial.constants.roughnessFactor = 0.6f;

    // === L字壁（コーナー凹角 → 強いAO） ===
    g_wallTransforms[0].SetPosition(8.0f, 1.5f, 0.0f);   // 壁1（Z方向）
    g_wallTransforms[1].SetPosition(8.0f + 3.0f, 1.5f, -2.85f); // 壁2（X方向）
    g_wallTransforms[1].SetRotation(0.0f, XM_PIDIV2, 0.0f);
    g_wallMaterial.constants.albedoFactor = { 0.75f, 0.72f, 0.68f, 1.0f };
    g_wallMaterial.constants.metallicFactor  = 0.0f;
    g_wallMaterial.constants.roughnessFactor = 0.85f;

    // === 段差（階段状） ===
    for (int i = 0; i < k_NumSteps; ++i)
    {
        float y = (i + 1) * 0.25f;  // 0.25, 0.5, 0.75, 1.0
        float z = -4.0f + i * 1.0f;
        g_stepTransforms[i].SetPosition(0.0f, y, z);
    }
    g_stepMaterial.constants.albedoFactor = { 0.55f, 0.55f, 0.6f, 1.0f };
    g_stepMaterial.constants.metallicFactor  = 0.0f;
    g_stepMaterial.constants.roughnessFactor = 0.8f;

    // === 回転キューブ ===
    g_cubeTransform.SetPosition(3.0f, 0.5f, -2.0f);
    g_cubeMaterial.constants.albedoFactor = { 0.95f, 0.93f, 0.88f, 1.0f };
    g_cubeMaterial.constants.metallicFactor  = 1.0f;
    g_cubeMaterial.constants.roughnessFactor = 0.3f;

    // ライト設定
    GX::LightData lights[3];
    lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
    lights[1] = GX::Light::CreatePoint({ -3.0f, 3.0f, -3.0f }, 15.0f, { 0.2f, 0.5f, 1.0f }, 10.0f);
    lights[2] = GX::Light::CreateSpot({ 4.0f, 4.0f, -2.0f }, { -0.5f, -1.0f, 0.3f },
                                        20.0f, 30.0f, { 1.0f, 0.8f, 0.3f }, 15.0f);
    g_renderer3D.SetLights(lights, 3, { 0.03f, 0.03f, 0.04f });

    // フォグ設定（Linear）
    g_renderer3D.SetFog(GX::FogMode::Linear, { 0.6f, 0.65f, 0.75f }, 30.0f, 150.0f);

    // スカイボックスの太陽方向をDirectionalライトと合わせる
    g_renderer3D.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
    g_renderer3D.GetSkybox().SetColors({ 0.2f, 0.4f, 0.85f }, { 0.6f, 0.65f, 0.75f });

    // カメラ
    uint32_t w = g_app.GetWindow().GetWidth();
    uint32_t h = g_app.GetWindow().GetHeight();
    g_camera.SetPerspective(XM_PIDIV4, static_cast<float>(w) / h, 0.1f, 1000.0f);
    g_camera.SetPosition(2.0f, 4.0f, -8.0f);
    g_camera.Rotate(0.35f, 0.0f);

    return true;
}

// ============================================================================
// 更新
// ============================================================================

void UpdateInput(float deltaTime)
{
    g_inputManager.Update();

    if (g_inputManager.CheckHitKey(VK_ESCAPE))
    {
        PostQuitMessage(0);
        return;
    }

    // トーンマッピングモード切替（トリガー入力で切替）
    auto& kb = g_inputManager.GetKeyboard();
    if (kb.IsKeyTriggered('1'))
        g_postEffect.SetTonemapMode(GX::TonemapMode::Reinhard);
    if (kb.IsKeyTriggered('2'))
        g_postEffect.SetTonemapMode(GX::TonemapMode::ACES);
    if (kb.IsKeyTriggered('3'))
        g_postEffect.SetTonemapMode(GX::TonemapMode::Uncharted2);

    // Bloom ON/OFF
    if (kb.IsKeyTriggered('4'))
        g_postEffect.GetBloom().SetEnabled(!g_postEffect.GetBloom().IsEnabled());

    // FXAA ON/OFF
    if (kb.IsKeyTriggered('5'))
        g_postEffect.SetFXAAEnabled(!g_postEffect.IsFXAAEnabled());

    // Vignette ON/OFF
    if (kb.IsKeyTriggered('6'))
        g_postEffect.SetVignetteEnabled(!g_postEffect.IsVignetteEnabled());

    // ColorGrading ON/OFF
    if (kb.IsKeyTriggered('7'))
        g_postEffect.SetColorGradingEnabled(!g_postEffect.IsColorGradingEnabled());

    // Shadow debug mode cycle
    if (kb.IsKeyTriggered('8'))
    {
        uint32_t mode = (g_renderer3D.GetShadowDebugMode() + 1) % 7;
        g_renderer3D.SetShadowDebugMode(mode);
    }

    // SSAO ON/OFF
    if (kb.IsKeyTriggered('9'))
        g_postEffect.GetSSAO().SetEnabled(!g_postEffect.GetSSAO().IsEnabled());

    // 露出調整
    if (g_inputManager.CheckHitKey(VK_OEM_PLUS) || g_inputManager.CheckHitKey(VK_ADD))
        g_postEffect.SetExposure(g_postEffect.GetExposure() + 0.5f * deltaTime);
    if (g_inputManager.CheckHitKey(VK_OEM_MINUS) || g_inputManager.CheckHitKey(VK_SUBTRACT))
        g_postEffect.SetExposure((std::max)(g_postEffect.GetExposure() - 0.5f * deltaTime, 0.1f));

    auto& mouse = g_inputManager.GetMouse();
    if (mouse.IsButtonTriggered(GX::MouseButton::Right))
    {
        g_mouseCaptured = !g_mouseCaptured;
        if (g_mouseCaptured)
        {
            g_lastMouseX = mouse.GetX();
            g_lastMouseY = mouse.GetY();
            ShowCursor(FALSE);
        }
        else
        {
            ShowCursor(TRUE);
        }
    }

    if (g_mouseCaptured)
    {
        int mx = mouse.GetX();
        int my = mouse.GetY();
        g_camera.Rotate(
            static_cast<float>(my - g_lastMouseY) * g_mouseSensitivity,
            static_cast<float>(mx - g_lastMouseX) * g_mouseSensitivity);
        g_lastMouseX = mx;
        g_lastMouseY = my;
    }

    float speed = g_cameraSpeed * deltaTime;
    if (g_inputManager.CheckHitKey(VK_SHIFT))
        speed *= 3.0f;

    if (g_inputManager.CheckHitKey('W')) g_camera.MoveForward(speed);
    if (g_inputManager.CheckHitKey('S')) g_camera.MoveForward(-speed);
    if (g_inputManager.CheckHitKey('D')) g_camera.MoveRight(speed);
    if (g_inputManager.CheckHitKey('A')) g_camera.MoveRight(-speed);
    if (g_inputManager.CheckHitKey('E')) g_camera.MoveUp(speed);
    if (g_inputManager.CheckHitKey('Q')) g_camera.MoveUp(-speed);

    g_cubeTransform.SetRotation(g_totalTime * 0.5f, g_totalTime * 0.7f, 0.0f);
}

// ============================================================================
// 描画
// ============================================================================

void RenderFrame(float deltaTime)
{
    g_totalTime += deltaTime;
    UpdateInput(deltaTime);

    g_frameIndex = g_swapChain.GetCurrentBackBufferIndex();
    g_commandQueue.GetFence().WaitForValue(g_frameFenceValues[g_frameIndex]);
    g_commandList.Reset(g_frameIndex, nullptr);
    auto* cmdList = g_commandList.Get();

    // === シャドウパス ===
    g_renderer3D.UpdateShadow(g_camera);

    // CSMパス
    for (uint32_t cascade = 0; cascade < GX::CascadedShadowMap::k_NumCascades; ++cascade)
    {
        g_renderer3D.BeginShadowPass(cmdList, g_frameIndex, cascade);
        DrawScene();
        g_renderer3D.EndShadowPass(cascade);
    }

    // スポットシャドウパス
    g_renderer3D.BeginSpotShadowPass(cmdList, g_frameIndex);
    DrawScene();
    g_renderer3D.EndSpotShadowPass();

    // ポイントシャドウパス（6面）
    for (uint32_t face = 0; face < 6; ++face)
    {
        g_renderer3D.BeginPointShadowPass(cmdList, g_frameIndex, face);
        DrawScene();
        g_renderer3D.EndPointShadowPass(face);
    }

    // === HDRシーン描画パス ===
    auto dsvHandle = g_renderer3D.GetDepthBuffer().GetDSVHandle();
    g_postEffect.BeginScene(cmdList, g_frameIndex, dsvHandle);

    // スカイボックス（最初に描画、深度書き込みOFF）
    {
        XMFLOAT4X4 viewF;
        XMStoreFloat4x4(&viewF, g_camera.GetViewMatrix());
        viewF._41 = 0.0f; viewF._42 = 0.0f; viewF._43 = 0.0f;
        XMMATRIX viewRotOnly = XMLoadFloat4x4(&viewF);

        XMFLOAT4X4 vp;
        XMStoreFloat4x4(&vp, XMMatrixTranspose(viewRotOnly * g_camera.GetProjectionMatrix()));
        g_renderer3D.GetSkybox().Draw(cmdList, g_frameIndex, vp);
    }

    // === 3D PBR描画 ===
    g_renderer3D.Begin(cmdList, g_frameIndex, g_camera, g_totalTime);
    DrawScene();
    g_renderer3D.End();

    // === デバッグプリミティブ ===
    {
        XMFLOAT4X4 vp;
        XMStoreFloat4x4(&vp, XMMatrixTranspose(g_camera.GetViewProjectionMatrix()));
        auto& prim = g_renderer3D.GetPrimitiveBatch3D();
        prim.Begin(cmdList, g_frameIndex, vp);
        prim.DrawWireSphere({ -3.0f, 3.0f, -3.0f }, 0.3f, { 0.2f, 0.5f, 1.0f, 0.8f });
        prim.DrawWireSphere({ 4.0f, 4.0f, -2.0f }, 0.3f, { 1.0f, 0.8f, 0.3f, 0.8f });
        prim.End();
    }

    // === ポストエフェクト: HDR → LDR ===
    g_postEffect.EndScene();

    // バックバッファ → RENDER_TARGET
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = g_swapChain.GetCurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    auto rtvHandle = g_swapChain.GetCurrentRTVHandle();

    // SSAO → Bloom → トーンマッピング → バックバッファ
    g_postEffect.Resolve(rtvHandle, g_renderer3D.GetDepthBuffer(), g_camera);

    // === 2Dテキスト（LDRバックバッファ上に直接描画）===
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    g_spriteBatch.Begin(cmdList, g_frameIndex);
    {
        if (g_fontHandle >= 0)
        {
            g_textRenderer.DrawFormatString(g_fontHandle, 10, 10, 0xFFFFFFFF,
                L"FPS: %.1f", g_app.GetTimer().GetFPS());

            auto pos = g_camera.GetPosition();
            g_textRenderer.DrawFormatString(g_fontHandle, 10, 35, 0xFF88BBFF,
                L"Camera: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

            // トーンマッピング＋ブルーム情報
            const wchar_t* tonemapNames[] = { L"Reinhard", L"ACES", L"Uncharted2" };
            uint32_t tmIdx = static_cast<uint32_t>(g_postEffect.GetTonemapMode());
            g_textRenderer.DrawFormatString(g_fontHandle, 10, 60, 0xFF88FF88,
                L"Tonemap: %s  Exposure: %.2f", tonemapNames[tmIdx], g_postEffect.GetExposure());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 85, 0xFF88FF88,
                L"Bloom: %s  Threshold: %.2f  Intensity: %.2f",
                g_postEffect.GetBloom().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetBloom().GetThreshold(),
                g_postEffect.GetBloom().GetIntensity());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 110, 0xFF88FF88,
                L"FXAA: %s  Vignette: %s  ChromAberr: %.4f  ColorGrading: %s",
                g_postEffect.IsFXAAEnabled() ? L"ON" : L"OFF",
                g_postEffect.IsVignetteEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetChromaticAberration(),
                g_postEffect.IsColorGradingEnabled() ? L"ON" : L"OFF");

            if (g_postEffect.IsColorGradingEnabled())
            {
                g_textRenderer.DrawFormatString(g_fontHandle, 10, 135, 0xFF88FF88,
                    L"Contrast: %.2f  Saturation: %.2f  Temperature: %.2f",
                    g_postEffect.GetContrast(), g_postEffect.GetSaturation(),
                    g_postEffect.GetTemperature());
            }

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 160, 0xFF88FF88,
                L"SSAO: %s  Radius: %.2f  Power: %.2f",
                g_postEffect.GetSSAO().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetSSAO().GetRadius(),
                g_postEffect.GetSSAO().GetPower());

            const wchar_t* shadowDebugNames[] = { L"OFF", L"Factor", L"Cascade", L"ShadowUV", L"RawDepth", L"Normal", L"ViewZ" };
            g_textRenderer.DrawFormatString(g_fontHandle, 10, 185, 0xFFFF8888,
                L"ShadowDebug: %s  Shadow: %s",
                shadowDebugNames[g_renderer3D.GetShadowDebugMode()],
                g_renderer3D.IsShadowEnabled() ? L"ON" : L"OFF");

            float helpY = static_cast<float>(g_swapChain.GetHeight()) - 60.0f;
            g_textRenderer.DrawString(g_fontHandle, 10, helpY,
                L"WASD: Move  QE: Up/Down  Shift: Fast  RClick: Mouse  ESC: Quit", 0xFFAAAAAA);
            g_textRenderer.DrawString(g_fontHandle, 10, helpY + 25,
                L"1/2/3: Tonemap  4: Bloom  5: FXAA  6: Vignette  7: ColorGrading  8: ShadowDbg  9: SSAO  +/-: Exposure", 0xFFFFCC44);
        }
    }
    g_spriteBatch.End();

    // バックバッファ → PRESENT
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    cmdList->ResourceBarrier(1, &barrier);

    g_commandList.Close();

    ID3D12CommandList* cmdLists[] = { cmdList };
    g_commandQueue.ExecuteCommandLists(cmdLists, 1);

    g_swapChain.Present(false);
    g_frameFenceValues[g_frameIndex] = g_commandQueue.GetFence().Signal(g_commandQueue.GetQueue());
}

void OnResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    g_commandQueue.Flush();
    g_swapChain.Resize(g_device.GetDevice(), width, height);
    g_spriteBatch.SetScreenSize(width, height);
    g_renderer3D.OnResize(width, height);
    g_postEffect.OnResize(g_device.GetDevice(), width, height);
    g_camera.SetPerspective(g_camera.GetFovY(), static_cast<float>(width) / height,
                             g_camera.GetNearZ(), g_camera.GetFarZ());
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    GX::ApplicationDesc appDesc;
    appDesc.title  = L"GXLib - Phase 4: Post-Effects (SSAO/Bloom/FXAA/Vignette/ColorGrading)";
    appDesc.width  = 1280;
    appDesc.height = 720;

    if (!g_app.Initialize(appDesc))
        return -1;

    g_inputManager.Initialize(g_app.GetWindow());

#ifdef _DEBUG
    bool enableDebug = true;
#else
    bool enableDebug = false;
#endif
    if (!g_device.Initialize(enableDebug))
        return -1;
    if (!InitializeGraphics())
        return -1;
    if (!InitializeRenderers())
        return -1;
    if (!InitializeScene())
        return -1;

    g_app.GetWindow().SetResizeCallback(OnResize);
    GX_LOG_INFO("=== GXLib Phase 4: Post-Effects (SSAO/Bloom/FXAA/Vignette/ColorGrading) ===");

    g_app.Run(RenderFrame);

    g_commandQueue.Flush();
    if (g_mouseCaptured) ShowCursor(TRUE);
    g_inputManager.Shutdown();
    g_fontManager.Shutdown();
    g_app.Shutdown();

    return 0;
}
