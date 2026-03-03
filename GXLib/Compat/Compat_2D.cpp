/// @file Compat_2D.cpp
/// @brief 簡易API 2D描画・ブレンドモード・描画色の実装
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"
#include "Compat/CompatUtil.h"

using Ctx = gx_internal::CompatContext;

namespace gx {

// ============================================================================
// ブレンドモード変換
// ============================================================================
static BlendMode ConvertBlendMode(int mode)
{
    switch (mode)
    {
    case GX_BLENDMODE_ALPHA:   return BlendMode::Alpha;
    case GX_BLENDMODE_ADD:     return BlendMode::Add;
    case GX_BLENDMODE_SUB:     return BlendMode::Sub;
    case GX_BLENDMODE_MUL:     return BlendMode::Mul;
    case GX_BLENDMODE_SCREEN:  return BlendMode::Screen;
    case GX_BLENDMODE_NOBLEND:
    default:                   return BlendMode::None;
    }
}

/// 描画前に毎回呼んで現在のブレンドモード・描画色をSpriteBatchに反映する
static void ApplyDrawState()
{
    auto& ctx = Ctx::Instance();
    ctx.spriteBatch.SetBlendMode(ConvertBlendMode(ctx.drawBlendMode));

    float a = ctx.drawBlendParam / 255.0f;
    float r = ctx.drawBright_r / 255.0f;
    float g = ctx.drawBright_g / 255.0f;
    float b = ctx.drawBright_b / 255.0f;
    ctx.spriteBatch.SetDrawColor(r, g, b, a);
}

// ============================================================================
// テクスチャ管理
// ============================================================================
int LoadGraph(const char* filePath)
{
    auto& ctx = Ctx::Instance();
    return ctx.spriteBatch.GetTextureManager().LoadTexture(
        gx_internal::ToWString(filePath));
}

int DeleteGraph(int handle)
{
    Ctx::Instance().spriteBatch.GetTextureManager().ReleaseTexture(handle);
    return 0;
}

int LoadDivGraph(const char* filePath, int allNum, int xNum, int yNum,
                 int xSize, int ySize, int* handleBuf)
{
    if (!handleBuf) return -1;
    if (allNum <= 0 || allNum > 4096) return -1;
    if (xNum <= 0 || yNum <= 0 || xSize <= 0 || ySize <= 0) return -1;

    auto& ctx = Ctx::Instance();
    int baseHandle = LoadGraph(filePath);
    if (baseHandle < 0) return -1;

    int firstHandle = ctx.spriteBatch.GetTextureManager().CreateRegionHandles(
        baseHandle, allNum, xNum, yNum, xSize, ySize);
    if (firstHandle < 0)
    {
        for (int i = 0; i < allNum; ++i)
            handleBuf[i] = baseHandle;
        return 0;
    }

    for (int i = 0; i < allNum; ++i)
        handleBuf[i] = firstHandle + i;

    return 0;
}

int GetGraphSize(int handle, int* width, int* height)
{
    auto& ctx = Ctx::Instance();
    auto* tex = ctx.spriteBatch.GetTextureManager().GetTexture(handle);
    if (!tex) return -1;
    if (width)  *width  = static_cast<int>(tex->GetWidth());
    if (height) *height = static_cast<int>(tex->GetHeight());
    return 0;
}

// ============================================================================
// スプライト描画
// ============================================================================
int DrawGraph(int x, int y, int handle, int transFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawGraph(
        static_cast<float>(x), static_cast<float>(y),
        handle, transFlag != 0);
    return 0;
}

int DrawRotaGraph(int cx, int cy, double extRate, double angle,
                  int handle, int transFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawRotaGraph(
        static_cast<float>(cx), static_cast<float>(cy),
        static_cast<float>(extRate), static_cast<float>(angle),
        handle, transFlag != 0);
    return 0;
}

int DrawExtendGraph(int x1, int y1, int x2, int y2, int handle, int transFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawExtendGraph(
        static_cast<float>(x1), static_cast<float>(y1),
        static_cast<float>(x2), static_cast<float>(y2),
        handle, transFlag != 0);
    return 0;
}

int DrawRectGraph(int x, int y, int srcX, int srcY, int w, int h,
                  int handle, int transFlag, int turnFlag)
{
    (void)turnFlag;
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawRectGraph(
        static_cast<float>(x), static_cast<float>(y),
        srcX, srcY, w, h,
        handle, transFlag != 0);
    return 0;
}

int DrawModiGraph(int x1, int y1, int x2, int y2, int x3, int y3,
                  int x4, int y4, int handle, int transFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawModiGraph(
        static_cast<float>(x1), static_cast<float>(y1),
        static_cast<float>(x2), static_cast<float>(y2),
        static_cast<float>(x3), static_cast<float>(y3),
        static_cast<float>(x4), static_cast<float>(y4),
        handle, transFlag != 0);
    return 0;
}

// ============================================================================
// プリミティブ描画
// ============================================================================
int DrawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawLine(
        static_cast<float>(x1), static_cast<float>(y1),
        static_cast<float>(x2), static_cast<float>(y2),
        color, thickness);
    return 0;
}

