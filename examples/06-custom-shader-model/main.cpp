/// @file main.cpp
/// @brief 06-custom-shader-model — カスタムシェーダモデル登録 (ADR-0017 L2)
///
/// このサンプルは Layer 2 の拡張ポイントを実際に使う例。
/// ShaderRegistry::RegisterCustomShaderModel で独自シェーダをPSOレジストリに登録し、
/// Material::shaderModel をそのIDに設定して描画する。
///
/// 学習ポイント / Learning points:
///   - ShaderRegistry::CustomShaderModelDesc で VS/PS パスを指定
///   - RegisterCustomShaderModel(id, desc) で登録 (id は 6-254 の範囲)
///   - Material::shaderModel = static_cast<ShaderModel>(id) で選択
///   - 組み込み 6 モデルの Material API (色・金属度・粗さ等) はそのまま使える
///     ※ カスタムシェーダ側でその cbuffer を読むかどうかは実装次第
///
/// このサンプルは Shaders/custom/Rainbow.hlsl を登録して、
/// ワールド座標ベースの虹色グラデーションでモデルを描画する。

#include "GXLib.h"
#include "Graphics/3D/ShaderRegistry.h"
#include "Graphics/3D/Renderer3D.h"
#include "Core/Engine.h"
#include <cmath>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("06 - Custom Shader Model (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // Layer 2: カスタムシェーダモデルを登録
    // =========================================================================
    constexpr uint32_t RAINBOW_ID = 100;   // 6-254 の範囲で任意

    gx::ShaderRegistry::CustomShaderModelDesc desc;
    desc.vsPath          = L"Shaders/custom/Rainbow.hlsl";
    desc.vsEntry         = "VSMain";
    desc.psPath          = L"Shaders/custom/Rainbow.hlsl";
    desc.psEntry         = "PSMain";
    desc.supportsSkinning = false;  // このサンプルはスタティックメッシュのみ

    // Engine から Renderer3D → ShaderRegistry を取得して登録
    auto& registry = gx::Engine::Instance().GetRenderer3D().GetShaderRegistry();
    bool registered = registry.RegisterCustomShaderModel(RAINBOW_ID, desc);
    if (!registered)
    {
        // ログに詳細が出ているはず (GX_LOG_ERROR)
        DrawString(10, 60, "[ERROR] Failed to register Rainbow shader model — check Shaders/custom/Rainbow.hlsl exists", GetColor(255, 100, 100));
    }

    // =========================================================================
    // モデル読み込み + カスタムシェーダモデル割り当て
    // =========================================================================
    int model = LoadModel("Assets/model/Cube.fbx");
    const bool hasModel = (model != -1);

    int materialsApplied = 0;
    if (hasModel && registered)
    {
        // モデルの全マテリアルを Rainbow に切り替える (Compat API, ADR-0017 L2)
        materialsApplied = SetModelShaderModel(model, RAINBOW_ID);
        if (materialsApplied < 0)
        {
            // ログに詳細あり
        }
    }

    SetCameraNearFar(0.1f, 1000.0f);

    unsigned int white = GetColor(255, 255, 255);
    unsigned int green = GetColor(100, 255, 100);
    unsigned int red   = GetColor(255, 100, 100);
    float yaw = 0.0f;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        yaw += dt * 0.8f;
        float camX = std::cos(yaw) * 5.0f;
        float camZ = std::sin(yaw) * 5.0f;
        SetCameraPositionAndTarget(VGet(camX, 2.0f, camZ), VGet(0.0f, 0.0f, 0.0f));

        if (hasModel) DrawModel(model);

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        DrawFormatString(10, 30, registered ? green : red,
                         "Custom shader model (id=%u) : %s",
                         RAINBOW_ID, registered ? "REGISTERED" : "FAILED");
        DrawFormatString(10, 50, white, "Registered custom models : %zu",
                         registry.GetCustomShaderModelCount());
        DrawFormatString(10, 70, (materialsApplied > 0) ? green : red,
                         "Materials applied        : %d", materialsApplied);
        DrawString(10, 100, "Model should render with world-Y-based rainbow gradient.", white);
        DrawString(10, 140, "ESC: quit", white);

        ScreenFlip();
    }

    // クリーンアップ
    if (registered) registry.UnregisterCustomShaderModel(RAINBOW_ID);
    if (hasModel) DeleteModel(model);
    GX_End();
    return 0;
}
