/// @file App.cpp
/// @brief gx::App と gx::Run() の実装
#include "pch.h"
#include "GX/App.h"
#include "Compat/CompatContext.h"

namespace gx {

Renderer3D& App::GetRenderer3D()
{
    return gx_internal::CompatContext::Instance().renderer3D;
}

Camera3D& App::GetCamera3D()
{
    return gx_internal::CompatContext::Instance().camera;
}

PostEffectPipeline& App::GetPostEffects()
{
    return gx_internal::CompatContext::Instance().postEffect;
}

InputManager& App::GetInput()
{
    return gx_internal::CompatContext::Instance().inputManager;
}

int Run(App& app)
{
    auto& ctx = gx_internal::CompatContext::Instance();

    // OnBoot: 初期化前の設定
    app.OnBoot();

    // Config適用
    Config config = app.GetConfig();
    ctx.windowTitle = config.title;
    ctx.windowMode  = config.windowed;
    ctx.graphWidth  = config.width;
    ctx.graphHeight = config.height;

    // 初期化
    if (!ctx.Initialize())
        return -1;

    ctx.SetVSync(config.vsync);

    // 背景色設定
    ctx.bgColor_r = static_cast<uint32_t>(config.background.r * 255);
    ctx.bgColor_g = static_cast<uint32_t>(config.background.g * 255);
    ctx.bgColor_b = static_cast<uint32_t>(config.background.b * 255);

    // Start: 初期化完了後フック
    app.Start();

    // フレームタイミング
    LARGE_INTEGER freq, prev, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);
    timeBeginPeriod(1);

    const double targetFrameTime = (config.targetFps > 0.0f)
        ? 1.0 / static_cast<double>(config.targetFps) : 0.0;

    // メインループ
    while (ctx.ProcessMessage() == 0)
    {
        // ESCキー終了
        if (config.allowEscapeExit && (GetAsyncKeyState(VK_ESCAPE) & 0x8000))
            break;

        // フレームタイミング
        QueryPerformanceCounter(&now);
        double elapsed = static_cast<double>(now.QuadPart - prev.QuadPart) / freq.QuadPart;

        // FPSリミッター
        if (targetFrameTime > 0.0 && elapsed < targetFrameTime)
        {
            double remaining = targetFrameTime - elapsed;
            if (remaining > 0.002)
                Sleep(static_cast<DWORD>((remaining - 0.001) * 1000.0));
            // スピンウェイト
            do {
                QueryPerformanceCounter(&now);
                elapsed = static_cast<double>(now.QuadPart - prev.QuadPart) / freq.QuadPart;
            } while (elapsed < targetFrameTime);
        }

        float dt = static_cast<float>(elapsed);
        if (dt > config.maxDeltaTime)
            dt = config.maxDeltaTime;
        prev = now;

        // Update
        app.Update(dt);

        // フォントアトラス更新
        ctx.fontManager.FlushAtlasUpdates();

        // BeginFrame（コマンドリスト準備）
        ctx.BeginFrame();

        // Draw
        app.Draw();

        // EndFrame（Present）
        ctx.EndFrame();
    }

    timeEndPeriod(1);

    // Release: クリーンアップフック
    app.Release();

    ctx.Shutdown();
    return 0;
}

} // namespace gx
