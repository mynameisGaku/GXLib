/// @file Compat_System.cpp
/// @brief 簡易API システム関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "Core/Logger.h"

using Ctx = gx_internal::CompatContext;

int GX_Init()
{
    return Ctx::Instance().Initialize() ? 0 : -1;
}

int GX_End()
{
    Ctx::Instance().Shutdown();
    return 0;
}

int ProcessMessage()
{
    return Ctx::Instance().ProcessMessage();
}

int SetMainWindowText(const char* title)
{
    Ctx::Instance().windowTitle = title ? title : "";
    return 0;
}

int ChangeWindowMode(int flag)
{
    Ctx::Instance().windowMode = (flag != 0);
    return 0;
}

int SetGraphMode(int width, int height, int colorBitNum)
{
    auto& ctx = Ctx::Instance();
    ctx.graphWidth   = width;
    ctx.graphHeight  = height;
    ctx.graphColorBit = colorBitNum;
    return 0;
}

// 0xFFRRGGBB形式のカラー値を生成。アルファは常に0xFF（不透明）。
unsigned int GetColor(int r, int g, int b)
{
    return 0xFF000000u
        | (static_cast<unsigned int>(r & 0xFF) << 16)
        | (static_cast<unsigned int>(g & 0xFF) << 8)
        | (static_cast<unsigned int>(b & 0xFF));
}

float GetDeltaTime()
{
    return Ctx::Instance().app.GetTimer().GetDeltaTime();
}

float GetFPS()
{
    return Ctx::Instance().app.GetTimer().GetFPS();
}

int GetNowCount()
{
    return static_cast<int>(GetTickCount64());
}

int SetDrawScreen(int screen)
{
    Ctx::Instance().drawScreen = screen;
    return 0;
}

// フォントアトラスの遅延GPU転送をここで行ってからフレームを開始する
int ClearDrawScreen()
{
    auto& ctx = Ctx::Instance();
    ctx.fontManager.FlushAtlasUpdates();
    ctx.BeginFrame();

    // バックバッファのクリア（BeginFrameではクリアしないのでここで行う）
    float clearColor[4] = {
        ctx.bgColor_r / 255.0f,
        ctx.bgColor_g / 255.0f,
        ctx.bgColor_b / 255.0f,
        1.0f
    };
    ctx.cmdList->ClearRenderTargetView(
        ctx.swapChain.GetCurrentRTVHandle(), clearColor, 0, nullptr);
    return 0;
}

int ScreenFlip()
{
    Ctx::Instance().EndFrame();
    return 0;
}

int SetBackgroundColor(int r, int g, int b)
{
    auto& ctx = Ctx::Instance();
    ctx.bgColor_r = static_cast<uint32_t>(r & 0xFF);
    ctx.bgColor_g = static_cast<uint32_t>(g & 0xFF);
    ctx.bgColor_b = static_cast<uint32_t>(b & 0xFF);
    return 0;
}

// ============================================================================
// ポストエフェクト制御
// ============================================================================
void SetPostFXMask(gx::PostFXFlag mask)
{
    Ctx::Instance().postFXMask = mask;
}

void SetPostFXEnabled(bool enabled)
{
    Ctx::Instance().postFXMask = enabled ? gx::PostFXFlag::All : gx::PostFXFlag::None;
}

// ============================================================================
// 中級者向け — 描画パイプラインアクセス
// ============================================================================
gx::Renderer3D&         GetRenderer3D()     { return Ctx::Instance().renderer3D; }
gx::Camera3D&           GetCamera3D()       { return Ctx::Instance().camera; }
gx::PostEffectPipeline& GetPostEffects()    { return Ctx::Instance().postEffect; }
gx::InputManager&       GetInputManager()   { return Ctx::Instance().inputManager; }
