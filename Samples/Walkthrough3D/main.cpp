/// @file Samples/Walkthrough3D/main.cpp
/// @brief GXFrameworkの3Dウォークスルー例。カメラ移動の基本操作を示す。

#include "FrameworkApp.h"
#include "GameScene.h"

#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Fog.h"

#include <format>

namespace
{

class WalkthroughScene : public GXFW::GameScene
{
public:
    const char* GetName() const override { return "Walkthrough3D"; }

protected:
    void OnSceneEnter(GXFW::SceneContext& ctx) override
    {
        auto& renderer = *ctx.renderer;
        auto& camera = *ctx.camera;
        auto& postFX = *ctx.postFX;

        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.GetSSAO().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        m_planeMesh    = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(30.0f, 30.0f, 30, 30));
        m_cubeMesh     = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));
        m_sphereMesh   = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.5f, 32, 16));
        m_cylinderMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateCylinder(0.25f, 0.25f, 3.0f, 16, 1));

        m_floorTransform.SetPosition(0, 0, 0);
        m_floorMat.constants.albedoFactor = { 0.5f, 0.5f, 0.52f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        float cubeColors[][3] = { {0.9f, 0.15f, 0.1f}, {0.1f, 0.85f, 0.15f}, {0.1f, 0.2f, 0.9f} };
        for (int i = 0; i < k_NumCubes; ++i)
        {
            m_cubeTransforms[i].SetPosition(-2.0f + i * 2.0f, 0.5f, 2.0f);
            m_cubeMats[i].constants.albedoFactor = { cubeColors[i][0], cubeColors[i][1], cubeColors[i][2], 1.0f };
            m_cubeMats[i].constants.roughnessFactor = 0.5f;
        }

        m_sphereTransforms[0].SetPosition(-2.0f, 0.5f, -1.0f);
        m_sphereMats[0].constants.albedoFactor = { 1.0f, 0.85f, 0.4f, 1.0f };
        m_sphereMats[0].constants.metallicFactor = 1.0f;
        m_sphereMats[0].constants.roughnessFactor = 0.2f;

        m_sphereTransforms[1].SetPosition(0.0f, 0.5f, -1.0f);
        m_sphereMats[1].constants.albedoFactor = { 0.95f, 0.95f, 0.9f, 1.0f };
        m_sphereMats[1].constants.roughnessFactor = 0.7f;

        m_sphereTransforms[2].SetPosition(2.0f, 0.5f, -1.0f);
        m_sphereMats[2].constants.albedoFactor = { 0.1f, 0.4f, 0.9f, 1.0f };
        m_sphereMats[2].constants.roughnessFactor = 0.4f;

        float pillarPos[][2] = { {-4, 4}, {4, 4}, {-4, -4}, {4, -4} };
        for (int i = 0; i < k_NumPillars; ++i)
            m_pillarTransforms[i].SetPosition(pillarPos[i][0], 1.5f, pillarPos[i][1]);
        m_pillarMat.constants.albedoFactor = { 0.6f, 0.6f, 0.62f, 1.0f };
        m_pillarMat.constants.roughnessFactor = 0.6f;

        GX::LightData lights[3];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        lights[1] = GX::Light::CreatePoint({ -3.0f, 3.0f, -3.0f }, 15.0f, { 1.0f, 0.95f, 0.9f }, 3.0f);
        lights[2] = GX::Light::CreateSpot({ 3.0f, 5.0f, -2.0f }, { -0.3f, -1.0f, 0.2f },
                                           20.0f, 30.0f, { 1.0f, 0.8f, 0.4f }, 10.0f);
        renderer.SetLights(lights, 3, { 0.05f, 0.05f, 0.05f });
        renderer.SetFog(GX::FogMode::Linear, { 0.7f, 0.7f, 0.7f }, 30.0f, 100.0f);
        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.5f, 0.55f, 0.6f }, { 0.75f, 0.75f, 0.75f });

        camera.SetPerspective(XM_PIDIV4, (float)ctx.swapChain->GetWidth() / ctx.swapChain->GetHeight(), 0.1f, 500.0f);
        camera.SetPosition(0.0f, 3.0f, -8.0f);
        camera.Rotate(0.3f, 0.0f);
    }

    void OnSceneUpdate(GXFW::SceneContext& ctx, float dt) override
    {
        auto& camera = *ctx.camera;
        auto& mouse = ctx.input->GetMouse();

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
            camera.Rotate((float)(my - m_lastMY) * m_mouseSens, (float)(mx - m_lastMX) * m_mouseSens);
            m_lastMX = mx;
            m_lastMY = my;
        }

        float speed = m_cameraSpeed * dt;
        if (ctx.input->CheckHitKey(VK_SHIFT)) speed *= 3.0f;
        if (ctx.input->CheckHitKey('W')) camera.MoveForward(speed);
        if (ctx.input->CheckHitKey('S')) camera.MoveForward(-speed);
        if (ctx.input->CheckHitKey('D')) camera.MoveRight(speed);
        if (ctx.input->CheckHitKey('A')) camera.MoveRight(-speed);
        if (ctx.input->CheckHitKey('E')) camera.MoveUp(speed);
        if (ctx.input->CheckHitKey('Q')) camera.MoveUp(-speed);
    }

    void OnSceneRender(GXFW::SceneContext& ctx) override
    {
        auto& renderer = *ctx.renderer;

        renderer.SetMaterial(m_floorMat);
        renderer.DrawMesh(m_planeMesh, m_floorTransform);

        for (int i = 0; i < k_NumCubes; ++i)
        {
            renderer.SetMaterial(m_cubeMats[i]);
            renderer.DrawMesh(m_cubeMesh, m_cubeTransforms[i]);
        }

        for (int i = 0; i < k_NumSpheres; ++i)
        {
            renderer.SetMaterial(m_sphereMats[i]);
            renderer.DrawMesh(m_sphereMesh, m_sphereTransforms[i]);
        }

        renderer.SetMaterial(m_pillarMat);
        for (int i = 0; i < k_NumPillars; ++i)
            renderer.DrawMesh(m_cylinderMesh, m_pillarTransforms[i]);
    }

    void OnSceneRenderUI(GXFW::SceneContext& ctx) override
    {
        auto pos = ctx.camera->GetPosition();
        ctx.DrawString(10.0f, 10.0f,
                       std::format(L"FPS: {:.1f}", ctx.app->GetTimer().GetFPS()),
                       0xFFFFFFFF);
        ctx.DrawString(10.0f, 35.0f,
                       std::format(L"Pos: ({:.1f}, {:.1f}, {:.1f})", pos.x, pos.y, pos.z),
                       0xFF88BBFF);
        ctx.DrawString(10.0f, 60.0f,
                       L"Bloom: ON  SSAO: ON  FXAA: ON  Tonemap: ACES",
                       0xFF88FF88);

        float helpY = static_cast<float>(ctx.swapChain->GetHeight()) - 30.0f;
        ctx.DrawString(10.0f, helpY,
                       L"WASD: Move  QE: Up/Down  Shift: Fast  RClick: Mouse  ESC: Quit",
                       0xFF888888);
    }

