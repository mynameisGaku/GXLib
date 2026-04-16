/// @file CompatContext.cpp
/// @brief 簡易API用内部コンテキスト実装
#include "pch.h"
#include "Compat/CompatContext.h"
#include "Compat/CompatTypes.h"
#include "Compat/CompatUtil.h"
#include "Core/Logger.h"
#include "Core/CrashReporter.h"
#include "Audio/IAudioDevice.h"
#include "Graphics/RenderProfile.h"
#include "Graphics/QualitySettings.h"

#include <thread>
#include <atomic>
#include <chrono>

namespace gx_internal
{

CompatContext& CompatContext::Instance()
{
    static CompatContext instance;
    return instance;
}

// ============================================================================
// Initialize — 3フェーズ初期化 + ロード画面
// ============================================================================

bool CompatContext::Initialize()
{
    // --- Phase 1: コア初期化 (同期) ---
    if (!InitCore())
        return false;

    // --- ロード画面用 CommandList 初期化 ---
    if (!m_loadingCmdList.Initialize(device))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize loading CommandList");
        return false;
    }

    // --- ロード画面初期化 ---
    gx::LoadingScreen loadingScreen;
    if (!loadingScreen.Initialize(device, commandQueue.GetQueue(),
                                   screenWidth, screenHeight))
    {
        GX_LOG_WARN("CompatContext: LoadingScreen init failed, falling back to sync init");
        // フォールバック: ロード画面なしで同期初期化
        std::atomic<float> dummyProgress{0.0f};
        std::atomic<bool> dummyFailed{false};
        InitHeavy(dummyProgress, dummyFailed);
        if (dummyFailed.load()) return false;
        if (!FinalizeInit()) return false;
        return true;
    }

    // フォントアトラスアップロードを確実に完了
    commandQueue.Flush();

    // --- Phase 2: バックグラウンド初期化 + ロード画面描画 ---
    std::atomic<float> progress{0.0f};
    std::atomic<bool>  failed{false};
    std::atomic<bool>  done{false};

    std::thread worker([this, &progress, &failed, &done]() {
        InitHeavy(progress, failed);
        done.store(true);
    });

    // メインスレッド: ロード画面ループ
    while (!done.load())
    {
        // メッセージポンプ（ウィンドウが応答なしにならないように）
        app.GetWindow().ProcessMessages();

        uint32_t fi = swapChain.GetCurrentBackBufferIndex();
        m_loadingCmdList.Reset(fi);
        auto* cmd = m_loadingCmdList.Get();

        loadingScreen.Render(cmd,
                             swapChain.GetCurrentRTVHandle(),
                             swapChain.GetCurrentBackBuffer(),
                             fi,
                             progress.load(),
                             L"Initializing");

        m_loadingCmdList.Close();
        ID3D12CommandList* lists[] = { cmd };
        commandQueue.ExecuteCommandLists(lists, 1);
        swapChain.Present(true);
        commandQueue.Flush();
    }

    worker.join();

    if (failed.load())
    {
        GX_LOG_ERROR("CompatContext: Background initialization failed");
        loadingScreen.Shutdown();
        return false;
    }

    // --- ロード画面1フレーム描画ヘルパー ---
    auto renderLoadingFrame = [&](float prog, float fadeAlpha) {
        app.GetWindow().ProcessMessages();
        uint32_t fi = swapChain.GetCurrentBackBufferIndex();
        m_loadingCmdList.Reset(fi);
        auto* cmd = m_loadingCmdList.Get();
        loadingScreen.Render(cmd,
                             swapChain.GetCurrentRTVHandle(),
                             swapChain.GetCurrentBackBuffer(),
                             fi, prog, L"Initializing", fadeAlpha);
        m_loadingCmdList.Close();
        ID3D12CommandList* lists[] = { cmd };
        commandQueue.ExecuteCommandLists(lists, 1);
        swapChain.Present(true);
        commandQueue.Flush();
    };

    // --- Phase 3: ファイナライズ ---
    // FinalizeInit 前後にフレームを挟み、画面フリーズを防ぐ
    renderLoadingFrame(1.0f, 1.0f);
    if (!FinalizeInit())
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
                fadeAlpha = 1.0f; // ホールド: まだ100%表示
            else
                fadeAlpha = 1.0f - (elapsed - k_HoldDuration) / k_FadeDuration;

