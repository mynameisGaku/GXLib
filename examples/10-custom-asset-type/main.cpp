/// @file main.cpp
/// @brief 10-custom-asset-type — AssetDatabase でカスタムファイル検出 (ADR-0017 L2)
///
/// 学習ポイント / Learning points:
///   - AssetDatabase::Instance().Initialize(root) でアセットフォルダをスキャン
///   - FindAsset(relativePath) で特定ファイルを検索
///   - DetectChanges() でホットリロード検出 (dirty フラグ)
///   - AssetEntry の fullPath / dirty を使ってファイルを読み直す
///
/// このサンプルは「レベル設定 JSON」を手動パースし、
/// Assets/levels/level1.json から情報を読み込む。
/// ファイルを外部エディタで変更すると自動で再読み込みされる。

#include "GXLib.h"
#include "Core/AssetDatabase.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>

struct LevelConfig
{
    float  playerSpawnX = 640.0f;
    float  playerSpawnY = 360.0f;
    int    enemyCount   = 5;
    gx::String levelName;
};

static bool ParseLevelJSON(const char* jsonText, size_t size, LevelConfig& out)
{
    (void)size;
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
            if (end) out.levelName = gx::String(q + 1, static_cast<size_t>(end - (q + 1)));
        }}
    }
    return true;
}

static bool LoadConfigFromFile(const gx::String& path, LevelConfig& out)
{
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0) { std::fclose(f); return false; }
    gx::Vector<char> buf(static_cast<size_t>(sz + 1));
    std::fread(buf.data(), 1, static_cast<size_t>(sz), f);
    buf[static_cast<size_t>(sz)] = '\0';
    std::fclose(f);
    return ParseLevelJSON(buf.data(), static_cast<size_t>(sz), out);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("10 - Custom Asset Type (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // AssetDatabase: アセットフォルダをスキャンして JSON を検出
    // =========================================================================
    auto& db = gx::AssetDatabase::Instance();
    db.Initialize("Assets");

    LevelConfig cfg;
    bool loaded = false;

    const gx::AssetEntry* entry = db.FindAsset("levels/level1.json");
    if (entry)
        loaded = LoadConfigFromFile(entry->fullPath, cfg);

    unsigned int white = GetColor(255, 255, 255);
    unsigned int green = GetColor(80, 220, 80);
    unsigned int red   = GetColor(220, 80, 80);
    unsigned int cyan  = GetColor(100, 220, 255);

    int reloadCount = 0;
    int framesSinceCheck = 0;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        // 毎 60 フレーム (≒1秒) にファイル変更を検出
        if (++framesSinceCheck >= 60)
        {
            framesSinceCheck = 0;
            int changed = db.DetectChanges();
            if (changed > 0)
            {
                entry = db.FindAsset("levels/level1.json");
                if (entry && entry->dirty)
                {
                    loaded = LoadConfigFromFile(entry->fullPath, cfg);
                    ++reloadCount;
                }
            }
        }

        DrawFormatString(10, 10, white, "FPS: %.1f   Reloads: %d", GetFPS(), reloadCount);

        if (loaded)
        {
            DrawString(10, 50, "--- LevelConfig loaded ---", green);
            DrawFormatString(10, 70,  cyan,  "Name           : %s", cfg.levelName.c_str());
            DrawFormatString(10, 90,  white, "Player spawn   : (%.0f, %.0f)",
                             cfg.playerSpawnX, cfg.playerSpawnY);
            DrawFormatString(10, 110, white, "Enemy count    : %d", cfg.enemyCount);

            DrawCircle(static_cast<int>(cfg.playerSpawnX),
                       static_cast<int>(cfg.playerSpawnY), 15, green, TRUE);
            DrawString(static_cast<int>(cfg.playerSpawnX) + 20,
                       static_cast<int>(cfg.playerSpawnY) - 10, "Player", green);

            for (int i = 0; i < cfg.enemyCount; ++i)
            {
                float a = 6.2831f * static_cast<float>(i) / static_cast<float>(cfg.enemyCount);
                int ex = static_cast<int>(cfg.playerSpawnX + std::cos(a) * 150.0f);
                int ey = static_cast<int>(cfg.playerSpawnY + std::sin(a) * 150.0f);
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
                "Then edit the file while running - auto-reloaded every second.", white);
        }

        DrawString(10, 180, "ESC: quit", white);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
