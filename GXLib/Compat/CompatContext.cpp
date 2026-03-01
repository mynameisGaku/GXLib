/// @file CompatContext.cpp
/// @brief 簡易API用内部コンテキスト実装
#include "pch.h"
#include "Compat/CompatContext.h"
#include "Compat/CompatTypes.h"
#include "Compat/CompatUtil.h"
#include "Core/Logger.h"
#include "Core/CrashReporter.h"
#include "Audio/IAudioDevice.h"

namespace gx_internal
{

CompatContext& CompatContext::Instance()
{
    static CompatContext instance;
    return instance;
}

bool CompatContext::Initialize()
{
    // CrashReporter 初期化（Application より前に行う）
    if (!gx::CrashReporter::Instance().IsInitialized())
    {
        gx::CrashReporterConfig crashCfg;
        gx::CrashReporter::Instance().Initialize(crashCfg);
        gx::CrashReporter::Instance().InstallHandler();
    }

    GX_LOG_INFO("CompatContext: Initializing...");

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

    // SpriteBatch（2Dスプライト描画）
    if (!spriteBatch.Initialize(device, commandQueue.GetQueue(),
                                screenWidth, screenHeight))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize SpriteBatch");
        return false;
    }

    // PrimitiveBatch（2Dプリミティブ描画）
    if (!primBatch.Initialize(device, screenWidth, screenHeight))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize PrimitiveBatch");
        return false;
    }

    // FontManager（フォント管理）
    if (!fontManager.Initialize(device, &spriteBatch.GetTextureManager()))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize FontManager");
        return false;
    }

    // TextRenderer（テキスト描画）
    textRenderer.Initialize(&spriteBatch, &fontManager);

    // デフォルトフォント作成 (MS Gothic 16pt)
    defaultFontHandle = fontManager.CreateFont(L"MS Gothic", 16);

    // InputManager（入力管理）
    inputManager.Initialize(app.GetWindow());

    // AudioManager（音声管理）
    if (!audioManager.Initialize())
    {
        GX_LOG_ERROR("CompatContext: AudioManager initialization failed (non-fatal)");
    }

    // Renderer3D
    if (!renderer3D.Initialize(device, commandQueue.GetQueue(),
                                screenWidth, screenHeight))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize Renderer3D");
        return false;
    }

    // PostEffectPipeline
    if (!postEffect.Initialize(device, screenWidth, screenHeight))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize PostEffectPipeline");
        return false;
    }

    // カメラ初期設定
    float aspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 1000.0f);

    // ServiceLocator にサービスを登録
    // 各サブシステムの所有権は CompatContext が持つため、カスタムデリータで非所有参照を登録
    auto& sl = gx::ServiceLocator::Instance();
    sl.Register<gx::IAudioDevice>(
        std::shared_ptr<gx::IAudioDevice>(&audioManager.GetDevice(), [](gx::IAudioDevice*) {}));

    GX_LOG_INFO("CompatContext: Initialized successfully");
    return true;
}

void CompatContext::Shutdown()
{
    GX_LOG_INFO("CompatContext: Shutting down...");

    // ServiceLocator のサービス登録を解除（サブシステム破棄前に実施）
    gx::ServiceLocator::Instance().Clear();

    commandQueue.Flush();

    audioManager.Shutdown();
    fontManager.Shutdown();
    app.Shutdown();

    // CrashReporter シャットダウン（最後に行う）
    if (gx::CrashReporter::Instance().IsInitialized())
    {
        gx::CrashReporter::Instance().Shutdown();
    }

    GX_LOG_INFO("CompatContext: Shutdown complete");
}

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
}

// DxLibのScreenFlip相当。バッチのフラッシュ→バリア遷移→コマンド実行→Present。
void CompatContext::EndFrame()
{
    if (!frameActive) return;

    FlushAll();

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
