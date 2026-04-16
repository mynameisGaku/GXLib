#pragma once
/// @file GX/Config.h
/// @brief エンジン設定

#include "Math/Color.h"
#include "Container/String.h"

namespace gx {
/// @addtogroup grp_gx_facade
/// @{

/// @brief エンジン初期化設定
struct Config
{
    gx::String   title   = "GXLib Application"; ///< ウィンドウタイトル
    int          width   = 1280;               ///< 画面幅（ピクセル）
    int          height  = 720;                ///< 画面高さ（ピクセル）
    bool         windowed = true;              ///< ウィンドウモード（false=フルスクリーン）
    bool         vsync   = false;              ///< 垂直同期の有効/無効
    Color        background = { 0.0f, 0.0f, 0.0f, 1.0f }; ///< 背景色（RGBA）

    // フレーム制御
    float        targetFps    = 60.0f;         ///< 目標フレームレート（FPS）
    float        maxDeltaTime = 0.1f;          ///< deltaTimeの上限値（秒、スパイク防止）

    // 自動制御
    bool         autoClear  = true;            ///< 毎フレーム自動クリアするか
    bool         autoPresent = true;           ///< 毎フレーム自動プレゼントするか
    bool         allowEscapeExit = true;       ///< Escキーでアプリを終了するか
};

/// @}
} // namespace gx
