/// @file main.cpp
/// @brief 07-hello-ecs — エンティティ / コンポーネント / クエリ (ADR-0017 L1.5)
///
/// 学習ポイント / Learning points:
///   - gx::World::CreateEntity でエンティティ作成
///   - AddComponent<T> で POD コンポーネント付与 (ADR-0004)
///   - ForEach<Position, Velocity> でクエリ駆動の一括更新
///   - System を使わず直接ループでもOK (プロトタイプ用)
///
/// 100個の粒子をECSで動かして画面に描画する。

#include "GXLib.h"
#include "ECS/World.h"
#include <cstdlib>

namespace {

// POD コンポーネント (ADR-0004 必須: virtual禁止・継承禁止)
struct Position { float x, y; };
struct Velocity { float vx, vy; };
struct ColorTag { unsigned int color; };

} // anonymous namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("07 - Hello ECS");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // ECS ワールド作成 + 100 粒子をスポーン
    // =========================================================================
    gx::ecs::World world;

    constexpr int N = 100;
    for (int i = 0; i < N; ++i)
    {
        auto e = world.CreateEntity();
        auto& p = world.AddComponent<Position>(e);
        auto& v = world.AddComponent<Velocity>(e);
        auto& c = world.AddComponent<ColorTag>(e);

        p.x = static_cast<float>(std::rand() % 1280);
        p.y = static_cast<float>(std::rand() % 720);
        v.vx = (std::rand() % 400 - 200) / 100.0f;
        v.vy = (std::rand() % 400 - 200) / 100.0f;
        c.color = GetColor(std::rand() % 256, std::rand() % 256, std::rand() % 256);
    }

    unsigned int white = GetColor(255, 255, 255);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        // =====================================================================
        // クエリ: (Position, Velocity) を持つ全エンティティを更新
        // =====================================================================
        world.ForEach<Position, Velocity>(
            [dt](gx::ecs::EntityID, Position& p, Velocity& v) {
                p.x += v.vx * 200.0f * dt;
                p.y += v.vy * 200.0f * dt;

                // 壁で跳ね返る
                if (p.x < 0.0f || p.x > 1280.0f) v.vx = -v.vx;
                if (p.y < 0.0f || p.y > 720.0f)  v.vy = -v.vy;
            });

        // =====================================================================
        // 描画: (Position, ColorTag) を持つ全エンティティを描画
        // =====================================================================
        world.ForEach<Position, ColorTag>(
            [](gx::ecs::EntityID, Position& p, ColorTag& c) {
                DrawCircle(static_cast<int>(p.x), static_cast<int>(p.y),
                           6, c.color, TRUE);
            });

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        DrawFormatString(10, 30, white, "Entities: %u  Query matches: %u",
                         world.GetEntityCount(),
                         world.CountEntities<Position, ColorTag>());
        DrawString(10, 60, "ESC: quit", white);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
