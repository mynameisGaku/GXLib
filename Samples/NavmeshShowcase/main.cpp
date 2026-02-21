/// @file Samples/NavmeshShowcase/main.cpp
/// @brief NavMesh A* navigation demo.
///
/// Demonstrates the grid-based NavMesh and NavAgent system.
/// A 50x50 navmesh grid is created with obstacles.
/// Left-click on the ground plane to set the agent's destination.
/// The agent computes an A* path and follows it.
///
/// Controls:
///   Left Click   - Set destination (ray cast onto XZ ground plane)
///   WASD / QE    - Camera movement
///   Right Click   - Toggle mouse capture for look
///   R             - Reset obstacles randomly
///   G             - Toggle navmesh debug draw
///   ESC           - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Fog.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "AI/NavMesh.h"
#include "AI/NavAgent.h"

#include <Windows.h>

class NavmeshApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: NavMesh Showcase";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6;
        config.bgG = 8;
        config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer  = ctx.renderer3D;
        auto& camera    = ctx.camera;
        auto& postFX    = ctx.postEffect;

        renderer.SetShadowEnabled(false);

        // Post effects
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // Ground plane mesh (50x50 world units)
        m_planeMesh = renderer.CreateGPUMesh(
            GX::MeshGenerator::CreatePlane(k_WorldSize, k_WorldSize, 50, 50));
        m_cubeMesh  = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));
        m_agentMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.3f, 16, 8));

        // Floor material
        m_floorTransform.SetPosition(0.0f, 0.0f, 0.0f);
        m_floorMat.constants.albedoFactor    = { 0.45f, 0.45f, 0.48f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        // Obstacle material
        m_obstacleMat.constants.albedoFactor    = { 0.5f, 0.2f, 0.15f, 1.0f };
        m_obstacleMat.constants.roughnessFactor = 0.6f;

        // Agent material
        m_agentMat.constants.albedoFactor    = { 0.2f, 0.7f, 1.0f, 1.0f };
        m_agentMat.constants.roughnessFactor = 0.3f;
        m_agentMat.constants.metallicFactor  = 0.8f;

        // Destination marker material
        m_destMat.constants.albedoFactor    = { 1.0f, 0.3f, 0.15f, 1.0f };
        m_destMat.constants.roughnessFactor = 0.4f;

        // Lights
        GX::LightData lights[2];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        lights[1] = GX::Light::CreatePoint({ 0.0f, 15.0f, 0.0f }, 60.0f, { 1.0f, 0.95f, 0.9f }, 2.0f);
        renderer.SetLights(lights, 2, { 0.08f, 0.08f, 0.1f });

        // Skybox
        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.5f, 0.55f, 0.6f }, { 0.75f, 0.75f, 0.75f });

        // Camera (top-down-ish)
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 25.0f, -18.0f);
        camera.Rotate(1.0f, 0.0f); // Look down

        // Build NavMesh
        BuildNavMesh();

        // Init agent
        m_agent.Initialize(&m_navMesh);
        m_agent.SetPosition({ -10.0f, 0.3f, -10.0f });
        m_agent.speed          = 6.0f;
        m_agent.angularSpeed   = 540.0f;
        m_agent.stoppingDistance = 0.3f;
        m_agent.height         = 0.3f;
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse  = ctx.inputManager.GetMouse();
        auto& kb     = ctx.inputManager.GetKeyboard();

        m_totalTime += dt;
        m_lastDt = dt;

        // ---------- Mouse capture for camera look ----------
        if (mouse.IsButtonTriggered(GX::MouseButton::Right))
        {
            m_mouseCaptured = !m_mouseCaptured;
            if (m_mouseCaptured)
            {
                m_lastMX = mouse.GetX();
                m_lastMY = mouse.GetY();
                ShowCursor(FALSE);
            }
            else
            {
                ShowCursor(TRUE);
            }
        }

        if (m_mouseCaptured)
        {
            int mx = mouse.GetX();
            int my = mouse.GetY();
            camera.Rotate(static_cast<float>(my - m_lastMY) * m_mouseSens,
                          static_cast<float>(mx - m_lastMX) * m_mouseSens);
            m_lastMX = mx;
            m_lastMY = my;
        }

        // Camera movement
        float speed = 10.0f * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        // ---------- Toggle navmesh debug draw ----------
        if (kb.IsKeyTriggered('G'))
            m_showNavMesh = !m_showNavMesh;

        // ---------- Reset obstacles ----------
        if (kb.IsKeyTriggered('R'))
            BuildNavMesh();

        // ---------- Left click: set destination ----------
        if (mouse.IsButtonTriggered(GX::MouseButton::Left) && !m_mouseCaptured)
        {
            XMFLOAT3 worldPos;
            if (ScreenToGround(mouse.GetX(), mouse.GetY(), worldPos))
            {
                m_destination = worldPos;
                m_hasDestination = true;
                m_agent.SetDestination(worldPos);
            }
        }

        // Update agent
        m_agent.Update(dt);

        // Update agent transform for rendering
        m_agentTransform.SetPosition(m_agent.GetPosition().x,
                                      m_agent.GetPosition().y,
                                      m_agent.GetPosition().z);
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        ctx.FlushAll();

        // --- HDR scene ---
        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);
        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_totalTime);

        // Floor
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_planeMesh, m_floorTransform);

        // Obstacles
        ctx.renderer3D.SetMaterial(m_obstacleMat);
        for (auto& t : m_obstacleTransforms)
            ctx.renderer3D.DrawMesh(m_cubeMesh, t);

        // Agent
        ctx.renderer3D.SetMaterial(m_agentMat);
        ctx.renderer3D.DrawMesh(m_agentMesh, m_agentTransform);

        // Destination marker
        if (m_hasDestination)
        {
            m_destTransform.SetPosition(m_destination.x, 0.15f, m_destination.z);
            ctx.renderer3D.SetMaterial(m_destMat);
            ctx.renderer3D.DrawMesh(m_agentMesh, m_destTransform);
        }

        ctx.renderer3D.End();

        // --- Debug draw (navmesh + path) ---
        if (m_showNavMesh || m_agent.HasPath())
        {
            auto& primBatch = ctx.renderer3D.GetPrimitiveBatch3D();
            XMFLOAT4X4 vp;
            XMStoreFloat4x4(&vp, XMMatrixTranspose(ctx.camera.GetViewProjectionMatrix()));
            primBatch.Begin(cmd, frameIndex, vp);

            if (m_showNavMesh)
                m_navMesh.DebugDraw(primBatch);

            // Draw agent path
            if (m_agent.HasPath())
            {
                m_navMesh.DebugDrawPath(primBatch, m_agent.GetPath(),
                                        { 1.0f, 1.0f, 0.2f, 1.0f });

                // Draw waypoint markers
                for (auto& wp : m_agent.GetPath())
                {
                    XMFLOAT3 p = { wp.x, wp.y + 0.2f, wp.z };
                    primBatch.DrawWireSphere(p, 0.12f, { 1.0f, 0.8f, 0.1f, 1.0f }, 6);
                }
            }

            // Agent direction indicator
            {
                XMFLOAT3 agentPos = m_agent.GetPosition();
                float yaw = m_agent.GetYaw();
                XMFLOAT3 fwd = { std::sin(yaw) * 0.8f, 0.0f, std::cos(yaw) * 0.8f };
                XMFLOAT3 end = { agentPos.x + fwd.x, agentPos.y + 0.1f, agentPos.z + fwd.z };
                XMFLOAT3 start = { agentPos.x, agentPos.y + 0.1f, agentPos.z };
                primBatch.DrawLine(start, end, { 0.3f, 0.9f, 1.0f, 1.0f });
            }

            primBatch.End();
        }

        ctx.postEffect.EndScene();

        // --- Post effect resolve ---
        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // --- HUD ---
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10, FormatT(TEXT("FPS: {:.1f}"), fps).c_str(), GetColor(255, 255, 255));
        DrawString(10, 35, FormatT(TEXT("Grid: {}x{} (cellSize={:.1f})"),
                   m_navMesh.GetGridWidth(), m_navMesh.GetGridHeight(),
                   m_navMesh.GetCellSize()).c_str(),
                   GetColor(120, 180, 255));

        XMFLOAT3 agentPos = m_agent.GetPosition();
        DrawString(10, 60, FormatT(TEXT("Agent: ({:.1f}, {:.1f}, {:.1f}) {}"),
                   agentPos.x, agentPos.y, agentPos.z,
                   m_agent.HasReachedDestination() ? TEXT("ARRIVED") : TEXT("moving...")).c_str(),
                   GetColor(100, 220, 255));

        DrawString(10, 90,
            TEXT("LClick: Set dest  WASD/QE: Camera  RClick: Look  G: NavMesh  R: Reset  ESC: Quit"),
            GetColor(136, 136, 136));
    }

