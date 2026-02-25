/// @file Samples/LODShowcase/main.cpp
/// @brief LODGroup screen-percentage based LOD switching demo.
///
/// Displays a grid of spheres with 3 LOD levels that switch based
/// on screen occupancy. LOD levels are color-coded for visualization.
///
/// Controls:
///   WASD / QE    - Camera movement
///   Right Click  - Toggle mouse capture for look
///   Tab          - Toggle LOD color visualization
///   H            - Toggle hysteresis
///   ESC          - Quit
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/LODGroup.h"
#include "Graphics/3D/Model.h"

#include <Windows.h>

class LODShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: LOD Group";
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

        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // Lights
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.5f);
        renderer.SetLights(lights, 1, { 0.1f, 0.1f, 0.12f });
        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);

        // Camera
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 5.0f, -5.0f);
        camera.Rotate(0.2f, 0.0f);

        // Floor
        m_floorMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(100.0f, 100.0f, 1, 1));
        m_floorMat.constants.albedoFactor    = { 0.35f, 0.35f, 0.38f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        // LOD meshes: high, medium, low poly spheres
        m_meshHigh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(1.0f, 32, 32));
        m_meshMid  = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(1.0f, 16, 16));
        m_meshLow  = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(1.0f, 8, 8));

        // Materials: normal and LOD-colored
        m_normalMat.constants.albedoFactor    = { 0.7f, 0.7f, 0.7f, 1.0f };
        m_normalMat.constants.roughnessFactor = 0.4f;
        m_normalMat.constants.metallicFactor  = 0.5f;

        m_lodHighMat.constants.albedoFactor    = { 0.2f, 0.8f, 0.2f, 1.0f };
        m_lodHighMat.constants.roughnessFactor = 0.4f;

        m_lodMidMat.constants.albedoFactor    = { 0.8f, 0.8f, 0.1f, 1.0f };
        m_lodMidMat.constants.roughnessFactor = 0.4f;

        m_lodLowMat.constants.albedoFactor    = { 0.8f, 0.2f, 0.1f, 1.0f };
        m_lodLowMat.constants.roughnessFactor = 0.4f;

        // Place objects in a grid
        for (int z = 0; z < k_GridSize; ++z)
        {
            for (int x = 0; x < k_GridSize; ++x)
            {
                ObjectInfo obj;
                obj.transform.SetPosition(
                    (x - k_GridSize / 2) * k_Spacing,
                    1.0f,
                    z * k_Spacing);
                m_objects.push_back(obj);
            }
        }
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse  = ctx.inputManager.GetMouse();
        auto& kb     = ctx.inputManager.GetKeyboard();

        m_totalTime += dt;
        m_lastDt = dt;

        // Camera
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

        float speed = 10.0f * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        if (kb.IsKeyTriggered(VK_TAB)) m_showLodColors = !m_showLodColors;
        if (kb.IsKeyTriggered('H')) m_useHysteresis = !m_useHysteresis;

        // Select LOD for each object
        m_lodCounts[0] = m_lodCounts[1] = m_lodCounts[2] = 0;
        for (auto& obj : m_objects)
        {
            obj.lodLevel = SelectLODLevel(camera, obj.transform);
            if (obj.lodLevel >= 0 && obj.lodLevel < 3)
                m_lodCounts[obj.lodLevel]++;
        }
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
        floorT.SetPosition(0.0f, 0.0f, 0.0f);
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_floorMesh, floorT);

        // Objects
        for (auto& obj : m_objects)
        {
            GX::GPUMesh* mesh = nullptr;
            GX::Material* mat = nullptr;

            switch (obj.lodLevel)
            {
            case 0: mesh = &m_meshHigh; mat = m_showLodColors ? &m_lodHighMat : &m_normalMat; break;
            case 1: mesh = &m_meshMid;  mat = m_showLodColors ? &m_lodMidMat  : &m_normalMat; break;
            case 2: mesh = &m_meshLow;  mat = m_showLodColors ? &m_lodLowMat  : &m_normalMat; break;
            default: continue;
            }

            ctx.renderer3D.SetMaterial(*mat);
            ctx.renderer3D.DrawMesh(*mesh, obj.transform);
        }

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        auto& db = ctx.renderer3D.GetDepthBuffer();
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(), db, ctx.camera, m_lastDt);
        db.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10,
            FormatT(TEXT("FPS: {:.1f}  Objects: {}  Hysteresis: {}"),
                    fps, m_objects.size(), m_useHysteresis ? TEXT("ON") : TEXT("OFF")).c_str(),
            GetColor(255, 255, 255));
        DrawString(10, 35,
            FormatT(TEXT("LOD0(High): {}  LOD1(Mid): {}  LOD2(Low): {}  ColorViz: {}"),
                    m_lodCounts[0], m_lodCounts[1], m_lodCounts[2],
                    m_showLodColors ? TEXT("ON") : TEXT("OFF")).c_str(),
            GetColor(100, 220, 255));
        DrawString(10, 60,
            TEXT("WASD/QE: Camera  RClick: Look  Tab: ColorViz  H: Hysteresis  ESC: Quit"),
            GetColor(136, 136, 136));
    }

private:
    static constexpr int   k_GridSize = 7;
    static constexpr float k_Spacing  = 4.0f;
    static constexpr float k_BoundingRadius = 1.0f;

    // Screen percentage thresholds
    static constexpr float k_LOD0Threshold = 0.15f;  // > 15% → High
    static constexpr float k_LOD1Threshold = 0.05f;   // > 5% → Mid
    static constexpr float k_Hysteresis    = 0.02f;

    struct ObjectInfo {
        GX::Transform3D transform;
        int lodLevel = 0;
    };

    int SelectLODLevel(const GX::Camera3D& camera, const GX::Transform3D& transform)
    {
        // Calculate screen percentage from bounding sphere
        XMFLOAT3 pos = transform.GetPosition();
        XMFLOAT3 camPos = camera.GetPosition();
        float dx = pos.x - camPos.x;
        float dy = pos.y - camPos.y;
        float dz = pos.z - camPos.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (dist < 0.01f) dist = 0.01f;

        float fovY = camera.GetFovY();
        float screenPct = k_BoundingRadius / (dist * std::tan(fovY * 0.5f));

        float t0 = k_LOD0Threshold;
        float t1 = k_LOD1Threshold;
        if (m_useHysteresis)
        {
            t0 -= k_Hysteresis;
            t1 -= k_Hysteresis;
        }

        if (screenPct > t0) return 0;
        if (screenPct > t1) return 1;
        return 2;
    }

    GX::GPUMesh m_floorMesh;
    GX::GPUMesh m_meshHigh, m_meshMid, m_meshLow;
    GX::Material m_floorMat, m_normalMat;
    GX::Material m_lodHighMat, m_lodMidMat, m_lodLowMat;

    std::vector<ObjectInfo> m_objects;
    int  m_lodCounts[3] = {};
    bool m_showLodColors = true;
    bool m_useHysteresis = true;

    float m_totalTime = 0.0f, m_lastDt = 0.0f;
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0, m_lastMY = 0;
};

GX_EASY_APP(LODShowcaseApp)
