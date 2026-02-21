/// @file Samples/Particle2DShowcase/main.cpp
/// @brief 2Dパーティクルシステムデモ
///
/// 4種のエフェクト（爆発、煙、軌跡、花火）を同時表示する。
/// マウス位置にエミッター追従、Spaceで爆発/花火バースト。
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/Rendering/ParticleSystem2D.h"

#include <cmath>
#include <Windows.h>

class Particle2DShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: 2D Particle System";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 10; config.bgG = 10; config.bgB = 20;
        return config;
    }

    void Start() override
    {
        // エフェクト1: 爆発（Circle, burst=50, Add, 赤→黄→透明）
        {
            GX::EmitterConfig2D cfg;
            cfg.shape = GX::EmitterShape2D::Circle;
            cfg.shapeRadius = 5.0f;
            cfg.emissionRate = 0.0f; // burst only
            cfg.lifeMin = 0.3f; cfg.lifeMax = 0.8f;
            cfg.speedMin = 100.0f; cfg.speedMax = 300.0f;
            cfg.directionAngle = 0.0f;
            cfg.directionSpread = 180.0f;
            cfg.sizeStart = 12.0f; cfg.sizeEnd = 2.0f;
            cfg.colorStart = GX::Color(1.0f, 0.8f, 0.1f, 1.0f);
            cfg.colorEnd   = GX::Color(1.0f, 0.1f, 0.0f, 0.0f);
            cfg.gravity = { 0, 50.0f };
            cfg.blendMode = GX::BlendMode::Add;
            cfg.maxParticles = 500;
            m_explosionIdx = m_system.AddEmitter(cfg);
            m_system.SetPosition(m_explosionIdx, 320.0f, 400.0f);
        }

        // エフェクト2: 煙（Circle, continuous, Alpha, 白→灰→透明）
        {
            GX::EmitterConfig2D cfg;
            cfg.shape = GX::EmitterShape2D::Circle;
            cfg.shapeRadius = 10.0f;
            cfg.emissionRate = 30.0f;
            cfg.lifeMin = 1.0f; cfg.lifeMax = 2.5f;
            cfg.speedMin = 10.0f; cfg.speedMax = 40.0f;
            cfg.directionAngle = -90.0f;
            cfg.directionSpread = 20.0f;
            cfg.sizeStart = 8.0f; cfg.sizeEnd = 30.0f;
            cfg.colorStart = GX::Color(0.7f, 0.7f, 0.7f, 0.5f);
            cfg.colorEnd   = GX::Color(0.3f, 0.3f, 0.3f, 0.0f);
            cfg.gravity = { 0, -20.0f };
            cfg.drag = 0.5f;
            cfg.blendMode = GX::BlendMode::Alpha;
            cfg.maxParticles = 300;
            m_smokeIdx = m_system.AddEmitter(cfg);
            m_system.SetPosition(m_smokeIdx, 640.0f, 500.0f);
        }

        // エフェクト3: 軌跡（Point, continuous, Add, 青→紫→透明）
        {
            GX::EmitterConfig2D cfg;
            cfg.shape = GX::EmitterShape2D::Point;
            cfg.emissionRate = 80.0f;
            cfg.lifeMin = 0.3f; cfg.lifeMax = 0.8f;
            cfg.speedMin = 5.0f; cfg.speedMax = 20.0f;
            cfg.directionAngle = 0.0f;
            cfg.directionSpread = 180.0f;
            cfg.sizeStart = 6.0f; cfg.sizeEnd = 1.0f;
            cfg.colorStart = GX::Color(0.2f, 0.4f, 1.0f, 1.0f);
            cfg.colorEnd   = GX::Color(0.6f, 0.1f, 1.0f, 0.0f);
            cfg.gravity = { 0, 0 };
            cfg.blendMode = GX::BlendMode::Add;
            cfg.maxParticles = 400;
            m_trailIdx = m_system.AddEmitter(cfg);
            m_system.SetPosition(m_trailIdx, 640.0f, 360.0f);
        }

        // エフェクト4: 花火（Point, burst, Add, ランダムカラー, 重力下向き）
        {
            GX::EmitterConfig2D cfg;
            cfg.shape = GX::EmitterShape2D::Point;
            cfg.emissionRate = 0.0f; // burst only
            cfg.lifeMin = 0.8f; cfg.lifeMax = 2.0f;
            cfg.speedMin = 100.0f; cfg.speedMax = 250.0f;
            cfg.directionAngle = 0.0f;
            cfg.directionSpread = 180.0f;
            cfg.sizeStart = 5.0f; cfg.sizeEnd = 1.0f;
            cfg.colorStart = GX::Color(1.0f, 1.0f, 1.0f, 1.0f);
            cfg.colorEnd   = GX::Color(1.0f, 0.5f, 0.0f, 0.0f);
            cfg.gravity = { 0, 200.0f };
            cfg.drag = 0.3f;
            cfg.blendMode = GX::BlendMode::Add;
            cfg.maxParticles = 1000;
            m_fireworkIdx = m_system.AddEmitter(cfg);
            m_system.SetPosition(m_fireworkIdx, 960.0f, 300.0f);
        }
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& kb  = ctx.inputManager.GetKeyboard();
        auto& mouse = ctx.inputManager.GetMouse();
        m_lastDt = dt;

        // マウス位置で軌跡エミッター追従
        int mx, my;
        GetMousePoint(&mx, &my);
        m_system.SetPosition(m_trailIdx, static_cast<float>(mx), static_cast<float>(my));

        // Spaceで爆発・花火バースト
        if (kb.IsKeyTriggered(VK_SPACE))
        {
            m_system.Burst(m_explosionIdx, 50);
            m_system.Burst(m_fireworkIdx, 200);

            // 爆発位置をランダム化
            float ex = 200.0f + static_cast<float>(rand() % 880);
            float ey = 200.0f + static_cast<float>(rand() % 320);
            m_system.SetPosition(m_explosionIdx, ex, ey);

            // 花火位置もランダム化
            float fx = 200.0f + static_cast<float>(rand() % 880);
            float fy = 150.0f + static_cast<float>(rand() % 250);
            m_system.SetPosition(m_fireworkIdx, fx, fy);
        }

        m_system.Update(dt);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        ctx.EnsureSpriteBatch();

        m_system.Draw(ctx.spriteBatch);

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        uint32_t totalParticles = m_system.GetAliveCount();
        DrawString(10, 10,
                   FormatT(TEXT("FPS: {:.1f}  Particles: {}"), fps, totalParticles).c_str(),
                   GetColor(255, 255, 255));
        DrawString(10, 35, TEXT("Space: Burst explosion/fireworks  Mouse: Trail follows cursor"),
                   GetColor(120, 180, 255));
        DrawString(10, 60, TEXT("ESC: Quit"),
                   GetColor(136, 136, 136));
    }

private:
    float m_lastDt = 0.0f;
    GX::ParticleSystem2D m_system;
    int m_explosionIdx = -1;
    int m_smokeIdx     = -1;
    int m_trailIdx     = -1;
    int m_fireworkIdx  = -1;
};

GX_EASY_APP(Particle2DShowcaseApp)
