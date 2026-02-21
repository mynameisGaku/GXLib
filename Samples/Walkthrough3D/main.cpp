/// @file Samples/Walkthrough3D/main.cpp
/// @brief 3Dウォークスルー。WASD/QEで移動するサンプル。
///
/// GXEasyの2D互換レイヤーの裏側にあるCompatContextを直接使い、
/// Renderer3D/Camera3D/PostEffectPipelineで3Dシーンを描画する。
/// DxLibには3D相当のAPIがないため、このサンプルはGXLib固有の使い方になる。
///
/// 構成:
///   - MeshGenerator: プリミティブメッシュ生成（平面/箱/球/円柱）
///   - Material: PBRマテリアル（albedo/metallic/roughness）
///   - Light: Directional/Point/Spotの3種ライト
///   - PostEffectPipeline: ACES Tonemap + Bloom + SSAO + FXAA
#include "GXEasy.h"
#include "Compat/CompatContext.h"      // GXEasyの内部コンテキスト
#include "Graphics/3D/MeshData.h"      // MeshGenerator（プリミティブ生成）
#include "Graphics/3D/Light.h"         // LightData（Directional/Point/Spot）
#include "Graphics/3D/Material.h"      // Material（PBRパラメータ）
#include "Graphics/3D/Fog.h"           // FogMode（Linear/Exp等）

#include <Windows.h>

