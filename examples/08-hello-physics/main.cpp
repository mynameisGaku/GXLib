/// @file main.cpp
/// @brief 08-hello-physics — 2D剛体とレイキャスト (ADR-0017 L1.5)
///
/// 学習ポイント / Learning points:
///   - gx::PhysicsWorld2D の初期化と毎フレーム Step
///   - AddBody でボックス剛体を落下させる
///   - Raycast でマウス位置から壁までの距離を測る
///   - 固定タイムステップ (ADR-0009: variable_timestep_in_physics_solver 禁止)
///
/// 注: Physics は Layer 1.5 (DXLib に相当機能なし)。
///     Compat 層ラッパーは現状なし、PhysicsWorld2D を直接使う。

#include "GXLib.h"
#include "Physics/PhysicsWorld2D.h"
#include "Physics/RigidBody2D.h"
#include "Physics/PhysicsShape.h"
#include <cstdlib>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("08 - Hello Physics");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // PhysicsWorld2D: 固定タイムステップ 60Hz (ADR-0009 準拠)
    // =========================================================================
    gx::PhysicsWorld2D world;
    world.SetGravity({ 0.0f, 980.0f });   // 下向き重力 (px/s^2)
    world.SetTimestep(1.0f / 60.0f);

    // 床 (静的ボディ)
    gx::BodyDesc2D floorDesc;
    floorDesc.position = { 640.0f, 680.0f };
    floorDesc.isStatic = true;
    floorDesc.shape    = gx::PhysicsShape2D::Box({ 600.0f, 20.0f });
    auto floorBody = world.AddBody(floorDesc);

    // SPACE で落下させる箱を10個スポーン
    gx::Vector<gx::BodyHandle2D> boxes;

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int green  = GetColor(80, 220, 80);
    unsigned int red    = GetColor(220, 80, 80);
    unsigned int yellow = GetColor(255, 220, 50);
    bool spaceWasDown = false;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        // SPACE: 箱を10個ドロップ
        bool spaceDown = (CheckHitKey(KEY_INPUT_SPACE) != 0);
        if (spaceDown && !spaceWasDown)
        {
            for (int i = 0; i < 10; ++i)
            {
                gx::BodyDesc2D d;
                d.position = { 100.0f + static_cast<float>(std::rand() % 1000), 50.0f };
                d.isStatic = false;
                d.mass     = 1.0f;
                d.restitution = 0.4f;
                d.shape = gx::PhysicsShape2D::Box({ 20.0f, 20.0f });
                boxes.push_back(world.AddBody(d));
            }
        }
        spaceWasDown = spaceDown;

        // 物理ステップ (固定 dt アキュムレータは内部で処理)
        world.Step(dt);

        // =====================================================================
        // マウス位置からレイキャスト (下向き)
        // =====================================================================
        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        gx::RaycastHit2D hit;
        bool hasHit = world.Raycast(
            { static_cast<float>(mx), static_cast<float>(my) },
            { 0.0f, 1.0f },           // 下向き
            2000.0f,
            hit);

        // 床 (green box)
        DrawBox(640 - 600, 680 - 20, 640 + 600, 680 + 20, green, TRUE);

        // 動的ボックスを描画
        for (auto h : boxes)
        {
            auto* body = world.GetBody(h);
            if (!body) continue;
            auto p = body->GetPosition();
            DrawBox(static_cast<int>(p.x) - 10, static_cast<int>(p.y) - 10,
                    static_cast<int>(p.x) + 10, static_cast<int>(p.y) + 10,
                    yellow, TRUE);
        }

        // Raycast レイと当たり点
        DrawLine(mx, my, mx, my + 400, white);
        if (hasHit)
        {
            DrawCircle(static_cast<int>(hit.point.x), static_cast<int>(hit.point.y),
                       6, red, TRUE);
            DrawFormatString(mx + 12, my - 10, red, "Hit @ %.0f", hit.distance);
        }

        DrawFormatString(10, 10, white, "FPS: %.1f  Bodies: %zu",
                         GetFPS(), boxes.size() + 1);
        DrawString(10, 30, "SPACE: drop 10 boxes   |   ESC: quit", white);
        DrawString(10, 50, "Mouse cursor casts a downward ray.", white);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
