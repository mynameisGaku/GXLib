/// @file Samples/LuaShowcase/main.cpp
/// @brief Lua scripting demo.
///
/// Lua スクリプトでプレイヤーの移動ロジックを制御するサンプル。
/// 矢印キーで青い矩形を動かす。移動ロジックは全て Lua 側で記述。
/// C++ は Lua から座標を取得して描画する。
///
/// Controls:
///   Arrow keys - Move player (via Lua)
///   ESC        - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Script/ScriptEngine.h"

#include <cmath>
#include <Windows.h>

class LuaShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Lua Scripting";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 20; config.bgG = 25; config.bgB = 40;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();

#ifdef GX_ENABLE_LUA
        if (m_script.Initialize())
        {
            m_script.SetContext(&ctx.spriteBatch, &ctx.renderer3D.GetTextureManager(), &ctx.inputManager);

            m_script.ExecuteString(R"lua(
                -- Player state managed entirely by Lua
                player = { x = 640, y = 360, size = 32, speed = 250 }

                function OnUpdate(dt)
                    if IsKeyDown(KEY_UP)    then player.y = player.y - player.speed * dt end
                    if IsKeyDown(KEY_DOWN)  then player.y = player.y + player.speed * dt end
                    if IsKeyDown(KEY_LEFT)  then player.x = player.x - player.speed * dt end
                    if IsKeyDown(KEY_RIGHT) then player.x = player.x + player.speed * dt end

                    -- Clamp to screen
                    player.x = Clamp(player.x, player.size, 1280 - player.size)
                    player.y = Clamp(player.y, player.size + 60, 720 - player.size)

                    -- Export to C++ via globals
                    _px = player.x
                    _py = player.y
                    _ps = player.size
                end
            )lua");

            m_luaReady = true;
        }
#endif
    }

    void Update(float dt) override
    {
#ifdef GX_ENABLE_LUA
        if (m_luaReady)
            m_script.CallFunction("OnUpdate", dt);
#endif
        m_time += dt;
    }

    void Draw() override
    {
        float px = 640.0f, py = 360.0f, psize = 32.0f;

#ifdef GX_ENABLE_LUA
        if (m_luaReady)
        {
            px = m_script.GetGlobalFloat("_px", 640.0f);
            py = m_script.GetGlobalFloat("_py", 360.0f);
            psize = m_script.GetGlobalFloat("_ps", 32.0f);
        }
#endif

        // 背景グリッド
        unsigned int gridColor = GetColor(40, 50, 70);
        for (int x = 0; x < 1280; x += 64)
            DrawLine(x, 60, x, 720, gridColor);
        for (int y = 60; y < 720; y += 64)
            DrawLine(0, y, 1280, y, gridColor);

        // プレイヤー矩形（Lua制御位置に描画）
        int hs = static_cast<int>(psize / 2);
        int ipx = static_cast<int>(px), ipy = static_cast<int>(py);
        DrawBox(ipx - hs, ipy - hs, ipx + hs, ipy + hs, GetColor(50, 140, 255), TRUE);
        DrawBox(ipx - hs, ipy - hs, ipx + hs, ipy + hs, GetColor(100, 200, 255), FALSE);

        // プレイヤー中心の十字マーク
        DrawLine(ipx - hs + 4, ipy, ipx + hs - 4, ipy, GetColor(200, 230, 255));
        DrawLine(ipx, ipy - hs + 4, ipx, ipy + hs - 4, GetColor(200, 230, 255));

        // ラベル
        const TString label = FormatT(TEXT("Player (Lua)"));
        DrawString(ipx - 30, ipy - hs - 18, label.c_str(), GetColor(150, 200, 255));

        // --- HUD ---
        DrawString(10, 5, TEXT("Lua Scripting Demo"), GetColor(255, 255, 255));

        const TString luaStatus = m_luaReady
            ? FormatT(TEXT("Lua: Active  |  Player: ({:.0f}, {:.0f})"), px, py)
            : TString(TEXT("Lua: Not available"));
        DrawString(10, 30, luaStatus.c_str(),
            m_luaReady ? GetColor(80, 255, 80) : GetColor(255, 80, 80));

        DrawString(10, 695, TEXT("Arrow keys = Move player (controlled by Lua script)"),
            GetColor(136, 136, 136));
    }

private:
    GX::ScriptEngine m_script;
    bool m_luaReady = false;
    float m_time = 0.0f;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    LuaShowcaseApp app;
    return GXEasy::Run(app, app.GetConfig());
}
