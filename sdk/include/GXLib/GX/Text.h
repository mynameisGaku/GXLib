#pragma once
/// @file GX/Text.h
/// @brief テキスト描画関数

// Forward declarations for existing Compat functions
int DrawString(int x, int y, const char* str, unsigned int color, unsigned int edgeColor = 0);
int DrawFormatString(int x, int y, unsigned int color, const char* fmt, ...);
int GetDrawStringWidth(const char* str, int strLen);
int CreateFontToHandle(const char* fontName, int size, int thick = -1, int fontType = -1);
int DeleteFontToHandle(int handle);
int DrawStringToHandle(int x, int y, const char* str, unsigned int color, int fontHandle, unsigned int edgeColor = 0);

namespace gx {
/// @addtogroup grp_gx_facade
/// @{

using ::DrawString;
using ::DrawFormatString;
using ::GetDrawStringWidth;
using ::CreateFontToHandle;
using ::DeleteFontToHandle;
using ::DrawStringToHandle;

/// @}
} // namespace gx
