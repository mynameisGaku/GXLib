/// @file main.cpp
/// @brief GXLib Phase 10 テストアプリケーション — GPUProfiler
///
/// HDR浮動小数点RTに3D描画 → PostFX → LDR + GUI オーバーレイ

#include "pch.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Device/CommandQueue.h"
#include "Graphics/Device/CommandList.h"
#include "Graphics/Device/SwapChain.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/PrimitiveBatch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Transform3D.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Fog.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/Layer/LayerStack.h"
#include "Graphics/Layer/LayerCompositor.h"
#include "Graphics/Layer/MaskScreen.h"
#include "Input/InputManager.h"
#include "GUI/UIContext.h"
#include "GUI/UIRenderer.h"
#include "GUI/StyleSheet.h"
#include "GUI/Widgets/Panel.h"
#include "GUI/Widgets/TextWidget.h"
#include "GUI/Widgets/Button.h"
#include "GUI/Widgets/Spacer.h"
#include "GUI/Widgets/ProgressBar.h"
#include "GUI/Widgets/Image.h"
#include "GUI/Widgets/CheckBox.h"
#include "GUI/Widgets/Slider.h"
#include "GUI/Widgets/ScrollView.h"
#include "GUI/Widgets/RadioButton.h"
#include "GUI/Widgets/DropDown.h"
#include "GUI/Widgets/ListView.h"
#include "GUI/Widgets/TabView.h"
#include "GUI/Widgets/Dialog.h"
#include "GUI/Widgets/Canvas.h"
#include "GUI/Widgets/TextInput.h"
#include "GUI/XMLParser.h"
#include "GUI/GUILoader.h"

// Phase 7: ファイル/ネットワーク/ムービー
#include "IO/FileSystem.h"

#include "IO/PhysicalFileProvider.h"
#include "IO/Archive.h"
#include "IO/ArchiveFileProvider.h"
#include "IO/AsyncLoader.h"
#include "IO/FileWatcher.h"
#include "IO/Network/HTTPClient.h"
#include "Movie/MoviePlayer.h"

// Phase 9: ShaderLibrary + ShaderHotReload（シェーダーの即時反映）
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Graphics/Pipeline/ShaderHotReload.h"

// Phase 10: GPUプロファイラ
#include "Graphics/Device/GPUProfiler.h"

// Phase 8: 数学/物理/当たり判定
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Math/Quaternion.h"
#include "Math/Color.h"
#include "Math/MathUtil.h"
#include "Math/Random.h"
#include "Math/Collision/Collision2D.h"
#include "Math/Collision/Collision3D.h"
#include "Math/Collision/Quadtree.h"
#include "Physics/RigidBody2D.h"
#include "Physics/PhysicsWorld2D.h"
#include "Physics/PhysicsWorld3D.h"

// ============================================================================
// グローバル変数
// ============================================================================
static GX::Application       g_app;
static GX::GraphicsDevice    g_device;
static GX::CommandQueue      g_commandQueue;
static GX::CommandList       g_commandList;
static GX::SwapChain         g_swapChain;

static GX::SpriteBatch       g_spriteBatch;
static GX::PrimitiveBatch   g_primBatch2D;
static GX::FontManager       g_fontManager;
static GX::TextRenderer      g_textRenderer;
static GX::InputManager      g_inputManager;

// 3D関連
static GX::Renderer3D        g_renderer3D;
static GX::Camera3D          g_camera;

// ポストエフェクトaa
static GX::PostEffectPipeline g_postEffect;

// レイヤーシステム
static GX::LayerStack       g_layerStack;
static GX::LayerCompositor  g_compositor;
static GX::RenderLayer*     g_sceneLayer = nullptr;  // Z:0, PostFX=true
static GX::RenderLayer*     g_uiLayer    = nullptr;  // Z:1000
static GX::MaskScreen       g_maskScreen;
static bool                 g_maskDemo   = false;     // マスクデモ表示

// GUI関連
static GX::GUI::UIRenderer   g_uiRenderer;
static GX::GUI::UIContext    g_uiContext;
static GX::GUI::StyleSheet  g_styleSheet;
static bool                 g_guiDemo    = false;     // GUIデモ表示
static int                  g_guiFontHandle = -1;
static int                  g_guiFontLarge  = -1;

// メッシュ
static GX::GPUMesh           g_sphereMesh;
static GX::GPUMesh           g_planeMesh;
static GX::GPUMesh           g_cubeMesh;
static GX::GPUMesh           g_cylinderMesh;
static GX::GPUMesh           g_tallBoxMesh;
static GX::GPUMesh           g_wallMesh;

// 球体（少数、AO確認用）
static constexpr int k_NumSpheres = 3;
static GX::Transform3D g_sphereTransforms[k_NumSpheres];
static GX::Material    g_sphereMaterials[k_NumSpheres];

// 地面
static GX::Transform3D g_planeTransform;
static GX::Material    g_planeMaterial;

// 箱の集まり（地面に置いた箱群 → 接地部AO確認）
static constexpr int k_NumBoxes = 6;
static GX::Transform3D g_boxTransforms[k_NumBoxes];
static GX::Material    g_boxMaterials[k_NumBoxes];

// 柱（円柱 → 地面との接合部AO確認）
static constexpr int k_NumPillars = 4;
static GX::Transform3D g_pillarTransforms[k_NumPillars];
static GX::Material    g_pillarMaterial;

// 壁（L字コーナー → 凹角AO確認）
static constexpr int k_NumWalls = 2;
static GX::Transform3D g_wallTransforms[k_NumWalls];
static GX::Material    g_wallMaterial;

// 段差（階段状 → 角のAO確認）
static constexpr int k_NumSteps = 4;
static GX::Transform3D g_stepTransforms[k_NumSteps];
static GX::Material    g_stepMaterial;

// 回転キューブ
static GX::Transform3D g_cubeTransform;
static GX::Material    g_cubeMaterial;

// SSRデモ用: ミラーウォール + カラフルオブジェクト
static GX::GPUMesh     g_mirrorMesh;
static GX::Transform3D g_mirrorTransform;
static GX::Material    g_mirrorMaterial;

static constexpr int k_NumSSRDemoObjs = 3;
static GX::Transform3D g_ssrDemoTransforms[k_NumSSRDemoObjs];
static GX::Material    g_ssrDemoMaterials[k_NumSSRDemoObjs];

// Phase 7: ファイル/ネット/ムービー
static GX::MoviePlayer       g_moviePlayer;
static GX::HTTPClient        g_httpClient;
static int                   g_httpStatusCode = 0;
static std::string           g_httpStatusText = "Not tested";
static bool                  g_archiveDemo    = false;

// Phase 8: 2D物理
static GX::PhysicsWorld2D    g_physicsWorld2D;
static bool                  g_physics2DDemo = false;
static constexpr int k_NumPhys2DBodies = 5;

// Phase 8: 3D物理（Jolt）
static GX::PhysicsWorld3D    g_physicsWorld3D;
static bool                  g_physics3DInit = false;
static GX::PhysicsShape*     g_floorShape = nullptr;
static GX::PhysicsShape*     g_ballShape = nullptr;
static GX::PhysicsShape*     g_boxPhysShape = nullptr;
static GX::PhysicsShape*     g_capsulePhysShape = nullptr;
static GX::PhysicsBodyID     g_floorBodyID;

enum class PhysShapeType { Sphere, Box, Capsule };
struct PhysObject {
    GX::PhysicsBodyID id;
    PhysShapeType     shapeType;
    GX::Material      material;
};
static std::vector<PhysObject> g_physObjects;
static GX::GPUMesh           g_physSphereMesh;
static GX::GPUMesh           g_physBoxMesh;
static GX::GPUMesh           g_physCapsuleMesh;
static constexpr int k_MaxPhysObjects = 100;

// Phase 9: シェーダーホットリロード
static bool g_showHotReloadStatus = false;

// Phase 10: GPUプロファイラ
static bool g_showProfiler = false;

static uint64_t g_frameFenceValues[GX::SwapChain::k_BufferCount] = {};
static uint32_t g_frameIndex = 0;
static float    g_totalTime  = 0.0f;
static int g_fontHandle = -1;

// カメラ制御
static float g_cameraSpeed      = 5.0f;
static float g_mouseSensitivity = 0.003f;
static bool  g_mouseCaptured    = false;
static int   g_lastMouseX = 0;
static int   g_lastMouseY = 0;

// ============================================================================
// シーン描画（シャドウパスとメインパスで共通）
// ============================================================================

