#pragma once
/// @file GXModelViewerApp.h
/// @brief ImGuiベースの3Dモデルビューアアプリケーション
///
/// DockSpace式レイアウトで複数パネルを配置し、3Dシーンをビューポートウィンドウ内に
/// RenderTarget経由で表示する。FBX/glTF/OBJ/GXMD/GXANの読み込み・エクスポート対応。

#include "pch.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/Timer.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Device/CommandQueue.h"
#include "Graphics/Device/CommandList.h"
#include "Graphics/Device/SwapChain.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/3D/ModelLoader.h"

// Panels
#include "Scene/SceneGraph.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertyPanel.h"
#include "Panels/LightingPanel.h"
#include "Panels/PostEffectPanel.h"
#include "Panels/TerrainPanel.h"
#include "Panels/SkyboxPanel.h"
#include "Panels/PerformancePanel.h"
#include "Panels/LogPanel.h"
#include "Panels/TimelinePanel.h"
#include "Panels/AnimatorPanel.h"
#include "Panels/BlendTreeEditor.h"
#include "Panels/TextureBrowser.h"
#include "Panels/ModelInfoPanel.h"
#include "Panels/SkeletonPanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "InfiniteGrid.h"

#include <imgui.h>
#include "ImGuizmo.h"

/// @brief ImGuiベースの3Dモデルビューア。オービットカメラ、ギズモ、ポストエフェクト付き。
class GXModelViewerApp
{
public:
    /// @brief アプリケーションの初期化。ウィンドウ、D3D12、ImGui、各パネルをセットアップする。
    /// @param hInstance Win32インスタンスハンドル
    /// @param width ウィンドウ幅（ピクセル）
    /// @param height ウィンドウ高さ（ピクセル）
    /// @param title ウィンドウタイトル
    /// @return 成功時true
    bool Initialize(HINSTANCE hInstance, uint32_t width, uint32_t height, const wchar_t* title);

    /// @brief メインループを実行する。ウィンドウが閉じられるまでブロックする。
    /// @return 終了コード
    int  Run();

    /// @brief GPUフラッシュ、ImGui破棄、ウィンドウ終了処理を行う。
    void Shutdown();

    /// @brief モデルファイルを読み込んでシーンに追加する。AssetBrowserPanelからも呼ばれる。
    /// @param filePath モデルファイルのUTF-8パス（.fbx/.gltf/.glb/.obj/.gxmd）
    void ImportModel(const std::string& filePath);

    /// @brief アニメーションファイルを選択中モデルにインポートする。
    /// @param filePath アニメーションファイルのUTF-8パス（.gxan/.fbx/.gltf/.glb）
    void ImportAnimation(const std::string& filePath);

private:
    // --- ImGuiライフサイクル ---

    /// @brief ImGuiコンテキスト・DX12バックエンド・ImPlot/ImNodesの初期化
    void InitImGui();
    /// @brief ImGui関連リソースの破棄（ImNodes→ImPlot→DX12→Win32→ImGui順）
    void ShutdownImGui();
    /// @brief ImGui新フレーム開始（Win32/DX12/ImGuizmo）
    void BeginImGuiFrame();
    /// @brief ImGui描画コマンドをコマンドリストに発行する
    /// @param cmdList 描画先のコマンドリスト
    void EndImGuiFrame(ID3D12GraphicsCommandList* cmdList);

    // --- UI構築 ---

    /// @brief メインビューポートにDockSpaceを配置する
    void BuildDockSpace();
    /// @brief 全パネルの描画とD&D/ダイアログ処理を行う（毎フレーム呼び出し）
    void UpdateUI();
    /// @brief メインメニューバー（File/View）を構築する
    void BuildMainMenuBar();
    /// @brief ビューポート左上にギズモ操作ボタンをオーバーレイ表示する
    /// @param imageMin ビューポート画像の左上スクリーン座標
    void DrawViewportToolbar(ImVec2 imageMin);

    // --- ビューポートピッキング ---

