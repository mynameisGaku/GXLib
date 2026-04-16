/// @file main.cpp
/// @brief 09-hello-animation — スケルトンアニメーション再生 (ADR-0017 L1.5)
///
/// 学習ポイント / Learning points:
///   - LoadModel で FBX 読み込み (内蔵スケルトン + クリップ含む)
///   - GetModelAnimationCount で利用可能なクリップ数を取得
///   - PlayModelAnimation でクリップ再生 (ループ指定)
///   - SetModelAnimationTime / GetModelAnimationTime で再生位置制御
///   - 数字キー 0-9 でクリップ切替

#include "GXLib.h"
#include <cmath>
#include <cstdio>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("09 - Hello Animation");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // アニメーション付きモデルを読み込む
    // =========================================================================
    int model = LoadModel("Assets/model/Capoeira.fbx");
    const bool hasModel = (model != -1);
    int animCount = hasModel ? GetModelAnimationCount(model) : 0;

    int currentAnim = 0;
    if (hasModel && animCount > 0)
        PlayModelAnimation(model, 0, TRUE /*loop*/);

    SetCameraNearFar(0.1f, 1000.0f);
    SetCameraPositionAndTarget(VGet(0.0f, 1.5f, 3.0f), VGet(0.0f, 1.0f, 0.0f));

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int yellow = GetColor(255, 220, 50);
    unsigned int red    = GetColor(255, 100, 100);
    unsigned int green  = GetColor(80, 220, 80);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        // 数字キー 0-9 でクリップ切替
        if (hasModel && animCount > 0)
        {
            for (int k = 0; k < 10 && k < animCount; ++k)
            {
                int vk = KEY_INPUT_0 + k;
                if (CheckHitKey(vk) && currentAnim != k)
                {
                    currentAnim = k;
                    PlayModelAnimation(model, k, TRUE);
                }
            }
        }

        if (hasModel) DrawModel(model);

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        if (!hasModel)
        {
            DrawString(10, 40, "[NOT FOUND] Assets/model/Capoeira.fbx", red);
            DrawString(10, 60, "Alternatively try Mutant.fbx or Paladin J Nordstrom.fbx", white);
        }
        else
        {
            DrawFormatString(10, 40, green, "Clips available : %d", animCount);
            DrawFormatString(10, 60, yellow, "Current clip    : %d", currentAnim);
            DrawString(10, 120, "0-9 : switch clip   |   ESC : quit", white);
        }

        ScreenFlip();
    }

    if (hasModel) DeleteModel(model);
    GX_End();
    return 0;
}