private:
    static constexpr float k_WorldSize   = 50.0f;
    static constexpr float k_HalfWorld   = k_WorldSize * 0.5f;
    static constexpr float k_CellSize    = 1.0f;
    static constexpr int   k_MaxObstacles = 40;

    void BuildNavMesh()
    {
        m_obstacleTransforms.clear();

        m_navMesh.Build(-k_HalfWorld, -k_HalfWorld,
                         k_HalfWorld,  k_HalfWorld,
                         k_CellSize, 0.9f, 45.0f);

        // Place random obstacles
        std::mt19937 rng(static_cast<unsigned>(m_totalTime * 1000.0f + 42));
        std::uniform_real_distribution<float> posDist(-k_HalfWorld + 3.0f, k_HalfWorld - 3.0f);
        std::uniform_real_distribution<float> sizeDist(1.0f, 4.0f);

        for (int i = 0; i < k_MaxObstacles; ++i)
        {
            float ox = posDist(rng);
            float oz = posDist(rng);
            float sx = sizeDist(rng);
            float sz = sizeDist(rng);

            // Skip obstacles too close to agent start or center
            if (std::abs(ox + 10.0f) < 3.0f && std::abs(oz + 10.0f) < 3.0f) continue;
            if (std::abs(ox) < 3.0f && std::abs(oz) < 3.0f) continue;

            GX::Transform3D t;
            t.SetPosition(ox, 0.5f, oz);
            t.SetScale(sx, 1.0f, sz);
            m_obstacleTransforms.push_back(t);

            // Mark cells covered by this obstacle as unwalkable
            int cellMinX = static_cast<int>(std::floor((ox - sx * 0.5f + k_HalfWorld) / k_CellSize));
            int cellMinZ = static_cast<int>(std::floor((oz - sz * 0.5f + k_HalfWorld) / k_CellSize));
            int cellMaxX = static_cast<int>(std::ceil((ox + sx * 0.5f + k_HalfWorld) / k_CellSize));
            int cellMaxZ = static_cast<int>(std::ceil((oz + sz * 0.5f + k_HalfWorld) / k_CellSize));

            for (int cz = cellMinZ; cz <= cellMaxZ; ++cz)
                for (int cx = cellMinX; cx <= cellMaxX; ++cx)
                    m_navMesh.SetCellWalkable(cx, cz, false);
        }

        // Re-init agent
        m_agent.Initialize(&m_navMesh);
        m_agent.SetPosition({ -10.0f, 0.3f, -10.0f });
        m_agent.speed          = 6.0f;
        m_agent.angularSpeed   = 540.0f;
        m_agent.stoppingDistance = 0.3f;
        m_agent.height         = 0.3f;
        m_hasDestination = false;
    }

    /// @brief Cast a ray from screen coords to the Y=0 ground plane
    bool ScreenToGround(int screenX, int screenY, XMFLOAT3& outPos)
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;

        float w = static_cast<float>(ctx.screenWidth);
        float h = static_cast<float>(ctx.screenHeight);

        // Normalized device coords
        float ndcX = (2.0f * screenX / w) - 1.0f;
        float ndcY = 1.0f - (2.0f * screenY / h);

        // Inverse view-projection
        XMMATRIX vp    = camera.GetViewProjectionMatrix();
        XMMATRIX invVP = XMMatrixInverse(nullptr, vp);

        // Near and far points in NDC
        XMVECTOR nearPt = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), invVP);
        XMVECTOR farPt  = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), invVP);

        // Ray direction
        XMVECTOR rayDir = XMVectorSubtract(farPt, nearPt);

        XMFLOAT3 origin, dir;
        XMStoreFloat3(&origin, nearPt);
        XMStoreFloat3(&dir, rayDir);

        // Intersect with Y=0 plane
        if (std::abs(dir.y) < 1e-6f) return false; // Ray parallel to ground

        float t = -origin.y / dir.y;
        if (t < 0.0f) return false; // Behind camera

        outPos.x = origin.x + dir.x * t;
        outPos.y = 0.0f;
        outPos.z = origin.z + dir.z * t;

        // Clamp to world bounds
        outPos.x = (std::max)(-k_HalfWorld + 1.0f, (std::min)(k_HalfWorld - 1.0f, outPos.x));
        outPos.z = (std::max)(-k_HalfWorld + 1.0f, (std::min)(k_HalfWorld - 1.0f, outPos.z));

        return true;
    }

    // --- Meshes ---
    GX::GPUMesh m_planeMesh;
    GX::GPUMesh m_cubeMesh;
    GX::GPUMesh m_agentMesh;

    // --- Transforms and materials ---
    GX::Transform3D m_floorTransform;
    GX::Material    m_floorMat;
    GX::Material    m_obstacleMat;
    GX::Material    m_agentMat;
    GX::Material    m_destMat;

    GX::Transform3D              m_agentTransform;
    GX::Transform3D              m_destTransform;
    std::vector<GX::Transform3D> m_obstacleTransforms;

    // --- NavMesh & Agent ---
    GX::NavMesh  m_navMesh;
    GX::NavAgent m_agent;

    XMFLOAT3 m_destination = { 0.0f, 0.0f, 0.0f };
    bool     m_hasDestination = false;
    bool     m_showNavMesh    = false;

    // --- Camera ---
    float m_mouseSens   = 0.003f;
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0;
    int   m_lastMY = 0;

    float m_totalTime = 0.0f;
    float m_lastDt    = 0.0f;
};

GX_EASY_APP(NavmeshApp)