void DrawScene(bool drawPhysics = true)
{
    // 地面
    g_renderer3D.SetMaterial(g_planeMaterial);
    g_renderer3D.DrawMesh(g_planeMesh, g_planeTransform);

    // 球体
    for (int i = 0; i < k_NumSpheres; ++i)
    {
        g_renderer3D.SetMaterial(g_sphereMaterials[i]);
        g_renderer3D.DrawMesh(g_sphereMesh, g_sphereTransforms[i]);
    }

    // 箱の集まり
    for (int i = 0; i < k_NumBoxes; ++i)
    {
        g_renderer3D.SetMaterial(g_boxMaterials[i]);
        g_renderer3D.DrawMesh(g_cubeMesh, g_boxTransforms[i]);
    }

    // 柱
    g_renderer3D.SetMaterial(g_pillarMaterial);
    for (int i = 0; i < k_NumPillars; ++i)
        g_renderer3D.DrawMesh(g_cylinderMesh, g_pillarTransforms[i]);

    // 壁（L字コーナー）
    g_renderer3D.SetMaterial(g_wallMaterial);
    for (int i = 0; i < k_NumWalls; ++i)
        g_renderer3D.DrawMesh(g_wallMesh, g_wallTransforms[i]);

    // 段差
    g_renderer3D.SetMaterial(g_stepMaterial);
    for (int i = 0; i < k_NumSteps; ++i)
        g_renderer3D.DrawMesh(g_tallBoxMesh, g_stepTransforms[i]);

    // 回転キューブ
    g_renderer3D.SetMaterial(g_cubeMaterial);
    g_renderer3D.DrawMesh(g_cubeMesh, g_cubeTransform);

    // SSRデモ: ミラーウォール
    g_renderer3D.SetMaterial(g_mirrorMaterial);
    g_renderer3D.DrawMesh(g_mirrorMesh, g_mirrorTransform);

    // SSRデモ: カラフルオブジェクト（ミラーの前）
    for (int i = 0; i < k_NumSSRDemoObjs; ++i)
    {
        g_renderer3D.SetMaterial(g_ssrDemoMaterials[i]);
        g_renderer3D.DrawMesh(g_sphereMesh, g_ssrDemoTransforms[i]);
    }

    // Phase 8: Jolt物理オブジェクトを描画（影パスでは省略してCB枠を節約）。
    // 影用描画は負荷が高いので、必要なものだけ描くと高速化につながる。
    if (drawPhysics && g_physics3DInit)
    {
        for (auto& obj : g_physObjects)
        {
            if (!obj.id.IsValid()) continue;
            GX::Matrix4x4 worldMat = g_physicsWorld3D.GetWorldTransform(obj.id);
            XMMATRIX xmWorld = XMLoadFloat4x4(&worldMat);
            g_renderer3D.SetMaterial(obj.material);
            switch (obj.shapeType)
            {
            case PhysShapeType::Sphere:  g_renderer3D.DrawMesh(g_physSphereMesh, xmWorld); break;
            case PhysShapeType::Box:     g_renderer3D.DrawMesh(g_physBoxMesh, xmWorld); break;
            case PhysShapeType::Capsule: g_renderer3D.DrawMesh(g_physCapsuleMesh, xmWorld); break;
            }
        }
    }
}

// ============================================================================
// 初期化
// ============================================================================

bool InitializeGraphics()
{
    auto* device = g_device.GetDevice();

    if (!g_commandQueue.Initialize(device))
        return false;
    if (!g_commandList.Initialize(device))
        return false;

    GX::SwapChainDesc scDesc;
    scDesc.hwnd   = g_app.GetWindow().GetHWND();
    scDesc.width  = g_app.GetWindow().GetWidth();
    scDesc.height = g_app.GetWindow().GetHeight();

    if (!g_swapChain.Initialize(g_device.GetFactory(), device, g_commandQueue.GetQueue(), scDesc))
        return false;

    return true;
}

bool InitializeRenderers()
{
    auto* device = g_device.GetDevice();
    auto* queue  = g_commandQueue.GetQueue();
    uint32_t w = g_app.GetWindow().GetWidth();
    uint32_t h = g_app.GetWindow().GetHeight();

    if (!g_spriteBatch.Initialize(device, queue, w, h))
        return false;
    if (!g_primBatch2D.Initialize(device, w, h))
        return false;
    if (!g_fontManager.Initialize(device, &g_spriteBatch.GetTextureManager()))
        return false;
    g_textRenderer.Initialize(&g_spriteBatch, &g_fontManager);

    if (!g_renderer3D.Initialize(device, queue, w, h))
        return false;

    // ポストエフェクトパイプライン
    if (!g_postEffect.Initialize(device, w, h))
        return false;

    // JSON設定ファイル読み込み（存在すれば）
    g_postEffect.LoadSettings("post_effects.json");

    // レイヤーシステム初期化
    g_sceneLayer = g_layerStack.CreateLayer(device, "Scene", 0, w, h);
    if (!g_sceneLayer) return false;
    g_sceneLayer->SetBlendMode(GX::LayerBlendMode::None);
    g_sceneLayer->SetPostFXEnabled(true);

    g_uiLayer = g_layerStack.CreateLayer(device, "UI", 1000, w, h);
    if (!g_uiLayer) return false;
    g_uiLayer->SetBlendMode(GX::LayerBlendMode::Alpha);

    if (!g_compositor.Initialize(device, w, h))
        return false;

    // マスクスクリーン
    if (!g_maskScreen.Create(device, w, h))
        return false;

    // GUI レンダラー
    if (!g_uiRenderer.Initialize(device, queue, w, h,
                                  &g_spriteBatch, &g_textRenderer, &g_fontManager))
        return false;
    if (!g_uiContext.Initialize(&g_uiRenderer, w, h))
        return false;

    // デザイン解像度: GUI コンテンツが 1:1 で収まるベース解像度
    // 画面がこれより小さい場合は自動で縮小される
    g_uiContext.SetDesignResolution(1280, 960);

    return true;
}