    /// @brief マウスクリックでビューポート内のエンティティをAABB判定で選択する
    void HandleViewportPicking();
    /// @brief エンティティのローカルAABBを計算する。スキンドモデルはCPUスキニングで正確なAABBを算出。
    /// @param entity 対象エンティティ
    /// @param outMin AABB最小点（出力）
    /// @param outMax AABB最大点（出力）
    /// @return AABB計算に成功した場合true
    static bool ComputeEntityAABB(const SceneEntity& entity, XMFLOAT3& outMin, XMFLOAT3& outMax);

    // --- オービットカメラ ---

    /// @brief 現在のyaw/pitch/distanceからカメラ位置・向きを再計算する
    void UpdateOrbitCamera();
    /// @brief マウス入力を処理する（右ドラッグ=回転、中ドラッグ=パン、ホイール=ズーム）
    void HandleOrbitInput();

    // --- エクスポート ---

    /// @brief 選択中エンティティのモデルをGXMDバイナリに書き出す
    /// @param outputPath 出力ファイルパス
    void ExportToGxmd(const std::string& outputPath);
    /// @brief 選択中エンティティのアニメーションをGXANバイナリに書き出す
    /// @param outputPath 出力ファイルパス
    void ExportToGxan(const std::string& outputPath);

    // --- レンダリング ---

    /// @brief 1フレーム分の描画処理（シャドウ→HDR→ポストエフェクト→ImGui→Present）
    /// @param deltaTime フレーム経過時間（秒）
    void RenderFrame(float deltaTime);
    /// @brief CSMシャドウパス用にシーン全体を描画する
    void DrawSceneForShadow();
    /// @brief ポイント/スポットシャドウパス用にエンティティだけを描画する
    void DrawEntitiesForShadow();
    /// @brief ウィンドウリサイズ時にSwapChainを再作成する（3Dシーンはビューポートサイズ駆動）
    /// @param width 新しいウィンドウ幅
    /// @param height 新しいウィンドウ高さ
    void OnResize(uint32_t width, uint32_t height);

    // --- ボーン可視化 ---

    /// @brief PrimitiveBatch3Dでボーン接続線とジョイント球を描画する
    /// @param entity 対象のスキンドエンティティ
    void DrawSkeletonOverlay(const SceneEntity& entity);

    // --- GXLibコアオブジェクト ---
    GX::Application     m_app;             ///< ウィンドウ・タイマー管理
    GX::GraphicsDevice  m_graphicsDevice;  ///< D3D12デバイス（デバッグレイヤー対応）
    GX::CommandQueue    m_commandQueue;    ///< GPUコマンドキュー
    GX::CommandList     m_commandList;     ///< ダブルバッファ対応コマンドリスト
    GX::SwapChain       m_swapChain;       ///< スワップチェーン（OSウィンドウ用）

    // --- 3Dレンダリング ---
    GX::Renderer3D           m_renderer3D;  ///< PBR/Toonレンダラ（CSM、マテリアル管理内蔵）
    GX::Camera3D             m_camera;      ///< オービットカメラ
    GX::PostEffectPipeline   m_postEffect;  ///< HDR→トーンマップ→FXAA等のポストエフェクトチェーン

    // --- シーンデータ ---
    SceneGraph          m_sceneGraph;       ///< エンティティ管理（選択・削除・親子関係）
    InfiniteGrid        m_infiniteGrid;     ///< Y=0平面の無限グリッド描画

    // --- リソース管理 ---
    GX::ModelLoader     m_modelLoader;      ///< glTF/FBX/OBJ/GXMDモデル読み込み
    GX::MaterialManager m_materialManager;  ///< マテリアルハンドル管理（Renderer3D内部とは別）
    GX::TextureManager  m_textureManager;   ///< テクスチャハンドル管理

    // --- ImGui ---
    GX::DescriptorHeap  m_imguiSrvHeap;         ///< ImGui専用SRVヒープ（shader-visible、256エントリ）
    bool                m_imguiInitialized = false;

    // --- ビューポートレンダーターゲット ---
    // 3Dシーン→m_viewportRT(LDR)→ImGui::Imageで表示
    GX::RenderTarget    m_viewportRT;
    uint32_t            m_viewportWidth  = 0;
    uint32_t            m_viewportHeight = 0;
    bool                m_viewportNeedsResize = false;  ///< ImGuiウィンドウサイズ変更時にtrueになる
    bool                m_viewportHovered = false;       ///< マウスがビューポート上にあるか
    bool                m_viewportFocused = false;       ///< ビューポートがフォーカスされているか
    uint32_t            m_viewportSrvIndex = 0;          ///< m_imguiSrvHeap内のSRVインデックス