            renderLoadingFrame(1.0f, fadeAlpha);
        }

        // 両方のバックバッファを確実に黒にする（パッと変わるのを防止）
        renderLoadingFrame(1.0f, 0.0f);
        renderLoadingFrame(1.0f, 0.0f);
    }

    loadingScreen.Shutdown();

    // ゲーム描画のフェードイン開始
    fadeInAlpha = 1.0f;

    GX_LOG_INFO("CompatContext: Initialized successfully");
    return true;
}

// ============================================================================
// InitCore — Phase 1: 最低限のサブシステム (同期)
// ============================================================================

bool CompatContext::InitCore()
{
    // CrashReporter 初期化（Application より前に行う）
    if (!gx::CrashReporter::Instance().IsInitialized())
    {
        gx::CrashReporterConfig crashCfg;
        gx::CrashReporter::Instance().Initialize(crashCfg);
        gx::CrashReporter::Instance().InstallHandler();
    }

    GX_LOG_INFO("CompatContext: Initializing (Phase 1: Core)...");

    // アプリケーション初期化
    gx::ApplicationDesc appDesc;
    appDesc.title  = gx_internal::ToWString(windowTitle.c_str());
    appDesc.width  = static_cast<uint32_t>(graphWidth);
    appDesc.height = static_cast<uint32_t>(graphHeight);
    if (!app.Initialize(appDesc))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize Application");
        return false;
    }

    // ウィンドウ作成後に CrashReporter にハンドルを設定
    gx::CrashReporter::Instance().SetWindowHandle(app.GetWindow().GetHWND());

    screenWidth  = static_cast<uint32_t>(graphWidth);
    screenHeight = static_cast<uint32_t>(graphHeight);

    // グラフィックスデバイス
    if (!graphicsDevice.Initialize())
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize GraphicsDevice");
        return false;
    }
    device = graphicsDevice.GetDevice();

    // コマンドキュー
    if (!commandQueue.Initialize(device))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize CommandQueue");
        return false;
    }

    // コマンドリスト
    if (!commandList.Initialize(device))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize CommandList");
        return false;
    }

    // スワップチェーン
    gx::SwapChainDesc scDesc;
    scDesc.hwnd   = app.GetWindow().GetHWND();
    scDesc.width  = screenWidth;
    scDesc.height = screenHeight;
    if (!swapChain.Initialize(graphicsDevice.GetFactory(), device,
                              commandQueue.GetQueue(), scDesc))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize SwapChain");
        return false;
    }

    return true;
}

// ============================================================================
// InitHeavy — Phase 2: 重いサブシステム (バックグラウンドスレッド)
// ============================================================================

void CompatContext::InitHeavy(std::atomic<float>& progress, std::atomic<bool>& failed)
{
    GX_LOG_INFO("CompatContext: Initializing (Phase 2: Heavy subsystems)...");

    // SpriteBatch
    if (!spriteBatch.Initialize(device, commandQueue.GetQueue(),
                                screenWidth, screenHeight))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize SpriteBatch");
        failed.store(true);
        return;
    }
    progress.store(0.10f);

    // PrimitiveBatch
    if (!primBatch.Initialize(device, screenWidth, screenHeight))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize PrimitiveBatch");
        failed.store(true);
        return;
    }
    progress.store(0.15f);

    // FontManager
    if (!fontManager.Initialize(device, &spriteBatch.GetTextureManager()))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize FontManager");
        failed.store(true);
        return;
    }
    textRenderer.Initialize(&spriteBatch, &fontManager);
    defaultFontHandle = fontManager.CreateFont(L"MS Gothic", 16);
    progress.store(0.20f);

    // InputManager
    inputManager.Initialize(app.GetWindow());
    progress.store(0.25f);

    // AudioManager (non-fatal)
    if (!audioManager.Initialize())
    {
        GX_LOG_ERROR("CompatContext: AudioManager initialization failed (non-fatal)");
    }
    progress.store(0.30f);

    // Renderer3D (PSO多数 — 最重)
    if (!renderer3D.Initialize(device, commandQueue.GetQueue(),
                                screenWidth, screenHeight))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize Renderer3D");
        failed.store(true);
        return;
    }
    progress.store(0.60f);

    // PostEffectPipeline (PSO多数 — 重い)
    postEffect.SetGraphicsDevice(&graphicsDevice);
    if (!postEffect.Initialize(device, screenWidth, screenHeight))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize PostEffectPipeline");
        failed.store(true);
        return;
    }
    progress.store(0.90f);

    progress.store(1.0f);
}