bool InitializeScene()
{
    g_fontHandle = g_fontManager.CreateFont(L"Meiryo", 20);
    if (g_fontHandle < 0)
        g_fontHandle = g_fontManager.CreateFont(L"MS Gothic", 20);

    // GUI フォント
    g_guiFontHandle = g_fontManager.CreateFont(L"Meiryo", 24);
    if (g_guiFontHandle < 0) g_guiFontHandle = g_fontHandle;
    g_guiFontLarge = g_fontManager.CreateFont(L"Meiryo", 48);
    if (g_guiFontLarge < 0) g_guiFontLarge = g_guiFontHandle;

    // === GUI デモ構築（Phase 6c: XML + GUILoader） ===
    {
        using namespace GX::GUI;
        uint32_t sw = g_app.GetWindow().GetWidth();
        uint32_t sh = g_app.GetWindow().GetHeight();

        // スタイルシート読み込み
        if (!g_styleSheet.LoadFromFile("Assets/ui/menu.css"))
            GX_LOG_WARN("Failed to load Assets/ui/menu.css");
        g_uiContext.SetStyleSheet(&g_styleSheet);

        // GUILoader でXMLからウィジェットツリーを構築
        GUILoader loader;
        loader.SetRenderer(&g_uiRenderer);
        loader.RegisterFont("default", g_guiFontHandle);
        loader.RegisterFont("large", g_guiFontLarge);
        loader.RegisterEvent("onStartGame", []() { GX_LOG_INFO("Button 'Start Game' clicked!"); });
        loader.RegisterEvent("onOpenOptions", []() { GX_LOG_INFO("Button 'Options' clicked!"); });
        loader.RegisterEvent("onExit", []() { PostQuitMessage(0); });
        loader.RegisterValueChangedEvent("onVolumeChanged", [](const std::string& v) {
            GX_LOG_INFO("Volume changed: %s", v.c_str());
        });
        loader.RegisterValueChangedEvent("onBrightnessChanged", [](const std::string& v) {
            GX_LOG_INFO("Brightness changed: %s", v.c_str());
        });
        loader.RegisterValueChangedEvent("onFullscreenChanged", [](const std::string& v) {
            GX_LOG_INFO("Fullscreen changed: %s", v.c_str());
        });
        loader.RegisterValueChangedEvent("onVSyncChanged", [](const std::string& v) {
            GX_LOG_INFO("V-Sync changed: %s", v.c_str());
        });
        loader.RegisterValueChangedEvent("onDifficultyChanged", [](const std::string& v) {
            GX_LOG_INFO("Difficulty changed: %s", v.c_str());
        });
        loader.RegisterValueChangedEvent("onResolutionChanged", [](const std::string& v) {
            GX_LOG_INFO("Resolution changed: %s", v.c_str());
        });
        loader.RegisterValueChangedEvent("onMapSelected", [](const std::string& v) {
            GX_LOG_INFO("Map selected: %s", v.c_str());
        });
        loader.RegisterValueChangedEvent("onNameChanged", [](const std::string& v) {
            GX_LOG_INFO("Name changed: %s", v.c_str());
        });

        // Dialog イベント
        loader.RegisterEvent("onShowDialog", []() {
            auto* dlg = g_uiContext.FindById("confirmDialog");
            if (dlg) dlg->visible = true;
        });
        loader.RegisterEvent("onDialogClose", []() {
            auto* dlg = g_uiContext.FindById("confirmDialog");
            if (dlg) dlg->visible = false;
        });
        loader.RegisterEvent("onDialogYes", []() {
            GX_LOG_INFO("Dialog: Yes clicked!");
            auto* dlg = g_uiContext.FindById("confirmDialog");
            if (dlg) dlg->visible = false;
        });
        loader.RegisterEvent("onDialogNo", []() {
            GX_LOG_INFO("Dialog: No clicked!");
            auto* dlg = g_uiContext.FindById("confirmDialog");
            if (dlg) dlg->visible = false;
        });

        // Canvas 描画コールバック
        loader.RegisterDrawCallback("onCanvasDraw",
            [](GX::GUI::UIRenderer& renderer, const GX::GUI::LayoutRect& rect) {
            // 簡単な棒グラフデモ
            float barW = 30.0f;
            float gap = 10.0f;
            float values[] = { 0.3f, 0.7f, 0.5f, 0.9f, 0.4f, 0.6f, 0.8f, 0.2f };
            GX::GUI::StyleColor colors[] = {
                {1.0f, 0.3f, 0.3f, 0.8f}, {0.3f, 1.0f, 0.3f, 0.8f},
                {0.3f, 0.3f, 1.0f, 0.8f}, {1.0f, 1.0f, 0.3f, 0.8f},
                {1.0f, 0.3f, 1.0f, 0.8f}, {0.3f, 1.0f, 1.0f, 0.8f},
                {1.0f, 0.6f, 0.2f, 0.8f}, {0.6f, 0.3f, 0.9f, 0.8f}
            };
            for (int i = 0; i < 8; ++i)
            {
                float x = rect.x + 10.0f + i * (barW + gap);
                float h = values[i] * (rect.height - 10.0f);
                float y = rect.y + rect.height - h - 5.0f;
                renderer.DrawSolidRect(x, y, barW, h, colors[i]);
            }
        });

        auto root = loader.BuildFromFile("Assets/ui/menu.xml");
        if (root)
        {
            root->computedStyle.width = StyleLength::Px(static_cast<float>(sw));
            root->computedStyle.height = StyleLength::Px(static_cast<float>(sh));
            g_uiContext.SetRoot(std::move(root));
        }
    }

    // メッシュ生成
    auto sphereData   = GX::MeshGenerator::CreateSphere(0.5f, 32, 16);
    auto planeData    = GX::MeshGenerator::CreatePlane(40.0f, 40.0f, 40, 40);
    auto boxData      = GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f);
    auto cylinderData = GX::MeshGenerator::CreateCylinder(0.3f, 0.3f, 3.0f, 16, 1);
    auto tallBoxData  = GX::MeshGenerator::CreateBox(2.0f, 0.5f, 3.0f);
    auto wallData     = GX::MeshGenerator::CreateBox(0.3f, 3.0f, 6.0f);

    g_sphereMesh   = g_renderer3D.CreateGPUMesh(sphereData);
    g_planeMesh    = g_renderer3D.CreateGPUMesh(planeData);
    g_cubeMesh     = g_renderer3D.CreateGPUMesh(boxData);
    g_cylinderMesh = g_renderer3D.CreateGPUMesh(cylinderData);
    g_tallBoxMesh  = g_renderer3D.CreateGPUMesh(tallBoxData);
    g_wallMesh     = g_renderer3D.CreateGPUMesh(wallData);

    // === 球体（地面に接するように配置 → 接地部AO） ===
    g_sphereTransforms[0].SetPosition(0.0f, 0.5f, 0.0f);
    g_sphereMaterials[0].constants.albedoFactor = { 0.8f, 0.2f, 0.2f, 1.0f };
    g_sphereMaterials[0].constants.metallicFactor  = 0.0f;
    g_sphereMaterials[0].constants.roughnessFactor = 0.5f;

    g_sphereTransforms[1].SetPosition(1.5f, 0.5f, 0.0f);
    g_sphereMaterials[1].constants.albedoFactor = { 0.2f, 0.8f, 0.2f, 1.0f };
    g_sphereMaterials[1].constants.metallicFactor  = 0.5f;
    g_sphereMaterials[1].constants.roughnessFactor = 0.3f;

    // 箱の角に置いた球 → 箱と球の間にAO
    g_sphereTransforms[2].SetPosition(-3.0f, 1.5f, 2.0f);
    g_sphereMaterials[2].constants.albedoFactor = { 0.2f, 0.2f, 0.8f, 1.0f };
    g_sphereMaterials[2].constants.metallicFactor  = 0.0f;
    g_sphereMaterials[2].constants.roughnessFactor = 0.8f;

    // === 地面 ===
    g_planeTransform.SetPosition(0.0f, 0.0f, 0.0f);
    g_planeMaterial.constants.albedoFactor = { 0.5f, 0.5f, 0.52f, 1.0f };
    g_planeMaterial.constants.metallicFactor  = 0.0f;
    g_planeMaterial.constants.roughnessFactor = 0.9f;

    // === 箱の集まり（密集配置 → 箱間のAO） ===
    {
        XMFLOAT4 boxColor = { 0.7f, 0.65f, 0.55f, 1.0f };
        // 地面に並べた箱
        float bx = -3.0f, bz = 0.0f;
        g_boxTransforms[0].SetPosition(bx, 0.5f, bz);
        g_boxTransforms[1].SetPosition(bx + 1.05f, 0.5f, bz);
        g_boxTransforms[2].SetPosition(bx + 0.5f, 0.5f, bz + 1.05f);
        // 積み上げた箱
        g_boxTransforms[3].SetPosition(bx, 1.5f, bz);
        g_boxTransforms[3].SetRotation(0.0f, 0.3f, 0.0f);
        // 大きさ違い
        g_boxTransforms[4].SetPosition(bx + 2.5f, 0.75f, bz);
        g_boxTransforms[4].SetScale(1.5f, 1.5f, 1.5f);
        // 傾いた箱
        g_boxTransforms[5].SetPosition(bx + 1.0f, 0.5f, bz - 1.5f);
        g_boxTransforms[5].SetRotation(0.0f, 0.78f, 0.0f);
        for (int i = 0; i < k_NumBoxes; ++i)
        {
            g_boxMaterials[i].constants.albedoFactor = boxColor;
            g_boxMaterials[i].constants.metallicFactor  = 0.0f;
            g_boxMaterials[i].constants.roughnessFactor = 0.7f;
        }
    }

    // === 柱（地面との接合部 → 根元AO） ===
    g_pillarTransforms[0].SetPosition(4.0f, 1.5f, 3.0f);
    g_pillarTransforms[1].SetPosition(6.0f, 1.5f, 3.0f);
    g_pillarTransforms[2].SetPosition(4.0f, 1.5f, 5.0f);
    g_pillarTransforms[3].SetPosition(6.0f, 1.5f, 5.0f);
    g_pillarMaterial.constants.albedoFactor = { 0.6f, 0.6f, 0.6f, 1.0f };
    g_pillarMaterial.constants.metallicFactor  = 0.0f;
    g_pillarMaterial.constants.roughnessFactor = 0.6f;

    // === L字壁（コーナー凹角 → 強いAO） ===
    g_wallTransforms[0].SetPosition(8.0f, 1.5f, 0.0f);   // 壁1（Z方向）
    g_wallTransforms[1].SetPosition(8.0f + 3.0f, 1.5f, -2.85f); // 壁2（X方向）
    g_wallTransforms[1].SetRotation(0.0f, XM_PIDIV2, 0.0f);
    g_wallMaterial.constants.albedoFactor = { 0.75f, 0.72f, 0.68f, 1.0f };
    g_wallMaterial.constants.metallicFactor  = 0.0f;
    g_wallMaterial.constants.roughnessFactor = 0.85f;

    // === 段差（階段状） ===
    for (int i = 0; i < k_NumSteps; ++i)
    {
        float y = (i + 1) * 0.25f;  // 0.25, 0.5, 0.75, 1.0
        float z = -4.0f + i * 1.0f;
        g_stepTransforms[i].SetPosition(0.0f, y, z);
    }
    g_stepMaterial.constants.albedoFactor = { 0.55f, 0.55f, 0.6f, 1.0f };
    g_stepMaterial.constants.metallicFactor  = 0.0f;
    g_stepMaterial.constants.roughnessFactor = 0.8f;

    // === 回転キューブ ===
    g_cubeTransform.SetPosition(3.0f, 0.5f, -2.0f);
    g_cubeMaterial.constants.albedoFactor = { 0.95f, 0.93f, 0.88f, 1.0f };
    g_cubeMaterial.constants.metallicFactor  = 1.0f;
    g_cubeMaterial.constants.roughnessFactor = 0.3f;

    // === SSRデモ: ミラーウォール ===
    {
        // 大きな薄い壁（鏡面）— 右側の明るいエリアに配置
        auto mirrorData = GX::MeshGenerator::CreateBox(0.1f, 4.0f, 8.0f);
        g_mirrorMesh = g_renderer3D.CreateGPUMesh(mirrorData);
        g_mirrorTransform.SetPosition(12.0f, 2.0f, 0.0f);
        // 完全な鏡面: metallic=1, roughness=0, 明るいシルバー
        g_mirrorMaterial.constants.albedoFactor = { 0.95f, 0.95f, 0.97f, 1.0f };
        g_mirrorMaterial.constants.metallicFactor  = 1.0f;
        g_mirrorMaterial.constants.roughnessFactor = 0.0f;
    }

    // === SSRデモ: ミラーの前にカラフルな球体（右側、青ライトから遠い） ===
    {
        // 赤い球
        g_ssrDemoTransforms[0].SetPosition(10.0f, 1.0f, -2.0f);
        g_ssrDemoTransforms[0].SetScale(1.5f, 1.5f, 1.5f);
        g_ssrDemoMaterials[0].constants.albedoFactor = { 1.0f, 0.1f, 0.1f, 1.0f };
        g_ssrDemoMaterials[0].constants.metallicFactor  = 0.0f;
        g_ssrDemoMaterials[0].constants.roughnessFactor = 0.3f;

        // 黄色い球
        g_ssrDemoTransforms[1].SetPosition(10.0f, 1.0f, 0.0f);
        g_ssrDemoTransforms[1].SetScale(1.5f, 1.5f, 1.5f);
        g_ssrDemoMaterials[1].constants.albedoFactor = { 1.0f, 0.9f, 0.1f, 1.0f };
        g_ssrDemoMaterials[1].constants.metallicFactor  = 0.0f;
        g_ssrDemoMaterials[1].constants.roughnessFactor = 0.3f;

        // 青い球
        g_ssrDemoTransforms[2].SetPosition(10.0f, 1.0f, 2.0f);
        g_ssrDemoTransforms[2].SetScale(1.5f, 1.5f, 1.5f);
        g_ssrDemoMaterials[2].constants.albedoFactor = { 0.1f, 0.3f, 1.0f, 1.0f };
        g_ssrDemoMaterials[2].constants.metallicFactor  = 0.0f;
        g_ssrDemoMaterials[2].constants.roughnessFactor = 0.3f;
    }

    // ライト設定
    GX::LightData lights[3];
    lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
    lights[1] = GX::Light::CreatePoint({ -3.0f, 3.0f, -3.0f }, 15.0f, { 1.0f, 0.95f, 0.9f }, 3.0f);
    lights[2] = GX::Light::CreateSpot({ 4.0f, 4.0f, -2.0f }, { -0.5f, -1.0f, 0.3f },
                                        20.0f, 30.0f, { 1.0f, 0.8f, 0.3f }, 15.0f);
    g_renderer3D.SetLights(lights, 3, { 0.05f, 0.05f, 0.05f });

    // フォグ設定（Linear）
    g_renderer3D.SetFog(GX::FogMode::Linear, { 0.7f, 0.7f, 0.7f }, 30.0f, 150.0f);

    // スカイボックスの太陽方向をDirectionalライトと合わせる
    g_renderer3D.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 5.0f);
    g_renderer3D.GetSkybox().SetColors({ 0.5f, 0.55f, 0.6f }, { 0.75f, 0.75f, 0.75f });

    // VolumetricLight: ディレクショナルライトと同じ方向・色を設定
    g_postEffect.GetVolumetricLight().SetLightDirection({ 0.3f, -1.0f, 0.5f });
    g_postEffect.GetVolumetricLight().SetLightColor({ 1.0f, 0.98f, 0.95f });

    // カメラ
    uint32_t w = g_app.GetWindow().GetWidth();
    uint32_t h = g_app.GetWindow().GetHeight();
    g_camera.SetPerspective(XM_PIDIV4, static_cast<float>(w) / h, 0.1f, 1000.0f);
    g_camera.SetPosition(2.0f, 4.0f, -8.0f);
    g_camera.Rotate(0.35f, 0.0f);

    // === Phase 8: 数学の検証 ===
    {
        GX::Vector3 a(1, 2, 3), b(4, 5, 6);
        float dot = a.Dot(b);    // 32
        GX::Vector3 cross = a.Cross(b); // (-3, 6, -3)
        float len = a.Length();  // 3.742...
        GX_LOG_INFO("Math Test: dot(1,2,3).(4,5,6)=%.1f  cross=(%.1f,%.1f,%.1f)  len=%.3f",
            dot, cross.x, cross.y, cross.z, len);

        GX::Matrix4x4 m = GX::Matrix4x4::RotationY(GX::MathUtil::PI / 4.0f);
        GX::Matrix4x4 inv = m.Inverse();
        GX::Matrix4x4 identity = m * inv;
        GX_LOG_INFO("Math Test: M*M^-1 diagonal = (%.3f, %.3f, %.3f, %.3f)",
            identity._11, identity._22, identity._33, identity._44);

        GX::Quaternion q = GX::Quaternion::FromAxisAngle(GX::Vector3(0, 1, 0), GX::MathUtil::PI / 2.0f);
        GX::Vector3 rotated = q.RotateVector(GX::Vector3(1, 0, 0));
        GX_LOG_INFO("Math Test: Rotate (1,0,0) by Y90 = (%.3f, %.3f, %.3f)",
            rotated.x, rotated.y, rotated.z);
    }

    // === Phase 8: 2D当たり判定テスト ===
    {
        GX::Circle c1({ 0.0f, 0.0f }, 1.0f);
        GX::Circle c2({ 1.5f, 0.0f }, 1.0f);
        bool hit = GX::Collision2D::TestCirclevsCircle(c1, c2);
        GX_LOG_INFO("Collision2D: Circle(0,0,r1) vs Circle(1.5,0,r1) = %s", hit ? "HIT" : "MISS");

        GX::AABB2D box1({-1, -1}, {1, 1});
        GX::AABB2D box2({0.5f, 0.5f}, {2, 2});
        bool boxHit = GX::Collision2D::TestAABBvsAABB(box1, box2);
        GX_LOG_INFO("Collision2D: AABB(-1,-1,1,1) vs AABB(0.5,0.5,2,2) = %s", boxHit ? "HIT" : "MISS");
    }

    // === Phase 8: 3D当たり判定テスト ===
    {
        GX::Sphere s1({ 0, 0, 0 }, 1.0f);
        GX::Sphere s2({ 3, 0, 0 }, 1.0f);
        bool hit = GX::Collision3D::TestSphereVsSphere(s1, s2);
        GX_LOG_INFO("Collision3D: Sphere(0,r1) vs Sphere(3,r1) = %s", hit ? "HIT" : "MISS");

        GX::Ray ray({ 0, 5, 0 }, { 0, -1, 0 });
        GX::AABB3D box({ -1, -1, -1 }, { 1, 1, 1 });
        float t;
        bool rayHit = GX::Collision3D::RaycastAABB(ray, box, t);
        GX_LOG_INFO("Collision3D: Ray(0,5,0 -> 0,-1,0) vs AABB = %s t=%.2f", rayHit ? "HIT" : "MISS", t);
    }

    // === Phase 8: 2D物理セットアップ ===
    {
        g_physicsWorld2D.SetGravity({ 0.0f, 300.0f });  // Y-down in screen space

        // 静的な床。Staticは動かないので固定地形に使う。
        auto* floor = g_physicsWorld2D.AddBody();
        floor->bodyType = GX::BodyType2D::Static;
        floor->position = { 640.0f, 680.0f };
        floor->shape.type = GX::ShapeType2D::AABB;
        floor->shape.halfExtents = { 600.0f, 20.0f };

        // 跳ねる円。Dynamicは重力で動き、反発する。
        GX::Random rng(42);
        for (int i = 0; i < k_NumPhys2DBodies; ++i)
        {
            auto* body = g_physicsWorld2D.AddBody();
            body->bodyType = GX::BodyType2D::Dynamic;
            body->position = { 200.0f + i * 100.0f, 100.0f + rng.Float(0.0f, 200.0f) };
            body->shape.type = GX::ShapeType2D::Circle;
            body->shape.radius = 15.0f + rng.Float(0.0f, 15.0f);
            body->restitution = 0.6f + rng.Float(0.0f, 0.3f);
            body->mass = body->shape.radius * 0.1f;
        }
    }

    // === Phase 8: 3D物理（Jolt）セットアップ ===
    {
        if (g_physicsWorld3D.Initialize(1024))
        {
            g_physics3DInit = true;
            g_physicsWorld3D.SetGravity({ 0.0f, -9.81f, 0.0f });

            // 床（静的ボックス）
            g_floorShape = g_physicsWorld3D.CreateBoxShape({ 50.0f, 0.5f, 50.0f });
            GX::PhysicsBodySettings floorSettings;
            floorSettings.position = { 0.0f, -0.5f, 0.0f };
            floorSettings.motionType = GX::MotionType3D::Static;
            floorSettings.layer = 0;
            g_floorBodyID = g_physicsWorld3D.AddBody(g_floorShape, floorSettings);

            // 物理形状の作成。形状ごとに衝突の計算方法が変わる。
            g_ballShape = g_physicsWorld3D.CreateSphereShape(0.4f);
            g_boxPhysShape = g_physicsWorld3D.CreateBoxShape({ 0.35f, 0.35f, 0.35f });
            g_capsulePhysShape = g_physicsWorld3D.CreateCapsuleShape(0.4f, 0.2f);

            // 形状ごとの表示用メッシュを用意
            g_physSphereMesh = g_renderer3D.CreateGPUMesh(
                GX::MeshGenerator::CreateSphere(0.4f, 16, 8));
            g_physBoxMesh = g_renderer3D.CreateGPUMesh(
                GX::MeshGenerator::CreateBox(0.7f, 0.7f, 0.7f));
            g_physCapsuleMesh = g_renderer3D.CreateGPUMesh(
                GX::MeshGenerator::CreateCylinder(0.2f, 0.2f, 1.2f, 12, 1));

            // 静的な坂（メッシュコライダーデモ）
            {
                auto rampData = GX::MeshGenerator::CreateBox(4.0f, 0.3f, 3.0f);
                std::vector<GX::Vector3> rampPositions;
                rampPositions.reserve(rampData.vertices.size());
                for (auto& v : rampData.vertices)
                    rampPositions.push_back({ v.position.x, v.position.y, v.position.z });

                auto* rampShape = g_physicsWorld3D.CreateMeshShape(
                    rampPositions.data(),
                    static_cast<uint32_t>(rampPositions.size()),
                    rampData.indices.data(),
                    static_cast<uint32_t>(rampData.indices.size()));

                GX::PhysicsBodySettings rampSettings;
                rampSettings.position = { 5.0f, 1.5f, 0.0f };
                rampSettings.rotation = GX::Quaternion::FromEuler(0.0f, 0.0f, -0.35f);
                rampSettings.motionType = GX::MotionType3D::Static;
                rampSettings.layer = 0;
                g_physicsWorld3D.AddBody(rampShape, rampSettings);
            }

            // 初期オブジェクトをいくつか追加（挙動を見やすくする）
            GX::Random rng(123);
            auto addPhysObj = [&](PhysShapeType type, const GX::Vector3& pos, const GX::Quaternion& rot,
                                  float r, float g, float b) {
                GX::PhysicsBodySettings bs;
                bs.position = pos;
                bs.rotation = rot;
                bs.motionType = GX::MotionType3D::Dynamic;
                bs.restitution = 0.5f;
                bs.mass = 1.0f;

                GX::PhysicsShape* shape = nullptr;
                switch (type) {
                case PhysShapeType::Sphere:  shape = g_ballShape; break;
                case PhysShapeType::Box:     shape = g_boxPhysShape; break;
                case PhysShapeType::Capsule: shape = g_capsulePhysShape; break;
                }

                PhysObject obj;
                obj.id = g_physicsWorld3D.AddBody(shape, bs);
                obj.shapeType = type;
                obj.material.constants.albedoFactor = { r, g, b, 1.0f };
                obj.material.constants.metallicFactor = 0.2f;
                obj.material.constants.roughnessFactor = 0.5f;
                g_physObjects.push_back(obj);
            };

            // 球
            for (int i = 0; i < 3; ++i)
                addPhysObj(PhysShapeType::Sphere,
                    { rng.Float(-2, 2), 5.0f + i * 2.0f, rng.Float(-2, 2) },
                    GX::Quaternion::Identity(), 0.9f, 0.3f, 0.1f);

            // 箱（傾きあり）
            for (int i = 0; i < 3; ++i)
                addPhysObj(PhysShapeType::Box,
                    { rng.Float(-3, 3), 6.0f + i * 2.0f, rng.Float(-3, 3) },
                    GX::Quaternion::FromEuler(rng.Float(-1, 1), rng.Float(-1, 1), rng.Float(-1, 1)),
                    0.2f, 0.5f, 0.9f);

            // カプセル
            for (int i = 0; i < 2; ++i)
                addPhysObj(PhysShapeType::Capsule,
                    { rng.Float(-2, 2), 8.0f + i * 2.0f, rng.Float(-2, 2) },
                    GX::Quaternion::FromEuler(rng.Float(-0.5f, 0.5f), 0, rng.Float(-0.5f, 0.5f)),
                    0.1f, 0.8f, 0.3f);

            GX_LOG_INFO("Jolt Physics: Initialized, floor + ramp + %d objects", (int)g_physObjects.size());
        }
        else
        {
            GX_LOG_WARN("Jolt Physics: Failed to initialize");
        }
    }

    return true;
}

