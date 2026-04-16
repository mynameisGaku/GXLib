#pragma once
/// @file GX/Text.h
/// @brief Text drawing functions / テキスト描画関数 — Layer 1 facade per ADR-0017
///
/// @code
/// // Simple text
/// gx::DrawString(10, 10, "Hello!", gx::GetColor(255, 255, 255));
///
/// // Formatted (printf-style)
/// gx::DrawFormatString(10, 30, white, "Score: %d", score);
///
/// // Custom font
/// int font = gx::CreateFontToHandle("MS Gothic", 24);
/// gx::DrawStringToHandle(10, 60, "Custom", white, font);
/// @endcode

/// @brief Draw text at (x, y). テキスト描画。
/// @param edgeColor Edge (outline) color, 0 = no edge.
/// @return 0 on success.
int DrawString(int x, int y, const char* str, unsigned int color, unsigned int edgeColor = 0);

/// @brief Draw formatted text (printf-style). 書式付きテキスト描画。
/// @code DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS()); @endcode
int DrawFormatString(int x, int y, unsigned int color, const char* fmt, ...);

/// @brief Measure text width in pixels. テキスト幅をピクセルで取得。
int GetDrawStringWidth(const char* str, int strLen);

/// @brief Create a named font handle. フォントハンドル作成。
/// @param fontName System font name (e.g. "MS Gothic"). @param size Pixel height.
/// @return Handle ≥ 0 on success, -1 on failure.
int CreateFontToHandle(const char* fontName, int size, int thick = -1, int fontType = -1);

/// @brief Release a font handle. フォントハンドル解放。
int DeleteFontToHandle(int handle);

/// @brief Draw text using a specific font handle. フォント指定テキスト描画。
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
