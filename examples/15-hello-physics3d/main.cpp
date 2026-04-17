/// @file main.cpp
/// @brief 15-hello-physics3d — 3D 物理シェイプ作成 + ボディ落下 (ADR-0017 L2)
///
/// 学習ポイント / Learning points:
///   - PhysicsWorld3D::CreateBoxShape / CreateSphereShape でシェイプ作成
///   - PhysicsBodySettings で位置・質量・摩擦・反発を設定
///   - AddBody(shape, settings) でワールドに追加
///   - Step(dt) で物理シミュレーション、GetPosition で結果取得
///   - SPACE キーでボックスを落下スポーン

#include "GXLib.h"
#include "Physics/PhysicsWorld3D.h"
#include "Physics/PhysicsShape.h"
#include <cstdlib>
#include <cmath>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("15 - Hello Physics 3D (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // PhysicsWorld3D 初期化
    // =========================================================================
    gx::PhysicsWorld3D world;
    world.Initialize();
    world.SetGravity({ 0.0f, -9.81f, 0.0f });

    // 床 (Static ボックス)
    auto* floorShape = world.CreateBoxShape({ 20.0f, 0.5f, 20.0f });
    gx::PhysicsBodySettings floorSettings;
    floorSettings.position   = { 0.0f, -0.5f, 0.0f };
    floorSettings.motionType = gx::MotionType3D::Static;
    floorSettings.layer      = 0;
    world.AddBody(floorShape, floorSettings);

    // 動的ボックスのリスト
    struct BoxInfo { gx::PhysicsBodyID id; gx::PhysicsShape* shape; };
    gx::Vector<BoxInfo> boxes;

    constexpr float FIXED_DT = 1.0f / 60.0f;
    float accumulator = 0.0f;
    float yaw = 0.0f;
    bool spaceWasDown = false;

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int green  = GetColor(80, 220, 80);
    unsigned int yellow = GetColor(255, 220, 50);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        // SPACE: ボックスをスポーン
        bool spaceDown = (CheckHitKey(KEY_INPUT_SPACE) != 0);
        if (spaceDown && !spaceWasDown)
        {
            for (int i = 0; i < 5; ++i)
            {
                float rx = -3.0f + static_cast<float>(std::rand() % 600) * 0.01f;
                float rz = -3.0f + static_cast<float>(std::rand() % 600) * 0.01f;

                auto* shape = world.CreateBoxShape({ 0.4f, 0.4f, 0.4f });
                gx::PhysicsBodySettings s;
                s.position    = { rx, 8.0f + static_cast<float>(i), rz };
                s.motionType  = gx::MotionType3D::Dynamic;
                s.mass        = 1.0f;
                s.restitution = 0.5f;
                auto bodyId = world.AddBody(shape, s);
                boxes.push_back({ bodyId, shape });
            }
        }
        spaceWasDown = spaceDown;

        // 物理ステップ (固定 dt)
        accumulator += dt;
        while (accumulator >= FIXED_DT)
        {
            world.Step(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        // 位置情報をテキスト表示 (Compat 層に 3D プリミティブ描画なし)
        DrawFormatString(10, 10, white, "FPS: %.1f  Bodies: %zu",
                         GetFPS(), boxes.size() + 1);
        DrawString(10, 30, "SPACE: drop 5 boxes  |  ESC: quit", white);
        DrawString(10, 50, "PhysicsWorld3D + CreateBoxShape + AddBody (ADR-0009 L2)", white);

        int y = 80;
        int shown = 0;
        for (const auto& b : boxes)
        {
            if (shown >= 20) break;
            auto pos = world.GetPosition(b.id);
            DrawFormatString(10, y, yellow, "Box %2d: (%.1f, %.1f, %.1f)",
                             shown, pos.x, pos.y, pos.z);
            y += 18;
            ++shown;
        }

        ScreenFlip();
    }

    // クリーンアップ
    for (auto& b : boxes)
    {
        world.RemoveBody(b.id);
        world.DestroyShape(b.shape);
    }
    world.DestroyShape(floorShape);

    GX_End();
    return 0;
}
