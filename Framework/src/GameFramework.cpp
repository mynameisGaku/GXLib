/// @file GameFramework.cpp
/// @brief GameFramework の実装
///
/// Sandbox と同等の完全なレンダリングパイプラインを内包する。
/// ユーザーは GameBase を継承するだけで、エンジン初期化から
/// シャドウ・PBR・ポストエフェクト・大気散乱・雲まで全てが動作する。

#include "GameFramework.h"
#include "Graphics/3D/CascadedShadowMap.h"
#include "Graphics/3D/Fog.h"

namespace gx
{

// ============================================================================
// GameBase — サブシステムへのアクセサ
// ============================================================================

Scene&               GameBase::GetScene()          { return m_framework->m_defaultScene; }
Camera3D&            GameBase::GetMainCamera()     { return m_framework->m_mainCamera; }
InputManager&        GameBase::GetInput()          { return m_framework->m_inputManager; }
Renderer3D&          GameBase::GetRenderer()       { return m_framework->m_renderer3D; }
AudioManager&        GameBase::GetAudio()          { return m_framework->m_audioManager; }
PostEffectPipeline&  GameBase::GetPostEffects()    { return m_framework->m_postEffect; }
SpriteBatch&         GameBase::GetSpriteBatch()    { return m_framework->m_spriteBatch; }
PrimitiveBatch&      GameBase::GetPrimitiveBatch() { return m_framework->m_primitiveBatch; }
TextRenderer&        GameBase::GetText()           { return m_framework->m_textRenderer; }
FontManager&         GameBase::GetFontManager()    { return m_framework->m_fontManager; }
SceneManager&        GameBase::GetSceneManager()   { return m_framework->m_sceneManager; }
Window&              GameBase::GetWindow()         { return m_framework->m_app.GetWindow(); }

float    GameBase::GetDeltaTime()    const { return m_framework->m_deltaTime; }
float    GameBase::GetTotalTime()    const { return m_framework->m_totalTime; }
uint32_t GameBase::GetScreenWidth()  const { return m_framework->m_screenWidth; }
uint32_t GameBase::GetScreenHeight() const { return m_framework->m_screenHeight; }
void     GameBase::ToggleDebugOverlay()    { m_framework->m_debugOverlay.Toggle(); }

// ============================================================================
// GameFramework::Run — エントリポイント
// ============================================================================

int GameFramework::Run(GameBase& game, const Config& config)
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    game.m_framework = this;
    m_vsync = config.vsync;

    if (!Initialize(config))
    {
        CoUninitialize();
        return -1;
    }

    SetupDefaultScene();
    SetupDefaultSky();
    game.OnStart();

    MainLoop(game);

    Shutdown(game);
    CoUninitialize();
    return 0;
}

// ============================================================================
// Initialize — エンジン全体の初期化
// ============================================================================

bool GameFramework::Initialize(const Config& config)
{
    // Application (Window + Timer)
    ApplicationDesc appDesc;
    appDesc.title  = config.title;
    appDesc.width  = config.width;
    appDesc.height = config.height;

    if (!m_app.Initialize(appDesc))
        return false;

    m_screenWidth  = m_app.GetWindow().GetWidth();
    m_screenHeight = m_app.GetWindow().GetHeight();

    // GraphicsDevice
#ifdef _DEBUG
    bool enableDebug = true;
#else
    bool enableDebug = false;
#endif
    if (!m_graphicsDevice.Initialize(enableDebug))
        return false;

    m_device = m_graphicsDevice.GetDevice();

    // Graphics pipeline
    if (!InitializeGraphics())
        return false;

    // Renderers
    if (!InitializeRenderers())
        return false;

    // Camera
    float aspect = static_cast<float>(m_screenWidth) / static_cast<float>(m_screenHeight);
    m_mainCamera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 1000.0f);
    m_mainCamera.SetPosition(0.0f, 3.0f, -10.0f);
    m_mainCamera.LookAt({ 0.0f, 0.0f, 0.0f });

    // RenderProfile + QualitySettings
    auto profile = RenderProfile::CreateDefault();
    profile.Apply(m_postEffect, &m_renderer3D);

    auto detectedLevel = QualitySettings::AutoDetect(m_graphicsDevice);
    m_qualitySettings.SetQualityLevel(detectedLevel);
    m_qualitySettings.Apply(m_postEffect);

    // ImGui 初期化（デバッグオーバーレイ用）
    // InputManager より先に登録する。ImGui のコールバックは常に false を返すので
    // InputManager 側のメッセージ処理はブロックされない。
    // 逆順だと InputManager が return true でチェーンを切り、ImGui にマウスイベントが届かない。
    m_imguiManager.Initialize(m_device, m_commandQueue.GetQueue(), m_app.GetWindow());

    // Input（ImGui の後に初期化 — コールバック登録順序が重要）
    m_inputManager.Initialize(m_app.GetWindow());

    GX_LOG_INFO("=== GameFramework: Initialized ===");
    return true;
}

