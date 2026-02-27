/// @file main.cpp
/// @brief GXLib Hello World
///
/// GXLib SDK のスタータープロジェクト。
/// このファイルを自由に書き換えてゲームを作ろう！

#include "GXLib.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("GXLib Hello World");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    unsigned int white = GetColor(255, 255, 255);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        DrawString(540, 340, "Hello, GXLib!", white);
        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());

        ScreenFlip();
    }

    GX_End();
    return 0;
}
