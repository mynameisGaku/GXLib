/// @file Samples/IBLShowcase/main.cpp
/// @brief イメージベースドライティング（IBL）デモ
///
/// metallic/roughness グラデーションの球体マトリクスでIBLの効果を視覚確認する。
/// 横=roughness(0→1), 縦=metallic(0→1) の 7x7=49個の球。
/// 1-3キーでSkybox色変更 → IBL反射が動的に変化。
///
/// 使用API:
///   - Renderer3DのIBL自動初期化（Skybox更新時に再生成）
///   - MeshGenerator::CreateSphere()
///   - Material (PBR metallic/roughness)
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"

#include <cmath>
#include <Windows.h>

class IBLShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Image-Based Lighting (IBL)";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 6; config.bgG = 8; config.bgB = 18;
        return config;
    }

    void Start() override
    {
        auto& ctx      = GX_Internal::CompatContext::Instance();
        auto& renderer  = ctx.renderer3D;
        auto& camera    = ctx.camera;
        auto& postFX    = ctx.postEffect;

        renderer.SetShadowEnabled(false);

        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // 球メッシュ
        m_sphereMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.45f, 24, 12));

        // 球マトリクス: 横=roughness, 縦=metallic
        const float spacing = 1.2f;
        const float xOffset = (k_GridSize - 1) * spacing * 0.5f;
        const float yOffset = (k_GridSize - 1) * spacing * 0.5f;

        for (int row = 0; row < k_GridSize; ++row)      // metallic
        {
            for (int col = 0; col < k_GridSize; ++col)  // roughness
            {
                int idx = row * k_GridSize + col;
                float metallic  = static_cast<float>(row) / (k_GridSize - 1);
                float roughness = static_cast<float>(col) / (k_GridSize - 1);

                // roughnessの最小値を0.05に（完全鏡面は不自然になりやすい）
                roughness = 0.05f + roughness * 0.95f;

                m_transforms[idx].SetPosition(
                    col * spacing - xOffset,
                    row * spacing - yOffset + k_GridSize * 0.5f + 1.0f,
                    0.0f
                );
                m_materials[idx].constants.albedoFactor = { 0.9f, 0.9f, 0.9f, 1.0f };
                m_materials[idx].constants.metallicFactor  = metallic;
                m_materials[idx].constants.roughnessFactor = roughness;
            }
        }

        // ライト（弱めにしてIBLの効果を際立たせる）
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -0.8f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 1.5f);
        renderer.SetLights(lights, 1, { 0.02f, 0.02f, 0.03f });

        // 初期Skybox色（プリセット1）
        ApplySkyboxPreset(0);

        // カメラ
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, k_GridSize * 0.5f + 1.0f, -12.0f);
        camera.LookAt({ 0.0f, k_GridSize * 0.5f + 1.0f, 0.0f });
    }

    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& kb  = ctx.inputManager.GetKeyboard();
        auto& camera = ctx.camera;

        m_totalTime += dt;
        m_lastDt = dt;

        // 1-3キーでSkybox色を変更
        if (kb.IsKeyTriggered('1')) ApplySkyboxPreset(0);
        if (kb.IsKeyTriggered('2')) ApplySkyboxPreset(1);
        if (kb.IsKeyTriggered('3')) ApplySkyboxPreset(2);

        // カメラ: ゆっくり左右に揺れる
        float sway = std::sin(m_totalTime * 0.3f) * 3.0f;
        camera.SetPosition(sway, k_GridSize * 0.5f + 1.0f, -12.0f);
        camera.LookAt({ 0.0f, k_GridSize * 0.5f + 1.0f, 0.0f });
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

        // 球マトリクス描画
        for (int i = 0; i < k_TotalSpheres; ++i)
        {
            ctx.renderer3D.SetMaterial(m_materials[i]);
            ctx.renderer3D.DrawMesh(m_sphereMesh, m_transforms[i]);
        }

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10, FormatT(TEXT("FPS: {:.1f}"), fps).c_str(), GetColor(255, 255, 255));
        DrawString(10, 35, TEXT("IBL: Metallic/Roughness sphere matrix (7x7)"),
                   GetColor(120, 180, 255));

        // 軸ラベル
        DrawString(10, 60, TEXT("  Rows (bottom->top): metallic 0 -> 1"), GetColor(180, 180, 180));
        DrawString(10, 80, TEXT("  Cols (left->right):  roughness 0 -> 1"), GetColor(180, 180, 180));

        // プリセット表示
        const TChar* presetNames[] = { TEXT("Blue Sky"), TEXT("Sunset"), TEXT("Night") };
        DrawString(10, 110,
                   FormatT(TEXT("Skybox: {} (1-3 to change)"), presetNames[m_currentPreset]).c_str(),
                   GetColor(255, 200, 100));
        DrawString(10, 135, TEXT("ESC: Quit"), GetColor(136, 136, 136));
    }

private:
    void ApplySkyboxPreset(int preset)
    {
        auto& renderer = GX_Internal::CompatContext::Instance().renderer3D;
        m_currentPreset = preset;

        struct SkyPreset { XMFLOAT3 top, bottom, sunDir; float sunInt; };
        SkyPreset presets[] = {
            // Blue Sky
            { {0.4f, 0.55f, 0.9f}, {0.75f, 0.8f, 0.9f}, {0.3f, -0.8f, 0.5f}, 5.0f },
            // Sunset
            { {0.15f, 0.1f, 0.4f}, {1.0f, 0.5f, 0.2f}, {-0.8f, -0.2f, 0.3f}, 8.0f },
            // Night
            { {0.02f, 0.02f, 0.08f}, {0.05f, 0.05f, 0.1f}, {0.0f, -1.0f, 0.0f}, 0.5f },
        };

        auto& p = presets[preset];
        renderer.GetSkybox().SetColors(p.top, p.bottom);
        renderer.GetSkybox().SetSun(p.sunDir, p.sunInt);
    }

    static constexpr int k_GridSize     = 7;
    static constexpr int k_TotalSpheres = k_GridSize * k_GridSize;

    float m_totalTime = 0.0f;
    float m_lastDt    = 0.0f;
    int   m_currentPreset = 0;

    GX::GPUMesh     m_sphereMesh;
    GX::Transform3D m_transforms[k_TotalSpheres];
    GX::Material    m_materials[k_TotalSpheres];
};

GX_EASY_APP(IBLShowcaseApp)
