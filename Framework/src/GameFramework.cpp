/// @file GameFramework.cpp
/// @brief GameFramework の実装
///
/// Sandbox と同等の完全なレンダリングパイプラインを内包する。
/// ユーザーは GameBase を継承するだけで、エンジン初期化から
/// シャドウ・PBR・ポストエフェクト・大気散乱・雲まで全てが動作する。

#include "GameFramework.h"
#include "Graphics/3D/CascadedShadowMap.h"
#include "Graphics/3D/Fog.h"

#include <thread>
#include <atomic>
#include <chrono>

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
// Initialize — 3フェーズ初期化 + ロード画面
// ============================================================================

bool GameFramework::Initialize(const Config& config)
{
    // --- Phase 1: コア初期化 (同期) ---
    if (!InitializeCore(config))
        return false;

    // --- ロード画面用 CommandList 初期化 ---
    if (!m_loadingCmdList.Initialize(m_device))
    {
        GX_LOG_ERROR("GameFramework: Failed to initialize loading CommandList");
        return false;
    }

    // --- ロード画面初期化 ---
    LoadingScreen loadingScreen;
    if (!loadingScreen.Initialize(m_device, m_commandQueue.GetQueue(),
                                   m_screenWidth, m_screenHeight))
    {
        GX_LOG_WARN("GameFramework: LoadingScreen init failed, falling back to sync init");
        std::atomic<float> dummyProgress{0.0f};
        std::atomic<bool> dummyFailed{false};
        InitializeHeavy(dummyProgress, dummyFailed);
        if (dummyFailed.load()) return false;
        if (!FinalizeInit(config)) return false;
        return true;
    }

    // フォントアトラスアップロードを確実に完了
    m_commandQueue.Flush();

    // --- Phase 2: バックグラウンド初期化 + ロード画面描画 ---
    std::atomic<float> progress{0.0f};
    std::atomic<bool>  failed{false};
    std::atomic<bool>  done{false};

    std::thread worker([this, &progress, &failed, &done]() {
        InitializeHeavy(progress, failed);
        done.store(true);
    });

    // メインスレッド: ロード画面ループ
    while (!done.load())
    {
        m_app.GetWindow().ProcessMessages();

        uint32_t fi = m_swapChain.GetCurrentBackBufferIndex();
        m_loadingCmdList.Reset(fi);
        auto* cmd = m_loadingCmdList.Get();

        loadingScreen.Render(cmd,
                             m_swapChain.GetCurrentRTVHandle(),
                             m_swapChain.GetCurrentBackBuffer(),
                             fi,
                             progress.load(),
                             L"Initializing");

        m_loadingCmdList.Close();
        ID3D12CommandList* lists[] = { cmd };
        m_commandQueue.ExecuteCommandLists(lists, 1);
        m_swapChain.Present(true);
        m_commandQueue.Flush();
    }

    worker.join();

    if (failed.load())
    {
        GX_LOG_ERROR("GameFramework: Background initialization failed");
        loadingScreen.Shutdown();
        return false;
    }

    // --- ロード画面1フレーム描画ヘルパー ---
    auto renderLoadingFrame = [&](float prog, float fadeAlpha) {
        m_app.GetWindow().ProcessMessages();
        uint32_t fi = m_swapChain.GetCurrentBackBufferIndex();
        m_loadingCmdList.Reset(fi);
        auto* cmd = m_loadingCmdList.Get();
        loadingScreen.Render(cmd,
                             m_swapChain.GetCurrentRTVHandle(),
                             m_swapChain.GetCurrentBackBuffer(),
                             fi, prog, L"Initializing", fadeAlpha);
        m_loadingCmdList.Close();
        ID3D12CommandList* lists[] = { cmd };
        m_commandQueue.ExecuteCommandLists(lists, 1);
        m_swapChain.Present(true);
        m_commandQueue.Flush();
    };

    // --- Phase 3: ファイナライズ ---
    renderLoadingFrame(1.0f, 1.0f);
    if (!FinalizeInit(config))
    {
        loadingScreen.Shutdown();
        return false;
    }
    // FinalizeInit による timer ジャンプを吸収
    renderLoadingFrame(1.0f, 1.0f);

    // --- 100%ホールド (0.3秒) → フェードアウト (0.6秒) → 黒フレーム ---
    {
        auto transitionStart = std::chrono::high_resolution_clock::now();
        constexpr float k_HoldDuration = 0.3f;
        constexpr float k_FadeDuration = 0.6f;
        constexpr float k_TotalDuration = k_HoldDuration + k_FadeDuration;

        while (true)
        {
            auto now = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(now - transitionStart).count();
            if (elapsed >= k_TotalDuration) break;

            float fadeAlpha;
            if (elapsed < k_HoldDuration)
                fadeAlpha = 1.0f;
            else
                fadeAlpha = 1.0f - (elapsed - k_HoldDuration) / k_FadeDuration;

            renderLoadingFrame(1.0f, fadeAlpha);
        }

        // 両方のバックバッファを確実に黒にする
        renderLoadingFrame(1.0f, 0.0f);
        renderLoadingFrame(1.0f, 0.0f);
    }

    loadingScreen.Shutdown();

    // ゲーム描画のフェードイン開始
    m_fadeInAlpha = 1.0f;

    GX_LOG_INFO("=== GameFramework: Initialized ===");
    return true;
}