// ============================================================================
// FinalizeInit — Phase 3: 初期化後のファイナライズ (同期)
// ============================================================================

bool CompatContext::FinalizeInit()
{
    GX_LOG_INFO("CompatContext: Initializing (Phase 3: Finalize)...");

    // カメラ初期設定
    float aspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 1000.0f);

    // 描画設定を適用
    auto defaultProfile = gx::RenderProfile::CreateDefault();
    defaultProfile.Apply(postEffect, &renderer3D);

    // GPU能力に応じた品質自動調整
    auto detectedLevel = gx::QualitySettings::AutoDetect(graphicsDevice);
    qualitySettings.SetQualityLevel(detectedLevel);
    qualitySettings.Apply(postEffect);
    GX_LOG_INFO("CompatContext: Auto quality=%d, VRAM=%llu MB, RT=%s",
                static_cast<int>(detectedLevel),
                static_cast<unsigned long long>(graphicsDevice.GetDedicatedVRAM() / (1024 * 1024)),
                graphicsDevice.SupportsRaytracing() ? "yes" : "no");

    // ImGui 初期化（デバッグオーバーレイ用）
    imguiManager.Initialize(device, commandQueue.GetQueue(), app.GetWindow());

    // ServiceLocator にサービスを登録
    auto& sl = gx::ServiceLocator::Instance();
    sl.Register<gx::IAudioDevice>(
        std::shared_ptr<gx::IAudioDevice>(&audioManager.GetDevice(), [](gx::IAudioDevice*) {}));

    return true;
}

// ============================================================================
// Shutdown
// ============================================================================

void CompatContext::Shutdown()
{
    GX_LOG_INFO("CompatContext: Shutting down...");

    // ServiceLocator のサービス登録を解除（サブシステム破棄前に実施）
    gx::ServiceLocator::Instance().Clear();

    commandQueue.Flush();

    imguiManager.Shutdown();
    // FontManager を AudioManager より先に終了する理由:
    // FontManager は WIC/DirectWrite/D2D の ComPtr を持つが、
    // AudioDevice::Shutdown が CoUninitialize() を呼ぶと wincodecs.dll の
    // 参照カウントが減少し、DLL がアンロードされると ComPtr::Reset 時に
    // vtable 解決不能でアクセス違反になる (WIC は COM で実装されているため)。
    // FontManager を先に終了すれば COM がまだ有効な状態で安全に Release される。
    fontManager.Shutdown();
    audioManager.Shutdown();
    app.Shutdown();

    // CrashReporter シャットダウン（最後に行う）
    if (gx::CrashReporter::Instance().IsInitialized())
    {
        gx::CrashReporter::Instance().Shutdown();
    }

    GX_LOG_INFO("CompatContext: Shutdown complete");
}

// ============================================================================
// ProcessMessage
// ============================================================================

int CompatContext::ProcessMessage()
{
    if (!app.GetWindow().ProcessMessages())
        return -1;

    inputManager.Update();
    audioManager.Update(app.GetTimer().GetDeltaTime());
    return 0;
}

// SpriteBatchとPrimitiveBatchは排他的（同時にBeginできない）。
// Ensure系メソッドが必要なバッチに自動切替する。
void CompatContext::EnsureSpriteBatch()
{
    if (!frameActive) return;

    if (activeBatch == ActiveBatch::Primitive)
    {
        primBatch.End();
    }
    if (activeBatch != ActiveBatch::Sprite)
    {
        spriteBatch.Begin(cmdList, frameIndex);
        activeBatch = ActiveBatch::Sprite;
    }
}

void CompatContext::EnsurePrimitiveBatch()
{
    if (!frameActive) return;

    if (activeBatch == ActiveBatch::Sprite)
    {
        spriteBatch.End();
    }
    if (activeBatch != ActiveBatch::Primitive)
    {
        primBatch.Begin(cmdList, frameIndex);
        activeBatch = ActiveBatch::Primitive;
    }
}

