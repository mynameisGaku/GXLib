#pragma once
/// @file GX/Draw2D.h
/// @brief 2D drawing functions / 2D描画関数 — Layer 1 facade per ADR-0017
///
/// Sprites, shapes, and blend modes. The `gx::` aliases use cleaner names
/// (LoadImage, DrawImage) while the DXLib-named globals remain available.
///
/// スプライト・図形・ブレンドモード。gx:: のエイリアスはクリーンな名前を使用
/// (LoadImage, DrawImage)。DXLib名のグローバル関数もそのまま使用可。
///
/// @code
/// int img = gx::LoadImage("player.png"); // or: LoadGraph("player.png")
/// gx::DrawImage(100, 200, img);          // or: DrawGraph(100, 200, img, TRUE)
/// gx::DrawBox(0, 0, 50, 50, gx::GetColor(255, 0, 0), TRUE);
/// @endcode

#include "Math/Color.h"

// Forward declarations — DXLib-compatible global names

/// @brief Load a texture from file. テクスチャ読み込み。
/// @return Handle ≥ 0 on success, -1 on failure (logged).
/// @code int h = LoadGraph("sprites/enemy.png"); @endcode
int LoadGraph(const char* path);

/// @brief Release a loaded texture. テクスチャ解放。
int DeleteGraph(int handle);

/// @brief Get texture dimensions. テクスチャサイズ取得。
/// @return 0 on success, -1 if handle is invalid.
int GetGraphSize(int handle, int* width, int* height);

/// @brief Draw a sprite at (x, y). スプライト描画。
/// @param transFlag TRUE for alpha-blended, FALSE for opaque.
int DrawGraph(int x, int y, int handle, int transFlag);

/// @brief Draw a sprite with rotation and scaling (center-origin). 回転拡大描画。
/// @param extRate Scale factor (1.0 = original). @param angle Radians.
int DrawRotaGraph(int x, int y, double extRate, double angle, int handle, int transFlag, int reverseX = 0, int reverseY = 0);

/// @brief Draw a sprite stretched to a rectangle. 矩形拡縮描画。
int DrawExtendGraph(int x1, int y1, int x2, int y2, int handle, int transFlag);

/// @brief Draw a line. 線描画。
int DrawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness = 1);

/// @brief Draw a filled or outlined rectangle. 矩形描画。
/// @param fillFlag TRUE = filled, FALSE = outline.
int DrawBox(int x1, int y1, int x2, int y2, unsigned int color, int fillFlag);

/// @brief Draw a circle. 円描画。
int DrawCircle(int x, int y, int r, unsigned int color, int fillFlag = 1, int lineThickness = 1);

/// @brief Draw a triangle. 三角形描画。
int DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int color, int fillFlag);

/// @brief Draw an ellipse. 楕円描画。
int DrawOval(int x, int y, int rx, int ry, unsigned int color, int fillFlag);

/// @brief Draw a single pixel. 点描画。
int DrawPixel(int x, int y, unsigned int color);

/// @brief Set blend mode. ブレンドモード設定。
/// @param blendMode GX_BLENDMODE_ALPHA / ADD / SUB / MUL / SCREEN / NOBLEND.
/// @param param Alpha value 0..255 (for ALPHA mode).
int SetDrawBlendMode(int blendMode, int param);

/// @brief Set draw brightness tint. 描画輝度設定。
int SetDrawBright(int r, int g, int b);

/// @brief Make a color value from RGB. RGB→カラー値。
/// @code unsigned int red = GetColor(255, 0, 0); @endcode
int GetColor(int r, int g, int b);

namespace gx {
/// @addtogroup grp_gx_facade
/// @{

/// @brief Load a texture. LoadGraph のクリーンなエイリアス。
/// @code int img = gx::LoadImage("player.png"); @endcode
inline int LoadImage(const char* path) { return LoadGraph(path); }

/// @brief Release a texture. DeleteGraph エイリアス。
inline int DeleteImage(int handle) { return DeleteGraph(handle); }

/// @brief Get texture size. GetGraphSize エイリアス。
inline int GetImageSize(int handle, int* w, int* h) { return GetGraphSize(handle, w, h); }

/// @brief Draw a sprite. DrawGraph エイリアス (transFlag defaults to TRUE).
/// @code gx::DrawImage(100, 200, img); @endcode
inline int DrawImage(int x, int y, int handle, int trans = 1) { return DrawGraph(x, y, handle, trans); }

/// @brief Draw with rotation+scale. DrawRotaGraph エイリアス。
inline int DrawRotaImage(int x, int y, double ext, double angle, int handle, int trans = 1)
{ return DrawRotaGraph(x, y, ext, angle, handle, trans); }

using ::DrawLine;
using ::DrawBox;
using ::DrawCircle;
using ::DrawTriangle;
using ::DrawOval;
using ::DrawPixel;
using ::SetDrawBlendMode;
using ::SetDrawBright;
using ::GetColor;

/// @}
} // namespace gx
