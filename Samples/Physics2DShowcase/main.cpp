/// @file Samples/Physics2DShowcase/main.cpp
/// @brief 2D physics simulation demo using PhysicsWorld2D.
///
/// Click to spawn dynamic boxes and circles that fall under gravity
/// and collide with the ground and walls.
///
/// Controls:
///   Left Click  - Spawn box at cursor
///   Right Click - Spawn circle at cursor
///   R           - Reset (remove all bodies)
///   Up/Down     - Adjust gravity strength
///   1/2/3       - Restitution preset (0.0 / 0.5 / 0.9)
///   ESC         - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Physics/PhysicsWorld2D.h"

#include <Windows.h>
#include <random>
#include <memory>

class Physics2DShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: 2D Physics";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 10; config.bgG = 10; config.bgB = 20;
        return config;
    }

    void Start() override
    {
        m_world = std::make_unique<GX::PhysicsWorld2D>();
        m_world->SetGravity({ 0.0f, m_gravityStrength });
        BuildWalls();
    }

    void Update(float dt) override
    {
        auto& ctx   = GX_Internal::CompatContext::Instance();
        auto& kb    = ctx.inputManager.GetKeyboard();
        auto& mouse = ctx.inputManager.GetMouse();
        m_lastDt = dt;

        // Spawn bodies
        int mx, my;
        GetMousePoint(&mx, &my);

        if (mouse.IsButtonTriggered(GX::MouseButton::Left))
            SpawnBox(static_cast<float>(mx), static_cast<float>(my));
        if (mouse.IsButtonTriggered(GX::MouseButton::Right))
            SpawnCircle(static_cast<float>(mx), static_cast<float>(my));

        // Reset
        if (kb.IsKeyTriggered('R'))
        {
            m_dynamicBodies.clear();
            m_walls.clear();
            m_world.reset(new GX::PhysicsWorld2D());
            m_world->SetGravity({ 0.0f, m_gravityStrength });
            BuildWalls();
        }

        // Gravity adjustment
        if (kb.IsKeyTriggered(VK_UP))
        {
            m_gravityStrength = (std::max)(50.0f, m_gravityStrength - 100.0f);
            m_world->SetGravity({ 0.0f, m_gravityStrength });
        }
        if (kb.IsKeyTriggered(VK_DOWN))
        {
            m_gravityStrength = (std::min)(2000.0f, m_gravityStrength + 100.0f);
            m_world->SetGravity({ 0.0f, m_gravityStrength });
        }

        // Restitution presets
        if (kb.IsKeyTriggered('1')) m_restitution = 0.0f;
        if (kb.IsKeyTriggered('2')) m_restitution = 0.5f;
        if (kb.IsKeyTriggered('3')) m_restitution = 0.9f;

        // Step physics
        m_world->Step(dt);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        ctx.EnsurePrimitiveBatch();

        // Draw walls (static bodies) as dark grey
        for (auto* wall : m_walls)
        {
            auto& shape = wall->shape;
            float x = wall->position.x;
            float y = wall->position.y;
            DrawBox(static_cast<int>(x - shape.halfExtents.x),
                    static_cast<int>(y - shape.halfExtents.y),
                    static_cast<int>(x + shape.halfExtents.x),
                    static_cast<int>(y + shape.halfExtents.y),
                    GetColor(60, 60, 80), TRUE);
        }

        // Draw dynamic bodies
        for (auto* body : m_dynamicBodies)
        {
            float x = body->position.x;
            float y = body->position.y;

            if (body->shape.type == GX::ShapeType2D::AABB)
            {
                float hw = body->shape.halfExtents.x;
                float hh = body->shape.halfExtents.y;
                // Simple axis-aligned drawing (rotation ignored for simplicity)
                DrawBox(static_cast<int>(x - hw), static_cast<int>(y - hh),
                        static_cast<int>(x + hw), static_cast<int>(y + hh),
                        GetColor(100, 180, 255), TRUE);
                DrawBox(static_cast<int>(x - hw), static_cast<int>(y - hh),
                        static_cast<int>(x + hw), static_cast<int>(y + hh),
                        GetColor(150, 220, 255), FALSE);
            }
            else // Circle
            {
                int r = static_cast<int>(body->shape.radius);
                DrawCircle(static_cast<int>(x), static_cast<int>(y), r,
                           GetColor(255, 150, 80), TRUE);
                DrawCircle(static_cast<int>(x), static_cast<int>(y), r,
                           GetColor(255, 200, 130), FALSE);
            }
        }

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10,
            FormatT(TEXT("FPS: {:.1f}  Bodies: {}  Gravity: {:.0f}  Restitution: {:.1f}"),
                    fps, m_dynamicBodies.size(), m_gravityStrength, m_restitution).c_str(),
            GetColor(255, 255, 255));
        DrawString(10, 35,
            TEXT("LClick: Box  RClick: Circle  R: Reset  Up/Down: Gravity  1/2/3: Restitution"),
            GetColor(120, 180, 255));
        DrawString(10, 60, TEXT("ESC: Quit"), GetColor(136, 136, 136));
    }

