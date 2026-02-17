/// @file Samples/PostEffectShowcase/main.cpp
/// @brief ポストエフェクトをON/OFFして試すサンプル。
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Fog.h"

#include <format>
#include <string>
#include <Windows.h>

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

class PostEffectApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: PostEffect Showcase";
        config.width = 1280;
        config.height = 720;
        config.bgR = 6;
        config.bgG = 8;
        config.bgB = 18;
		config.vsync = true;
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera = ctx.camera;
        auto& postFX = ctx.postEffect;

        // シャドウパスをこのサンプルでは回さないため、シャドウは無効化しておく
        renderer.SetShadowEnabled(false);

        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.GetSSAO().SetEnabled(true);
        postFX.SetFXAAEnabled(true);
        postFX.SetVignetteEnabled(true);
        postFX.SetColorGradingEnabled(true);
        postFX.GetDoF().SetEnabled(false);
        postFX.GetMotionBlur().SetEnabled(false);
        postFX.GetSSR().SetEnabled(false);
        postFX.GetOutline().SetEnabled(false);
        postFX.GetTAA().SetEnabled(false);

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

        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 3.0f, -8.0f);
        camera.Rotate(0.3f, 0.0f);
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& kb = ctx.inputManager.GetKeyboard();
        auto& mouse = ctx.inputManager.GetMouse();

        m_totalTime += dt;
        m_lastDt = dt;

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

        if (m_autoRotate)
        {
            camera.Rotate(0.0f, 0.4f * dt);
        }

        float speed = m_cameraSpeed * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);

        // キー入力でエフェクトを切り替える
        const int keys[k_NumEffects] = {
            '1', '2', '3', '4', '5',
            '6', '7', '8', '9', '0'
        };
        for (int i = 0; i < k_NumEffects; ++i)
        {
            if (kb.IsKeyTriggered(keys[i]))
                ToggleEffect(i);
        }

        if (kb.IsKeyTriggered('R'))
            m_autoRotate = !m_autoRotate;
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        ctx.FlushAll();

        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);
        ctx.renderer3D.Begin(cmd, frameIndex, ctx.camera, m_totalTime);

        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_planeMesh, m_floorTransform);

        for (int i = 0; i < k_NumCubes; ++i)
        {
            ctx.renderer3D.SetMaterial(m_cubeMats[i]);
            ctx.renderer3D.DrawMesh(m_cubeMesh, m_cubeTransforms[i]);
        }

        for (int i = 0; i < k_NumSpheres; ++i)
        {
            ctx.renderer3D.SetMaterial(m_sphereMats[i]);
            ctx.renderer3D.DrawMesh(m_sphereMesh, m_sphereTransforms[i]);
        }

        ctx.renderer3D.SetMaterial(m_pillarMat);
        for (int i = 0; i < k_NumPillars; ++i)
            ctx.renderer3D.DrawMesh(m_cylinderMesh, m_pillarTransforms[i]);

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               ctx.renderer3D.GetDepthBuffer(),
                               ctx.camera, m_lastDt);

        // UI描画
        float panelX = 10.0f;
        float panelY = 10.0f;
        float panelW = 320.0f;
        float panelH = 260.0f;

        DrawBox((int)panelX, (int)panelY, (int)(panelX + panelW), (int)(panelY + panelH),
                GetColor(0, 0, 0), TRUE);
        DrawString((int)panelX + 8, (int)panelY + 8, TEXT("Post-Effect Showcase"),
                   GetColor(68, 204, 255));

        float y = panelY + 36.0f;
        for (int i = 0; i < k_NumEffects; ++i)
        {
            unsigned int col = IsEffectEnabled(i) ? GetColor(136, 255, 136) : GetColor(136, 136, 136);
            const int keyNum = (i == 9) ? 0 : (i + 1);
            const TString line = FormatT(TEXT("[{}] {}"), keyNum, k_EffectNames[i]);
            DrawString((int)panelX + 8, (int)y, line.c_str(), col);
            y += 20.0f;
        }

        DrawString((int)panelX + 8, (int)(y + 10),
                   TEXT("R: Auto rotate camera"),
                   GetColor(136, 136, 136));

        DrawString(10, (int)ctx.swapChain.GetHeight() - 30,
                   TEXT("WASD/QE Move  Shift Fast  RClick Mouse  ESC Quit"),
                   GetColor(136, 136, 136));
    }

private:
    static constexpr int k_NumCubes = 3;
    static constexpr int k_NumSpheres = 3;
    static constexpr int k_NumPillars = 4;
    static constexpr int k_NumEffects = 10;
    static constexpr const TChar* k_EffectNames[k_NumEffects] = {
        TEXT("Bloom"), TEXT("SSAO"), TEXT("FXAA"), TEXT("Vignette"), TEXT("ColorGrad"),
        TEXT("DoF"), TEXT("MotionBlur"), TEXT("SSR"), TEXT("Outline"), TEXT("TAA")
    };

    float m_cameraSpeed = 5.0f;
    float m_mouseSens = 0.003f;
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0;
    int   m_lastMY = 0;
    bool  m_autoRotate = false;

    float m_totalTime = 0.0f;
    float m_lastDt = 0.0f;

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

    bool IsEffectEnabled(int idx) const
    {
        auto& fx = GX_Internal::CompatContext::Instance().postEffect;
        switch (idx)
        {
        case 0: return fx.GetBloom().IsEnabled();
        case 1: return fx.GetSSAO().IsEnabled();
        case 2: return fx.IsFXAAEnabled();
        case 3: return fx.IsVignetteEnabled();
        case 4: return fx.IsColorGradingEnabled();
        case 5: return fx.GetDoF().IsEnabled();
        case 6: return fx.GetMotionBlur().IsEnabled();
        case 7: return fx.GetSSR().IsEnabled();
        case 8: return fx.GetOutline().IsEnabled();
        case 9: return fx.GetTAA().IsEnabled();
        default: return false;
        }
    }

    void ToggleEffect(int idx)
    {
        auto& fx = GX_Internal::CompatContext::Instance().postEffect;
        switch (idx)
        {
        case 0: fx.GetBloom().SetEnabled(!fx.GetBloom().IsEnabled()); break;
        case 1: fx.GetSSAO().SetEnabled(!fx.GetSSAO().IsEnabled()); break;
        case 2: fx.SetFXAAEnabled(!fx.IsFXAAEnabled()); break;
        case 3: fx.SetVignetteEnabled(!fx.IsVignetteEnabled()); break;
        case 4: fx.SetColorGradingEnabled(!fx.IsColorGradingEnabled()); break;
        case 5: fx.GetDoF().SetEnabled(!fx.GetDoF().IsEnabled()); break;
        case 6: fx.GetMotionBlur().SetEnabled(!fx.GetMotionBlur().IsEnabled()); break;
        case 7: fx.GetSSR().SetEnabled(!fx.GetSSR().IsEnabled()); break;
        case 8: fx.GetOutline().SetEnabled(!fx.GetOutline().IsEnabled()); break;
        case 9: fx.GetTAA().SetEnabled(!fx.GetTAA().IsEnabled()); break;
        default: break;
        }
    }
};

GX_EASY_APP(PostEffectApp)