// ============================================================================
// InitializeCore — Phase 1: コアサブシステム (同期)
// ============================================================================

bool GameFramework::InitializeCore(const Config& config)
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

    // Graphics pipeline (CommandQueue, CommandList, SwapChain)
    if (!InitializeGraphics())
        return false;

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

// ============================================================================
// InitializeHeavy — Phase 2: 重いサブシステム (バックグラウンド)
// ============================================================================

void GameFramework::InitializeHeavy(std::atomic<float>& progress, std::atomic<bool>& failed)
{
    GX_LOG_INFO("GameFramework: Initializing (Phase 2: Heavy subsystems)...");

    auto* queue = m_commandQueue.GetQueue();

    // SpriteBatch
    if (!m_spriteBatch.Initialize(m_device, queue, m_screenWidth, m_screenHeight))
    { failed.store(true); return; }
    progress.store(0.10f);

    // PrimitiveBatch
    if (!m_primitiveBatch.Initialize(m_device, m_screenWidth, m_screenHeight))
    { failed.store(true); return; }
    progress.store(0.15f);

    // FontManager + TextRenderer
    if (!m_fontManager.Initialize(m_device, &m_spriteBatch.GetTextureManager()))
    { failed.store(true); return; }
    m_textRenderer.Initialize(&m_spriteBatch, &m_fontManager);
    progress.store(0.20f);

    // AudioManager (non-fatal)
    m_audioManager.Initialize();
    progress.store(0.30f);

    // Renderer3D (PSO多数 — 最重)
    if (!m_renderer3D.Initialize(m_device, queue, m_screenWidth, m_screenHeight))
    { failed.store(true); return; }
    progress.store(0.60f);

    // PostEffectPipeline (PSO多数)
    m_postEffect.SetGraphicsDevice(&m_graphicsDevice);
    if (!m_postEffect.Initialize(m_device, m_screenWidth, m_screenHeight))
    { failed.store(true); return; }
    progress.store(0.90f);

    // デフォルトフォント
    m_defaultFont = m_fontManager.CreateFont(L"Meiryo", 20);
    if (m_defaultFont < 0)
        m_defaultFont = m_fontManager.CreateFont(L"MS Gothic", 20);

    progress.store(1.0f);
}

// ============================================================================
// FinalizeInit — Phase 3: ファイナライズ (同期)
// ============================================================================

bool GameFramework::FinalizeInit(const Config& config)
{
    GX_LOG_INFO("GameFramework: Initializing (Phase 3: Finalize)...");

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

    // ImGui 初期化
    m_imguiManager.Initialize(m_device, m_commandQueue.GetQueue(), m_app.GetWindow());

    // Input（ImGui の後に初期化 — コールバック登録順序が重要）
    m_inputManager.Initialize(m_app.GetWindow());

    return true;
}

// ============================================================================
// InitializeRenderers — 後方互換用 (未使用、3フェーズ移行済み)
// ============================================================================

bool GameFramework::InitializeRenderers()
{
    // InitializeHeavy + FinalizeInit に分割済み
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
    m_ambientColor = Vector3{ 0.25f, 0.28f, 0.35f };

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
        Vector3{ 0.37f, 0.35f, 0.34f }    // 地平: 暗灰色 (Unityライク)
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
    clouds.SetMSOctaves(6);
    clouds.SetMSAttenuation(0.3f);
    clouds.SetMSContribution(0.7f);
    clouds.SetMSEccentricity(0.5f);
    clouds.SetPowderAmount(0.8f);
    clouds.SetAmbientBottom(0.20f);
    clouds.SetAmbientTop(0.55f);
    clouds.SetAtmosphereDensity(0.00003f);

    // --- SkyAtmosphere (物理ベース大気散乱) ---
    auto& sky = m_postEffect.GetSkyAtmosphere();
    sky.SetEnabled(true);
    sky.SetSunDirection(m_sunDirection);
    sky.SetSunColor(Vector3{ 1.0f, 0.98f, 0.95f });
    sky.SetSunIntensity(20.0f);

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

    // シーン内の LightComponent からライトデータを収集（シャドウパスの前に必要）
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

    // --- シャドウパス (CSM) ---
    m_renderer3D.UpdateShadow(m_mainCamera);
    for (uint32_t cascade = 0; cascade < CascadedShadowMap::k_NumCascades; ++cascade)
    {
        m_renderer3D.BeginShadowPass(cmdList, m_frameIndex, cascade);
        m_sceneRenderer.Render(m_defaultScene, m_renderer3D);
        game.OnDraw3D();
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

        // --- フェードイン: 黒オーバーレイを減衰させる ---
        if (m_fadeInAlpha > 0.0f)
        {
            uint32_t a = static_cast<uint32_t>(m_fadeInAlpha * 255.0f);
            if (a > 0)
            {
                m_primitiveBatch.Begin(cmdList, m_frameIndex);
                m_primitiveBatch.DrawBox(0.0f, 0.0f,
                                          static_cast<float>(m_screenWidth),
                                          static_cast<float>(m_screenHeight),
                                          (a << 24), true);
                m_primitiveBatch.End();
            }
            constexpr float k_FadeInDuration = 0.6f;
            m_fadeInAlpha -= deltaTime / k_FadeInDuration;
            if (m_fadeInAlpha < 0.0f) m_fadeInAlpha = 0.0f;
        }

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
