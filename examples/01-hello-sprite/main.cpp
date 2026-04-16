/// @file main.cpp
/// @brief 01-hello-sprite — スプライトをマウス位置に追従させる (ADR-0017 L1)
///
/// 学習ポイント / Learning points:
///   - LoadGraph でテクスチャを読み込む / Load a texture
///   - DrawRotaGraph で中心基準の回転描画 / Draw with center-based rotation
///   - アセット不在時のグレースフルフォールバック / Asset-missing fallback

#include "GXLib.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("01 - Hello Sprite");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    int sprite = LoadGraph("Assets/ui/icon.png");
    const bool hasSprite = (sprite != -1);

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int yellow = GetColor(255, 220,  50);
    float angle = 0.0f;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        angle += GetDeltaTime() * 2.0f;

        if (hasSprite)
            DrawRotaGraph(mx, my, 1.0, angle, sprite, TRUE);
        else
            DrawCircle(mx, my, 32, yellow, TRUE);  // fallback

        DrawFormatString(10, 10, white, "FPS: %.1f  Mouse: (%d, %d)", GetFPS(), mx, my);
        DrawString(10, 34, "Move mouse. ESC to quit.", white);

        ScreenFlip();
    }

    if (hasSprite) DeleteGraph(sprite);
    GX_End();
    return 0;
}
