/// @file Samples/Platformer2D/main.cpp
/// @brief 2Dプラットフォーマー。ジャンプでコインを集めるサンプル。
#include "GXEasy.h"

#include <format>
#include <string>

#ifdef UNICODE
using TChar = wchar_t;
#else
using TChar = char;
#endif

using TString = std::basic_string<TChar>;

template <class... Args>
TString FormatT(const TChar* fmt, Args&&... args)
{
#ifdef UNICODE
    return std::vformat(fmt, std::make_wformat_args(std::forward<Args>(args)...));
#else
    return std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
#endif
}

class PlatformerApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: Platformer2D";
        config.width = 1280;
        config.height = 720;
        config.bgR = 18;
        config.bgG = 22;
        config.bgB = 32;
        return config;
    }

    void Start() override
    {
        BuildLevel();
    }

    void Update(float dt) override
    {
        if (dt > 0.1f) dt = 0.1f;

        // 左右移動
        float move = 0.0f;
        if (CheckHitKey(KEY_INPUT_LEFT))  move -= k_PlayerSpeed;
        if (CheckHitKey(KEY_INPUT_RIGHT)) move += k_PlayerSpeed;
        m_playerX += move * dt;

        // 重力
        m_playerVY += k_Gravity * dt;
        m_playerY += m_playerVY * dt;

        // 床との当たり判定（接地）
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

        // ジャンプ
        if (m_onGround && CheckHitKey(KEY_INPUT_SPACE))
        {
            m_playerVY = k_JumpVel;
            m_onGround = false;
        }

        // コイン取得
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

        // カメラ追従（スクロール）
        m_camX = m_playerX - k_ScreenW * 0.5f;
        if (m_camX < 0.0f) m_camX = 0.0f;
    }

    void Draw() override
    {
        DrawBox(0, 0, static_cast<int>(k_ScreenW), static_cast<int>(k_ScreenH),
                GetColor(30, 40, 60), TRUE);

        // 地形
        for (int i = 0; i < m_platformCount; ++i)
        {
            const Platform& p = m_platforms[i];
            float sx = p.x - m_camX;
            float sy = p.y - m_camY;
            DrawBox(static_cast<int>(sx), static_cast<int>(sy),
                    static_cast<int>(sx + p.w), static_cast<int>(sy + p.h),
                    p.color, TRUE);
        }

        // コイン
        for (int i = 0; i < m_coinCount; ++i)
        {
            if (!m_coins[i].alive) continue;
            int cx = static_cast<int>(m_coins[i].x - m_camX);
            int cy = static_cast<int>(m_coins[i].y - m_camY);
            unsigned int coinColor = GetColor(255, 220, 80);
            unsigned int coinEdge  = GetColor(200, 160, 50);
            DrawCircle(cx, cy, 10, coinColor, TRUE);
            DrawCircle(cx, cy, 10, coinEdge, FALSE);
        }

        // プレイヤー描画
        float px = m_playerX - m_camX;
        float py = m_playerY - m_camY;
        float hw = k_PlayerW * 0.5f;
        float h = k_PlayerH;
        DrawBox(static_cast<int>(px - hw), static_cast<int>(py - h),
                static_cast<int>(px + hw), static_cast<int>(py),
                GetColor(68, 136, 255), TRUE);
        DrawBox(static_cast<int>(px - 6), static_cast<int>(py - h + 8),
                static_cast<int>(px - 2), static_cast<int>(py - h + 14),
                GetColor(255, 255, 255), TRUE);
        DrawBox(static_cast<int>(px + 2), static_cast<int>(py - h + 8),
                static_cast<int>(px + 6), static_cast<int>(py - h + 14),
                GetColor(255, 255, 255), TRUE);

        if (m_collected >= m_totalCoins)
        {
            DrawString(static_cast<int>(k_ScreenW / 2 - 120), static_cast<int>(k_ScreenH / 2 - 10),
                       TEXT("ALL COINS COLLECTED!"), GetColor(255, 240, 180));
        }

        const TString coinText = FormatT(TEXT("Coins: {}/{}"), m_collected, m_totalCoins);
        DrawString(10, static_cast<int>(k_ScreenH - 30),
                   coinText.c_str(), GetColor(220, 220, 220));
    }

private:
    struct Platform
    {
        float x, y, w, h;
        unsigned int color;
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

    void AddPlatform(float x, float y, float w, float h, unsigned int color)
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

        unsigned int ground = GetColor(60, 90, 60);
        unsigned int plat = GetColor(80, 80, 120);

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

GX_EASY_APP(PlatformerApp)