bool GameFramework::InitializeGraphics()
{
    if (!m_commandQueue.Initialize(m_device))
        return false;
    if (!m_commandList.Initialize(m_device))
        return false;

    SwapChainDesc scDesc;
    scDesc.hwnd   = m_app.GetWindow().GetHWND();
    scDesc.width  = m_screenWidth;
    scDesc.height = m_screenHeight;

    if (!m_swapChain.Initialize(m_graphicsDevice.GetFactory(), m_device,
                                 m_commandQueue.GetQueue(), scDesc))
        return false;

    return true;
}

bool GameFramework::InitializeRenderers()
{
    auto* queue = m_commandQueue.GetQueue();

    if (!m_spriteBatch.Initialize(m_device, queue, m_screenWidth, m_screenHeight))
        return false;
    if (!m_primitiveBatch.Initialize(m_device, m_screenWidth, m_screenHeight))
        return false;
    if (!m_fontManager.Initialize(m_device, &m_spriteBatch.GetTextureManager()))
        return false;
    m_textRenderer.Initialize(&m_spriteBatch, &m_fontManager);

    if (!m_renderer3D.Initialize(m_device, queue, m_screenWidth, m_screenHeight))
        return false;
    m_postEffect.SetGraphicsDevice(&m_graphicsDevice);
    if (!m_postEffect.Initialize(m_device, m_screenWidth, m_screenHeight))
        return false;

    // AudioManager
    m_audioManager.Initialize();

    // デフォルトフォント
    m_defaultFont = m_fontManager.CreateFont(L"Meiryo", 20);
    if (m_defaultFont < 0)
        m_defaultFont = m_fontManager.CreateFont(L"MS Gothic", 20);

    return true;
}

// ============================================================================
// SetupDefaultScene — デフォルトシーン構築
// ============================================================================

void GameFramework::SetupDefaultScene()
{
    // Main Camera エンティティ
    auto* camEntity = m_defaultScene.CreateEntity("Main Camera");
    auto* camComp = camEntity->AddComponent<CameraComponent>();
    camComp->camera = m_mainCamera;
    camComp->isMain = true;

    // Directional Light エンティティ (太陽光)
    auto* lightEntity = m_defaultScene.CreateEntity("Directional Light");
    auto* lightComp = lightEntity->AddComponent<LightComponent>();
    lightComp->lightData = Light::CreateDirectional(
        m_sunDirection,                    // direction
        Vector3{ 1.0f, 0.98f, 0.95f },    // warm white
        1.5f                                // intensity
    );

    // Transform を設定
    auto* lightTransform = lightEntity->GetComponent<TransformComponent>();
    if (lightTransform)
        lightTransform->position = { 0.0f, 10.0f, 0.0f };

    // やや青みがかった環境光
    m_ambientColor = Vector3{ 0.15f, 0.15f, 0.2f };

    GX_LOG_INFO("GameFramework: Default scene created (Camera + Directional Light + Sky)");
}

// ============================================================================
// SetupDefaultSky — スカイボックス + ボリューメトリッククラウド
// ============================================================================

