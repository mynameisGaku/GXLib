/// @file main.cpp
/// @brief 04-hello-3d — モデル読み込みとオービットカメラ (ADR-0017 L1)
///
/// 学習ポイント / Learning points:
///   - LoadModel / DrawModel / DeleteModel
///   - SetCameraPositionAndTarget / SetCameraNearFar
///   - 極座標カメラオービット / Polar-coordinate camera orbit
///
/// NOTE: SetDirectionalLight / SetAmbientLight は Compat 層に未実装。
///       エンジン側のデフォルトライトが自動適用される。
///       (Tracked as gap in implementation-gap-analysis-2026-04-16.md)

#include "GXLib.h"
#include <cmath>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("04 - Hello 3D");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    int model = LoadModel("Assets/model/Cube.fbx");
    const bool hasModel = (model != -1);

    SetCameraNearFar(0.1f, 1000.0f);

    unsigned int white = GetColor(255, 255, 255);
    unsigned int red   = GetColor(255, 100, 100);
    float yaw = 0.0f;
    constexpr float ORBIT_DIST = 5.0f;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        yaw += dt * 0.8f;

        // 極座標 → デカルト座標 でカメラ位置を計算
        float camX = std::cos(yaw) * ORBIT_DIST;
        float camZ = std::sin(yaw) * ORBIT_DIST;
        SetCameraPositionAndTarget(VGet(camX, 2.0f, camZ), VGet(0.0f, 0.0f, 0.0f));

        if (hasModel)
        {
            SetModelPosition(model, VGet(0.0f, 0.0f, 0.0f));
            SetModelScale(model, VGet(1.0f, 1.0f, 1.0f));
            DrawModel(model);
        }

        DrawFormatString(10, 10, white, "FPS: %.1f  Yaw: %.2f rad", GetFPS(), yaw);
        DrawString(10, 30, "ESC: quit", white);
        if (!hasModel)
            DrawString(10, 60, "[NOT FOUND] Assets/model/Cube.fbx — see log for details", red);

        ScreenFlip();
    }

    if (hasModel) DeleteModel(model);
    GX_End();
    return 0;
}
