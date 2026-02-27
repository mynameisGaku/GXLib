#pragma once
/// @file GX/System.h
/// @brief システム関数（手続き型API）
///
/// DXLib互換の初期化・終了・メッセージ処理を gx:: 名前空間で提供。

#include "GX/Config.h"

// Forward declarations for existing Compat functions
int  GX_Init();
void GX_End();
int  ProcessMessage();
void SetMainWindowText(const char* title);
void ChangeWindowMode(int flag);
void SetGraphMode(int width, int height, int colorBit);
int  ClearDrawScreen();
int  ScreenFlip();
void SetBackgroundColor(int r, int g, int b);
int  GetColor(int r, int g, int b);
int  GetNowCount();

namespace gx {

/// @brief エンジンを初期化する（デフォルト設定）
inline int Init() { return GX_Init(); }

/// @brief エンジンを初期化する（設定指定）
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

/// @brief エンジンを終了する
inline void End() { GX_End(); }

/// @brief ウィンドウメッセージを処理する
/// @return 0=正常, -1=終了要求
inline int ProcessMsg() { return ProcessMessage(); }

/// @brief 画面をクリアする
inline int ClearScreen() { return ClearDrawScreen(); }

/// @brief 画面を更新する（フリップ）
inline int Present() { return ScreenFlip(); }

} // namespace gx