    // --- ピッキング用ビューポート矩形 ---
    ImVec2              m_viewportImageMin  = {};   ///< ビューポート画像のスクリーン左上座標
    ImVec2              m_viewportImageSize = {};   ///< ビューポート画像のサイズ

    // --- フレーム同期 ---
    uint64_t m_frameFenceValues[GX::SwapChain::k_BufferCount] = {};
    uint32_t m_frameIndex = 0;
    float    m_totalTime  = 0.0f;  ///< 累積経過時間（シェーダーアニメーション用）

    // --- アプリ状態 ---
    uint32_t m_width  = 0;     ///< ウィンドウ幅
    uint32_t m_height = 0;     ///< ウィンドウ高さ
    bool     m_running = true;
    bool     m_showDemoWindow = false;

    // --- オービットカメラパラメータ ---
    float m_orbitYaw   = 0.0f;    ///< 水平回転角（ラジアン）
    float m_orbitPitch = 0.5f;    ///< 仰角（ラジアン、±π/2制限）
    float m_orbitDistance = 8.0f;  ///< ターゲットからの距離
    float m_orbitMaxDistance = 200.0f;
    XMFLOAT3 m_orbitTarget = { 0.0f, 0.0f, 0.0f };  ///< 注視点（パン移動可能）
    bool  m_orbitDragActive = false;   ///< ドラッグ操作中フラグ
    int   m_prevSelectedBone = -1;     ///< ボーン選択変化検出用

    // --- パネル表示フラグ（View メニューで切り替え可能） ---
    bool m_showSceneHierarchy = true;
    bool m_showProperties     = true;
    bool m_showLighting       = true;
    bool m_showPostEffects    = true;
    bool m_showSkybox         = false;
    bool m_showTerrain        = false;
    bool m_showPerformance    = true;
    bool m_showLog            = true;
    bool m_showTimeline       = false;
    bool m_showAnimator       = false;
    bool m_showBlendTree      = false;
    bool m_showTextureBrowser = false;
    bool m_showModelInfo      = true;
    bool m_showSkeleton       = false;
    bool m_showAssetBrowser   = true;

    // --- パネルインスタンス（各パネルは独立クラスで参照渡しで描画） ---
    SceneHierarchyPanel m_sceneHierarchyPanel;
    PropertyPanel       m_propertyPanel;
    LightingPanel       m_lightingPanel;
    PostEffectPanel     m_postEffectPanel;
    TerrainPanel        m_terrainPanel;
    SkyboxPanel         m_skyboxPanel;
    PerformancePanel    m_performancePanel;
    LogPanel            m_logPanel;
    TimelinePanel       m_timelinePanel;
    AnimatorPanel       m_animatorPanel;
    BlendTreeEditor     m_blendTreeEditor;
    TextureBrowser      m_textureBrowser;
    ModelInfoPanel      m_modelInfoPanel;
    SkeletonPanel       m_skeletonPanel;
    AssetBrowserPanel   m_assetBrowserPanel;

    // --- ドラッグ&ドロップ ---
    std::vector<std::string> m_pendingDropFiles;  ///< WM_DROPFILESで受け取ったファイルパスキュー

    // --- デバッグ表示 ---
    bool m_showBounds = false;  ///< AABB表示ON/OFF（Bキーでトグル）

    // --- 背景色 ---
    float m_bgColor[3] = {0.4f, 0.55f, 0.8f};  ///< スカイボックスの上部色に連動
    bool  m_showBgColorPicker = false;

    // --- ギズモ設定 ---
    ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::TRANSLATE;  ///< 現在のギズモ操作（移動/回転/拡縮）
    ImGuizmo::MODE      m_gizmoMode      = ImGuizmo::WORLD;     ///< 座標空間（ワールド/ローカル）
    bool  m_useSnap        = false;     ///< スナップON/OFF
    float m_snapTranslation = 0.5f;     ///< 移動スナップ値
    float m_snapRotation    = 15.0f;    ///< 回転スナップ値（度）
    float m_snapScale       = 0.1f;     ///< スケールスナップ値
};