void GameFramework::SetupDefaultSky()
{
    // --- Skybox (プロシージャルグラデーション + 太陽ディスク) ---
    auto& skybox = m_renderer3D.GetSkybox();
    skybox.SetColors(
        Vector3{ 0.15f, 0.3f, 0.8f },    // 天頂: 深い青
        Vector3{ 0.6f, 0.75f, 0.95f }     // 地平: 淡い水色
    );
    skybox.SetSun(m_sunDirection, 8.0f);   // HDR 太陽ハイライト

    // --- VolumetricClouds (リアルタイム雲) ---
    auto& clouds = m_postEffect.GetVolumetricClouds();
    clouds.SetEnabled(true);
    clouds.SetCloudBottom(1500.0f);        // 雲底 1.5km
    clouds.SetCloudTop(3500.0f);           // 雲頂 3.5km
    clouds.SetCoverage(0.45f);             // 適度な雲量
    clouds.SetDensity(0.04f);              // 柔らかい密度
    clouds.SetWindSpeed(8.0f);             // 穏やかな風
    clouds.SetWindDirection(Vector3{ 1.0f, 0.0f, 0.3f });
    clouds.SetSunDirection(m_sunDirection);
    clouds.SetSunColor(Vector3{ 1.0f, 0.98f, 0.95f });
    clouds.SetSilverLiningIntensity(0.6f); // 太陽側の雲の縁が光る

    // --- LensFlare (物理ベースレンズフレア) ---
    auto& lensFlare = m_postEffect.GetLensFlare();
    lensFlare.SetEnabled(true);
    lensFlare.SetLightDirection(m_sunDirection);
    lensFlare.SetLightColor(Vector3{ 1.0f, 0.98f, 0.95f });
    lensFlare.SetIntensity(1.0f);

    // IBL を Skybox カラーから更新 (環境マップ生成)
    m_renderer3D.GetIBL().UpdateFromSkybox(
        skybox.GetTopColor(), skybox.GetBottomColor(),
        skybox.GetSunDirection(), skybox.GetSunIntensity()
    );

    GX_LOG_INFO("GameFramework: Sky configured (Skybox + Clouds)");
}

// ============================================================================
// MainLoop — メインループ
// ============================================================================

void GameFramework::MainLoop(GameBase& game)
{
    m_app.Run([&](float deltaTime) {
        m_deltaTime = deltaTime;
        m_totalTime += deltaTime;

        m_inputManager.Update();
        m_audioManager.Update(deltaTime);

        game.OnUpdate(deltaTime);
        m_defaultScene.Update(deltaTime);
        game.OnLateUpdate(deltaTime);

        // SceneRenderer でアニメーション更新
        m_sceneRenderer.UpdateAnimations(m_defaultScene, deltaTime);

        // フォントアトラス更新
        m_fontManager.FlushAtlasUpdates();

        RenderFrame(game, deltaTime);
    });
}

// ============================================================================
// RenderFrame — 1フレームの描画
// ============================================================================