/// @brief 3Dウォークスルーのメインクラス
///
/// CompatContextからRenderer3D/Camera3D/PostEffectPipelineを取得し、
/// PBRシーンを構築して自由に歩き回れるようにしている。
class WalkthroughApp : public GXEasy::App
{
public:
    /// @brief ウィンドウ設定
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: Walkthrough3D";
        config.width = 1280;
        config.height = 720;
        config.bgR = 6;
        config.bgG = 8;
        config.bgB = 18;
        return config;
    }

    /// @brief シーンの初期化（メッシュ・マテリアル・ライト・カメラの構築）
    void Start() override
    {
        // CompatContextはGXEasyの内部シングルトン。
        // Renderer3D/Camera3D/PostEffectPipelineなどの主要オブジェクトを保持している。
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& renderer = ctx.renderer3D;
        auto& camera = ctx.camera;
        auto& postFX = ctx.postEffect;

        // このサンプルではシャドウパスを回さないため無効にする。
        // 有効にする場合はBeginShadowPass〜EndShadowPassでDrawScene呼び出しが必要。
        renderer.SetShadowEnabled(false);

        // --- ポストエフェクト設定 ---
        // ACESトーンマッピング + Bloom + SSAO + FXAA の組み合わせ
        postFX.SetTonemapMode(GX::TonemapMode::ACES);
        postFX.GetBloom().SetEnabled(true);
        postFX.GetSSAO().SetEnabled(true);
        postFX.SetFXAAEnabled(true);

        // --- メッシュ生成 ---
        // MeshGeneratorでプリミティブを生成し、CreateGPUMeshでGPUバッファ化する
        m_planeMesh    = renderer.CreateGPUMesh(GX::MeshGenerator::CreatePlane(30.0f, 30.0f, 30, 30));
        m_cubeMesh     = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));
        m_sphereMesh   = renderer.CreateGPUMesh(GX::MeshGenerator::CreateSphere(0.5f, 32, 16));
        m_cylinderMesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateCylinder(0.25f, 0.25f, 3.0f, 16, 1));

        // --- マテリアル設定 ---
        // PBRのalbedoFactor（色）、metallicFactor（金属度）、roughnessFactor（粗さ）で質感を調整
        m_floorTransform.SetPosition(0, 0, 0);
        m_floorMat.constants.albedoFactor = { 0.5f, 0.5f, 0.52f, 1.0f };
        m_floorMat.constants.roughnessFactor = 0.9f;  // ざらざらした床

        // 赤/緑/青の3色キューブ
        float cubeColors[][3] = { {0.9f, 0.15f, 0.1f}, {0.1f, 0.85f, 0.15f}, {0.1f, 0.2f, 0.9f} };
        for (int i = 0; i < k_NumCubes; ++i)
        {
            m_cubeTransforms[i].SetPosition(-2.0f + i * 2.0f, 0.5f, 2.0f);
            m_cubeMats[i].constants.albedoFactor = { cubeColors[i][0], cubeColors[i][1], cubeColors[i][2], 1.0f };
            m_cubeMats[i].constants.roughnessFactor = 0.5f;
        }

        // 金属球（metallic=1.0, 低roughnessで鏡面反射が強い）
        m_sphereTransforms[0].SetPosition(-2.0f, 0.5f, -1.0f);
        m_sphereMats[0].constants.albedoFactor = { 1.0f, 0.85f, 0.4f, 1.0f };
        m_sphereMats[0].constants.metallicFactor = 1.0f;
        m_sphereMats[0].constants.roughnessFactor = 0.2f;

        // 非金属・粗い球（ざらざらした白）
        m_sphereTransforms[1].SetPosition(0.0f, 0.5f, -1.0f);
        m_sphereMats[1].constants.albedoFactor = { 0.95f, 0.95f, 0.9f, 1.0f };
        m_sphereMats[1].constants.roughnessFactor = 0.7f;

        // 中程度の粗さの青い球
        m_sphereTransforms[2].SetPosition(2.0f, 0.5f, -1.0f);
        m_sphereMats[2].constants.albedoFactor = { 0.1f, 0.4f, 0.9f, 1.0f };
        m_sphereMats[2].constants.roughnessFactor = 0.4f;

        // 四隅の柱（同一マテリアルを共有）
        float pillarPos[][2] = { {-4, 4}, {4, 4}, {-4, -4}, {4, -4} };
        for (int i = 0; i < k_NumPillars; ++i)
            m_pillarTransforms[i].SetPosition(pillarPos[i][0], 1.5f, pillarPos[i][1]);
        m_pillarMat.constants.albedoFactor = { 0.6f, 0.6f, 0.62f, 1.0f };
        m_pillarMat.constants.roughnessFactor = 0.6f;

        // --- ライト設定 ---
        // Directional（太陽）+ Point（暖色の照明）+ Spot（スポットライト）の3灯構成
        GX::LightData lights[3];
        lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        lights[1] = GX::Light::CreatePoint({ -3.0f, 3.0f, -3.0f }, 15.0f, { 1.0f, 0.95f, 0.9f }, 3.0f);
        lights[2] = GX::Light::CreateSpot({ 3.0f, 5.0f, -2.0f }, { -0.3f, -1.0f, 0.2f },
                                           20.0f, 30.0f, { 1.0f, 0.8f, 0.4f }, 10.0f);
        renderer.SetLights(lights, 3, { 0.05f, 0.05f, 0.05f });  // ambient=暗めの環境光

        // フォグ（遠距離で白くぼかす）
        renderer.SetFog(GX::FogMode::Linear, { 0.7f, 0.7f, 0.7f }, 30.0f, 100.0f);

        // プロシージャルスカイボックス（太陽方向はDirectionalライトと揃える）
        renderer.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
        renderer.GetSkybox().SetColors({ 0.5f, 0.55f, 0.6f }, { 0.75f, 0.75f, 0.75f });

        // --- カメラ設定 ---
        float aspect = static_cast<float>(ctx.swapChain.GetWidth()) / ctx.swapChain.GetHeight();
        camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
        camera.SetPosition(0.0f, 3.0f, -8.0f);
        camera.Rotate(0.3f, 0.0f);  // やや見下ろす角度
    }

    /// @brief カメラ操作（WASD移動 + マウス視点回転）
    void Update(float dt) override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& camera = ctx.camera;
        auto& mouse = ctx.inputManager.GetMouse();

        m_totalTime += dt;
        m_lastDt = dt;

        // --- マウスキャプチャ ---
        // 右クリックでマウスキャプチャのON/OFFを切り替える
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

        // キャプチャ中はマウス差分で視点回転
        if (m_mouseCaptured)
        {
            int mx = mouse.GetX();
            int my = mouse.GetY();
            camera.Rotate((float)(my - m_lastMY) * m_mouseSens, (float)(mx - m_lastMX) * m_mouseSens);
            m_lastMX = mx;
            m_lastMY = my;
        }

        // --- WASD/QE カメラ移動 ---
        // Shiftで3倍速
        float speed = m_cameraSpeed * dt;
        if (CheckHitKey(KEY_INPUT_LSHIFT)) speed *= 3.0f;
        if (CheckHitKey(KEY_INPUT_W)) camera.MoveForward(speed);
        if (CheckHitKey(KEY_INPUT_S)) camera.MoveForward(-speed);
        if (CheckHitKey(KEY_INPUT_D)) camera.MoveRight(speed);
        if (CheckHitKey(KEY_INPUT_A)) camera.MoveRight(-speed);
        if (CheckHitKey(KEY_INPUT_E)) camera.MoveUp(speed);
        if (CheckHitKey(KEY_INPUT_Q)) camera.MoveUp(-speed);
    }

    /// @brief 3Dシーンの描画
    ///
    /// 描画フローは:
    ///   1. FlushAll(): GXEasy側の2Dバッチをフラッシュ
    ///   2. BeginScene〜EndScene: HDR RTにPBR描画
    ///   3. DepthBuffer遷移: ポストエフェクト（SSAO等）がDepthを読むため
    ///   4. Resolve: HDR→LDR変換してバックバッファに出力
    ///   5. DepthBuffer復帰: 次フレームのDEPTH_WRITEに備える
    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto* cmd = ctx.cmdList;
        const uint32_t frameIndex = ctx.frameIndex;

        // GXEasyの2DバッチをFlush（3D描画前にクリアしないとステート競合する）
        ctx.FlushAll();

        // --- HDRシーン描画開始 ---
        // BeginSceneでHDR RTをバインドしDepthバッファをクリアする
        ctx.postEffect.BeginScene(cmd, frameIndex,
                                  ctx.renderer3D.GetDepthBuffer().GetDSVHandle(),
                                  ctx.camera);
        // Begin〜Endの間でSetMaterial→DrawMeshを繰り返す
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
        ctx.postEffect.EndScene();  // HDR RTへの描画を終了

        // --- ポストエフェクト解決 ---
        // SSAOやDoFがDepthBufferをSRVとして読むため、状態遷移が必要
        auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // HDR→LDR変換（Tonemap/Bloom/SSAO/FXAA等）→ バックバッファに出力
        ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                               depthBuffer,
                               ctx.camera, m_lastDt);

        // 次フレームのDepth描画に備えてDEPTH_WRITEに戻す
        depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // --- HUD（2D描画） ---
        // 3D描画の後にDxLib互換のDrawStringで2Dテキストを重ねる
        const float fps = (m_lastDt > 0.0f) ? (1.0f / m_lastDt) : 0.0f;
        const TString fpsText = FormatT(TEXT("FPS: {:.1f}"), fps);
        DrawString(10, 10, fpsText.c_str(), GetColor(255, 255, 255));
        auto pos = ctx.camera.GetPosition();
        const TString posText = FormatT(TEXT("Pos: ({:.1f}, {:.1f}, {:.1f})"), pos.x, pos.y, pos.z);
        DrawString(10, 35, posText.c_str(), GetColor(120, 180, 255));
        DrawString(10, 60,
                   TEXT("WASD: Move  QE: Up/Down  Shift: Fast  RClick: Mouse  ESC: Quit"),
                   GetColor(136, 136, 136));
    }

private:
    static constexpr int k_NumCubes = 3;    ///< キューブの数
    static constexpr int k_NumSpheres = 3;  ///< 球体の数（質感違い）
    static constexpr int k_NumPillars = 4;  ///< 柱の数（四隅）

    // --- カメラ制御パラメータ ---
    float m_cameraSpeed = 5.0f;     ///< 移動速度（m/s）
    float m_mouseSens = 0.003f;     ///< マウス感度
    bool  m_mouseCaptured = false;
    int   m_lastMX = 0;
    int   m_lastMY = 0;

    float m_totalTime = 0.0f;
    float m_lastDt = 0.0f;

    // --- GPUメッシュハンドル ---
    // CreateGPUMeshの戻り値。VB/IBのGPUリソースを内包している。
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

// エントリーポイント
GX_EASY_APP(WalkthroughApp)


