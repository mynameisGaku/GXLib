#pragma once
/// @file GXModelViewerApp.h
/// @brief ImGui-based 3D model viewer application

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

/// @brief GXModelViewer application class
class GXModelViewerApp
{
public:
    bool Initialize(HINSTANCE hInstance, uint32_t width, uint32_t height, const wchar_t* title);
    int  Run();
    void Shutdown();

    // Public import methods (used by AssetBrowserPanel)
    void ImportModel(const std::string& filePath);
    void ImportAnimation(const std::string& filePath);

private:
    // ImGui lifecycle
    void InitImGui();
    void ShutdownImGui();
    void BeginImGuiFrame();
    void EndImGuiFrame(ID3D12GraphicsCommandList* cmdList);

    // UI
    void BuildDockSpace();
    void UpdateUI();
    void BuildMainMenuBar();
    void DrawViewportToolbar(ImVec2 imageMin);

    // Viewport picking
    void HandleViewportPicking();
    static bool ComputeEntityAABB(const SceneEntity& entity, XMFLOAT3& outMin, XMFLOAT3& outMax);

    // Orbit camera
    void UpdateOrbitCamera();
    void HandleOrbitInput();

    // Model export
    void ExportToGxmd(const std::string& outputPath);
    void ExportToGxan(const std::string& outputPath);

    // Rendering
    void RenderFrame(float deltaTime);
    void DrawSceneForShadow();
    void DrawEntitiesForShadow();
    void OnResize(uint32_t width, uint32_t height);

    // Bone visualization
    void DrawSkeletonOverlay(const SceneEntity& entity);

    // GXLib core
    GX::Application     m_app;
    GX::GraphicsDevice  m_graphicsDevice;
    GX::CommandQueue    m_commandQueue;
    GX::CommandList     m_commandList;
    GX::SwapChain       m_swapChain;

    // Rendering
    GX::Renderer3D           m_renderer3D;
    GX::Camera3D             m_camera;
    GX::PostEffectPipeline   m_postEffect;

    // Scene data
    SceneGraph          m_sceneGraph;
    InfiniteGrid        m_infiniteGrid;

    // Resource managers
    GX::ModelLoader     m_modelLoader;
    GX::MaterialManager m_materialManager;
    GX::TextureManager  m_textureManager;

    // ImGui
    GX::DescriptorHeap  m_imguiSrvHeap;
    bool                m_imguiInitialized = false;

    // Viewport render target (3D scene -> this RT -> ImGui::Image)
    GX::RenderTarget    m_viewportRT;
    uint32_t            m_viewportWidth  = 0;
    uint32_t            m_viewportHeight = 0;
    bool                m_viewportNeedsResize = false;
    bool                m_viewportHovered = false;
    bool                m_viewportFocused = false;
    uint32_t            m_viewportSrvIndex = 0;  // index in m_imguiSrvHeap

    // Viewport image rect (for picking)
    ImVec2              m_viewportImageMin  = {};
    ImVec2              m_viewportImageSize = {};

    // Frame synchronization
    uint64_t m_frameFenceValues[GX::SwapChain::k_BufferCount] = {};
    uint32_t m_frameIndex = 0;
    float    m_totalTime  = 0.0f;

    // State
    uint32_t m_width  = 0;
    uint32_t m_height = 0;
    bool     m_running = true;
    bool     m_showDemoWindow = false;

    // Orbit camera
    float m_orbitYaw   = 0.0f;
    float m_orbitPitch = 0.5f;
    float m_orbitDistance = 8.0f;
    float m_orbitMaxDistance = 200.0f;
    XMFLOAT3 m_orbitTarget = { 0.0f, 0.0f, 0.0f };
    bool  m_orbitDragActive = false;
    int   m_prevSelectedBone = -1;

    // Panel visibility flags
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

    // Panel instances
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

    // Drag & drop
    std::vector<std::string> m_pendingDropFiles;

    // Debug
    bool m_showBounds = false;

    // Background color
    float m_bgColor[3] = {0.4f, 0.55f, 0.8f};
    bool  m_showBgColorPicker = false;

    // Gizmo
    ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE      m_gizmoMode      = ImGuizmo::WORLD;
    bool  m_useSnap        = false;
    float m_snapTranslation = 0.5f;
    float m_snapRotation    = 15.0f;
    float m_snapScale       = 0.1f;
};
