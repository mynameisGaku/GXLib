/// @file CompatContext.cpp
/// @brief 簡易API用内部コンテキスト実装
#include "pch.h"
#include "Compat/CompatContext.h"
#include "Compat/CompatTypes.h"
#include "Core/Logger.h"

namespace GX_Internal
{

CompatContext& CompatContext::Instance()
{
    static CompatContext instance;
    return instance;
}

bool CompatContext::Initialize()
{
    GX_LOG_INFO("CompatContext: Initializing...");

    // アプリケーション初期化
    GX::ApplicationDesc appDesc;
    appDesc.title  = windowTitle;
    appDesc.width  = static_cast<uint32_t>(graphWidth);
    appDesc.height = static_cast<uint32_t>(graphHeight);
    if (!app.Initialize(appDesc))
    {
        GX_LOG_ERROR("CompatContext: Failed to initialize Application");
        return false;
    }

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
    GX::SwapChainDesc scDesc;
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

    GX_LOG_INFO("CompatContext: Initialized successfully");
    return true;
}

void CompatContext::Shutdown()
{
    GX_LOG_INFO("CompatContext: Shutting down...");

    commandQueue.Flush();

    audioManager.Shutdown();
    fontManager.Shutdown();
    app.Shutdown();

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

    // レンダーターゲット設定＆クリア
    auto rtvHandle = swapChain.GetCurrentRTVHandle();
    float clearColor[4] = {
        bgColor_r / 255.0f,
        bgColor_g / 255.0f,
        bgColor_b / 255.0f,
        1.0f
    };
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
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

} // namespace GX_Internal
