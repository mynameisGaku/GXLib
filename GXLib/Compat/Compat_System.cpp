/// @file Compat_System.cpp
/// @brief 簡易API システム関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "Core/Logger.h"

using Ctx = GX_Internal::CompatContext;

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

int SetMainWindowText(const TCHAR* title)
{
    auto& ctx = Ctx::Instance();
#ifdef UNICODE
    ctx.windowTitle = title;
#else
    int len = MultiByteToWideChar(CP_ACP, 0, title, -1, nullptr, 0);
    ctx.windowTitle.resize(len - 1);
    MultiByteToWideChar(CP_ACP, 0, title, -1, ctx.windowTitle.data(), len);
#endif
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

unsigned int GetColor(int r, int g, int b)
{
    return 0xFF000000u
        | (static_cast<unsigned int>(r & 0xFF) << 16)
        | (static_cast<unsigned int>(g & 0xFF) << 8)
        | (static_cast<unsigned int>(b & 0xFF));
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

int ClearDrawScreen()
{
    auto& ctx = Ctx::Instance();
    ctx.fontManager.FlushAtlasUpdates();
    ctx.BeginFrame();
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