int DrawBox(int x1, int y1, int x2, int y2, unsigned int color, int fillFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawBox(
        static_cast<float>(x1), static_cast<float>(y1),
        static_cast<float>(x2), static_cast<float>(y2),
        color, fillFlag != 0);
    return 0;
}

int DrawCircle(int cx, int cy, int r, unsigned int color, int fillFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawCircle(
        static_cast<float>(cx), static_cast<float>(cy),
        static_cast<float>(r), color, fillFlag != 0);
    return 0;
}

int DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3,
                 unsigned int color, int fillFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawTriangle(
        static_cast<float>(x1), static_cast<float>(y1),
        static_cast<float>(x2), static_cast<float>(y2),
        static_cast<float>(x3), static_cast<float>(y3),
        color, fillFlag != 0);
    return 0;
}

int DrawOval(int cx, int cy, int rx, int ry, unsigned int color, int fillFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawOval(
        static_cast<float>(cx), static_cast<float>(cy),
        static_cast<float>(rx), static_cast<float>(ry),
        color, fillFlag != 0);
    return 0;
}

int DrawPixel(int x, int y, unsigned int color)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawPixel(static_cast<float>(x), static_cast<float>(y), color);
    return 0;
}

// ============================================================================
// float座標版（サブピクセル精度）
// ============================================================================

// --- スプライト ---
int DrawGraphF(float x, float y, int handle, int transFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawGraph(x, y, handle, transFlag != 0);
    return 0;
}

int DrawRotaGraphF(float cx, float cy, double extRate, double angle,
                   int handle, int transFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawRotaGraph(cx, cy,
        static_cast<float>(extRate), static_cast<float>(angle),
        handle, transFlag != 0);
    return 0;
}

int DrawExtendGraphF(float x1, float y1, float x2, float y2,
                     int handle, int transFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawExtendGraph(x1, y1, x2, y2, handle, transFlag != 0);
    return 0;
}

int DrawRectGraphF(float x, float y, int srcX, int srcY, int w, int h,
                   int handle, int transFlag, int turnFlag)
{
    (void)turnFlag;
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawRectGraph(x, y, srcX, srcY, w, h, handle, transFlag != 0);
    return 0;
}

int DrawModiGraphF(float x1, float y1, float x2, float y2, float x3, float y3,
                   float x4, float y4, int handle, int transFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsureSpriteBatch();
    ApplyDrawState();
    ctx.spriteBatch.DrawModiGraph(x1, y1, x2, y2, x3, y3, x4, y4,
                                  handle, transFlag != 0);
    return 0;
}

// --- プリミティブ ---
int DrawLineF(float x1, float y1, float x2, float y2, unsigned int color, int thickness)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawLine(x1, y1, x2, y2, color, thickness);
    return 0;
}

int DrawBoxF(float x1, float y1, float x2, float y2, unsigned int color, int fillFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawBox(x1, y1, x2, y2, color, fillFlag != 0);
    return 0;
}

int DrawCircleF(float cx, float cy, float r, unsigned int color, int fillFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawCircle(cx, cy, r, color, fillFlag != 0);
    return 0;
}

int DrawTriangleF(float x1, float y1, float x2, float y2, float x3, float y3,
                  unsigned int color, int fillFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawTriangle(x1, y1, x2, y2, x3, y3, color, fillFlag != 0);
    return 0;
}

int DrawOvalF(float cx, float cy, float rx, float ry, unsigned int color, int fillFlag)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawOval(cx, cy, rx, ry, color, fillFlag != 0);
    return 0;
}

int DrawPixelF(float x, float y, unsigned int color)
{
    auto& ctx = Ctx::Instance();
    ctx.EnsurePrimitiveBatch();
    ctx.primBatch.DrawPixel(x, y, color);
    return 0;
}

// ============================================================================
// ブレンドモード・描画色
// ============================================================================
int SetDrawBlendMode(int blendMode, int blendParam)
{
    auto& ctx = Ctx::Instance();
    ctx.drawBlendMode  = blendMode;
    ctx.drawBlendParam = blendParam;
    return 0;
}

int SetDrawBright(int r, int g, int b)
{
    auto& ctx = Ctx::Instance();
    ctx.drawBright_r = static_cast<uint32_t>(r & 0xFF);
    ctx.drawBright_g = static_cast<uint32_t>(g & 0xFF);
    ctx.drawBright_b = static_cast<uint32_t>(b & 0xFF);
    return 0;
}

} // namespace gx
