/// @file Samples/Shooting2D/main.cpp
/// @brief GXFrameworkを使った2Dシューティング例。入力・更新・描画の基本が分かる。

#include "FrameworkApp.h"
#include "GameScene.h"

#include <format>

namespace
{

class ShootingScene : public GXFW::GameScene
{
public:
    const char* GetName() const override { return "Shooting2D"; }

protected:
    void OnSceneEnter(GXFW::SceneContext& ctx) override
    {
        (void)ctx;
        ResetGame();
    }

    void OnSceneUpdate(GXFW::SceneContext& ctx, float dt) override
    {
        if (dt > 0.1f)
            dt = 0.1f;

        if (m_gameOver)
        {
            if (ctx.input->CheckHitKey(VK_RETURN))
                ResetGame();
            return;
        }

        m_totalTime += dt;

        // プレイヤー移動。キー入力で左右に動かす。
        float speed = 400.0f * dt;
        if (ctx.input->CheckHitKey(VK_LEFT))  m_playerX -= speed;
        if (ctx.input->CheckHitKey(VK_RIGHT)) m_playerX += speed;
        if (m_playerX < m_playerSize) m_playerX = m_playerSize;
        if (m_playerX > k_ScreenW - m_playerSize) m_playerX = k_ScreenW - m_playerSize;

        // 発射処理。クールダウンと弾数上限で連射を制御する。
        m_shootCooldown -= dt;
        if (ctx.input->CheckHitKey(VK_SPACE) && m_shootCooldown <= 0.0f
            && m_bulletCount < k_MaxBullets)
        {
            Bullet& b = m_bullets[m_bulletCount++];
            b.x = m_playerX;
            b.y = m_playerY - m_playerSize;
            b.alive = true;
            m_shootCooldown = 0.12f;
        }

        // 弾の更新。上に進め、画面外なら非表示にする。
        for (int i = 0; i < m_bulletCount; ++i)
        {
            if (!m_bullets[i].alive) continue;
            m_bullets[i].y -= 600.0f * dt;
            if (m_bullets[i].y < -10.0f)
                m_bullets[i].alive = false;
        }

        // 敵の出現。時間経過で少しずつ難しくする。
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

        // 敵の更新。画面外に出たらゲームオーバーにする。
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

        // 当たり判定（弾 vs 敵）。当たれば両方消してスコア加算。
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

    void OnSceneRenderUI(GXFW::SceneContext& ctx) override
    {
        // 背景。まず画面全体を塗りつぶす。
        ctx.DrawBox(0, 0, k_ScreenW, k_ScreenH, GXFW::SceneContext::Color(10, 10, 30), true);

        // プレイヤー表示。四角で簡易的に描く。
        ctx.DrawBox(m_playerX - m_playerSize, m_playerY - m_playerSize,
                    m_playerX + m_playerSize, m_playerY + m_playerSize,
                    GXFW::SceneContext::Color(80, 200, 255), true);

        // 弾の表示。小さい四角を描く。
        for (int i = 0; i < m_bulletCount; ++i)
        {
            if (!m_bullets[i].alive) continue;
            ctx.DrawBox(m_bullets[i].x - 3.0f, m_bullets[i].y - 6.0f,
                        m_bullets[i].x + 3.0f, m_bullets[i].y + 6.0f,
                        GXFW::SceneContext::Color(255, 255, 68), true);
        }

        // 敵の表示。円で描く。
        for (int i = 0; i < m_enemyCount; ++i)
        {
            if (!m_enemies[i].alive) continue;
            ctx.DrawCircle(m_enemies[i].x, m_enemies[i].y, m_enemies[i].radius,
                           GXFW::SceneContext::Color(255, 120, 80), true);
        }

        // スコア表示。数字を文字列として描画する。
        ctx.DrawString(10.0f, 10.0f, std::format(L"Score: {}", m_score),
                       GXFW::SceneContext::Color(255, 255, 255));

        if (m_gameOver)
        {
            ctx.DrawString(k_ScreenW / 2.0f - 80.0f, k_ScreenH / 2.0f - 20.0f,
                           L"GAME OVER", GXFW::SceneContext::Color(255, 200, 200));
            ctx.DrawString(k_ScreenW / 2.0f - 130.0f, k_ScreenH / 2.0f + 15.0f,
                           L"Press Enter to Restart", GXFW::SceneContext::Color(200, 200, 255));
        }

        ctx.DrawString(10.0f, k_ScreenH - 30.0f,
                       L"Arrow: Move  Space: Shoot", GXFW::SceneContext::Color(150, 150, 150));
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

    unsigned int m_rngState = 12345;

    // 円と矩形の簡易当たり判定。
    // 円中心から矩形までの最短距離が半径より小さいかで判定する。
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

    // 乱数（Xorshift）。高速な簡易乱数で敵の位置や速度に使う。
    float RandFloat(float minVal, float maxVal)
    {
        m_rngState ^= m_rngState << 13;
        m_rngState ^= m_rngState >> 17;
        m_rngState ^= m_rngState << 5;
        float t = (m_rngState & 0x7FFFFFFF) / (float)0x7FFFFFFF;
        return minVal + t * (maxVal - minVal);
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

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    GXFW::FrameworkApp app;
    GXFW::AppConfig config;
    config.title = L"GXLib Sample: Shooting2D";
    config.width = 1280;
    config.height = 720;
    config.enableDebug = true;

    if (!app.Initialize(config))
        return -1;

    app.SetScene(std::make_unique<ShootingScene>());
    app.Run();
    app.Shutdown();
    return 0;
}
