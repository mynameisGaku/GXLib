/// @file Samples/InstanceShowcase/main.cpp
/// @brief インスタンシング描画デモ
///
/// 10x10x10 = 1000個の金属球を DrawModelInstanced() 1回で描画する。
/// 各球はsin波で上下にアニメーションし、位相差でウェーブを形成する。
///
/// 使用API:
///   - Renderer3D::DrawModelInstanced(model, transforms, count)
///   - MeshGenerator::CreateSphere()
///   - Transform3D 配列の格子配置
#include "GXEasy.h"
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Mesh.h"

#include <cmath>
#include <Windows.h>

class InstanceShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"GXLib Sample: Instance Rendering (1000 Spheres)";
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

        // ポストエフェクト
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // 球メッシュからModelを構築
        auto meshData = GX::MeshGenerator::CreateSphere(0.4f, 16, 8);

        m_sphereModel = std::make_unique<GX::Model>();
        m_sphereModel->SetVertexType(GX::MeshVertexType::PBR);

        uint32_t vbSize = static_cast<uint32_t>(meshData.vertices.size() * sizeof(GX::Vertex3D_PBR));
        m_sphereModel->GetMesh().CreateVertexBuffer(
            ctx.device, meshData.vertices.data(), vbSize, sizeof(GX::Vertex3D_PBR));

        uint32_t ibSize = static_cast<uint32_t>(meshData.indices.size() * sizeof(uint32_t));
        m_sphereModel->GetMesh().CreateIndexBuffer(
            ctx.device, meshData.indices.data(), ibSize, DXGI_FORMAT_R32_UINT);

        // サブメッシュ（全インデックスを1つのサブメッシュとして登録）
        GX::SubMesh sub;
        sub.indexCount  = static_cast<uint32_t>(meshData.indices.size());
        sub.indexOffset = 0;
        sub.vertexOffset = 0;

        // マテリアルをMaterialManagerに登録
        GX::Material mat;
        mat.constants.albedoFactor    = { 0.95f, 0.8f, 0.4f, 1.0f };
        mat.constants.metallicFactor  = 1.0f;
        mat.constants.roughnessFactor = 0.3f;
        int matHandle = renderer.GetMaterialManager().CreateMaterial(mat);
        sub.materialHandle = matHandle;

        m_sphereModel->GetMesh().AddSubMesh(sub);
        m_sphereModel->AddMaterial(matHandle);

        // Transform3D配列の初期化（10x10x10格子）
        const float spacing = 2.0f;
        const float offset  = (k_GridSize - 1) * spacing * 0.5f;
        for (int x = 0; x < k_GridSize; ++x)
        {
            for (int y = 0; y < k_GridSize; ++y)
            {
                for (int z = 0; z < k_GridSize; ++z)
                {
                    int idx = x * k_GridSize * k_GridSize + y * k_GridSize + z;
                    m_transforms[idx].SetPosition(
                        x * spacing - offset,
                        y * spacing - offset + 12.0f,
                        z * spacing - offset
                    );
                }
            }
        }

        // 床
        m_floorMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(60.0f, 60.0f, 1, 1));
        m_floorTransform.SetPosition(0, -2, 0);
        m_floorMat.constants.albedoFactor    = { 0.3f, 0.3f, 0.32f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;

        // ライト
        GX::LightData lights[2];
        lights[0] = GX::Light::CreateDirectional({ 0.4f, -1.0f, 0.3f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        lights[1] = GX::Light::CreatePoint({ 0.0f, 20.0f, 0.0f }, 40.0f, { 0.8f, 0.9f, 1.0f }, 5.0f);
        renderer.SetLights(lights, 2, { 0.08f, 0.08f, 0.1f });

        // スカイボックス
        renderer.GetSkybox().SetSun({ 0.4f, -1.0f, 0.3f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.4f, 0.5f, 0.7f }, { 0.7f, 0.75f, 0.8f });

        // カメラ
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 15.0f, -30.0f);
        camera.Rotate(0.15f, 0.0f);
    }

    void Update(float dt) override
    {
        auto& ctx    = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;

        m_totalTime += dt;
        m_lastDt = dt;

        // sin波でY位置をアニメーション
        const float spacing = 2.0f;
        const float offset  = (k_GridSize - 1) * spacing * 0.5f;
        for (int x = 0; x < k_GridSize; ++x)
        {
            for (int y = 0; y < k_GridSize; ++y)
            {
                for (int z = 0; z < k_GridSize; ++z)
                {
                    int idx = x * k_GridSize * k_GridSize + y * k_GridSize + z;
                    float phase = (float)(x + z) * 0.5f;
                    float wave  = std::sin(m_totalTime * 2.0f + phase) * 1.5f;
                    m_transforms[idx].SetPosition(
                        x * spacing - offset,
                        y * spacing - offset + 12.0f + wave,
                        z * spacing - offset
                    );
                }
            }
        }

        // カメラ自動回転
        float camAngle = m_totalTime * 0.2f;
        float camDist  = 35.0f;
        camera.SetPosition(std::cos(camAngle) * camDist, 15.0f, std::sin(camAngle) * camDist);
        camera.LookAt({ 0.0f, 10.0f, 0.0f });
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

        // 床
        ctx.renderer3D.SetMaterial(m_floorMat);
        ctx.renderer3D.DrawMesh(m_floorMesh, m_floorTransform);

        // 1000個の球をインスタンシング描画
        ctx.renderer3D.DrawModelInstanced(*m_sphereModel, m_transforms, k_TotalInstances);

        ctx.renderer3D.End();
        ctx.postEffect.EndScene();

        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer, ctx.camera, m_lastDt);
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // HUD
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        DrawString(10, 10, FormatT(TEXT("FPS: {:.1f}  Instances: {}"), fps, k_TotalInstances).c_str(),
                   GetColor(255, 255, 255));
        DrawString(10, 35, TEXT("1000 metallic spheres drawn with 1 DrawModelInstanced() call"),
                   GetColor(120, 180, 255));
        DrawString(10, 60, TEXT("Camera auto-rotates. ESC: Quit"),
                   GetColor(136, 136, 136));
    }

private:
    static constexpr int k_GridSize       = 10;
    static constexpr int k_TotalInstances = k_GridSize * k_GridSize * k_GridSize;

    float m_totalTime = 0.0f;
    float m_lastDt    = 0.0f;

    std::unique_ptr<GX::Model> m_sphereModel;
    GX::Transform3D m_transforms[k_TotalInstances];

    GX::GPUMesh     m_floorMesh;
    GX::Transform3D m_floorTransform;
    GX::Material    m_floorMat;
};

GX_EASY_APP(InstanceShowcaseApp)
