/// @file Samples/Shooting2D/main.cpp
/// @brief 2Dシューティング。左右移動と射撃を体験するサンプル。
#include "GXEasy.h"

#include <format>
#include <string>
#include <random>

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

class ShootingApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: Shooting2D";
        config.width = 1280;
        config.height = 720;
        config.bgR = 8;
        config.bgG = 8;
        config.bgB = 20;
        return config;
    }

    void Start() override
    {
        ResetGame();
    }

    void Update(float dt) override
    {
        if (dt > 0.1f) dt = 0.1f;

        if (m_gameOver)
        {
            if (CheckHitKey(KEY_INPUT_RETURN))
                ResetGame();
            return;
        }

        m_totalTime += dt;

        // プレイヤー移動（左右）
        float speed = 400.0f * dt;
        if (CheckHitKey(KEY_INPUT_LEFT))  m_playerX -= speed;
        if (CheckHitKey(KEY_INPUT_RIGHT)) m_playerX += speed;
        if (m_playerX < m_playerSize) m_playerX = m_playerSize;
        if (m_playerX > k_ScreenW - m_playerSize) m_playerX = k_ScreenW - m_playerSize;

        // 射撃クールダウン更新
        m_shootCooldown -= dt;
        if (CheckHitKey(KEY_INPUT_SPACE) && m_shootCooldown <= 0.0f
            && m_bulletCount < k_MaxBullets)
        {
            Bullet& b = m_bullets[m_bulletCount++];
            b.x = m_playerX;
            b.y = m_playerY - m_playerSize;
            b.alive = true;
            m_shootCooldown = 0.12f;
        }

        // 弾の更新
        for (int i = 0; i < m_bulletCount; ++i)
        {
            if (!m_bullets[i].alive) continue;
            m_bullets[i].y -= 600.0f * dt;
            if (m_bullets[i].y < -10.0f)
                m_bullets[i].alive = false;
        }

        // 敵の出現
        m_spawnTimer -= dt;
        if (m_spawnTimer <= 0.0f)
        {
            if (m_enemyCount < k_MaxEnemies)
            {
                Enemy& e = m_enemies[m_enemyCount++];
                e.x = RandFloat(40.0f, k_ScreenW - 40.0f);
                e.y = -30.0f;
                e.radius = RandFloat(12.0f, 28.0f);
                e.speed = RandFloat(100.0f, 250.0f + m_totalTime * 3.0f);
                e.alive = true;
            }
            m_spawnTimer = m_spawnInterval;
            if (m_spawnInterval > 0.3f) m_spawnInterval -= 0.02f;
        }

        // 敵の更新
        for (int i = 0; i < m_enemyCount; ++i)
        {
            if (!m_enemies[i].alive) continue;
            m_enemies[i].y += m_enemies[i].speed * dt;
            if (m_enemies[i].y > k_ScreenH + 40.0f)
            {
                m_enemies[i].alive = false;
                m_gameOver = true;
            }
        }

        // 当たり判定（弾 vs 敵）
        for (int i = 0; i < m_bulletCount; ++i)
        {
            if (!m_bullets[i].alive) continue;
            for (int j = 0; j < m_enemyCount; ++j)
            {
                if (!m_enemies[j].alive) continue;
                if (HitCircleRect(m_enemies[j].x, m_enemies[j].y, m_enemies[j].radius,
                                  m_bullets[i].x - 3.0f, m_bullets[i].y - 6.0f, 6.0f, 12.0f))
                {
                    m_bullets[i].alive = false;
                    m_enemies[j].alive = false;
                    m_score += 10;
                    break;
                }
            }
        }
    }

    void Draw() override
    {
        DrawBox(0, 0, static_cast<int>(k_ScreenW), static_cast<int>(k_ScreenH),
                GetColor(10, 10, 30), TRUE);

        // プレイヤー描画
        DrawBox(static_cast<int>(m_playerX - m_playerSize), static_cast<int>(m_playerY - m_playerSize),
                static_cast<int>(m_playerX + m_playerSize), static_cast<int>(m_playerY + m_playerSize),
                GetColor(80, 200, 255), TRUE);

        // 弾
        for (int i = 0; i < m_bulletCount; ++i)
        {
            if (!m_bullets[i].alive) continue;
            DrawBox(static_cast<int>(m_bullets[i].x - 3.0f), static_cast<int>(m_bullets[i].y - 6.0f),
                    static_cast<int>(m_bullets[i].x + 3.0f), static_cast<int>(m_bullets[i].y + 6.0f),
                    GetColor(255, 255, 68), TRUE);
        }

        // 敵
        for (int i = 0; i < m_enemyCount; ++i)
        {
            if (!m_enemies[i].alive) continue;
            DrawCircle(static_cast<int>(m_enemies[i].x), static_cast<int>(m_enemies[i].y),
                       static_cast<int>(m_enemies[i].radius), GetColor(255, 120, 80), TRUE);
        }

        // スコア
        const TString scoreText = FormatT(TEXT("Score: {}"), m_score);
        DrawString(10, 10, scoreText.c_str(), GetColor(255, 255, 255));

        if (m_gameOver)
        {
            DrawString(static_cast<int>(k_ScreenW / 2 - 80), static_cast<int>(k_ScreenH / 2 - 20),
                       TEXT("GAME OVER"), GetColor(255, 200, 200));
            DrawString(static_cast<int>(k_ScreenW / 2 - 130), static_cast<int>(k_ScreenH / 2 + 15),
                       TEXT("Press Enter to Restart"), GetColor(200, 200, 255));
        }

        DrawString(10, static_cast<int>(k_ScreenH - 30),
                   TEXT("Arrow: Move  Space: Shoot"), GetColor(150, 150, 150));
    }