// ============================================================================
// 更新
// ============================================================================

void UpdateInput(float deltaTime)
{
    g_inputManager.Update();

    if (g_inputManager.CheckHitKey(VK_ESCAPE))
    {
        PostQuitMessage(0);
        return;
    }

    // トーンマッピングモード切替（トリガー入力で切替）
    auto& kb = g_inputManager.GetKeyboard();
    if (kb.IsKeyTriggered('1'))
        g_postEffect.SetTonemapMode(GX::TonemapMode::Reinhard);
    if (kb.IsKeyTriggered('2'))
        g_postEffect.SetTonemapMode(GX::TonemapMode::ACES);
    if (kb.IsKeyTriggered('3'))
        g_postEffect.SetTonemapMode(GX::TonemapMode::Uncharted2);

    // Bloom のON/OFF
    if (kb.IsKeyTriggered('4'))
        g_postEffect.GetBloom().SetEnabled(!g_postEffect.GetBloom().IsEnabled());

    // FXAA のON/OFF
    if (kb.IsKeyTriggered('5'))
        g_postEffect.SetFXAAEnabled(!g_postEffect.IsFXAAEnabled());

    // Vignette のON/OFF
    if (kb.IsKeyTriggered('6'))
        g_postEffect.SetVignetteEnabled(!g_postEffect.IsVignetteEnabled());

    // ColorGrading のON/OFF
    if (kb.IsKeyTriggered('7'))
        g_postEffect.SetColorGradingEnabled(!g_postEffect.IsColorGradingEnabled());

    // シャドウデバッグ切替（0-8）
    if (kb.IsKeyTriggered('8'))
    {
        uint32_t mode = (g_renderer3D.GetShadowDebugMode() + 1) % 10;
        g_renderer3D.SetShadowDebugMode(mode);
    }

    // SSAO のON/OFF
    if (kb.IsKeyTriggered('9'))
        g_postEffect.GetSSAO().SetEnabled(!g_postEffect.GetSSAO().IsEnabled());

    // DoF のON/OFF
    if (kb.IsKeyTriggered('0'))
        g_postEffect.GetDoF().SetEnabled(!g_postEffect.GetDoF().IsEnabled());

    // Motion Blur のON/OFF
    if (kb.IsKeyTriggered('B'))
        g_postEffect.GetMotionBlur().SetEnabled(!g_postEffect.GetMotionBlur().IsEnabled());

    // SSR のON/OFF
    if (kb.IsKeyTriggered('R'))
        g_postEffect.GetSSR().SetEnabled(!g_postEffect.GetSSR().IsEnabled());

    // Outline のON/OFF
    if (kb.IsKeyTriggered('O'))
        g_postEffect.GetOutline().SetEnabled(!g_postEffect.GetOutline().IsEnabled());

    // VolumetricLight のON/OFF
    if (kb.IsKeyTriggered('V'))
        g_postEffect.GetVolumetricLight().SetEnabled(!g_postEffect.GetVolumetricLight().IsEnabled());

    // TAA のON/OFF
    if (kb.IsKeyTriggered('T'))
        g_postEffect.GetTAA().SetEnabled(!g_postEffect.GetTAA().IsEnabled());

    // AutoExposure のON/OFF
    if (kb.IsKeyTriggered('X'))
        g_postEffect.GetAutoExposure().SetEnabled(!g_postEffect.GetAutoExposure().IsEnabled());

    // マスクデモのON/OFF
    if (kb.IsKeyTriggered('L'))
        g_maskDemo = !g_maskDemo;

    // GUIデモのON/OFF
    if (kb.IsKeyTriggered('U'))
        g_guiDemo = !g_guiDemo;

    // F9: ホットリロードステータス表示
    if (kb.IsKeyTriggered(VK_F9))
        g_showHotReloadStatus = !g_showHotReloadStatus;

    // P: GPUプロファイラのON/OFF
    if (kb.IsKeyTriggered('P'))
    {
        g_showProfiler = !g_showProfiler;
        GX::GPUProfiler::Instance().SetEnabled(g_showProfiler);
    }

    // F12: 設定保存
    if (kb.IsKeyTriggered(VK_F12))
        g_postEffect.SaveSettings("post_effects.json");

    // DoF フォーカス距離調整 (F/G)
    if (g_inputManager.CheckHitKey('F'))
        g_postEffect.GetDoF().SetFocalDistance(g_postEffect.GetDoF().GetFocalDistance() + 5.0f * deltaTime);
    if (g_inputManager.CheckHitKey('G'))
        g_postEffect.GetDoF().SetFocalDistance((std::max)(g_postEffect.GetDoF().GetFocalDistance() - 5.0f * deltaTime, 0.5f));

    // 露出調整
    if (g_inputManager.CheckHitKey(VK_OEM_PLUS) || g_inputManager.CheckHitKey(VK_ADD))
        g_postEffect.SetExposure(g_postEffect.GetExposure() + 0.5f * deltaTime);
    if (g_inputManager.CheckHitKey(VK_OEM_MINUS) || g_inputManager.CheckHitKey(VK_SUBTRACT))
        g_postEffect.SetExposure((std::max)(g_postEffect.GetExposure() - 0.5f * deltaTime, 0.1f));

    auto& mouse = g_inputManager.GetMouse();
    if (mouse.IsButtonTriggered(GX::MouseButton::Right))
    {
        g_mouseCaptured = !g_mouseCaptured;
        if (g_mouseCaptured)
        {
            g_lastMouseX = mouse.GetX();
            g_lastMouseY = mouse.GetY();
            ShowCursor(FALSE);
        }
        else
        {
            ShowCursor(TRUE);
        }
    }

    if (g_mouseCaptured)
    {
        int mx = mouse.GetX();
        int my = mouse.GetY();
        g_camera.Rotate(
            static_cast<float>(my - g_lastMouseY) * g_mouseSensitivity,
            static_cast<float>(mx - g_lastMouseX) * g_mouseSensitivity);
        g_lastMouseX = mx;
        g_lastMouseY = my;
    }

    float speed = g_cameraSpeed * deltaTime;
    if (g_inputManager.CheckHitKey(VK_SHIFT))
        speed *= 3.0f;

    if (g_inputManager.CheckHitKey('W')) g_camera.MoveForward(speed);
    if (g_inputManager.CheckHitKey('S')) g_camera.MoveForward(-speed);
    if (g_inputManager.CheckHitKey('D')) g_camera.MoveRight(speed);
    if (g_inputManager.CheckHitKey('A')) g_camera.MoveRight(-speed);
    if (g_inputManager.CheckHitKey('E')) g_camera.MoveUp(speed);
    if (g_inputManager.CheckHitKey('Q')) g_camera.MoveUp(-speed);

    // Phase 8: 2D物理デモのON/OFF
    if (kb.IsKeyTriggered(VK_F7))
        g_physics2DDemo = !g_physics2DDemo;

    // Phase 8: 3D物理オブジェクト追加（ランダム形状）
    if (kb.IsKeyTriggered(VK_F8) && g_physics3DInit && (int)g_physObjects.size() < k_MaxPhysObjects)
    {
        GX::Random& rng = GX::Random::Global();
        int shapeIdx = rng.Int(0, 2);
        PhysShapeType type = static_cast<PhysShapeType>(shapeIdx);
        GX::PhysicsShape* shape = nullptr;
        float r = 0, g = 0, b = 0;
        switch (type) {
        case PhysShapeType::Sphere:  shape = g_ballShape;        r = rng.Float(0.5f, 1.0f); g = rng.Float(0.1f, 0.4f); b = rng.Float(0.05f, 0.2f); break;
        case PhysShapeType::Box:     shape = g_boxPhysShape;     r = rng.Float(0.1f, 0.3f); g = rng.Float(0.3f, 0.6f); b = rng.Float(0.7f, 1.0f); break;
        case PhysShapeType::Capsule: shape = g_capsulePhysShape; r = rng.Float(0.1f, 0.3f); g = rng.Float(0.6f, 1.0f); b = rng.Float(0.1f, 0.4f); break;
        }

        GX::PhysicsBodySettings bs;
        bs.position = { rng.Float(-4, 4), 8.0f + rng.Float(0, 4), rng.Float(-4, 4) };
        bs.rotation = GX::Quaternion::FromEuler(rng.Float(-1, 1), rng.Float(-1, 1), rng.Float(-1, 1));
        bs.motionType = GX::MotionType3D::Dynamic;
        bs.restitution = 0.5f;
        bs.mass = 1.0f;

        PhysObject obj;
        obj.id = g_physicsWorld3D.AddBody(shape, bs);
        obj.shapeType = type;
        obj.material.constants.albedoFactor = { r, g, b, 1.0f };
        obj.material.constants.metallicFactor = rng.Float(0.0f, 0.5f);
        obj.material.constants.roughnessFactor = rng.Float(0.3f, 0.8f);
        g_physObjects.push_back(obj);
    }

    // ムービープレイヤー操作
    if (kb.IsKeyTriggered(VK_F5))
    {
        if (g_moviePlayer.GetState() == GX::MovieState::Playing)
            g_moviePlayer.Pause();
        else
            g_moviePlayer.Play();
    }
    if (kb.IsKeyTriggered(VK_F6))
        g_moviePlayer.Stop();

    g_cubeTransform.SetRotation(g_totalTime * 0.5f, g_totalTime * 0.7f, 0.0f);
}

