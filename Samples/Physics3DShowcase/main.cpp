/// @file Samples/Physics3DShowcase/main.cpp
/// @brief 3D physics simulation demo using PhysicsWorld3D (Jolt Physics).
///
/// Demonstrates dynamic bodies (boxes, spheres, capsules) falling and
/// colliding in a 3D environment. Includes preset scenes.
///
/// Controls:
///   Left Click  - Shoot sphere from camera
///   1           - Domino preset
///   2           - Box tower preset
///   R           - Reset scene
///   WASD / QE   - Camera movement
///   Right Click - Toggle mouse capture for look
///   ESC         - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Physics/PhysicsWorld3D.h"

#include <Windows.h>

class Physics3DShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: 3D Physics (Jolt)";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera   = ctx.camera;
        auto& postFX   = ctx.postEffect;

        renderer.SetShadowEnabled(false);
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // Meshes
        m_planeMesh   = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(100.0f, 100.0f, 1, 1));
        m_boxMesh     = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));
        m_sphereMesh  = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.5f, 16, 8));

        // Materials
        m_floorMat.constants.albedoFactor    = { 0.4f, 0.42f, 0.45f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        m_boxMat.constants.albedoFactor    = { 0.7f, 0.4f, 0.2f, 1.0f };
        m_boxMat.constants.roughnessFactor = 0.6f;

        m_sphereMat.constants.albedoFactor    = { 0.2f, 0.6f, 1.0f, 1.0f };
        m_sphereMat.constants.roughnessFactor = 0.3f;
        m_sphereMat.constants.metallicFactor  = 0.7f;

        // Lights
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.5f);
        renderer.SetLights(lights, 1, { 0.1f, 0.1f, 0.12f });
        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);

        // Camera
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 8.0f, -15.0f);
        camera.Rotate(0.3f, 0.0f);

        // Physics
        m_physics.Initialize();
        m_physics.SetGravity({ 0.0f, -9.81f, 0.0f });
        m_groundShape = m_physics.CreateBoxShape({ 50.0f, 0.5f, 50.0f });
        SetupDomino();
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse  = ctx.inputManager.GetMouse();
        auto& kb     = ctx.inputManager.GetKeyboard();

        m_totalTime += dt;
        m_lastDt = dt;

        // Camera look
        if (mouse.IsButtonTriggered(GX::MouseButton::Right))
        {
            m_mouseCaptured = !m_mouseCaptured;
            m_lastMX = mouse.GetX();
            m_lastMY = mouse.GetY();
            ShowCursor(m_mouseCaptured ? FALSE : TRUE);
        }
        if (m_mouseCaptured)
        {
            int mx = mouse.GetX(), my = mouse.GetY();
            camera.Rotate(static_cast<float>(my - m_lastMY) * 0.003f,
                          static_cast<float>(mx - m_lastMX) * 0.003f);
            m_lastMX = mx; m_lastMY = my;
        }

        // Camera movement
        float speed = 12.0f * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        // Shoot sphere
        if (mouse.IsButtonTriggered(GX::MouseButton::Left) && !m_mouseCaptured)
            ShootSphere();

        // Presets
        if (kb.IsKeyTriggered('1')) { ClearDynamics(); SetupDomino(); }
        if (kb.IsKeyTriggered('2')) { ClearDynamics(); SetupTower(); }
        if (kb.IsKeyTriggered('R')) { ClearDynamics(); }

        // Step physics
        m_physics.Step(dt);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t fi = ctx.frameIndex;

        ctx.FlushAll();
        ctx.postEffect.BeginScene(cmd, fi, ctx.renderer3D.GetDepthBuffer().GetDSVHandle(), ctx.camera);
        ctx.renderer3D.Begin(cmd, fi, ctx.camera, m_totalTime);

        // Floor
        GX::Transform3D floorT;
        floorT.SetPosition(0.0f, -0.5f, 0.0f);
        floorT.SetScale(100.0f, 1.0f, 100.0f);
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_planeMesh, floorT);

        // Dynamic bodies
        for (auto& entry : m_bodies)
        {
            GX::Vector3 pos = m_physics.GetPosition(entry.id);
            GX::Quaternion rot = m_physics.GetRotation(entry.id);
            GX::Vector3 euler = rot.ToEuler();
            GX::Transform3D t;
            t.SetPosition(pos.x, pos.y, pos.z);
            t.SetRotation(euler);
            t.SetScale(entry.scaleX, entry.scaleY, entry.scaleZ);

            if (entry.isSphere)
            {
                ctx.renderer3D.SetMaterial(m_sphereMat);
                ctx.renderer3D.DrawMesh(m_sphereMesh, t);
            }
            else
            {
                ctx.renderer3D.SetMaterial(m_boxMat);
                ctx.renderer3D.DrawMesh(m_boxMesh, t);
            }
        }

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        auto& db = ctx.renderer3D.GetDepthBuffer();
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(), db, ctx.camera, m_lastDt);
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10, FormatT(TEXT("FPS: {:.1f}  Bodies: {}"), fps, m_bodies.size()).c_str(),
                   GetColor(255, 255, 255));
        DrawString(10, 35,
            TEXT("LClick: Shoot  1: Domino  2: Tower  R: Reset  WASD/QE: Camera  RClick: Look"),
            GetColor(120, 180, 255));
    }

    void Release() override
    {
        ClearDynamics();
        if (m_groundShape) { m_physics.DestroyShape(m_groundShape); m_groundShape = nullptr; }
        m_physics.Shutdown();
    }

