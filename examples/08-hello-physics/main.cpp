/// @file main.cpp
/// @brief 08-hello-physics — 2D剛体とレイキャスト (ADR-0017 L1.5)
///
/// 学習ポイント / Learning points:
///   - gx::PhysicsWorld2D の初期化と毎フレーム Step
///   - AddBody() で RigidBody2D* を取得し、プロパティを直接設定
///   - ColliderShape2D で AABB (ボックス) 形状を定義
///   - Raycast(origin, dir, maxDist, &outBody, &outPoint) で光線判定
///   - 固定タイムステップ (ADR-0009: variable_timestep_in_physics_solver 禁止)
///
/// 注: Physics は Layer 1.5 (DXLib に相当機能なし)。
///     Compat 層ラッパーは現状なし、PhysicsWorld2D を直接使う。

#include "GXLib.h"
#include "Physics/PhysicsWorld2D.h"
#include "Physics/RigidBody2D.h"
#include <cstdlib>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("08 - Hello Physics");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // PhysicsWorld2D: 重力を設定し、毎フレーム固定 dt で Step する
    // =========================================================================
    gx::PhysicsWorld2D world;
    world.SetGravity({ 0.0f, 980.0f });

    constexpr float FIXED_DT = 1.0f / 60.0f;
    float accumulator = 0.0f;

    // 床 (静的ボディ): AddBody() でポインタ取得→プロパティ直接設定
    auto* floor = world.AddBody();
    floor->position    = { 640.0f, 680.0f };
    floor->bodyType    = gx::BodyType2D::Static;
    floor->shape.type  = gx::ShapeType2D::AABB;
    floor->shape.halfExtents = { 600.0f, 20.0f };

    // SPACE で落下させる箱のポインタ配列
    gx::Vector<gx::RigidBody2D*> boxes;

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
                auto* b = world.AddBody();
                b->position    = { 100.0f + static_cast<float>(std::rand() % 1000), 50.0f };
                b->bodyType    = gx::BodyType2D::Dynamic;
                b->mass        = 1.0f;
                b->restitution = 0.4f;
                b->shape.type  = gx::ShapeType2D::AABB;
                b->shape.halfExtents = { 10.0f, 10.0f };
                boxes.push_back(b);
            }
        }
        spaceWasDown = spaceDown;

        // 物理ステップ (固定 dt アキュムレータ — ADR-0009 準拠)
        accumulator += dt;
        while (accumulator >= FIXED_DT)
        {
            world.Step(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        // =====================================================================
        // マウス位置からレイキャスト (下向き)
        // =====================================================================
        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        gx::RigidBody2D* hitBody = nullptr;
        gx::Vector2 hitPoint;
        bool hasHit = world.Raycast(
            { static_cast<float>(mx), static_cast<float>(my) },
            { 0.0f, 1.0f },
            2000.0f,
            &hitBody, &hitPoint);

        // 床 (green box)
        DrawBox(640 - 600, 680 - 20, 640 + 600, 680 + 20, green, TRUE);

        // 動的ボックスを描画 (ポインタ直接アクセス)
        for (auto* b : boxes)
        {
            int px = static_cast<int>(b->position.x);
            int py = static_cast<int>(b->position.y);
            int hw = static_cast<int>(b->shape.halfExtents.x);
            int hh = static_cast<int>(b->shape.halfExtents.y);
            DrawBox(px - hw, py - hh, px + hw, py + hh, yellow, TRUE);
        }

        // Raycast レイと当たり点
        DrawLine(mx, my, mx, my + 400, white);
        if (hasHit)
        {
            DrawCircle(static_cast<int>(hitPoint.x), static_cast<int>(hitPoint.y),
                       6, red, TRUE);
            float dist = hitPoint.y - static_cast<float>(my);
            DrawFormatString(mx + 12, my - 10, red, "Hit @ %.0f", dist);
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