private:
    void BuildWalls()
    {
        m_walls.clear();

        // Floor
        auto* floor = m_world->AddBody();
        floor->bodyType = GX::BodyType2D::Static;
        floor->position = { 640.0f, 700.0f };
        floor->shape.type = GX::ShapeType2D::AABB;
        floor->shape.halfExtents = { 640.0f, 20.0f };
        m_walls.push_back(floor);

        // Left wall
        auto* left = m_world->AddBody();
        left->bodyType = GX::BodyType2D::Static;
        left->position = { -10.0f, 360.0f };
        left->shape.type = GX::ShapeType2D::AABB;
        left->shape.halfExtents = { 20.0f, 360.0f };
        m_walls.push_back(left);

        // Right wall
        auto* right = m_world->AddBody();
        right->bodyType = GX::BodyType2D::Static;
        right->position = { 1290.0f, 360.0f };
        right->shape.type = GX::ShapeType2D::AABB;
        right->shape.halfExtents = { 20.0f, 360.0f };
        m_walls.push_back(right);
    }

    void SpawnBox(float x, float y)
    {
        std::uniform_real_distribution<float> sizeDist(10.0f, 30.0f);
        float hw = sizeDist(m_rng);
        float hh = sizeDist(m_rng);

        auto* body = m_world->AddBody();
        body->bodyType = GX::BodyType2D::Dynamic;
        body->position = { x, y };
        body->shape.type = GX::ShapeType2D::AABB;
        body->shape.halfExtents = { hw, hh };
        body->mass = hw * hh * 0.01f;
        body->restitution = m_restitution;
        body->friction = 0.3f;
        m_dynamicBodies.push_back(body);
    }

    void SpawnCircle(float x, float y)
    {
        std::uniform_real_distribution<float> sizeDist(8.0f, 25.0f);
        float r = sizeDist(m_rng);

        auto* body = m_world->AddBody();
        body->bodyType = GX::BodyType2D::Dynamic;
        body->position = { x, y };
        body->shape.type = GX::ShapeType2D::Circle;
        body->shape.radius = r;
        body->mass = r * r * 0.01f;
        body->restitution = m_restitution;
        body->friction = 0.3f;
        m_dynamicBodies.push_back(body);
    }

    std::unique_ptr<GX::PhysicsWorld2D> m_world;
    std::vector<GX::RigidBody2D*> m_walls;
    std::vector<GX::RigidBody2D*> m_dynamicBodies;

    float m_gravityStrength = 500.0f;
    float m_restitution = 0.3f;
    float m_lastDt = 0.0f;

    std::mt19937 m_rng{ 42 };
};

GX_EASY_APP(Physics2DShowcaseApp)
