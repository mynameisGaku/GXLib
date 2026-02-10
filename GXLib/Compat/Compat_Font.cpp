/// @file Compat_Font.cpp
/// @brief 簡易API フォント・テキスト描画関数の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"

using Ctx = GX_Internal::CompatContext;

/// TCHAR文字列をstd::wstringに変換するヘルパー
static std::wstring ToWString(const TCHAR* str)
{
#ifdef UNICODE
    return std::wstring(str);
#else
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str, -1, result.data(), len);
    return result;
#endif
}

// ============================================================================
// デフォルトフォントでの描画
// ============================================================================
int DrawString(int x, int y, const TCHAR* str, unsigned int color)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ctx.textRenderer.DrawString(
        ctx.defaultFontHandle,
        static_cast<float>(x), static_cast<float>(y),
        ToWString(str), color);
    return 0;
}

int DrawFormatString(int x, int y, unsigned int color, const TCHAR* format, ...)
{
    TCHAR buf[1024];
    va_list args;
    va_start(args, format);
#ifdef UNICODE
    vswprintf_s(buf, format, args);
#else
    vsprintf_s(buf, format, args);
#endif
    va_end(args);
    return DrawString(x, y, buf, color);
}

int GetDrawStringWidth(const TCHAR* str, int strLen)
{
    auto& ctx = Ctx::Instance();
    std::wstring wstr = ToWString(str);
    if (strLen >= 0 && static_cast<size_t>(strLen) < wstr.size())
        wstr.resize(strLen);
    return ctx.textRenderer.GetStringWidth(ctx.defaultFontHandle, wstr);
}

// ============================================================================
// フォントハンドル
// ============================================================================
int CreateFontToHandle(const TCHAR* fontName, int size, int thick, int fontType)
{
    (void)thick;
    (void)fontType;
    auto& ctx = Ctx::Instance();
    return ctx.fontManager.CreateFont(ToWString(fontName), size);
}

int DeleteFontToHandle(int handle)
{
    (void)handle;
    return 0;
}

int DrawStringToHandle(int x, int y, const TCHAR* str, unsigned int color, int fontHandle)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ctx.textRenderer.DrawString(
        fontHandle,
        static_cast<float>(x), static_cast<float>(y),
        ToWString(str), color);
    return 0;
}

int DrawFormatStringToHandle(int x, int y, unsigned int color, int fontHandle,
                              const TCHAR* format, ...)
{
    TCHAR buf[1024];
    va_list args;
    va_start(args, format);
#ifdef UNICODE
    vswprintf_s(buf, format, args);
#else
    vsprintf_s(buf, format, args);
#endif
    va_end(args);
    return DrawStringToHandle(x, y, buf, color, fontHandle);
}

int GetDrawStringWidthToHandle(const TCHAR* str, int strLen, int fontHandle)
{
    auto& ctx = Ctx::Instance();
    std::wstring wstr = ToWString(str);
    if (strLen >= 0 && static_cast<size_t>(strLen) < wstr.size())
        wstr.resize(strLen);
    return ctx.textRenderer.GetStringWidth(fontHandle, wstr);
}
