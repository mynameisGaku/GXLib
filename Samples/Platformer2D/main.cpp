/// @file Samples/Platformer2D/main.cpp
/// @brief GXFrameworkを使った2Dプラットフォーマー例。重力や当たり判定の基本が学べる。

#include "FrameworkApp.h"
#include "GameScene.h"

#include <format>

namespace
{

class PlatformerScene : public GXFW::GameScene
{
public:
    const char* GetName() const override { return "Platformer2D"; }

protected:
    void OnSceneEnter(GXFW::SceneContext& ctx) override
    {
        (void)ctx;
        BuildLevel();
    }

    void OnSceneUpdate(GXFW::SceneContext& ctx, float dt) override
    {
        if (dt > 0.1f)
            dt = 0.1f;

        // 左右移動。速度は定数で、dtでフレーム依存をなくす。
        float move = 0.0f;
        if (ctx.input->CheckHitKey(VK_LEFT))  move -= k_PlayerSpeed;
        if (ctx.input->CheckHitKey(VK_RIGHT)) move += k_PlayerSpeed;

        m_playerX += move * dt;

        // 重力の適用。速度→位置の順で更新する。
        m_playerVY += k_Gravity * dt;
        m_playerY += m_playerVY * dt;

        // 地面判定。足元が足場の上なら着地させる。
        // ここは初心者がつまずきやすいので、判定順と条件を丁寧に追う。
        m_onGround = false;
        for (int i = 0; i < m_platformCount; ++i)
        {
            const Platform& p = m_platforms[i];
            if (m_playerX + k_PlayerW * 0.5f < p.x || m_playerX - k_PlayerW * 0.5f > p.x + p.w)
                continue;

            float footY = m_playerY;
            if (footY >= p.y && footY <= p.y + p.h && m_playerVY >= 0.0f)
            {
                m_playerY = p.y;
                m_playerVY = 0.0f;
                m_onGround = true;
            }
        }

        // ジャンプ。地面にいるときだけ上向き速度を与える。
        if (m_onGround && ctx.input->CheckHitKey(VK_SPACE))
        {
            m_playerVY = k_JumpVel;
            m_onGround = false;
        }

        // コイン取得判定。プレイヤーとコインの距離で判定する。
        for (int i = 0; i < m_coinCount; ++i)
        {
            if (!m_coins[i].alive) continue;
            float dx = m_playerX - m_coins[i].x;
            float dy = (m_playerY - k_PlayerH * 0.5f) - m_coins[i].y;
            if (dx * dx + dy * dy < 20.0f * 20.0f)
            {
                m_coins[i].alive = false;
                m_collected++;
            }
        }

        // カメラ追従。プレイヤー中心に画面をずらす。
        m_camX = m_playerX - k_ScreenW * 0.5f;
        if (m_camX < 0.0f) m_camX = 0.0f;
    }

    void OnSceneRenderUI(GXFW::SceneContext& ctx) override
    {
        ctx.DrawBox(0, 0, k_ScreenW, k_ScreenH, GXFW::SceneContext::Color(30, 40, 60), true);

        // 足場描画。カメラ分だけ座標をずらして描く。
        for (int i = 0; i < m_platformCount; ++i)
        {
            const Platform& p = m_platforms[i];
            float sx = p.x - m_camX;
            float sy = p.y - m_camY;
            ctx.DrawBox(sx, sy, sx + p.w, sy + p.h, p.color, true);
        }

        // コイン描画。小さな円を描いて表現する。
        for (int i = 0; i < m_coinCount; ++i)
        {
            if (!m_coins[i].alive) continue;
            int cx = static_cast<int>(m_coins[i].x - m_camX);
            int cy = static_cast<int>(m_coins[i].y - m_camY);
            uint32_t coinColor = GXFW::SceneContext::Color(255, 220, 80);
            uint32_t coinEdge = GXFW::SceneContext::Color(200, 160, 50);
            ctx.DrawCircle((float)cx, (float)cy, 10.0f, coinColor, true);
            ctx.DrawCircle((float)cx, (float)cy, 10.0f, coinEdge, false);
        }

        // プレイヤー描画。四角と目で簡易的に表現する。
        float px = m_playerX - m_camX;
        float py = m_playerY - m_camY;
        float hw = k_PlayerW * 0.5f;
        float h = k_PlayerH;
        ctx.DrawBox(px - hw, py - h, px + hw, py, GXFW::SceneContext::Color(68, 136, 255), true);
        ctx.DrawBox(px - 6, py - h + 8, px - 2, py - h + 14, GXFW::SceneContext::Color(255, 255, 255), true);
        ctx.DrawBox(px + 2, py - h + 8, px + 6, py - h + 14, GXFW::SceneContext::Color(255, 255, 255), true);

        if (m_collected >= m_totalCoins)
        {
            ctx.DrawString(k_ScreenW / 2.0f - 120.0f, k_ScreenH / 2.0f - 10.0f,
                           L"ALL COINS COLLECTED!", GXFW::SceneContext::Color(255, 240, 180));
        }

        ctx.DrawString(10.0f, k_ScreenH - 30.0f,
                       std::format(L"Coins: {}/{}", m_collected, m_totalCoins),
                       GXFW::SceneContext::Color(220, 220, 220));
    }

private:
    struct Platform
    {
        float x, y, w, h;
        uint32_t color;
    };

