/// @file FrameworkApp.cpp
/// @brief 簡易アプリラッパーの実装。初期化から描画までの流れをひとまとめにする。
#include "FrameworkApp.h"
#include "Core/Logger.h"
#include "scenes/BootScene.h"

namespace GXFW
{

bool FrameworkApp::Initialize(const AppConfig& config)
{
    GX::ApplicationDesc desc;
    desc.title = config.title;
    desc.width = config.width;
    desc.height = config.height;
    if (!m_app.Initialize(desc))
        return false;

    m_input.Initialize(m_app.GetWindow());

    if (!m_device.Initialize(config.enableDebug))
        return false;

    auto* device = m_device.GetDevice();
    if (!m_commandQueue.Initialize(device))
        return false;
    if (!m_commandList.Initialize(device))
        return false;

    GX::SwapChainDesc scDesc;
    scDesc.hwnd = m_app.GetWindow().GetHWND();
    scDesc.width = config.width;
    scDesc.height = config.height;
    if (!m_swapChain.Initialize(m_device.GetFactory(), device, m_commandQueue.GetQueue(), scDesc))
        return false;

    if (!m_renderer.Initialize(device, m_commandQueue.GetQueue(), config.width, config.height))
        return false;
    m_renderer.SetShadowEnabled(false);

    if (!m_postFX.Initialize(device, config.width, config.height))
        return false;
    m_postFX.SetTonemapMode(GX::TonemapMode::ACES);

    if (!m_spriteBatch.Initialize(device, m_commandQueue.GetQueue(), config.width, config.height))
        return false;
    if (!m_primitiveBatch.Initialize(device, config.width, config.height))
        return false;
    if (!m_fontManager.Initialize(device, &m_spriteBatch.GetTextureManager()))
        return false;
    m_textRenderer.Initialize(&m_spriteBatch, &m_fontManager);
    m_defaultFont = m_fontManager.CreateFont(L"Consolas", 18);
    if (m_defaultFont < 0)
        m_defaultFont = m_fontManager.CreateFont(L"MS Gothic", 18);

    if (!m_physics.Initialize(1024))
        return false;

    m_camera.SetPerspective(XM_PIDIV4, static_cast<float>(config.width) / config.height, 0.1f, 500.0f);
    m_camera.SetPosition(0.0f, 1.5f, -4.0f);

    // シーン用の共通コンテキスト。必要なサブシステム参照を集める。
    m_context.app = &m_app;
    m_context.graphics = &m_device;
    m_context.commandQueue = &m_commandQueue;
    m_context.commandList = &m_commandList;
    m_context.swapChain = &m_swapChain;
    m_context.renderer = &m_renderer;
    m_context.camera = &m_camera;
    m_context.postFX = &m_postFX;
    m_context.input = &m_input;
    m_context.physics = &m_physics;
    m_context.spriteBatch = &m_spriteBatch;
    m_context.primitiveBatch = &m_primitiveBatch;
    m_context.textRenderer = &m_textRenderer;
    m_context.fontManager = &m_fontManager;
    m_context.defaultFont = m_defaultFont;

    // 既定のブートシーンを設定。未指定時の入口として使う。
    m_sceneManager.SetScene(std::make_unique<BootScene>(), m_context);

    m_app.GetWindow().SetResizeCallback([this](uint32_t w, uint32_t h) { OnResize(w, h); });

    GX_LOG_INFO("GXFramework: Initialized");
    return true;
}

void FrameworkApp::SetScene(std::unique_ptr<Scene> scene)
{
    m_sceneManager.SetScene(std::move(scene), m_context);
}

void FrameworkApp::Run()
{
    m_app.Run([this](float dt) { RenderFrame(dt); });
}

void FrameworkApp::Shutdown()
{
    m_commandQueue.Flush();
    m_physics.Shutdown();
    m_input.Shutdown();
    m_fontManager.Shutdown();
    m_app.Shutdown();
}

void FrameworkApp::RenderFrame(float dt)
{
    m_totalTime += dt;
    m_context.deltaTime = dt;
    m_context.totalTime = m_totalTime;

    m_input.Update();

    if (m_input.CheckHitKey(VK_ESCAPE))
    {
        PostQuitMessage(0);
        return;
    }

    m_sceneManager.Update(m_context, dt);
    m_physics.Step(dt);

    m_frameIndex = m_swapChain.GetCurrentBackBufferIndex();
    m_context.frameIndex = m_frameIndex;

    m_commandQueue.GetFence().WaitForValue(m_fenceValues[m_frameIndex]);
    m_commandList.Reset(m_frameIndex, nullptr);
    auto* cmd = m_commandList.Get();
    m_context.cmd = cmd;

    // 3D 描画（HDR）。HDR ターゲットにシーンを描く。
    auto dsvHandle = m_renderer.GetDepthBuffer().GetDSVHandle();
    m_postFX.BeginScene(cmd, m_frameIndex, dsvHandle, m_camera);

    m_renderer.Begin(cmd, m_frameIndex, m_camera, m_totalTime);
    m_sceneManager.Render(m_context);
    m_renderer.End();

    m_postFX.EndScene();

    // バックバッファへ解決。ポストエフェクト結果を出力する。
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_swapChain.GetCurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmd->ResourceBarrier(1, &barrier);

    auto rtvHandle = m_swapChain.GetCurrentRTVHandle();
    m_postFX.Resolve(rtvHandle, m_renderer.GetDepthBuffer(), m_camera, dt);

    // UI / 2D オーバーレイ。2D描画を前面に重ねる。
    m_context.Begin2D();
    m_sceneManager.RenderUI(m_context);
    m_context.End2D();

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cmd->ResourceBarrier(1, &barrier);

    m_commandList.Close();
    ID3D12CommandList* lists[] = { cmd };
    m_commandQueue.ExecuteCommandLists(lists, 1);
    m_swapChain.Present(false);
    m_fenceValues[m_frameIndex] = m_commandQueue.GetFence().Signal(m_commandQueue.GetQueue());
}

void FrameworkApp::OnResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    m_commandQueue.Flush();
    m_swapChain.Resize(m_device.GetDevice(), width, height);
    m_spriteBatch.SetScreenSize(width, height);
    m_primitiveBatch.SetScreenSize(width, height);
    m_renderer.OnResize(width, height);
    m_postFX.OnResize(m_device.GetDevice(), width, height);
    m_camera.SetPerspective(m_camera.GetFovY(), static_cast<float>(width) / height,
                            m_camera.GetNearZ(), m_camera.GetFarZ());
}

} // namespace GXFW