private:
    static constexpr int k_NumCubes = 3;
    static constexpr int k_NumSpheres = 3;
    static constexpr int k_NumPillars = 4;

    float m_cameraSpeed = 5.0f;
    float m_mouseSens = 0.003f;
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0;
    int   m_lastMY = 0;

    GX::GPUMesh m_planeMesh;
    GX::GPUMesh m_cubeMesh;
    GX::GPUMesh m_sphereMesh;
    GX::GPUMesh m_cylinderMesh;

    GX::Transform3D m_floorTransform;
    GX::Material    m_floorMat;

    GX::Transform3D m_cubeTransforms[k_NumCubes];
    GX::Material    m_cubeMats[k_NumCubes];

    GX::Transform3D m_sphereTransforms[k_NumSpheres];
    GX::Material    m_sphereMats[k_NumSpheres];

    GX::Transform3D m_pillarTransforms[k_NumPillars];
    GX::Material    m_pillarMat;
};

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    GXFW::FrameworkApp app;
    GXFW::AppConfig config;
    config.title = L"GXLib Sample: Walkthrough3D";
    config.width = 1280;
    config.height = 720;
    config.enableDebug = true;

    if (!app.Initialize(config))
        return -1;

    app.SetScene(std::make_unique<WalkthroughScene>());
    app.Run();
    app.Shutdown();
    return 0;
}