    struct Coin
    {
        float x, y;
        bool alive = true;
    };

    static constexpr float k_ScreenW = 1280.0f;
    static constexpr float k_ScreenH = 720.0f;
    static constexpr float k_Gravity = 800.0f;
    static constexpr float k_JumpVel = -450.0f;
    static constexpr float k_PlayerSpeed = 300.0f;
    static constexpr float k_PlayerW = 28.0f;
    static constexpr float k_PlayerH = 40.0f;
    static constexpr int k_MaxPlatforms = 16;
    static constexpr int k_MaxCoins = 16;

    float m_playerX = 100.0f;
    float m_playerY = 400.0f;
    float m_playerVY = 0.0f;
    bool m_onGround = false;
    int m_collected = 0;
    int m_totalCoins = 0;

    float m_camX = 0.0f;
    float m_camY = 0.0f;

    Platform m_platforms[k_MaxPlatforms];
    int m_platformCount = 0;
    Coin m_coins[k_MaxCoins];
    int m_coinCount = 0;

    void AddPlatform(float x, float y, float w, float h, uint32_t color)
    {
        if (m_platformCount >= k_MaxPlatforms) return;
        Platform& p = m_platforms[m_platformCount++];
        p.x = x; p.y = y; p.w = w; p.h = h; p.color = color;
    }

    void AddCoin(float x, float y)
    {
        if (m_coinCount >= k_MaxCoins) return;
        Coin& c = m_coins[m_coinCount++];
        c.x = x; c.y = y; c.alive = true;
    }

    void BuildLevel()
    {
        m_platformCount = 0;
        m_coinCount = 0;
        m_collected = 0;

        uint32_t ground = GXFW::SceneContext::Color(60, 90, 60);
        uint32_t plat = GXFW::SceneContext::Color(80, 80, 120);

        AddPlatform(-200.0f, 500.0f, 3000.0f, 40.0f, ground);
        AddPlatform(200.0f, 380.0f, 180.0f, 20.0f, plat);
        AddPlatform(450.0f, 300.0f, 160.0f, 20.0f, plat);
        AddPlatform(700.0f, 350.0f, 200.0f, 20.0f, plat);
        AddPlatform(950.0f, 260.0f, 140.0f, 20.0f, plat);
        AddPlatform(1150.0f, 200.0f, 180.0f, 20.0f, plat);
        AddPlatform(1400.0f, 320.0f, 200.0f, 20.0f, plat);
        AddPlatform(1650.0f, 400.0f, 160.0f, 20.0f, plat);
        AddPlatform(1900.0f, 280.0f, 220.0f, 20.0f, plat);
        AddPlatform(2200.0f, 350.0f, 180.0f, 20.0f, plat);
        AddPlatform(2450.0f, 220.0f, 160.0f, 20.0f, plat);

        AddCoin(280.0f, 350.0f);
        AddCoin(520.0f, 270.0f);
        AddCoin(790.0f, 320.0f);
        AddCoin(1010.0f, 230.0f);
        AddCoin(1230.0f, 170.0f);
        AddCoin(1490.0f, 290.0f);
        AddCoin(1720.0f, 370.0f);
        AddCoin(2000.0f, 250.0f);
        AddCoin(2280.0f, 320.0f);
        AddCoin(2520.0f, 190.0f);

        m_totalCoins = m_coinCount;
        m_playerX = 100.0f;
        m_playerY = 400.0f;
        m_playerVY = 0.0f;
    }
};

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    GXFW::FrameworkApp app;
    GXFW::AppConfig config;
    config.title = L"GXLib Sample: Platformer2D";
    config.width = 1280;
    config.height = 720;
    config.enableDebug = true;

    if (!app.Initialize(config))
        return -1;

    app.SetScene(std::make_unique<PlatformerScene>());
    app.Run();
    app.Shutdown();
    return 0;
}