// ============================================================================
// 描画
// ============================================================================

void RenderFrame(float deltaTime)
{
    g_totalTime += deltaTime;
    UpdateInput(deltaTime);

    // Phase 9: シェーダーホットリロード更新（変更を即反映）
    GX::ShaderHotReload::Instance().Update(deltaTime);

    // Phase 7: 非同期システム更新
    g_httpClient.Update();
    g_moviePlayer.Update(g_device);

    // Phase 8: 物理ステップ（時間dtで進める）
    if (g_physics2DDemo)
        g_physicsWorld2D.Step(deltaTime);
    if (g_physics3DInit)
        g_physicsWorld3D.Step(deltaTime);

    // フレーム境界: dirty なフォントアトラスをGPUにアップロード
    g_fontManager.FlushAtlasUpdates();

    g_frameIndex = g_swapChain.GetCurrentBackBufferIndex();
    g_commandQueue.GetFence().WaitForValue(g_frameFenceValues[g_frameIndex]);
    g_commandList.Reset(g_frameIndex, nullptr);
    auto* cmdList = g_commandList.Get();

    // Phase 10: GPU Profiler フレーム開始
    GX::GPUProfiler::Instance().BeginFrame(cmdList, g_frameIndex);

    // === シャドウパス ===
    GX::GPUProfiler::Instance().BeginScope(cmdList, "Shadow");
    g_renderer3D.UpdateShadow(g_camera);

    // CSMパス (physics objects skip shadow for performance)
    for (uint32_t cascade = 0; cascade < GX::CascadedShadowMap::k_NumCascades; ++cascade)
    {
        g_renderer3D.BeginShadowPass(cmdList, g_frameIndex, cascade);
        DrawScene(false);
        g_renderer3D.EndShadowPass(cascade);
    }

    // スポットシャドウパス
    g_renderer3D.BeginSpotShadowPass(cmdList, g_frameIndex);
    DrawScene(false);
    g_renderer3D.EndSpotShadowPass();

    // ポイントシャドウパス（6面）
    for (uint32_t face = 0; face < 6; ++face)
    {
        g_renderer3D.BeginPointShadowPass(cmdList, g_frameIndex, face);
        DrawScene(false);
        g_renderer3D.EndPointShadowPass(face);
    }

    GX::GPUProfiler::Instance().EndScope(cmdList);

    // === HDRシーン描画パス ===
    GX::GPUProfiler::Instance().BeginScope(cmdList, "Scene");
    auto dsvHandle = g_renderer3D.GetDepthBuffer().GetDSVHandle();
    g_postEffect.BeginScene(cmdList, g_frameIndex, dsvHandle, g_camera);

    // スカイボックス（最初に描画、深度書き込みOFF）
    {
        XMFLOAT4X4 viewF;
        XMStoreFloat4x4(&viewF, g_camera.GetViewMatrix());
        viewF._41 = 0.0f; viewF._42 = 0.0f; viewF._43 = 0.0f;
        XMMATRIX viewRotOnly = XMLoadFloat4x4(&viewF);

        XMFLOAT4X4 vp;
        XMStoreFloat4x4(&vp, XMMatrixTranspose(viewRotOnly * g_camera.GetProjectionMatrix()));
        g_renderer3D.GetSkybox().Draw(cmdList, g_frameIndex, vp);
    }

    // === 3D PBR描画 ===
    g_renderer3D.Begin(cmdList, g_frameIndex, g_camera, g_totalTime);
    DrawScene();
    g_renderer3D.End();

    // === デバッグプリミティブ ===
    {
        XMFLOAT4X4 vp;
        XMStoreFloat4x4(&vp, XMMatrixTranspose(g_camera.GetViewProjectionMatrix()));
        auto& prim = g_renderer3D.GetPrimitiveBatch3D();
        prim.Begin(cmdList, g_frameIndex, vp);
        prim.DrawWireSphere({ -3.0f, 3.0f, -3.0f }, 0.3f, { 0.2f, 0.5f, 1.0f, 0.8f });
        prim.DrawWireSphere({ 4.0f, 4.0f, -2.0f }, 0.3f, { 1.0f, 0.8f, 0.3f, 0.8f });
        prim.End();
    }

    // === ポストエフェクト: HDR → LDR (Sceneレイヤーに出力) ===
    GX::GPUProfiler::Instance().EndScope(cmdList);
    g_postEffect.EndScene();

    // Sceneレイヤー → PostEffectPipeline.Resolve
    GX::GPUProfiler::Instance().BeginScope(cmdList, "PostFX");
    g_sceneLayer->GetRenderTarget().TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    g_postEffect.Resolve(g_sceneLayer->GetRTVHandle(),
                         g_renderer3D.GetDepthBuffer(), g_camera, deltaTime);
    g_sceneLayer->GetRenderTarget().TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    GX::GPUProfiler::Instance().EndScope(cmdList);

    // === GUI 更新 ===
    if (g_guiDemo)
    {
        g_uiContext.Update(deltaTime, g_inputManager);

        // ProgressBar アニメーション
        auto* progressWidget = g_uiContext.GetRoot() ? g_uiContext.GetRoot()->FindById("progressLoading") : nullptr;
        if (progressWidget)
        {
            auto* pb = static_cast<GX::GUI::ProgressBar*>(progressWidget);
            pb->SetValue(fmod(g_totalTime * 0.15f, 1.0f));
        }
    }

    // === UIレイヤー: テキスト描画 ===
    GX::GPUProfiler::Instance().BeginScope(cmdList, "UI");
    g_uiLayer->Begin(cmdList);
    g_uiLayer->Clear(cmdList, 0, 0, 0, 0);

    // --- GUI 描画 ---
    if (g_guiDemo)
    {
        g_uiRenderer.Begin(cmdList, g_frameIndex);
        g_uiContext.Render();
        g_uiRenderer.End();
    }

    // Phase 8: 2D物理描画（UIレイヤーでPrimitiveBatchを使用）
    // 2DはUIレイヤーに描くと、3Dの上に重ねやすい。
    if (g_physics2DDemo)
    {
        g_primBatch2D.Begin(cmdList, g_frameIndex);

        std::vector<GX::RigidBody2D*> allBodies;
        g_physicsWorld2D.QueryAABB(GX::AABB2D({-1000.0f, -1000.0f}, {2000.0f, 2000.0f}), allBodies);
        for (auto* body : allBodies)
        {
            float px = body->position.x;
            float py = body->position.y;
            if (body->bodyType == GX::BodyType2D::Static)
            {
                float hw = body->shape.halfExtents.x;
                float hh = body->shape.halfExtents.y;
                g_primBatch2D.DrawBox(px - hw, py - hh, px + hw, py + hh, 0xFF444444, true);
            }
            else
            {
                float r = body->shape.radius;
                g_primBatch2D.DrawCircle(px, py, r, 0xFF2288FF, true);
            }
        }

        g_primBatch2D.End();
    }

    g_spriteBatch.Begin(cmdList, g_frameIndex);
    {
        if (g_fontHandle >= 0)
        {
            g_textRenderer.DrawFormatString(g_fontHandle, 10, 10, 0xFFFFFFFF,
                L"FPS: %.1f", g_app.GetTimer().GetFPS());

            auto pos = g_camera.GetPosition();
            g_textRenderer.DrawFormatString(g_fontHandle, 10, 35, 0xFF88BBFF,
                L"Camera: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

            const wchar_t* tonemapNames[] = { L"Reinhard", L"ACES", L"Uncharted2" };
            uint32_t tmIdx = static_cast<uint32_t>(g_postEffect.GetTonemapMode());
            g_textRenderer.DrawFormatString(g_fontHandle, 10, 60, 0xFF88FF88,
                L"Tonemap: %s  Exposure: %.2f", tonemapNames[tmIdx], g_postEffect.GetExposure());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 85, 0xFF88FF88,
                L"Bloom: %s  Threshold: %.2f  Intensity: %.2f",
                g_postEffect.GetBloom().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetBloom().GetThreshold(),
                g_postEffect.GetBloom().GetIntensity());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 110, 0xFF88FF88,
                L"FXAA: %s  Vignette: %s  ChromAberr: %.4f  ColorGrading: %s",
                g_postEffect.IsFXAAEnabled() ? L"ON" : L"OFF",
                g_postEffect.IsVignetteEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetChromaticAberration(),
                g_postEffect.IsColorGradingEnabled() ? L"ON" : L"OFF");

            if (g_postEffect.IsColorGradingEnabled())
            {
                g_textRenderer.DrawFormatString(g_fontHandle, 10, 135, 0xFF88FF88,
                    L"Contrast: %.2f  Saturation: %.2f  Temperature: %.2f",
                    g_postEffect.GetContrast(), g_postEffect.GetSaturation(),
                    g_postEffect.GetTemperature());
            }

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 160, 0xFF88FF88,
                L"SSAO: %s  Radius: %.2f  Power: %.2f",
                g_postEffect.GetSSAO().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetSSAO().GetRadius(),
                g_postEffect.GetSSAO().GetPower());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 185, 0xFF88FF88,
                L"DoF: %s  FocalDist: %.1f  Range: %.1f  Radius: %.1f",
                g_postEffect.GetDoF().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetDoF().GetFocalDistance(),
                g_postEffect.GetDoF().GetFocalRange(),
                g_postEffect.GetDoF().GetBokehRadius());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 210, 0xFF88FF88,
                L"MotionBlur: %s  Intensity: %.2f  Samples: %d",
                g_postEffect.GetMotionBlur().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetMotionBlur().GetIntensity(),
                g_postEffect.GetMotionBlur().GetSampleCount());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 235, 0xFF88FF88,
                L"SSR: %s  Steps: %d  Intensity: %.2f",
                g_postEffect.GetSSR().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetSSR().GetMaxSteps(),
                g_postEffect.GetSSR().GetIntensity());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 260, 0xFF88FF88,
                L"Outline: %s  DepthTh: %.2f  NormalTh: %.2f",
                g_postEffect.GetOutline().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetOutline().GetDepthThreshold(),
                g_postEffect.GetOutline().GetNormalThreshold());

            {
                auto& vl = g_postEffect.GetVolumetricLight();
                auto sunPos = vl.GetLastSunScreenPos();
                g_textRenderer.DrawFormatString(g_fontHandle, 10, 285, 0xFF88FF88,
                    L"GodRay: %s  I:%.1f  SunUV:(%.2f,%.2f)  Visible:%.2f",
                    vl.IsEnabled() ? L"ON" : L"OFF",
                    vl.GetIntensity(),
                    sunPos.x, sunPos.y, vl.GetLastSunVisible());
            }

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 310, 0xFF88FF88,
                L"TAA: %s  BlendFactor: %.2f",
                g_postEffect.GetTAA().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetTAA().GetBlendFactor());

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 335, 0xFF88FF88,
                L"AutoExposure: %s  Exposure: %.2f  Speed: %.1f",
                g_postEffect.GetAutoExposure().IsEnabled() ? L"ON" : L"OFF",
                g_postEffect.GetAutoExposure().GetCurrentExposure(),
                g_postEffect.GetAutoExposure().GetAdaptationSpeed());

            const wchar_t* shadowDebugNames[] = { L"OFF", L"Factor", L"Cascade", L"ShadowUV", L"RawDepth", L"Normal", L"ViewZ", L"Albedo", L"Light", L"LightCol" };
            g_textRenderer.DrawFormatString(g_fontHandle, 10, 360, 0xFFFF8888,
                L"ShadowDebug: %s  Shadow: %s",
                shadowDebugNames[g_renderer3D.GetShadowDebugMode()],
                g_renderer3D.IsShadowEnabled() ? L"ON" : L"OFF");

            g_textRenderer.DrawFormatString(g_fontHandle, 10, 385, 0xFF88FF88,
                L"Layers: %d  Mask: %s  GUI: %s  Phys2D: %s  JoltObjs: %d/%d",
                g_layerStack.GetLayerCount(),
                g_maskDemo ? L"ON" : L"OFF",
                g_guiDemo ? L"ON" : L"OFF",
                g_physics2DDemo ? L"ON" : L"OFF",
                static_cast<int>(g_physObjects.size()), k_MaxPhysObjects);

            // 日本語テスト描画
            g_textRenderer.DrawString(g_fontHandle, 10, 460, L"日本語テスト: こんにちは世界！", 0xFFFFFF00);
            g_textRenderer.DrawString(g_fontHandle, 10, 485, L"カタカナ: アイウエオ", 0xFF88FF88);
            g_textRenderer.DrawString(g_fontHandle, 10, 510, L"漢字: 東京都渋谷区", 0xFFFF8888);

            // Phase 7 状態表示
            {
                int wLen = MultiByteToWideChar(CP_UTF8, 0, g_httpStatusText.c_str(), -1, nullptr, 0);
                std::wstring wStatus(wLen - 1, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, g_httpStatusText.c_str(), -1, wStatus.data(), wLen);
                g_textRenderer.DrawFormatString(g_fontHandle, 10, 410, 0xFF88FFFF,
                    L"VFS: ON  Archive: %s  HTTP: %d %s",
                    g_archiveDemo ? L"OK" : L"N/A",
                    g_httpStatusCode,
                    wStatus.c_str());
            }

            {
                const wchar_t* movieStateStr = L"N/A";
                if (g_moviePlayer.GetWidth() > 0)
                {
                    switch (g_moviePlayer.GetState())
                    {
                    case GX::MovieState::Playing: movieStateStr = L"Playing"; break;
                    case GX::MovieState::Paused:  movieStateStr = L"Paused"; break;
                    case GX::MovieState::Stopped: movieStateStr = L"Stopped"; break;
                    }
                }
                g_textRenderer.DrawFormatString(g_fontHandle, 10, 435, 0xFF88FFFF,
                    L"Movie: %s  (F5:Play/Pause  F6:Stop)",
                    movieStateStr);
            }

            float helpY = static_cast<float>(g_swapChain.GetHeight()) - 80.0f;
            g_textRenderer.DrawString(g_fontHandle, 10, helpY,
                L"WASD: Move  QE: Up/Down  Shift: Fast  RClick: Mouse  ESC: Quit", 0xFFAAAAAA);
            g_textRenderer.DrawString(g_fontHandle, 10, helpY + 25,
                L"1/2/3: Tonemap  4: Bloom  5: FXAA  6: Vignette  7: ColorGrading  8: ShadowDbg  9: SSAO", 0xFFFFCC44);
            g_textRenderer.DrawString(g_fontHandle, 10, helpY + 50,
                L"0:DoF B:MBlur R:SSR O:Outline V:GodRays T:TAA X:AutoExp P:Profile L:Mask U:GUI F7:2D F8:Add F9:Reload F12:Save", 0xFFFFCC44);
        }
    }

    g_spriteBatch.End();

    // === Shader Hot Reload エラーオーバーレイ / ステータス ===
    if (GX::ShaderHotReload::Instance().HasError())
    {
        // 赤半透明背景
        float sw = static_cast<float>(g_swapChain.GetWidth());
        g_primBatch2D.Begin(cmdList, g_frameIndex);
        g_primBatch2D.DrawBox(0, 0, sw, 60, 0xCC220000, true);
        g_primBatch2D.End();

        g_spriteBatch.Begin(cmdList, g_frameIndex);
        if (g_fontHandle >= 0)
        {
            auto& errMsg = GX::ShaderHotReload::Instance().GetErrorMessage();
            int wLen = MultiByteToWideChar(CP_UTF8, 0, errMsg.c_str(), -1, nullptr, 0);
            std::wstring wErr(wLen - 1, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, errMsg.c_str(), -1, wErr.data(), wLen);
            g_textRenderer.DrawString(g_fontHandle, 10, 5, L"[Shader Error]", 0xFFFF4444);
            g_textRenderer.DrawString(g_fontHandle, 10, 30, wErr.c_str(), 0xFFFFAAAA);
        }
        g_spriteBatch.End();
    }
    else if (g_showHotReloadStatus)
    {
        g_spriteBatch.Begin(cmdList, g_frameIndex);
        if (g_fontHandle >= 0)
        {
            g_textRenderer.DrawString(g_fontHandle, 10,
                static_cast<float>(g_swapChain.GetHeight()) - 105.0f,
                L"[F9] ShaderHotReload: Active (watching Shaders/)", 0xFF44FF44);
        }
        g_spriteBatch.End();
    }

    // === GPU Profiler オーバーレイ ===
    if (g_showProfiler && g_fontHandle >= 0)
    {
        auto& profiler = GX::GPUProfiler::Instance();
        float sw = static_cast<float>(g_swapChain.GetWidth());

        // 背景ボックス
        float boxW = 300.0f;
        float boxX = sw - boxW - 10.0f;
        float boxY = 10.0f;
        float lineH = 20.0f;
        float boxH = lineH * (2.0f + static_cast<float>(profiler.GetResults().size()));

        g_primBatch2D.Begin(cmdList, g_frameIndex);
        g_primBatch2D.DrawBox(boxX, boxY, boxX + boxW, boxY + boxH, 0xCC000000, true);
        g_primBatch2D.End();

        g_spriteBatch.Begin(cmdList, g_frameIndex);
        float y = boxY + 4.0f;

        g_textRenderer.DrawFormatString(g_fontHandle, boxX + 8.0f, y, 0xFF44FF44,
            L"[P] GPU Profiler  Total: %.2f ms", profiler.GetFrameGPUTimeMs());
        y += lineH;

        for (const auto& r : profiler.GetResults())
        {
            int wLen = MultiByteToWideChar(CP_UTF8, 0, r.name, -1, nullptr, 0);
            std::wstring wName(wLen - 1, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, r.name, -1, wName.data(), wLen);

            // バーの幅比率 (最大10msを100%とする)
            float barRatio = (std::min)(r.durationMs / 10.0f, 1.0f);
            uint32_t color = (r.durationMs > 5.0f) ? 0xFFFF4444 :
                             (r.durationMs > 2.0f) ? 0xFFFFCC44 : 0xFF88FF88;

            g_textRenderer.DrawFormatString(g_fontHandle, boxX + 8.0f, y, color,
                L"  %-12s %6.2f ms", wName.c_str(), r.durationMs);
            y += lineH;
        }

        g_spriteBatch.End();
    }

    g_uiLayer->End();
    GX::GPUProfiler::Instance().EndScope(cmdList);

    // === マスクデモ ===
    if (g_maskDemo)
    {
        g_maskScreen.Clear(cmdList, 0);
        g_maskScreen.DrawFillRect(cmdList, g_frameIndex,
            100.0f, 100.0f, 400.0f, 300.0f, 1.0f);
        g_maskScreen.DrawCircle(cmdList, g_frameIndex,
            800.0f, 360.0f, 200.0f, 1.0f);
        g_uiLayer->SetMask(g_maskScreen.GetAsLayer());
    }
    else
    {
        g_uiLayer->SetMask(nullptr);
    }

    // === コンポジション → バックバッファ ===
    GX::GPUProfiler::Instance().BeginScope(cmdList, "Composite");
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = g_swapChain.GetCurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    auto rtvHandle = g_swapChain.GetCurrentRTVHandle();
    g_compositor.Composite(cmdList, g_frameIndex, rtvHandle, g_layerStack);

    // バックバッファ → PRESENT
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    cmdList->ResourceBarrier(1, &barrier);
    GX::GPUProfiler::Instance().EndScope(cmdList);

    // Phase 10: GPU Profiler フレーム終了
    GX::GPUProfiler::Instance().EndFrame(cmdList);

    g_commandList.Close();

    ID3D12CommandList* cmdLists[] = { cmdList };
    g_commandQueue.ExecuteCommandLists(cmdLists, 1);

    g_swapChain.Present(false);
    g_frameFenceValues[g_frameIndex] = g_commandQueue.GetFence().Signal(g_commandQueue.GetQueue());
}

void OnResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    g_commandQueue.Flush();
    g_swapChain.Resize(g_device.GetDevice(), width, height);
    g_spriteBatch.SetScreenSize(width, height);
    g_primBatch2D.SetScreenSize(width, height);
    g_renderer3D.OnResize(width, height);
    g_postEffect.OnResize(g_device.GetDevice(), width, height);
    g_layerStack.OnResize(g_device.GetDevice(), width, height);
    g_compositor.OnResize(g_device.GetDevice(), width, height);
    g_maskScreen.OnResize(g_device.GetDevice(), width, height);
    g_uiRenderer.OnResize(width, height);
    g_uiContext.OnResize(width, height);
    g_camera.SetPerspective(g_camera.GetFovY(), static_cast<float>(width) / height,
                             g_camera.GetNearZ(), g_camera.GetFarZ());
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    // Phase 10b: CRTメモリリーク検出 (Debug時のみ)
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif

    // --- Phase 7: ファイルシステム初期化 ---
    {
        auto physProvider = std::make_shared<GX::PhysicalFileProvider>(".");
        GX::FileSystem::Instance().Mount("", physProvider);
        GX_LOG_INFO("FileSystem: PhysicalFileProvider mounted at root");
    }

    // --- Phase 7: アーカイブデモ（Assetsからテストアーカイブ作成） ---
    {
        GX::ArchiveWriter writer;
        writer.SetPassword("testkey123");
        writer.SetCompression(true);

        // CSSファイルをアーカイブに詰める
        auto cssData = GX::FileSystem::Instance().ReadFile("Assets/ui/menu.css");
        if (cssData.IsValid())
        {
            writer.AddFile("Assets/ui/menu.css", cssData.Data(), cssData.Size());
            GX_LOG_INFO("Archive: Added Assets/ui/menu.css (%zu bytes)", cssData.Size());
        }

        if (writer.Save("test_archive.gxarc"))
        {
            GX_LOG_INFO("Archive: test_archive.gxarc created");

            // 検証: 開いて読み戻す
            auto arcProvider = std::make_shared<GX::ArchiveFileProvider>();
            if (arcProvider->Open("test_archive.gxarc", "testkey123"))
            {
                auto arcData = arcProvider->Read("Assets/ui/menu.css");
                if (arcData.IsValid())
                {
                    g_archiveDemo = true;
                    GX_LOG_INFO("Archive: Verified read-back (%zu bytes, match=%s)",
                        arcData.Size(),
                        (arcData.Size() == cssData.Size()) ? "true" : "false");
                }
            }
        }
    }

    // --- Phase 7: HTTPデモ（非同期） ---
    g_httpClient.GetAsync("https://httpbin.org/get", [](GX::HTTPResponse resp) {
        g_httpStatusCode = resp.statusCode;
        if (resp.IsSuccess())
            g_httpStatusText = std::format("OK ({} bytes)", resp.body.size());
        else
            g_httpStatusText = std::format("Error (code={})", resp.statusCode);
        GX_LOG_INFO("HTTP: GET httpbin.org/get -> %d (%zu bytes)",
            resp.statusCode, resp.body.size());
    });

    GX::ApplicationDesc appDesc;
    appDesc.title  = L"GXLib Phase10 [GPUProfiler]";
    appDesc.width  = 1280;
    appDesc.height = 720;

    if (!g_app.Initialize(appDesc))
        return -1;

    g_inputManager.Initialize(g_app.GetWindow());

    // WM_CHAR → UIContext へルーティング
    g_app.GetWindow().AddMessageCallback([](HWND, UINT msg, WPARAM wParam, LPARAM) -> bool {
        if (msg == WM_CHAR)
            return g_uiContext.ProcessCharMessage(static_cast<wchar_t>(wParam));
        return false;
    });

