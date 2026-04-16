#pragma once
/// @file GX/System.h
/// @brief System functions / システム関数 — Layer 1 facade per ADR-0017
///
/// Engine lifecycle (init / shutdown), message pump, frame control,
/// background color. Mirror DXLib's system API with the `gx::` namespace
/// and friendlier names where appropriate.
///
/// エンジンのライフサイクル (初期化・終了)、メッセージ処理、フレーム制御、
/// 背景色。DXLibのシステムAPIを gx:: 名前空間でクリーンな名前で提供する。
///
/// @code
/// // DXLib-style (flat)
/// ChangeWindowMode(TRUE);
/// SetGraphMode(1280, 720, 32);
/// if (GX_Init() == -1) return -1;
/// while (ProcessMessage() == 0) { ClearDrawScreen(); /*...*/ ScreenFlip(); }
/// GX_End();
///
/// // gx:: (cleaner names)
/// gx::Init({.title = "My Game", .width = 1280, .height = 720});
/// while (gx::ProcessMsg() == 0) { gx::ClearScreen(); /*...*/ gx::Present(); }
/// gx::End();
/// @endcode

#include "GX/Config.h"

// Forward declarations — DXLib-compatible global names

/// @brief Initialize the engine with current settings. Call after SetGraphMode etc.
/// @return 0 on success, -1 on failure (error logged via gx::Logger).
int  GX_Init();

/// @brief Shut down the engine and release all resources. エンジン終了。
void GX_End();

/// @brief Process pending Win32 messages. Call once per frame.
/// @return 0 = normal, -1 = quit requested (window closed or WM_QUIT).
/// @code while (ProcessMessage() == 0) { /* game loop */ } @endcode
int  ProcessMessage();

/// @brief Set the window title. ウィンドウタイトル設定。
void SetMainWindowText(const char* title);

/// @brief Switch between windowed / fullscreen mode. Call before GX_Init.
/// @param flag TRUE = windowed, FALSE = fullscreen.
void ChangeWindowMode(int flag);

/// @brief Set render resolution and color depth. Call before GX_Init.
/// @param width, height Client area in pixels.
/// @param colorBit 32 (standard) or 16 (legacy).
void SetGraphMode(int width, int height, int colorBit);

/// @brief Clear the current draw target with the background color.
int  ClearDrawScreen();

/// @brief Present the back buffer (flip). フレーム確定。
/// Combines vsync + buffer swap.
int  ScreenFlip();

/// @brief Set the clear color used by ClearDrawScreen. 背景色を設定。
void SetBackgroundColor(int r, int g, int b);

/// @brief Make a 32-bit color value from RGB components. RGB→色値。
int  GetColor(int r, int g, int b);

/// @brief Milliseconds since engine start. DXLib-compatible tick counter.
int  GetNowCount();

namespace gx {
/// @addtogroup grp_gx_facade
/// @{

/// @brief Initialize the engine with default settings. デフォルト設定で初期化。
/// @return 0 on success, -1 on failure.
inline int Init() { return GX_Init(); }

/// @brief Initialize the engine with explicit configuration. 設定を指定して初期化。
/// @param config Width/height/title/windowed/background values.
/// @return 0 on success, -1 on failure.
/// @code gx::Init({.title="My Game", .width=1280, .height=720, .windowed=true}); @endcode
inline int Init(const Config& config)
{
    ChangeWindowMode(config.windowed ? 1 : 0);
    SetGraphMode(config.width, config.height, 32);
    SetMainWindowText(config.title.c_str());
    int result = GX_Init();
    if (result == 0)
    {
        SetBackgroundColor(
            static_cast<int>(config.background.r * 255),
            static_cast<int>(config.background.g * 255),
            static_cast<int>(config.background.b * 255));
    }
    return result;
}

/// @brief Shut down the engine. 終了。
inline void End() { GX_End(); }

/// @brief Process Win32 messages. 毎フレーム呼び出す。
/// @return 0 = normal, -1 = quit requested.
inline int ProcessMsg() { return ProcessMessage(); }

/// @brief Clear the back buffer. 画面クリア。
inline int ClearScreen() { return ClearDrawScreen(); }

/// @brief Flip the back buffer to the display. 画面更新。
inline int Present() { return ScreenFlip(); }

/// @}
} // namespace gx