private:
    struct Bullet
    {
        float x = 0.0f;
        float y = 0.0f;
        bool alive = false;
    };

    struct Enemy
    {
        float x = 0.0f;
        float y = 0.0f;
        float radius = 0.0f;
        float speed = 0.0f;
        bool alive = false;
    };

    static constexpr float k_ScreenW = 1280.0f;
    static constexpr float k_ScreenH = 720.0f;
    static constexpr int k_MaxBullets = 64;
    static constexpr int k_MaxEnemies = 32;

    float m_playerX = k_ScreenW / 2.0f;
    float m_playerY = k_ScreenH - 60.0f;
    float m_playerSize = 20.0f;
    Bullet m_bullets[k_MaxBullets];
    int m_bulletCount = 0;
    Enemy m_enemies[k_MaxEnemies];
    int m_enemyCount = 0;
    int m_score = 0;
    bool m_gameOver = false;
    float m_spawnTimer = 0.0f;
    float m_spawnInterval = 1.2f;
    float m_shootCooldown = 0.0f;
    float m_totalTime = 0.0f;

    // 円と矩形の当たり判定（弾 vs 敵）
    static bool HitCircleRect(float cx, float cy, float cr,
                              float rx, float ry, float rw, float rh)
    {
        float nearX = cx;
        float nearY = cy;
        if (nearX < rx) nearX = rx;
        if (nearX > rx + rw) nearX = rx + rw;
        if (nearY < ry) nearY = ry;
        if (nearY > ry + rh) nearY = ry + rh;
        float dx = cx - nearX;
        float dy = cy - nearY;
        return (dx * dx + dy * dy) < (cr * cr);
    }

    std::mt19937 m_rng{ std::random_device{}() };

    float RandFloat(float minVal, float maxVal)
    {
        std::uniform_real_distribution<float> dist(minVal, maxVal);
        return dist(m_rng);
    }

    void ResetGame()
    {
        m_playerX = k_ScreenW / 2.0f;
        m_bulletCount = 0;
        m_enemyCount = 0;
        m_score = 0;
        m_gameOver = false;
        m_spawnTimer = 0.0f;
        m_spawnInterval = 1.2f;
        m_shootCooldown = 0.0f;
        m_totalTime = 0.0f;
    }
};

GX_EASY_APP(ShootingApp)