#ifdef _DEBUG
    bool enableDebug = true;
#else
    bool enableDebug = false;
#endif
    if (!g_device.Initialize(enableDebug))
        return -1;
    if (!InitializeGraphics())
        return -1;
    if (!InitializeRenderers())
        return -1;
    if (!InitializeScene())
        return -1;

    // Phase 9: ShaderLibrary + ShaderHotReload（シェーダーの即時反映）
    GX::ShaderLibrary::Instance().Initialize(g_device.GetDevice());
    GX::ShaderHotReload::Instance().Initialize(g_device.GetDevice(), &g_commandQueue);

    // Phase 10: GPUプロファイラ
    GX::GPUProfiler::Instance().Initialize(g_device.GetDevice(), g_commandQueue.GetQueue());

    g_app.GetWindow().SetResizeCallback(OnResize);
    GX_LOG_INFO("=== GXLib Phase 10: GPUProfiler ===");

    g_app.Run(RenderFrame);

    g_physicsWorld3D.Shutdown();
    g_moviePlayer.Close();
    GX::GPUProfiler::Instance().Shutdown();
    GX::ShaderHotReload::Instance().Shutdown();
    GX::ShaderLibrary::Instance().Shutdown();
    g_commandQueue.Flush();
    GX::FileSystem::Instance().Clear();
    if (g_mouseCaptured) ShowCursor(TRUE);
    g_inputManager.Shutdown();
    g_fontManager.Shutdown();
    g_app.Shutdown();

    // Phase 10b: DXGI生存オブジェクトレポート (リーク検出)
#ifdef _DEBUG
    GX::GraphicsDevice::ReportLiveObjects();
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}
