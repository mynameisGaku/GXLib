/// @file main.cpp
/// @brief 10-custom-asset-type — AssetDatabase::RegisterType<T> 拡張 (ADR-0017 L2)
///
/// 学習ポイント / Learning points:
///   - AssetDatabase::RegisterType<T>(deserializer, reloadHandler) で自作アセット型を登録
///   - ホットリロード対応: ファイル書き換え時に reloadHandler が呼ばれる
///   - AssetId は FNV-1a ハッシュされた論理パスで参照 (ADR-0007)
///
/// このサンプルは「レベル設定 JSON」という自作アセット型を登録し、
/// Assets/levels/level1.json からキャラクター初期位置と敵数を読み込む。

#include "GXLib.h"
#include "Core/AssetDatabase.h"
#include <cstring>
#include <cstdlib>

// =========================================================================
// 自作アセット型: レベル設定
// =========================================================================
struct LevelConfig
{
    float  playerSpawnX = 640.0f;
    float  playerSpawnY = 360.0f;
    int    enemyCount   = 5;
    gx::String levelName = "Unnamed";
};

// 簡易 JSON パーサ (例示用 - 実装は適当)
static bool ParseLevelJSON(const char* jsonText, size_t size, LevelConfig& out)
{
    (void)size;
    // 実装は簡略化: "playerSpawnX":NUM, "enemyCount":NUM, "levelName":"STR" を探す
    if (const char* p = std::strstr(jsonText, "playerSpawnX"))
        out.playerSpawnX = static_cast<float>(std::atof(std::strchr(p, ':') + 1));
    if (const char* p = std::strstr(jsonText, "playerSpawnY"))
        out.playerSpawnY = static_cast<float>(std::atof(std::strchr(p, ':') + 1));
    if (const char* p = std::strstr(jsonText, "enemyCount"))
        out.enemyCount = std::atoi(std::strchr(p, ':') + 1);
    if (const char* p = std::strstr(jsonText, "levelName"))
    {
        const char* q = std::strchr(p, '"');
        if (q) { q = std::strchr(q + 1, '"'); if (q) {
            const char* end = std::strchr(q + 1, '"');
            if (end) out.levelName = gx::String(q + 1, end);
        }}
    }
    return true;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("10 - Custom Asset Type (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // Layer 2: AssetDatabase に LevelConfig 型を登録
    // =========================================================================
    auto& db = gx::AssetDatabase::Instance();

    gx::AssetTypeDesc<LevelConfig> desc;
    desc.extension = ".json";    // 対象拡張子
    desc.deserializer = [](const uint8_t* bytes, size_t size, void* out) -> bool {
        auto* cfg = static_cast<LevelConfig*>(out);
        return ParseLevelJSON(reinterpret_cast<const char*>(bytes), size, *cfg);
    };
    desc.reloadHandler = [](gx::AssetId id, void* asset) {
        GX_LOG_INFO("LevelConfig reloaded: AssetId=0x%llx", id);
        (void)asset;
    };
    db.RegisterType<LevelConfig>(desc);

    // ロード (ホットリロードも自動で効く)
    auto handle = db.Get<LevelConfig>(gx::AssetId("levels/level1.json"));
    LevelConfig* cfg = handle.Resolve();

    unsigned int white = GetColor(255, 255, 255);
    unsigned int green = GetColor(80, 220, 80);
    unsigned int red   = GetColor(220, 80, 80);
    unsigned int cyan  = GetColor(100, 220, 255);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        // 毎フレーム Resolve() でホットリロード後の新データに追従
        cfg = handle.Resolve();

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());

        if (cfg)
        {
            DrawString(10, 50, "--- LevelConfig loaded ---", green);
            DrawFormatString(10, 70,  cyan,  "Name           : %s", cfg->levelName.c_str());
            DrawFormatString(10, 90,  white, "Player spawn   : (%.0f, %.0f)",
                             cfg->playerSpawnX, cfg->playerSpawnY);
            DrawFormatString(10, 110, white, "Enemy count    : %d", cfg->enemyCount);

            // 設定値を実際に反映: プレイヤースポーン位置にマーカーを描く
            DrawCircle(static_cast<int>(cfg->playerSpawnX),
                       static_cast<int>(cfg->playerSpawnY), 15, green, TRUE);
            DrawString(static_cast<int>(cfg->playerSpawnX) + 20,
                       static_cast<int>(cfg->playerSpawnY) - 10, "Player", green);

            // 敵マーカーを放射状に配置
            for (int i = 0; i < cfg->enemyCount; ++i)
            {
                float a = 6.2831f * i / cfg->enemyCount;
                int ex = static_cast<int>(cfg->playerSpawnX + std::cosf(a) * 150.0f);
                int ey = static_cast<int>(cfg->playerSpawnY + std::sinf(a) * 150.0f);
                DrawCircle(ex, ey, 10, red, TRUE);
            }
        }
        else
        {
            DrawString(10, 50, "[NOT FOUND] Assets/levels/level1.json", red);
            DrawString(10, 70, "Create one with:", white);
            DrawString(10, 90,
                "{ \"levelName\": \"Stage 1\", \"playerSpawnX\": 400, \"playerSpawnY\": 300, \"enemyCount\": 8 }", cyan);
            DrawString(10, 110,
                "Then edit the file while running - reloadHandler fires automatically.", white);
        }

        DrawString(10, 180, "ESC: quit", white);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
