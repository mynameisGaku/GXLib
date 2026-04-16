/// @file Compat_Font.cpp
/// @brief 簡易API フォント・テキスト描画関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "Compat/CompatUtil.h"

using Ctx = gx_internal::CompatContext;

namespace gx {

// ============================================================================
// デフォルトフォントでの描画
// ============================================================================
int DrawString(int x, int y, const char* str, unsigned int color)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ctx.textRenderer.DrawString(
        ctx.defaultFontHandle,
        static_cast<float>(x), static_cast<float>(y),
        gx_internal::ToWString(str), color);
    return 0;
}

int DrawFormatString(int x, int y, unsigned int color, const char* format, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, format);
    vsprintf_s(buf, format, args);
    va_end(args);
    return DrawString(x, y, buf, color);
}

int GetDrawStringWidth(const char* str, int strLen)
{
    auto& ctx = Ctx::Instance();
    WString wstr = gx_internal::ToWString(str);
    if (strLen >= 0 && static_cast<size_t>(strLen) < wstr.size())
        wstr.resize(strLen);
    return ctx.textRenderer.GetStringWidth(ctx.defaultFontHandle, wstr);
}

// ============================================================================
// フォントハンドル
// ============================================================================
int CreateFontToHandle(const char* fontName, int size, int thick, int fontType)
{
    (void)thick;
    (void)fontType;
    auto& ctx = Ctx::Instance();
    return ctx.fontManager.CreateFont(gx_internal::ToWString(fontName), size);
}

int DeleteFontToHandle(int handle)
{
    (void)handle;
    return 0;
}

int DrawStringToHandle(int x, int y, const char* str, unsigned int color, int fontHandle)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ctx.textRenderer.DrawString(
        fontHandle,
        static_cast<float>(x), static_cast<float>(y),
        gx_internal::ToWString(str), color);
    return 0;
}

int DrawFormatStringToHandle(int x, int y, unsigned int color, int fontHandle,
                              const char* format, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, format);
    vsprintf_s(buf, format, args);
    va_end(args);
    return DrawStringToHandle(x, y, buf, color, fontHandle);
}

int GetDrawStringWidthToHandle(const char* str, int strLen, int fontHandle)
{
    auto& ctx = Ctx::Instance();
    WString wstr = gx_internal::ToWString(str);
    if (strLen >= 0 && static_cast<size_t>(strLen) < wstr.size())
        wstr.resize(strLen);
    return ctx.textRenderer.GetStringWidth(fontHandle, wstr);
}

// ============================================================================
// float座標版テキスト描画
// ============================================================================
int DrawStringF(float x, float y, const char* str, unsigned int color)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ctx.textRenderer.DrawString(
        ctx.defaultFontHandle, x, y,
        gx_internal::ToWString(str), color);
    return 0;
}

int DrawStringToHandleF(float x, float y, const char* str, unsigned int color, int fontHandle)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ctx.textRenderer.DrawString(
        fontHandle, x, y,
        gx_internal::ToWString(str), color);
    return 0;
}

} // namespace gx
