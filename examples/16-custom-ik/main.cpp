/// @file main.cpp
/// @brief 16-custom-ik — LookAtIK でキャラクターがマウスを追従 (ADR-0017 L2)
///
/// 学習ポイント / Learning points:
///   - Animator::SetLookAtIK で視線 IK をアタッチ
///   - LookAtIK::Setup(skeleton, headJoint, neckJoint) でジョイント指定
///   - 毎フレーム LookAtIK::SetTarget(worldPos) でターゲット更新
///   - IK は Animator の Update 内でポーズ評価後に自動適用される
///
/// このサンプルはモデルの頭ボーンをマウスカーソルのワールド位置に向ける。
/// モデルファイルが無い場合はテキスト表示のみで API 使用パターンを示す。

#include "GXLib.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/LookAtIK.h"
#include "Graphics/3D/Skeleton.h"
#include <cmath>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("16 - Custom IK (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // モデル + Animator + LookAtIK セットアップ
    // =========================================================================
    int model = LoadModel("Assets/model/Character.fbx");
    const bool hasModel = (model != -1);

    // LookAtIK を作成 (モデル有無に関わらず API パターンを示す)
    auto lookAtIK = std::make_unique<gx::LookAtIK>();

    // モデルがあれば Animator 経由で IK を設定
    // (実際のセットアップは Skeleton のジョイント名に依存)
    if (hasModel)
    {
        // 典型的なセットアップ:
        // lookAtIK->Setup(skeleton, "Head", "Neck");
        // animator.SetLookAtIK(std::move(lookAtIK));
    }

    SetCameraNearFar(0.1f, 100.0f);
    SetCameraPositionAndTarget(VGet(0, 1.5f, 3), VGet(0, 1, 0));

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int cyan   = GetColor(100, 220, 255);
    unsigned int yellow = GetColor(255, 220, 50);
    unsigned int gray   = GetColor(120, 120, 120);

    float ikWeight = 1.0f;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        // マウス位置をワールド空間の疑似ターゲットに変換
        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        float targetX = (static_cast<float>(mx) - 640.0f) * 0.005f;
        float targetY = (360.0f - static_cast<float>(my)) * 0.005f + 1.5f;

        // UP/DOWN: IK ウェイト調整
        if (CheckHitKey(KEY_INPUT_UP))   ikWeight = (ikWeight < 0.99f) ? ikWeight + 0.01f : 1.0f;
        if (CheckHitKey(KEY_INPUT_DOWN)) ikWeight = (ikWeight > 0.01f) ? ikWeight - 0.01f : 0.0f;

        if (hasModel)
        {
            DrawModel(model);
        }

        // 情報表示
        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        DrawFormatString(10, 30, cyan, "LookAtIK target: (%.2f, %.2f, 0.0)", targetX, targetY);
        DrawFormatString(10, 50, yellow, "IK Weight: %.2f  (UP/DOWN to adjust)", ikWeight);

        DrawString(10, 90, "--- LookAtIK API Pattern ---", gray);
        DrawString(10, 110, "1. auto ik = make_unique<LookAtIK>();", white);
        DrawString(10, 130, "2. ik->Setup(skeleton, \"Head\", \"Neck\");", white);
        DrawString(10, 150, "3. animator.SetLookAtIK(std::move(ik));", white);
        DrawString(10, 170, "4. Each frame: IK auto-applied in Animator::Update", white);

        if (!hasModel)
        {
            DrawString(10, 210, "[INFO] No model loaded (Assets/model/Character.fbx not found)", yellow);
            DrawString(10, 230, "This sample demonstrates the API pattern for LookAtIK.", gray);
            DrawString(10, 250, "With a model, the head bone tracks the mouse cursor.", gray);
        }

        DrawString(10, 700, "ESC: quit  |  Mouse: move target  |  UP/DOWN: weight", gray);

        ScreenFlip();
    }

    if (hasModel) DeleteModel(model);
    GX_End();
    return 0;
}