void CompatContext::FlushAll()
{
    if (activeBatch == ActiveBatch::Sprite)
    {
        spriteBatch.End();
    }
    else if (activeBatch == ActiveBatch::Primitive)
    {
        primBatch.End();
    }
    activeBatch = ActiveBatch::None;
}

// DxLibのClearDrawScreen相当。コマンドリストのリセットからレンダーターゲット設定まで。
void CompatContext::BeginFrame()
{
    if (frameActive) return;

    frameIndex = swapChain.GetCurrentBackBufferIndex();
    commandList.Reset(frameIndex);
    cmdList = commandList.Get();

    // バックバッファをレンダーターゲット状態に遷移
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = swapChain.GetCurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // レンダーターゲット設定（クリアはClearDrawScreen側で行う）
    auto rtvHandle = swapChain.GetCurrentRTVHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // ビューポートとシザー
    D3D12_VIEWPORT viewport = {};
    viewport.Width    = static_cast<float>(screenWidth);
    viewport.Height   = static_cast<float>(screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = { 0, 0,
        static_cast<LONG>(screenWidth),
        static_cast<LONG>(screenHeight) };
    cmdList->RSSetScissorRects(1, &scissor);

    frameActive = true;

    // ImGui フレーム開始（overlay 非表示時もゼロコスト）
    imguiManager.BeginFrame();
}

// DxLibのScreenFlip相当。バッチのフラッシュ→バリア遷移→コマンド実行→Present。
void CompatContext::EndFrame()
{
    if (!frameActive) return;

    // デバッグオーバーレイ（ユーザー描画の上に最後に重ねる）
    FlushAll();
    {
        gx::DebugOverlayContext dctx{};
        dctx.renderer3D   = &renderer3D;
        dctx.postEffect   = &postEffect;
        dctx.audioManager = &audioManager;
        dctx.quality      = &qualitySettings;
        dctx.deltaTime    = app.GetTimer().GetDeltaTime();
        dctx.fps          = app.GetTimer().GetFPS();
        dctx.screenWidth  = screenWidth;
        dctx.screenHeight = screenHeight;
        debugOverlay.Draw(dctx);
    }
    imguiManager.EndFrame(cmdList);

    // --- フェードイン: 黒オーバーレイを減衰させる ---
    if (fadeInAlpha > 0.0f)
    {
        uint32_t a = static_cast<uint32_t>(fadeInAlpha * 255.0f);
        if (a > 0)
        {
            primBatch.Begin(cmdList, frameIndex);
            primBatch.DrawBox(0.0f, 0.0f,
                              static_cast<float>(screenWidth),
                              static_cast<float>(screenHeight),
                              (a << 24), true);
            primBatch.End();
        }
        constexpr float k_FadeInDuration = 0.6f;
        fadeInAlpha -= app.GetTimer().GetDeltaTime() / k_FadeInDuration;
        if (fadeInAlpha < 0.0f) fadeInAlpha = 0.0f;
    }

    // バックバッファをPresent状態に遷移
    D3D12_RESOURCE_BARRIER presentBarrier = {};
    presentBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    presentBarrier.Transition.pResource   = swapChain.GetCurrentBackBuffer();
    presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    presentBarrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &presentBarrier);

    commandList.Close();

    // コマンド実行
    ID3D12CommandList* lists[] = { cmdList };
    commandQueue.ExecuteCommandLists(lists, 1);

    // 画面表示
    swapChain.Present(vsyncEnabled);

    // GPU待機（フレーム同期）
    commandQueue.Flush();

    frameActive = false;

    // タイマー更新
    app.GetTimer().Tick();
}

int CompatContext::AllocateModelHandle()
{
    if (!modelFreeHandles.empty())
    {
        int h = modelFreeHandles.back();
        modelFreeHandles.pop_back();
        return h;
    }
    if (modelNextHandle >= k_MaxModels)
    {
        GX_LOG_ERROR("CompatContext: model handle limit reached (max: %d)", k_MaxModels);
        return -1;
    }
    int h = modelNextHandle++;
    if (static_cast<size_t>(h) >= models.size())
        models.resize(h + 1);
    return h;
}

} // namespace gx_internal
