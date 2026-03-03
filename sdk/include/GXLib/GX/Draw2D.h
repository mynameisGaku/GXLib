#pragma once
/// @file GX/Draw2D.h
/// @brief 2D描画関数（手続き型API）

#include "Math/Color.h"

// Forward declarations for existing Compat functions
int LoadGraph(const char* path);
int DeleteGraph(int handle);
int GetGraphSize(int handle, int* width, int* height);
int DrawGraph(int x, int y, int handle, int transFlag);
int DrawRotaGraph(int x, int y, double extRate, double angle, int handle, int transFlag, int reverseX = 0, int reverseY = 0);
int DrawExtendGraph(int x1, int y1, int x2, int y2, int handle, int transFlag);
int DrawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness = 1);
int DrawBox(int x1, int y1, int x2, int y2, unsigned int color, int fillFlag);
int DrawCircle(int x, int y, int r, unsigned int color, int fillFlag = 1, int lineThickness = 1);
int DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int color, int fillFlag);
int DrawOval(int x, int y, int rx, int ry, unsigned int color, int fillFlag);
int DrawPixel(int x, int y, unsigned int color);
int SetDrawBlendMode(int blendMode, int param);
int SetDrawBright(int r, int g, int b);
int GetColor(int r, int g, int b);

namespace gx {
/// @addtogroup grp_gx_facade
/// @{

// --- 画像描画 ---
inline int LoadImage(const char* path) { return LoadGraph(path); }
inline int DeleteImage(int handle) { return DeleteGraph(handle); }
inline int GetImageSize(int handle, int* w, int* h) { return GetGraphSize(handle, w, h); }
inline int DrawImage(int x, int y, int handle, int trans = 1) { return DrawGraph(x, y, handle, trans); }
inline int DrawRotaImage(int x, int y, double ext, double angle, int handle, int trans = 1)
{ return DrawRotaGraph(x, y, ext, angle, handle, trans); }

// --- プリミティブ描画 ---
using ::DrawLine;
using ::DrawBox;
using ::DrawCircle;
using ::DrawTriangle;
using ::DrawOval;
using ::DrawPixel;

// --- ブレンド/色設定 ---
using ::SetDrawBlendMode;
using ::SetDrawBright;
using ::GetColor;

/// @}
} // namespace gx