void GameFramework::RenderFrame(GameBase& game, float deltaTime)
{
    // フレーム同期
    m_frameIndex = m_swapChain.GetCurrentBackBufferIndex();
    m_commandQueue.GetFence().WaitForValue(m_frameFenceValues[m_frameIndex]);
    m_commandList.Reset(m_frameIndex, nullptr);
    auto* cmdList = m_commandList.Get();

    // ImGui フレーム開始
    m_imguiManager.BeginFrame();

    // --- シャドウパス (CSM) ---
    m_renderer3D.UpdateShadow(m_mainCamera);
    for (uint32_t cascade = 0; cascade < CascadedShadowMap::k_NumCascades; ++cascade)
    {
        m_renderer3D.BeginShadowPass(cmdList, m_frameIndex, cascade);
        m_sceneRenderer.Render(m_defaultScene, m_renderer3D);
        m_renderer3D.EndShadowPass(cascade);
    }

    // --- HDR シーン描画 ---
    auto dsvHandle = m_renderer3D.GetDepthBuffer().GetDSVHandle();
    m_postEffect.BeginScene(cmdList, m_frameIndex, dsvHandle, m_mainCamera);

    // スカイボックス描画 (深度書き込みなし、z=1 の最背面)
    {
        Matrix4x4 viewF = m_mainCamera.GetViewMatrix();
        viewF._41 = 0.0f; viewF._42 = 0.0f; viewF._43 = 0.0f;  // 平行移動を除去
        XMMATRIX viewRotOnly = XMLoadFloat4x4(XM(&viewF));
        Matrix4x4 vp;
        XMStoreFloat4x4(XM(&vp), XMMatrixTranspose(viewRotOnly * ToXMMATRIX(m_mainCamera.GetProjectionMatrix())));
        m_renderer3D.GetSkybox().Draw(cmdList, m_frameIndex, vp);
    }

    // シーン内の LightComponent からライトデータを収集
    {
        gx::Vector<LightData> lights;
        auto lightComps = m_defaultScene.FindComponentsOfType<LightComponent>();
        for (auto* lc : lightComps)
        {
            if (lc->IsEnabled())
                lights.push_back(lc->lightData);
        }
        if (!lights.empty())
            m_renderer3D.SetLights(lights.data(), static_cast<uint32_t>(lights.size()), m_ambientColor);
    }

    // 3D PBR 描画
    m_renderer3D.Begin(cmdList, m_frameIndex, m_mainCamera, m_totalTime);
    m_sceneRenderer.Render(m_defaultScene, m_renderer3D, m_mainCamera);
    game.OnDraw3D();
    m_renderer3D.End();

    // ポストエフェクト: HDR → LDR (VolumetricClouds もこの中で実行される)
    m_postEffect.EndScene();
    {
        auto backBufferRTV = m_swapChain.GetCurrentRTVHandle();
        auto* backBuffer   = m_swapChain.GetCurrentBackBuffer();

        // バックバッファを RT 状態に遷移
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = backBuffer;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        m_postEffect.Resolve(backBufferRTV, m_renderer3D.GetDepthBuffer(),
                             m_mainCamera, deltaTime);

        // --- 2D 描画 ---
        m_spriteBatch.Begin(cmdList, m_frameIndex);
        game.OnDraw2D();
        m_spriteBatch.End();

        // --- デバッグオーバーレイ (ImGui) ---
        {
            DebugOverlayContext dctx{};
            dctx.renderer3D   = &m_renderer3D;
            dctx.postEffect   = &m_postEffect;
            dctx.audioManager = &m_audioManager;
            dctx.quality      = &m_qualitySettings;
            dctx.deltaTime    = deltaTime;
            dctx.fps          = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
            dctx.screenWidth  = m_screenWidth;
            dctx.screenHeight = m_screenHeight;
            m_debugOverlay.Draw(dctx);
        }
        // ImGui 描画前にレンダーターゲット・ビューポート・シザーを明示的に設定
        cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);
        D3D12_VIEWPORT vp = { 0, 0, static_cast<float>(m_screenWidth),
                               static_cast<float>(m_screenHeight), 0.0f, 1.0f };
        D3D12_RECT scissor = { 0, 0, static_cast<LONG>(m_screenWidth),
                                static_cast<LONG>(m_screenHeight) };
        cmdList->RSSetViewports(1, &vp);
        cmdList->RSSetScissorRects(1, &scissor);
        m_imguiManager.EndFrame(cmdList);

        // Present 状態に遷移
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        cmdList->ResourceBarrier(1, &barrier);
    }

    m_commandList.Close();
    ID3D12CommandList* cmdLists[] = { cmdList };
    m_commandQueue.ExecuteCommandLists(cmdLists, 1);
    m_frameFenceValues[m_frameIndex] = m_commandQueue.GetFence().Signal(m_commandQueue.GetQueue());
    m_swapChain.Present(m_vsync);
}

// ============================================================================
// Shutdown — 終了処理
// ============================================================================

void GameFramework::Shutdown(GameBase& game)
{
    game.OnDestroy();

    m_commandQueue.Flush();
    m_imguiManager.Shutdown();
    m_audioManager.Shutdown();
    m_inputManager.Shutdown();
    m_fontManager.Shutdown();
    m_app.Shutdown();

    GX_LOG_INFO("=== GameFramework: Shutdown ===");
}

} // namespace gx