private:
    struct BodyEntry {
        GX::PhysicsBodyID id;
        float scaleX, scaleY, scaleZ;
        bool isSphere;
    };

    void ClearDynamics()
    {
        for (auto& e : m_bodies)
            m_physics.RemoveBody(e.id);
        m_bodies.clear();
        for (auto* s : m_dynamicShapes)
            m_physics.DestroyShape(s);
        m_dynamicShapes.clear();
    }

    GX::PhysicsBodyID AddStaticGround()
    {
        GX::PhysicsBodySettings settings;
        settings.position = { 0.0f, -0.5f, 0.0f };
        settings.motionType = GX::MotionType3D::Static;
        return m_physics.AddBody(m_groundShape, settings);
    }

    void SetupDomino()
    {
        m_groundBody = AddStaticGround();

        auto* dominoShape = m_physics.CreateBoxShape({ 0.15f, 0.5f, 0.4f });
        m_dynamicShapes.push_back(dominoShape);

        for (int i = 0; i < 20; ++i)
        {
            GX::PhysicsBodySettings s;
            s.position = { static_cast<float>(i) * 0.8f - 8.0f, 0.5f, 0.0f };
            s.motionType = GX::MotionType3D::Dynamic;
            s.mass = 1.0f;
            s.friction = 0.6f;
            s.restitution = 0.1f;
            auto id = m_physics.AddBody(dominoShape, s);
            m_bodies.push_back({ id, 0.3f, 1.0f, 0.8f, false });
        }

        // Starter ball
        auto* ballShape = m_physics.CreateSphereShape(0.4f);
        m_dynamicShapes.push_back(ballShape);
        GX::PhysicsBodySettings bs;
        bs.position = { -10.0f, 1.0f, 0.0f };
        bs.motionType = GX::MotionType3D::Dynamic;
        bs.mass = 5.0f;
        auto ballId = m_physics.AddBody(ballShape, bs);
        m_physics.SetLinearVelocity(ballId, { 6.0f, 0.0f, 0.0f });
        m_bodies.push_back({ ballId, 0.8f, 0.8f, 0.8f, true });
    }

    void SetupTower()
    {
        m_groundBody = AddStaticGround();

        auto* boxShape = m_physics.CreateBoxShape({ 0.5f, 0.25f, 0.5f });
        m_dynamicShapes.push_back(boxShape);

        for (int layer = 0; layer < 8; ++layer)
        {
            float y = 0.25f + layer * 0.5f;
            for (int x = -1; x <= 1; ++x)
            {
                GX::PhysicsBodySettings s;
                s.position = { static_cast<float>(x) * 1.05f, y, 0.0f };
                s.motionType = GX::MotionType3D::Dynamic;
                s.mass = 1.0f;
                s.friction = 0.5f;
                auto id = m_physics.AddBody(boxShape, s);
                m_bodies.push_back({ id, 1.0f, 0.5f, 1.0f, false });
            }
        }
    }

    void ShootSphere()
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        XMFLOAT3 pos = ctx.camera.GetPosition();
        XMFLOAT3 fwd = ctx.camera.GetForward();

        auto* shape = m_physics.CreateSphereShape(0.3f);
        m_dynamicShapes.push_back(shape);

        GX::PhysicsBodySettings s;
        s.position = { pos.x, pos.y, pos.z };
        s.motionType = GX::MotionType3D::Dynamic;
        s.mass = 3.0f;
        s.restitution = 0.4f;
        auto id = m_physics.AddBody(shape, s);
        m_physics.SetLinearVelocity(id, { fwd.x * 20.0f, fwd.y * 20.0f, fwd.z * 20.0f });
        m_bodies.push_back({ id, 0.6f, 0.6f, 0.6f, true });
    }

    GX::PhysicsWorld3D m_physics;
    GX::PhysicsShape*  m_groundShape = nullptr;
    GX::PhysicsBodyID  m_groundBody;
    std::vector<GX::PhysicsShape*> m_dynamicShapes;
    std::vector<BodyEntry> m_bodies;

    GX::GPUMesh  m_planeMesh, m_boxMesh, m_sphereMesh;
    GX::Material m_floorMat, m_boxMat, m_sphereMat;

    float m_totalTime = 0.0f, m_lastDt = 0.0f;
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0, m_lastMY = 0;
};

GX_EASY_APP(Physics3DShowcaseApp)
