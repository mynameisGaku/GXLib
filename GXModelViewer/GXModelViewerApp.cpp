/// @file GXModelViewerApp.cpp
/// @brief GXModelViewer application implementation

#include "GXModelViewerApp.h"
#include "ModelExporter.h"
#include "Scene/SceneSerializer.h"
#include "Core/Logger.h"
#include "Graphics/3D/Transform3D.h"
#include "Math/Collision/Collision3D.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "implot.h"
#include "imnodes.h"
#include "ImGuizmo.h"
#include "ImGuiFileDialog.h"

#include <anim_loader.h>

#include <shellapi.h>
#include <algorithm>
#include <filesystem>

// Forward declaration for ImGui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================================
// Initialize
// ============================================================================

bool GXModelViewerApp::Initialize(HINSTANCE hInstance, uint32_t width, uint32_t height, const wchar_t* title)
{
    m_width  = width;
    m_height = height;

    // Create application window
    GX::ApplicationDesc appDesc;
    appDesc.title  = title;
    appDesc.width  = width;
    appDesc.height = height;
    if (!m_app.Initialize(appDesc))
        return false;

    // Hook ImGui message handler into GXLib's Window message callback system
    m_app.GetWindow().AddMessageCallback([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> bool {
        LRESULT result = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        return result != 0;
    });

    // Drag & drop support
    DragAcceptFiles(m_app.GetWindow().GetHWND(), TRUE);
    m_app.GetWindow().AddMessageCallback([this](HWND, UINT msg, WPARAM wp, LPARAM) -> bool {
        if (msg == WM_DROPFILES)
        {
            HDROP hDrop = reinterpret_cast<HDROP>(wp);
            UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < count; ++i)
            {
                wchar_t path[MAX_PATH];
                DragQueryFileW(hDrop, i, path, MAX_PATH);
                int len = WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
                std::string utf8(len - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, path, -1, utf8.data(), len, nullptr, nullptr);
                m_pendingDropFiles.push_back(std::move(utf8));
            }
            DragFinish(hDrop);
            return true;
        }
        return false;
    });

    // Resize callback
    m_app.GetWindow().SetResizeCallback([this](uint32_t w, uint32_t h) {
        OnResize(w, h);
    });

    // Initialize graphics device
#ifdef _DEBUG
    bool enableDebug = true;
#else
    bool enableDebug = false;
#endif
    if (!m_graphicsDevice.Initialize(enableDebug))
        return false;

    auto* device = m_graphicsDevice.GetDevice();

    // Command queue
    if (!m_commandQueue.Initialize(device))
        return false;

    // Command list
    if (!m_commandList.Initialize(device))
        return false;

    // Swap chain
    GX::SwapChainDesc scDesc;
    scDesc.hwnd   = m_app.GetWindow().GetHWND();
    scDesc.width  = width;
    scDesc.height = height;
    if (!m_swapChain.Initialize(m_graphicsDevice.GetFactory(), device, m_commandQueue.GetQueue(), scDesc))
        return false;

    // Renderer3D
    if (!m_renderer3D.Initialize(device, m_commandQueue.GetQueue(), width, height))
        return false;

    // Enable CSM shadows
    m_renderer3D.SetShadowEnabled(true);

    // PostEffectPipeline (HDR -> tonemap -> backbuffer)
    if (!m_postEffect.Initialize(device, width, height))
        return false;
    m_postEffect.SetTonemapMode(GX::TonemapMode::ACES);
    m_postEffect.SetFXAAEnabled(true);

    // Set up skybox
    m_renderer3D.GetSkybox().SetColors({ 0.4f, 0.55f, 0.8f }, { 0.7f, 0.75f, 0.85f });
    m_renderer3D.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 3.0f);

    // Initialize infinite grid
    {
        GX::Shader gridShader;
        gridShader.Initialize();
        if (!m_infiniteGrid.Initialize(device, gridShader))
            GX_LOG_WARN("InfiniteGrid initialization failed");
    }

    // Set up default lights
    GX::LightData lights[2];
    lights[0] = GX::Light::CreateDirectional({ 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
    lights[1] = GX::Light::CreatePoint({ -3.0f, 4.0f, -3.0f }, 20.0f, { 1.0f, 0.95f, 0.9f }, 2.0f);
    m_renderer3D.SetLights(lights, 2, { 0.15f, 0.15f, 0.18f });

    // Initialize lighting panel
    m_lightingPanel.Initialize();

    // Initialize texture manager
    m_textureManager.Initialize(device, m_commandQueue.GetQueue());

    // Set up camera (orbit mode)
    m_camera.SetPerspective(XM_PIDIV4, static_cast<float>(width) / height, 0.1f, 1000.0f);
    UpdateOrbitCamera();

    // Initialize ImGui
    InitImGui();

    // Create viewport render target (3D scene renders here, shown via ImGui::Image)
    m_viewportWidth  = width;
    m_viewportHeight = height;
    m_viewportRT.Create(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM);

    // Allocate an SRV in ImGui's heap for the viewport texture
    m_viewportSrvIndex = m_imguiSrvHeap.AllocateIndex();
    device->CreateShaderResourceView(
        m_viewportRT.GetResource(), nullptr,
        m_imguiSrvHeap.GetCPUHandle(m_viewportSrvIndex));

    // Set asset browser root to current working directory
    {
        char cwd[MAX_PATH];
        if (GetCurrentDirectoryA(MAX_PATH, cwd))
            m_assetBrowserPanel.SetRootPath(cwd);
    }

    GX_LOG_INFO("=== GXModelViewer initialized ===");
    return true;
}

// ============================================================================
// ImGui Setup
// ============================================================================

void GXModelViewerApp::InitImGui()
{
    auto* device = m_graphicsDevice.GetDevice();

    // Create a dedicated SRV descriptor heap for ImGui (shader visible)
    m_imguiSrvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256, true);

    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Dark style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 4.0f;
    style.FrameRounding    = 2.0f;
    style.GrabRounding     = 2.0f;
    style.ScrollbarRounding = 4.0f;

    // Init Win32 backend
    ImGui_ImplWin32_Init(m_app.GetWindow().GetHWND());

    // Init DX12 backend using the new InitInfo struct
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device            = device;
    initInfo.CommandQueue      = m_commandQueue.GetQueue();
    initInfo.NumFramesInFlight = GX::SwapChain::k_BufferCount;
    initInfo.RTVFormat         = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.SrvDescriptorHeap = m_imguiSrvHeap.GetHeap();

    // Descriptor allocator callbacks for ImGui texture management
    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
                                       D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu,
                                       D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu) {
        // We stored the DescriptorHeap pointer in UserData during init
        auto* heap = static_cast<GX::DescriptorHeap*>(info->UserData);
        uint32_t idx = heap->AllocateIndex();
        *out_cpu = heap->GetCPUHandle(idx);
        *out_gpu = heap->GetGPUHandle(idx);
    };
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info,
                                      D3D12_CPU_DESCRIPTOR_HANDLE cpu,
                                      D3D12_GPU_DESCRIPTOR_HANDLE gpu) {
        // We could track and free the index, but for simplicity we let it leak
        // since ImGui typically only allocates a small number of textures
        (void)info; (void)cpu; (void)gpu;
    };
    initInfo.UserData = &m_imguiSrvHeap;

    ImGui_ImplDX12_Init(&initInfo);

    // Initialize extension contexts
    ImPlot::CreateContext();
    ImNodes::CreateContext();

    m_imguiInitialized = true;
}

void GXModelViewerApp::ShutdownImGui()
{
    if (!m_imguiInitialized)
        return;

    ImNodes::DestroyContext();
    ImPlot::DestroyContext();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_imguiInitialized = false;
}

void GXModelViewerApp::BeginImGuiFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void GXModelViewerApp::EndImGuiFrame(ID3D12GraphicsCommandList* cmdList)
{
    ImGui::Render();

    // Set ImGui's SRV descriptor heap before rendering draw data
    ID3D12DescriptorHeap* heaps[] = { m_imguiSrvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

// ============================================================================
// UI
// ============================================================================

void GXModelViewerApp::BuildDockSpace()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
}

void GXModelViewerApp::BuildMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Import Model...", "Ctrl+O"))
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ImportModelDlg",
                    "Import Model",
                    ".gltf,.glb,.obj,.fbx,.gxmd",
                    config);
            }
            {
                SceneEntity* sel = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
                bool hasSkeleton = sel && sel->model && sel->model->HasSkeleton();
                if (ImGui::MenuItem("Import Animation...", nullptr, false, hasSkeleton))
                {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    ImGuiFileDialog::Instance()->OpenDialog(
                        "ImportAnimDlg",
                        "Import Animation",
                        ".gxan,.fbx,.gltf,.glb",
                        config);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Export to GXMD..."))
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ExportGxmdDlg",
                    "Export to GXMD",
                    ".gxmd",
                    config);
            }
            if (ImGui::MenuItem("Export to GXAN..."))
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ExportGxanDlg",
                    "Export to GXAN",
                    ".gxan",
                    config);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Scene..."))
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
                ImGuiFileDialog::Instance()->OpenDialog(
                    "SaveSceneDlg",
                    "Save Scene",
                    ".json",
                    config);
            }
            if (ImGui::MenuItem("Load Scene..."))
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog(
                    "LoadSceneDlg",
                    "Load Scene",
                    ".json",
                    config);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                m_running = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Scene Hierarchy", nullptr, &m_showSceneHierarchy);
            ImGui::MenuItem("Properties", nullptr, &m_showProperties);
            ImGui::MenuItem("Model Info", nullptr, &m_showModelInfo);
            ImGui::MenuItem("Skeleton", nullptr, &m_showSkeleton);
            ImGui::MenuItem("Lighting", nullptr, &m_showLighting);
            ImGui::MenuItem("Post Effects", nullptr, &m_showPostEffects);
            ImGui::MenuItem("Skybox", nullptr, &m_showSkybox);
            ImGui::MenuItem("Terrain", nullptr, &m_showTerrain);
            ImGui::Separator();
            ImGui::MenuItem("Timeline", nullptr, &m_showTimeline);
            ImGui::MenuItem("Animator", nullptr, &m_showAnimator);
            ImGui::MenuItem("Blend Tree", nullptr, &m_showBlendTree);
            ImGui::Separator();
            ImGui::MenuItem("Texture Browser", nullptr, &m_showTextureBrowser);
            ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
            ImGui::MenuItem("Performance", nullptr, &m_showPerformance);
            ImGui::MenuItem("Log", nullptr, &m_showLog);
            ImGui::Separator();
            // Wireframe (global toggle for all entities)
            {
                bool anyWireframe = false;
                for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
                {
                    SceneEntity* e = m_sceneGraph.GetEntity(ei);
                    if (e && e->showWireframe) { anyWireframe = true; break; }
                }
                if (ImGui::MenuItem("Wireframe (Global)", "W", anyWireframe))
                {
                    bool newVal = !anyWireframe;
                    for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
                    {
                        SceneEntity* e = m_sceneGraph.GetEntity(ei);
                        if (e) e->showWireframe = newVal;
                    }
                }
            }
            ImGui::MenuItem("Background Color", nullptr, &m_showBgColorPicker);
            ImGui::MenuItem("Show Bounds", "B", &m_showBounds);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemoWindow);
            if (ImGui::MenuItem("Reset Camera"))
            {
                m_orbitYaw      = 0.0f;
                m_orbitPitch    = 0.5f;
                m_orbitDistance  = 8.0f;
                m_orbitMaxDistance = 200.0f;
                m_orbitTarget   = { 0.0f, 0.0f, 0.0f };
                UpdateOrbitCamera();
            }
            ImGui::EndMenu();
        }

        // FPS display on the right side
        {
            float fps = m_app.GetTimer().GetFPS();
            char fpsText[32];
            snprintf(fpsText, sizeof(fpsText), "%.1f FPS", fps);
            float textWidth = ImGui::CalcTextSize(fpsText).x;
            ImGui::SameLine(ImGui::GetWindowWidth() - textWidth - 20.0f);
            ImGui::Text("%s", fpsText);
        }

        ImGui::EndMainMenuBar();
    }
}

void GXModelViewerApp::UpdateUI()
{
    // Process drag & drop files
    for (auto& path : m_pendingDropFiles)
    {
        auto dot = path.rfind('.');
        if (dot != std::string::npos)
        {
            std::string ext = path.substr(dot);
            // Convert extension to lowercase
            for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".gxmd")
                ImportModel(path);
            else if (ext == ".gxan")
                ImportAnimation(path);
        }
    }
    m_pendingDropFiles.clear();

    BuildMainMenuBar();
    BuildDockSpace();

    // Show ImGui demo window for reference
    if (m_showDemoWindow)
        ImGui::ShowDemoWindow(&m_showDemoWindow);

    // --- Viewport Window (3D scene) ---
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Viewport"))
    {
        ImVec2 size = ImGui::GetContentRegionAvail();
        uint32_t newW = (std::max)(1u, static_cast<uint32_t>(size.x));
        uint32_t newH = (std::max)(1u, static_cast<uint32_t>(size.y));
        if (newW != m_viewportWidth || newH != m_viewportHeight)
        {
            m_viewportWidth  = newW;
            m_viewportHeight = newH;
            m_viewportNeedsResize = true;
        }
        ImTextureID texID = static_cast<ImTextureID>(
            m_imguiSrvHeap.GetGPUHandle(m_viewportSrvIndex).ptr);
        ImGui::Image(texID, size);
        ImVec2 imageMin  = ImGui::GetItemRectMin();
        ImVec2 imageSize = ImGui::GetItemRectSize();
        m_viewportHovered = ImGui::IsWindowHovered();
        m_viewportFocused = ImGui::IsWindowFocused();

        // Save viewport rect for picking
        m_viewportImageMin  = imageMin;
        m_viewportImageSize = imageSize;

        // --- Drag-drop target on viewport ---
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                std::string droppedPath(static_cast<const char*>(payload->Data));
                auto dot = droppedPath.rfind('.');
                if (dot != std::string::npos)
                {
                    std::string ext = droppedPath.substr(dot);
                    for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
                    if (ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".gxmd")
                        ImportModel(droppedPath);
                    else if (ext == ".gxan")
                        ImportAnimation(droppedPath);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // --- ImGuizmo gizmo overlay ---
        SceneEntity* gizmoEntity = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
        if (gizmoEntity && gizmoEntity->model)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            // Use the actual image rect for accurate gizmo hit detection
            ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

            // Camera matrices → float[16]
            XMFLOAT4X4 viewF, projF;
            XMStoreFloat4x4(&viewF, m_camera.GetViewMatrix());
            XMStoreFloat4x4(&projF, m_camera.GetProjectionMatrix());

            // Entity world matrix → float[16]
            XMFLOAT4X4 worldF;
            XMStoreFloat4x4(&worldF, gizmoEntity->transform.GetWorldMatrix());

            // Snap values
            float snapValues[3] = {};
            if (m_useSnap)
            {
                if (m_gizmoOperation == ImGuizmo::TRANSLATE)
                    snapValues[0] = snapValues[1] = snapValues[2] = m_snapTranslation;
                else if (m_gizmoOperation == ImGuizmo::ROTATE)
                    snapValues[0] = snapValues[1] = snapValues[2] = m_snapRotation;
                else if (m_gizmoOperation == ImGuizmo::SCALE)
                    snapValues[0] = snapValues[1] = snapValues[2] = m_snapScale;
            }

            if (ImGuizmo::Manipulate(&viewF._11, &projF._11,
                m_gizmoOperation, m_gizmoMode,
                &worldF._11, nullptr,
                m_useSnap ? snapValues : nullptr))
            {
                // Only update the component matching the active gizmo operation.
                // We do NOT use DecomposeMatrixToComponents because it uses XYZ Euler
                // convention, which doesn't match DirectXMath's ZXY (RollPitchYaw).

                if (m_gizmoOperation == ImGuizmo::TRANSLATE)
                {
                    // Translation is in row 3 of the row-major matrix
                    gizmoEntity->transform.SetPosition(worldF._41, worldF._42, worldF._43);
                }
                else if (m_gizmoOperation == ImGuizmo::SCALE)
                {
                    // Scale = row lengths of the upper 3x3 (world = S * R * T)
                    float sx = sqrtf(worldF._11 * worldF._11 + worldF._12 * worldF._12 + worldF._13 * worldF._13);
                    float sy = sqrtf(worldF._21 * worldF._21 + worldF._22 * worldF._22 + worldF._23 * worldF._23);
                    float sz = sqrtf(worldF._31 * worldF._31 + worldF._32 * worldF._32 + worldF._33 * worldF._33);
                    gizmoEntity->transform.SetScale(sx, sy, sz);
                }
                else if (m_gizmoOperation == ImGuizmo::ROTATE)
                {
                    // Extract scale to normalize rotation rows
                    float sx = sqrtf(worldF._11 * worldF._11 + worldF._12 * worldF._12 + worldF._13 * worldF._13);
                    float sy = sqrtf(worldF._21 * worldF._21 + worldF._22 * worldF._22 + worldF._23 * worldF._23);
                    float sz = sqrtf(worldF._31 * worldF._31 + worldF._32 * worldF._32 + worldF._33 * worldF._33);
                    if (sx < 1e-6f) sx = 1e-6f;
                    if (sy < 1e-6f) sy = 1e-6f;
                    if (sz < 1e-6f) sz = 1e-6f;

                    // ZXY Euler decomposition matching XMMatrixRotationRollPitchYaw(pitch, yaw, roll)
                    // R = Rz(roll) * Rx(pitch) * Ry(yaw)
                    // R._32 = -sin(pitch),  R._31 = cos(pitch)*sin(yaw),  R._33 = cos(pitch)*cos(yaw)
                    // R._12 = sin(roll)*cos(pitch),  R._22 = cos(roll)*cos(pitch)
                    float r32 = worldF._32 / sz;
                    float pitch = asinf((std::max)(-1.0f, (std::min)(1.0f, -r32)));
                    float yaw   = atan2f(worldF._31, worldF._33);  // both /sz, cancels
                    float roll  = atan2f(worldF._12 / sx, worldF._22 / sy);

                    gizmoEntity->transform.SetRotation(pitch, yaw, roll);
                }
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();

    // --- Viewport toolbar (overlay window) ---
    DrawViewportToolbar(m_viewportImageMin);

    // --- Editor Panels ---

    // Scene Hierarchy
    if (m_showSceneHierarchy)
        m_sceneHierarchyPanel.Draw(m_sceneGraph);

    // Get selected entity for panel connections
    SceneEntity* selEntity = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);

    // Inspector (tabbed: Properties + Model Info + Skeleton)
    if (m_showProperties || m_showModelInfo || m_showSkeleton)
    {
        if (ImGui::Begin("Inspector"))
        {
            if (ImGui::BeginTabBar("InspectorTabs"))
            {
                if (m_showProperties && ImGui::BeginTabItem("Properties"))
                {
                    m_propertyPanel.DrawContent(m_sceneGraph, m_renderer3D.GetMaterialManager(), m_renderer3D.GetTextureManager(),
                                                m_gizmoOperation, m_gizmoMode, m_useSnap,
                                                m_snapTranslation, m_snapRotation, m_snapScale);
                    ImGui::EndTabItem();
                }
                if (m_showModelInfo && ImGui::BeginTabItem("Model Info"))
                {
                    m_modelInfoPanel.DrawContent(m_sceneGraph);
                    ImGui::EndTabItem();
                }
                if (m_showSkeleton && ImGui::BeginTabItem("Skeleton"))
                {
                    m_skeletonPanel.DrawContent(m_sceneGraph, selEntity ? selEntity->animator.get() : nullptr);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    // Rendering (tabbed: Lighting + Post Effects + Skybox)
    if (m_showLighting || m_showPostEffects || m_showSkybox)
    {
        if (ImGui::Begin("Rendering"))
        {
            if (ImGui::BeginTabBar("RenderingTabs"))
            {
                if (m_showLighting && ImGui::BeginTabItem("Lighting"))
                {
                    m_lightingPanel.DrawContent(m_renderer3D);
                    ImGui::EndTabItem();
                }
                if (m_showPostEffects && ImGui::BeginTabItem("Post Effects"))
                {
                    m_postEffectPanel.DrawContent(m_postEffect);
                    ImGui::EndTabItem();
                }
                if (m_showSkybox && ImGui::BeginTabItem("Skybox"))
                {
                    m_skyboxPanel.DrawContent(m_renderer3D.GetSkybox());
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    // Terrain (standalone)
    if (m_showTerrain)
        m_terrainPanel.Draw();

    // Timeline
    if (m_showTimeline)
        m_timelinePanel.Draw(
            selEntity ? selEntity->animator.get() : nullptr,
            selEntity ? selEntity->model : nullptr,
            m_app.GetTimer().GetDeltaTime(),
            selEntity ? &selEntity->selectedClipIndex : nullptr);

    // Animator (no state machine assigned yet)
    if (m_showAnimator)
        m_animatorPanel.Draw(nullptr);

    // Blend Tree (no blend tree assigned yet)
    if (m_showBlendTree)
        m_blendTreeEditor.Draw(nullptr);

    // Texture Browser
    if (m_showTextureBrowser)
        m_textureBrowser.Draw(m_textureManager);

    // Asset Browser
    if (m_showAssetBrowser)
        m_assetBrowserPanel.Draw(*this);

    // Camera
    if (ImGui::Begin("Camera"))
    {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)",
            m_camera.GetPosition().x, m_camera.GetPosition().y, m_camera.GetPosition().z);
        if (ImGui::SliderFloat("Distance", &m_orbitDistance, 0.1f, 200.0f))
            UpdateOrbitCamera();
        if (ImGui::SliderFloat("Yaw", &m_orbitYaw, -XM_PI, XM_PI))
            UpdateOrbitCamera();
        if (ImGui::SliderFloat("Pitch", &m_orbitPitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f))
            UpdateOrbitCamera();
    }
    ImGui::End();

    // Performance
    if (m_showPerformance)
        m_performancePanel.Draw(m_app.GetTimer().GetDeltaTime(), m_app.GetTimer().GetFPS());

    // Log
    if (m_showLog)
        m_logPanel.Draw();

    // Background color picker
    if (m_showBgColorPicker)
    {
        if (ImGui::Begin("Background Color", &m_showBgColorPicker))
        {
            if (ImGui::ColorEdit3("Color", m_bgColor))
            {
                m_renderer3D.GetSkybox().SetColors(
                    { m_bgColor[0], m_bgColor[1], m_bgColor[2] },
                    { (std::min)(m_bgColor[0] + 0.3f, 1.0f),
                      (std::min)(m_bgColor[1] + 0.3f, 1.0f),
                      (std::min)(m_bgColor[2] + 0.3f, 1.0f) });
            }
        }
        ImGui::End();
    }

    // Keyboard shortcuts (when ImGui doesn't want keyboard)
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        // Space: play/pause toggle
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
        {
            SceneEntity* sel = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
            if (sel && sel->animator)
            {
                if (sel->animator->IsPlaying() && !sel->animator->IsPaused())
                    sel->animator->Pause();
                else if (sel->animator->IsPaused())
                    sel->animator->Resume();
                else if (sel->animator->GetCurrentClip())
                    sel->animator->Play(sel->animator->GetCurrentClip(), true, 1.0f);
            }
        }

        // F: focus camera on selected entity
        if (ImGui::IsKeyPressed(ImGuiKey_F))
        {
            SceneEntity* sel = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
            if (sel && sel->model)
            {
                XMFLOAT3 aabbMin, aabbMax;
                if (ComputeEntityAABB(*sel, aabbMin, aabbMax))
                {
                    m_orbitTarget = {
                        (aabbMin.x + aabbMax.x) * 0.5f,
                        (aabbMin.y + aabbMax.y) * 0.5f,
                        (aabbMin.z + aabbMax.z) * 0.5f
                    };
                    float maxExtent = (std::max)({aabbMax.x - aabbMin.x, aabbMax.y - aabbMin.y, aabbMax.z - aabbMin.z});
                    m_orbitDistance = maxExtent * 1.5f;
                    if (m_orbitDistance < 1.0f) m_orbitDistance = 5.0f;
                    m_orbitMaxDistance = (std::max)(200.0f, maxExtent * 10.0f);
                    m_orbitYaw   = 0.0f;
                    m_orbitPitch = 0.3f;
                    UpdateOrbitCamera();
                }
            }
        }

        // W: wireframe toggle for selected entity
        if (ImGui::IsKeyPressed(ImGuiKey_W))
        {
            SceneEntity* sel = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
            if (sel) sel->showWireframe = !sel->showWireframe;
        }

        // Gizmo mode shortcuts
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            m_gizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_E))
            m_gizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            m_gizmoOperation = ImGuizmo::SCALE;
        if (ImGui::IsKeyPressed(ImGuiKey_L))
            m_gizmoMode = (m_gizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        if (ImGui::IsKeyPressed(ImGuiKey_B))
            m_showBounds = !m_showBounds;
    }

    // Handle orbit camera with mouse input (right-drag to rotate, scroll to zoom)
    HandleOrbitInput();

    // Viewport click picking (after orbit input to not interfere)
    HandleViewportPicking();

    // ImGuiFileDialog: must call Display() every frame to render the dialog
    if (ImGuiFileDialog::Instance()->Display("ImportModelDlg",
        ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            ImportModel(filePath);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Import Animation dialog
    if (ImGuiFileDialog::Instance()->Display("ImportAnimDlg",
        ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
            ImportAnimation(ImGuiFileDialog::Instance()->GetFilePathName());
        ImGuiFileDialog::Instance()->Close();
    }

    // Export GXMD dialog
    if (ImGuiFileDialog::Instance()->Display("ExportGxmdDlg",
        ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
            ExportToGxmd(ImGuiFileDialog::Instance()->GetFilePathName());
        ImGuiFileDialog::Instance()->Close();
    }

    // Export GXAN dialog
    if (ImGuiFileDialog::Instance()->Display("ExportGxanDlg",
        ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
            ExportToGxan(ImGuiFileDialog::Instance()->GetFilePathName());
        ImGuiFileDialog::Instance()->Close();
    }

    // Save Scene dialog
    if (ImGuiFileDialog::Instance()->Display("SaveSceneDlg",
        ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
            if (SceneSerializer::SaveToFile(m_sceneGraph, path))
                GX_LOG_INFO("Scene saved: %s", path.c_str());
            else
                GX_LOG_ERROR("Failed to save scene: %s", path.c_str());
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Load Scene dialog
    if (ImGuiFileDialog::Instance()->Display("LoadSceneDlg",
        ImGuiWindowFlags_NoCollapse, ImVec2(600, 400)))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
            if (SceneSerializer::LoadFromFile(m_sceneGraph, path))
            {
                GX_LOG_INFO("Scene loaded: %s", path.c_str());
                // Re-import models from stored paths
                for (int i = 0; i < m_sceneGraph.GetEntityCount(); ++i)
                {
                    SceneEntity* ent = m_sceneGraph.GetEntity(i);
                    if (ent && !ent->sourcePath.empty() && !ent->model)
                    {
                        ImportModel(ent->sourcePath);
                    }
                }
            }
            else
                GX_LOG_ERROR("Failed to load scene: %s", path.c_str());
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

// ============================================================================
// Import Model
// ============================================================================

void GXModelViewerApp::ImportModel(const std::string& filePath)
{
    // Convert UTF-8 std::string to std::wstring for ModelLoader
    int wlen = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0);
    std::wstring wpath(wlen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, wpath.data(), wlen);

    auto* device = m_graphicsDevice.GetDevice();
    auto model = m_modelLoader.LoadFromFile(wpath, device,
        m_renderer3D.GetTextureManager(), m_renderer3D.GetMaterialManager());
    if (!model)
    {
        GX_LOG_ERROR("Failed to import model: %s", filePath.c_str());
        return;
    }

    // Extract file name for entity name
    std::string entityName = filePath;
    auto lastSlash = entityName.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        entityName = entityName.substr(lastSlash + 1);

    // Add to scene graph
    int idx = m_sceneGraph.AddEntity(entityName);
    SceneEntity* entity = m_sceneGraph.GetEntity(idx);
    entity->ownedModel = std::move(model);
    entity->model = entity->ownedModel.get();
    entity->sourcePath = filePath;
    m_sceneGraph.selectedEntity = idx;

    // サブメッシュ可視性初期化
    entity->submeshVisibility.assign(entity->model->GetSubMeshCount(), true);

    // スキンドモデルならAnimator作成
    if (entity->model->HasSkeleton())
    {
        entity->animator = std::make_unique<GX::Animator>();
        entity->animator->SetSkeleton(entity->model->GetSkeleton());
        if (entity->model->GetAnimationCount() > 0)
        {
            entity->selectedClipIndex = 0;
            entity->animator->Play(&entity->model->GetAnimations()[0], true, 1.0f);
            m_showTimeline = true; // auto-show timeline for animated models
        }
        else
        {
            entity->animator->EvaluateBindPose();
        }
    }

    GX_LOG_INFO("Imported model: %s (%u submeshes, %u anims)",
                entityName.c_str(),
                entity->model->GetSubMeshCount(),
                entity->model->GetAnimationCount());
}

// ============================================================================
// Export
// ============================================================================

void GXModelViewerApp::ExportToGxmd(const std::string& outputPath)
{
    auto* entity = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
    if (!entity || !entity->model)
    {
        GX_LOG_ERROR("No model selected for GXMD export");
        return;
    }
    if (ModelExporter::ExportToGxmd(*entity, m_renderer3D.GetMaterialManager(), m_renderer3D.GetTextureManager(), outputPath))
        GX_LOG_INFO("Exported GXMD: %s", outputPath.c_str());
    else
        GX_LOG_ERROR("Failed to export GXMD");
}

void GXModelViewerApp::ExportToGxan(const std::string& outputPath)
{
    auto* entity = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
    if (!entity || !entity->model)
    {
        GX_LOG_ERROR("No model selected for GXAN export");
        return;
    }
    if (ModelExporter::ExportToGxan(*entity, outputPath))
        GX_LOG_INFO("Exported GXAN: %s", outputPath.c_str());
    else
        GX_LOG_ERROR("Failed to export GXAN");
}

void GXModelViewerApp::ImportAnimation(const std::string& filePath)
{
    auto* entity = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
    if (!entity || !entity->model || !entity->model->HasSkeleton())
    {
        GX_LOG_ERROR("No skinned model selected for animation import");
        return;
    }

    // UTF-8 → wstring
    int wlen = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0);
    std::wstring wpath(wlen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, wpath.data(), wlen);

    // Determine extension
    auto dot = filePath.rfind('.');
    std::string ext = (dot != std::string::npos) ? filePath.substr(dot) : "";
    for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    GX::Skeleton* skeleton = entity->model->GetSkeleton();

    // Extract clip name from file name
    std::string clipBaseName = filePath;
    auto lastSlash = clipBaseName.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        clipBaseName = clipBaseName.substr(lastSlash + 1);
    auto dotPos = clipBaseName.rfind('.');
    if (dotPos != std::string::npos)
        clipBaseName = clipBaseName.substr(0, dotPos);

    uint32_t importedCount = 0;

    if (ext == ".gxan")
    {
        // GXAN: bone-name based channels
        auto loaded = gxloader::LoadGxanW(wpath);
        if (!loaded)
        {
            GX_LOG_ERROR("Failed to load GXAN: %s", filePath.c_str());
            return;
        }

        GX::AnimationClip clip;
        clip.SetName(clipBaseName);
        clip.SetDuration(loaded->duration);

        std::unordered_map<int, GX::AnimationChannel> channelMap;
        for (const auto& ch : loaded->channels)
        {
            int jointIdx = skeleton->FindJointIndex(ch.boneName);
            if (jointIdx < 0) continue;

            auto& animCh = channelMap[jointIdx];
            animCh.jointIndex = jointIdx;
            animCh.interpolation = static_cast<GX::InterpolationType>(ch.interpolation);

            if (ch.target == 0)
                for (const auto& key : ch.vecKeys)
                    animCh.translationKeys.push_back({ key.time, { key.value[0], key.value[1], key.value[2] } });
            else if (ch.target == 1)
                for (const auto& key : ch.quatKeys)
                    animCh.rotationKeys.push_back({ key.time, { key.value[0], key.value[1], key.value[2], key.value[3] } });
            else if (ch.target == 2)
                for (const auto& key : ch.vecKeys)
                    animCh.scaleKeys.push_back({ key.time, { key.value[0], key.value[1], key.value[2] } });
        }

        for (auto& [idx, animCh] : channelMap)
            clip.AddChannel(animCh);

        entity->model->AddAnimation(std::move(clip));
        importedCount = 1;
    }
    else
    {
        // FBX/glTF/GXMD: load full model temporarily, extract animations via bone name remapping
        auto* device = m_graphicsDevice.GetDevice();
        GX::TextureManager tmpTexMgr;
        tmpTexMgr.Initialize(device, m_commandQueue.GetQueue());
        GX::MaterialManager tmpMatMgr;

        auto srcModel = m_modelLoader.LoadFromFile(wpath, device, tmpTexMgr, tmpMatMgr);
        if (!srcModel)
        {
            GX_LOG_ERROR("Failed to load model for animation extraction: %s", filePath.c_str());
            return;
        }

        if (srcModel->GetAnimationCount() == 0)
        {
            GX_LOG_ERROR("Source file contains no animations: %s", filePath.c_str());
            return;
        }

        const GX::Skeleton* srcSkeleton = srcModel->GetSkeleton();
        if (!srcSkeleton)
        {
            GX_LOG_ERROR("Source file has no skeleton for bone remapping: %s", filePath.c_str());
            return;
        }

        const auto& srcJoints = srcSkeleton->GetJoints();
        const auto& srcAnims = srcModel->GetAnimations();

        for (const auto& srcClip : srcAnims)
        {
            GX::AnimationClip newClip;
            std::string name = srcClip.GetName();
            if (name.empty())
                name = clipBaseName + "_" + std::to_string(importedCount);
            newClip.SetName(name);
            newClip.SetDuration(srcClip.GetDuration());

            for (const auto& srcCh : srcClip.GetChannels())
            {
                // Get bone name from source skeleton
                if (srcCh.jointIndex < 0 || srcCh.jointIndex >= static_cast<int>(srcJoints.size()))
                    continue;
                const std::string& boneName = srcJoints[srcCh.jointIndex].name;

                // Find in target skeleton
                int targetIdx = skeleton->FindJointIndex(boneName);
                if (targetIdx < 0) continue;

                GX::AnimationChannel newCh;
                newCh.jointIndex = targetIdx;
                newCh.interpolation = srcCh.interpolation;
                newCh.translationKeys = srcCh.translationKeys;
                newCh.rotationKeys = srcCh.rotationKeys;
                newCh.scaleKeys = srcCh.scaleKeys;
                newClip.AddChannel(newCh);
            }

            entity->model->AddAnimation(std::move(newClip));
            ++importedCount;
        }
    }

    if (importedCount == 0)
    {
        GX_LOG_WARN("No animations could be imported from: %s", filePath.c_str());
        return;
    }

    // Switch to first newly imported clip
    uint32_t firstNewIdx = entity->model->GetAnimationCount() - importedCount;
    if (entity->animator)
    {
        entity->selectedClipIndex = static_cast<int>(firstNewIdx);
        entity->animator->Play(&entity->model->GetAnimations()[firstNewIdx], true, 1.0f);
    }

    // Enable timeline
    m_showTimeline = true;

    GX_LOG_INFO("Imported %u animation(s) from: %s", importedCount, filePath.c_str());
}

// ============================================================================
// Orbit Camera
// ============================================================================

void GXModelViewerApp::UpdateOrbitCamera()
{
    float x = m_orbitDistance * cosf(m_orbitPitch) * sinf(m_orbitYaw);
    float y = m_orbitDistance * sinf(m_orbitPitch);
    float z = m_orbitDistance * cosf(m_orbitPitch) * cosf(m_orbitYaw);

    m_camera.SetPosition(
        m_orbitTarget.x + x,
        m_orbitTarget.y + y,
        m_orbitTarget.z + z
    );

    // Sync Camera3D pitch/yaw so Free mode faces the orbit target
    // orbit→camera mapping: cameraPitch = -orbitPitch, cameraYaw = orbitYaw + PI
    m_camera.SetPitch(-m_orbitPitch);
    m_camera.SetYaw(m_orbitYaw + XM_PI);
}

void GXModelViewerApp::HandleOrbitInput()
{
    ImGuiIO& io = ImGui::GetIO();

    // Track drag start: only begin orbit/pan if mouse is over the Viewport window and not over gizmo
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
    {
        m_orbitDragActive = m_viewportHovered && !ImGuizmo::IsOver();
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle))
    {
        m_orbitDragActive = false;
    }

    // Right-drag to orbit
    if (m_orbitDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        ImVec2 delta = io.MouseDelta;
        m_orbitYaw   -= delta.x * 0.01f;
        m_orbitPitch += delta.y * 0.01f;

        // Clamp pitch to avoid gimbal lock
        m_orbitPitch = (std::max)(-XM_PIDIV2 + 0.01f, (std::min)(m_orbitPitch, XM_PIDIV2 - 0.01f));
        UpdateOrbitCamera();
    }

    // Middle-drag to pan
    if (m_orbitDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        ImVec2 delta = io.MouseDelta;
        float panSpeed = m_orbitDistance * 0.002f;

        // Compute camera right and up vectors for panning
        XMFLOAT3 right = m_camera.GetRight();
        XMFLOAT3 up    = m_camera.GetUp();

        m_orbitTarget.x -= right.x * delta.x * panSpeed;
        m_orbitTarget.y -= right.y * delta.x * panSpeed;
        m_orbitTarget.z -= right.z * delta.x * panSpeed;

        m_orbitTarget.x += up.x * delta.y * panSpeed;
        m_orbitTarget.y += up.y * delta.y * panSpeed;
        m_orbitTarget.z += up.z * delta.y * panSpeed;

        UpdateOrbitCamera();
    }

    // Scroll to zoom (only when hovering the Viewport window and gizmo not in use)
    if (m_viewportHovered && !ImGuizmo::IsUsing() && io.MouseWheel != 0.0f)
    {
        m_orbitDistance -= io.MouseWheel * m_orbitDistance * 0.15f;
        m_orbitDistance = (std::max)(0.1f, (std::min)(m_orbitDistance, m_orbitMaxDistance));
        UpdateOrbitCamera();
    }
}

// ============================================================================
// Rendering
// ============================================================================

void GXModelViewerApp::RenderFrame(float deltaTime)
{
    m_totalTime += deltaTime;

    // Handle viewport resize (deferred from ImGui frame)
    if (m_viewportNeedsResize && m_viewportWidth > 0 && m_viewportHeight > 0)
    {
        m_viewportNeedsResize = false;
        m_commandQueue.Flush();

        auto* device = m_graphicsDevice.GetDevice();
        m_viewportRT.Create(device, m_viewportWidth, m_viewportHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
        m_postEffect.OnResize(device, m_viewportWidth, m_viewportHeight);
        m_renderer3D.OnResize(m_viewportWidth, m_viewportHeight);
        m_camera.SetPerspective(m_camera.GetFovY(),
            static_cast<float>(m_viewportWidth) / m_viewportHeight,
            m_camera.GetNearZ(), m_camera.GetFarZ());

        // Re-create SRV in ImGui's heap for the new viewport RT resource
        device->CreateShaderResourceView(
            m_viewportRT.GetResource(), nullptr,
            m_imguiSrvHeap.GetCPUHandle(m_viewportSrvIndex));
    }

    m_frameIndex = m_swapChain.GetCurrentBackBufferIndex();
    m_commandQueue.GetFence().WaitForValue(m_frameFenceValues[m_frameIndex]);
    m_commandList.Reset(m_frameIndex, nullptr);
    auto* cmdList = m_commandList.Get();

    // === シャドウパス ===
    m_renderer3D.UpdateShadow(m_camera);

    // CSMパス（4カスケード）
    for (uint32_t cascade = 0; cascade < GX::CascadedShadowMap::k_NumCascades; ++cascade)
    {
        m_renderer3D.BeginShadowPass(cmdList, m_frameIndex, cascade);
        DrawSceneForShadow();
        m_renderer3D.EndShadowPass(cascade);
    }

    // スポットシャドウパス（スポットライトがある場合のみ描画）
    m_renderer3D.BeginSpotShadowPass(cmdList, m_frameIndex);
    if (m_renderer3D.IsInShadowPass())
        DrawEntitiesForShadow();
    m_renderer3D.EndSpotShadowPass();

    // ポイントシャドウパス（ポイントライトがある場合のみ描画）
    for (uint32_t face = 0; face < 6; ++face)
    {
        m_renderer3D.BeginPointShadowPass(cmdList, m_frameIndex, face);
        if (m_renderer3D.IsInShadowPass())
            DrawEntitiesForShadow();
        m_renderer3D.EndPointShadowPass(face);
    }

    // --- PostEffect BeginScene: binds HDR render target + depth buffer ---
    auto dsvHandle = m_renderer3D.GetDepthBuffer().GetDSVHandle();
    m_postEffect.BeginScene(cmdList, m_frameIndex, dsvHandle, m_camera);

    // Draw skybox into HDR RT (rotation-only view so sky stays at infinity)
    {
        XMFLOAT4X4 viewF;
        XMStoreFloat4x4(&viewF, m_camera.GetViewMatrix());
        viewF._41 = 0.0f; viewF._42 = 0.0f; viewF._43 = 0.0f;
        XMMATRIX viewRotOnly = XMLoadFloat4x4(&viewF);
        XMFLOAT4X4 vpMat;
        XMStoreFloat4x4(&vpMat, XMMatrixTranspose(viewRotOnly * m_camera.GetProjectionMatrix()));
        m_renderer3D.GetSkybox().Draw(cmdList, m_frameIndex, vpMat);
    }

    // 3D Renderer pass: draw scene into HDR RT
    m_renderer3D.Begin(cmdList, m_frameIndex, m_camera, m_totalTime);

    // Draw scene entities (imported models)
    for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
    {
        SceneEntity* entity = m_sceneGraph.GetEntity(ei);
        if (!entity || !entity->model || !entity->visible) continue;

        // Animator更新
        if (entity->animator)
            entity->animator->Update(deltaTime);

        // マテリアルオーバーライド
        if (entity->useMaterialOverride)
            m_renderer3D.SetMaterialOverride(&entity->materialOverride);

        // ワイヤフレーム
        if (entity->showWireframe) m_renderer3D.SetWireframeMode(true);

        // サブメッシュマスク判定
        bool hasMask = false;
        for (size_t si = 0; si < entity->submeshVisibility.size(); ++si)
        {
            if (!entity->submeshVisibility[si]) { hasMask = true; break; }
        }

        // 描画
        if (entity->model->IsSkinned() && entity->animator)
        {
            if (hasMask)
                m_renderer3D.DrawSkinnedModel(*entity->model, entity->transform,
                                               *entity->animator, entity->submeshVisibility);
            else
                m_renderer3D.DrawSkinnedModel(*entity->model, entity->transform,
                                               *entity->animator);
        }
        else
        {
            if (hasMask)
                m_renderer3D.DrawModel(*entity->model, entity->transform,
                                        entity->submeshVisibility);
            else
                m_renderer3D.DrawModel(*entity->model, entity->transform);
        }

        if (entity->showWireframe) m_renderer3D.SetWireframeMode(false);
        if (entity->useMaterialOverride) m_renderer3D.ClearMaterialOverride();
    }

    m_renderer3D.End();

    // ボーン可視化（PrimitiveBatch3Dでオーバーレイ）
    {
        bool anyBoneVis = false;
        for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
        {
            SceneEntity* entity = m_sceneGraph.GetEntity(ei);
            if (entity && entity->visible && entity->showBones &&
                entity->model && entity->model->HasSkeleton() && entity->animator)
            {
                anyBoneVis = true;
                break;
            }
        }
        if (anyBoneVis)
        {
            auto& pb3d = m_renderer3D.GetPrimitiveBatch3D();
            XMFLOAT4X4 vpMat;
            XMStoreFloat4x4(&vpMat, XMMatrixTranspose(m_camera.GetViewMatrix() * m_camera.GetProjectionMatrix()));
            pb3d.Begin(cmdList, m_frameIndex, vpMat);
            for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
            {
                SceneEntity* entity = m_sceneGraph.GetEntity(ei);
                if (!entity || !entity->visible || !entity->showBones) continue;
                if (!entity->model || !entity->model->HasSkeleton() || !entity->animator) continue;
                DrawSkeletonOverlay(*entity);
            }
            pb3d.End();
        }
    }

    // オービットターゲット球描画 + AABB可視化
    {
        auto& pb3d = m_renderer3D.GetPrimitiveBatch3D();
        XMFLOAT4X4 vpMat;
        XMStoreFloat4x4(&vpMat, XMMatrixTranspose(
            m_camera.GetViewMatrix() * m_camera.GetProjectionMatrix()));
        pb3d.Begin(cmdList, m_frameIndex, vpMat);
        pb3d.DrawWireSphere(m_orbitTarget, 0.05f, {1.0f, 1.0f, 1.0f, 0.4f}, 12);

        // AABB bounds visualization
        if (m_showBounds)
        {
            for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
            {
                SceneEntity* entity = m_sceneGraph.GetEntity(ei);
                if (!entity || !entity->model || !entity->visible) continue;

                XMFLOAT3 localMin, localMax;
                if (!ComputeEntityAABB(*entity, localMin, localMax)) continue;

                // Transform 8 corners to world and draw OBB edges
                XMMATRIX worldMat = entity->transform.GetWorldMatrix();
                XMFLOAT3 corners[8];
                for (int c = 0; c < 8; ++c)
                {
                    float cx = (c & 1) ? localMax.x : localMin.x;
                    float cy = (c & 2) ? localMax.y : localMin.y;
                    float cz = (c & 4) ? localMax.z : localMin.z;
                    XMVECTOR pt = XMVector3Transform(XMVectorSet(cx, cy, cz, 1.0f), worldMat);
                    XMStoreFloat3(&corners[c], pt);
                }

                bool isSelected = (ei == m_sceneGraph.selectedEntity);
                XMFLOAT4 boxColor = isSelected
                    ? XMFLOAT4{0.0f, 1.0f, 0.0f, 1.0f}
                    : XMFLOAT4{1.0f, 1.0f, 0.0f, 0.6f};

                // 12 edges of a box: bottom 4, top 4, vertical 4
                // corner index: bit0=X, bit1=Y, bit2=Z
                int edges[12][2] = {
                    {0,1},{2,3},{4,5},{6,7}, // X edges
                    {0,2},{1,3},{4,6},{5,7}, // Y edges
                    {0,4},{1,5},{2,6},{3,7}, // Z edges
                };
                for (auto& e : edges)
                    pb3d.DrawLine(corners[e[0]], corners[e[1]], boxColor);
            }
        }

        pb3d.End();
    }

    // Infinite grid (draws into HDR RT with depth test)
    m_infiniteGrid.Draw(cmdList, m_frameIndex, m_camera);

    // --- PostEffect EndScene ---
    m_postEffect.EndScene();

    // --- Transition viewport RT: PSR -> RENDER_TARGET ---
    m_viewportRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // --- PostEffect Resolve: HDR -> tonemap/FXAA -> viewport RT ---
    auto vpRtvHandle = m_viewportRT.GetRTVHandle();
    m_postEffect.Resolve(vpRtvHandle, m_renderer3D.GetDepthBuffer(), m_camera, deltaTime);

    // --- Transition viewport RT: RENDER_TARGET -> PSR (for ImGui sampling) ---
    m_viewportRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // --- Transition backbuffer: PRESENT -> RENDER_TARGET ---
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_swapChain.GetCurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // --- Clear backbuffer and render ImGui (which includes the viewport texture) ---
    auto rtvHandle = m_swapChain.GetCurrentRTVHandle();
    float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width    = static_cast<float>(m_width);
    viewport.Height   = static_cast<float>(m_height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect = {};
    scissorRect.left   = 0;
    scissorRect.top    = 0;
    scissorRect.right  = static_cast<LONG>(m_width);
    scissorRect.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &scissorRect);

    EndImGuiFrame(cmdList);

    // --- Transition backbuffer: RENDER_TARGET -> PRESENT ---
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    cmdList->ResourceBarrier(1, &barrier);

    m_commandList.Close();

    ID3D12CommandList* cmdLists[] = { cmdList };
    m_commandQueue.ExecuteCommandLists(cmdLists, 1);

    m_swapChain.Present(false);
    m_frameFenceValues[m_frameIndex] = m_commandQueue.GetFence().Signal(m_commandQueue.GetQueue());
}

void GXModelViewerApp::OnResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;

    m_width  = width;
    m_height = height;

    // Only resize the SwapChain for the OS window.
    // 3D scene / PostEffect / DepthBuffer are driven by the ImGui viewport window size.
    m_commandQueue.Flush();
    m_swapChain.Resize(m_graphicsDevice.GetDevice(), width, height);
}

// ============================================================================
// Main loop
// ============================================================================

int GXModelViewerApp::Run()
{
    m_app.GetTimer().Reset();

    while (m_running)
    {
        // Process Windows messages
        if (!m_app.GetWindow().ProcessMessages())
            break;

        m_app.GetTimer().Tick();
        float dt = m_app.GetTimer().GetDeltaTime();

        // ImGui new frame
        BeginImGuiFrame();

        // Build UI
        UpdateUI();

        // Bone selection change: move camera to bone, auto-enable showBones
        if (m_sceneGraph.selectedBone != m_prevSelectedBone)
        {
            m_prevSelectedBone = m_sceneGraph.selectedBone;
            if (m_sceneGraph.selectedBone >= 0)
            {
                SceneEntity* sel = m_sceneGraph.GetEntity(m_sceneGraph.selectedEntity);
                if (sel && sel->animator && sel->model && sel->model->HasSkeleton())
                {
                    sel->showBones = true;
                    const auto& gt = sel->animator->GetGlobalTransforms();
                    int bi = m_sceneGraph.selectedBone;
                    if (bi < static_cast<int>(gt.size()))
                    {
                        XMVECTOR posLocal = XMVectorSet(gt[bi]._41, gt[bi]._42, gt[bi]._43, 1.0f);
                        XMVECTOR posWorld = XMVector3Transform(posLocal, sel->transform.GetWorldMatrix());
                        XMStoreFloat3(&m_orbitTarget, posWorld);
                        UpdateOrbitCamera();
                    }
                }
            }
        }

        // Flush GPU before destroying entities (resources may still be in-flight)
        if (m_sceneGraph.HasPendingRemovals())
        {
            m_commandQueue.Flush();
            m_sceneGraph.ProcessPendingRemovals();
        }

        // Render 3D scene + ImGui
        RenderFrame(dt);
    }

    // Wait for GPU to finish before cleanup
    m_commandQueue.Flush();
    return 0;
}

// ============================================================================
// Shadow pass scene draw
// ============================================================================

void GXModelViewerApp::DrawSceneForShadow()
{
    // シーンエンティティ
    for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
    {
        SceneEntity* entity = m_sceneGraph.GetEntity(ei);
        if (!entity || !entity->model || !entity->visible) continue;

        if (entity->model->IsSkinned() && entity->animator)
            m_renderer3D.DrawSkinnedModel(*entity->model, entity->transform, *entity->animator);
        else
            m_renderer3D.DrawModel(*entity->model, entity->transform);
    }
}

void GXModelViewerApp::DrawEntitiesForShadow()
{
    // エンティティのみ描画（グリッド除外 — ポイント/スポットシャドウ用）
    for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
    {
        SceneEntity* entity = m_sceneGraph.GetEntity(ei);
        if (!entity || !entity->model || !entity->visible) continue;

        if (entity->model->IsSkinned() && entity->animator)
            m_renderer3D.DrawSkinnedModel(*entity->model, entity->transform, *entity->animator);
        else
            m_renderer3D.DrawModel(*entity->model, entity->transform);
    }
}

// ============================================================================
// Bone Visualization
// ============================================================================

void GXModelViewerApp::DrawSkeletonOverlay(const SceneEntity& entity)
{
    if (!entity.model || !entity.model->HasSkeleton() || !entity.animator)
        return;

    const auto& joints = entity.model->GetSkeleton()->GetJoints();
    const auto& globalTransforms = entity.animator->GetGlobalTransforms();
    auto& pb3d = m_renderer3D.GetPrimitiveBatch3D();

    // Get entity world transform
    XMMATRIX worldMatrix = entity.transform.GetWorldMatrix();

    int selectedBone = m_sceneGraph.selectedBone;

    for (size_t i = 0; i < joints.size(); ++i)
    {
        if (i >= globalTransforms.size()) break;

        // Joint position in model space
        XMVECTOR jointPosLocal = XMVectorSet(
            globalTransforms[i]._41,
            globalTransforms[i]._42,
            globalTransforms[i]._43,
            1.0f);

        // Transform to world space
        XMVECTOR jointPosWorld = XMVector3Transform(jointPosLocal, worldMatrix);
        XMFLOAT3 jPos;
        XMStoreFloat3(&jPos, jointPosWorld);

        bool isSelected = (static_cast<int>(i) == selectedBone);

        if (isSelected)
        {
            // Selected bone: cyan sphere, larger
            pb3d.DrawWireSphere(jPos, 0.04f, { 0.0f, 1.0f, 1.0f, 1.0f }, 16);

            // Draw local axes (X=red, Y=green, Z=blue)
            XMMATRIX jointWorld = XMLoadFloat4x4(&globalTransforms[i]) * worldMatrix;
            float axisLen = 0.1f;
            XMVECTOR origin = jointPosWorld;
            XMVECTOR xDir = XMVector3Normalize(jointWorld.r[0]);
            XMVECTOR yDir = XMVector3Normalize(jointWorld.r[1]);
            XMVECTOR zDir = XMVector3Normalize(jointWorld.r[2]);

            XMFLOAT3 oPos;
            XMStoreFloat3(&oPos, origin);

            XMFLOAT3 xEnd, yEnd, zEnd;
            XMStoreFloat3(&xEnd, origin + xDir * axisLen);
            XMStoreFloat3(&yEnd, origin + yDir * axisLen);
            XMStoreFloat3(&zEnd, origin + zDir * axisLen);

            pb3d.DrawLine(oPos, xEnd, { 1.0f, 0.0f, 0.0f, 1.0f });
            pb3d.DrawLine(oPos, yEnd, { 0.0f, 1.0f, 0.0f, 1.0f });
            pb3d.DrawLine(oPos, zEnd, { 0.0f, 0.0f, 1.0f, 1.0f });
        }
        else
        {
            // Default: yellow sphere
            pb3d.DrawWireSphere(jPos, 0.015f, { 1.0f, 1.0f, 0.0f, 1.0f }, 8);
        }

        // Draw bone line to parent (green)
        if (joints[i].parentIndex >= 0 && static_cast<size_t>(joints[i].parentIndex) < globalTransforms.size())
        {
            XMVECTOR parentPosLocal = XMVectorSet(
                globalTransforms[joints[i].parentIndex]._41,
                globalTransforms[joints[i].parentIndex]._42,
                globalTransforms[joints[i].parentIndex]._43,
                1.0f);
            XMVECTOR parentPosWorld = XMVector3Transform(parentPosLocal, worldMatrix);
            XMFLOAT3 pPos;
            XMStoreFloat3(&pPos, parentPosWorld);

            pb3d.DrawLine(pPos, jPos, { 0.0f, 1.0f, 0.0f, 1.0f });
        }
    }
}

// ============================================================================
// Viewport Toolbar
// ============================================================================

void GXModelViewerApp::DrawViewportToolbar(ImVec2 imageMin)
{
    // Draw as an independent overlay window positioned at viewport top-left
    ImGui::SetNextWindowPos(ImVec2(imageMin.x + 4.0f, imageMin.y + 4.0f));
    ImGui::SetNextWindowBgAlpha(0.65f);

    ImGuiWindowFlags overlayFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 2.0f));

    if (ImGui::Begin("##ViewportToolbar", nullptr, overlayFlags))
    {
        ImVec2 btnSize(24.0f, 24.0f);

        // Translate button
        bool isTranslate = (m_gizmoOperation == ImGuizmo::TRANSLATE);
        if (isTranslate)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button("T", btnSize))
            m_gizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (T)");
        if (isTranslate)
            ImGui::PopStyleColor();

        ImGui::SameLine();

        // Rotate button
        bool isRotate = (m_gizmoOperation == ImGuizmo::ROTATE);
        if (isRotate)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button("R", btnSize))
            m_gizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");
        if (isRotate)
            ImGui::PopStyleColor();

        ImGui::SameLine();

        // Scale button
        bool isScale = (m_gizmoOperation == ImGuizmo::SCALE);
        if (isScale)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button("S", btnSize))
            m_gizmoOperation = ImGuizmo::SCALE;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");
        if (isScale)
            ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::Text("|");
        ImGui::SameLine();

        // World/Local toggle
        bool isLocal = (m_gizmoMode == ImGuizmo::LOCAL);
        if (isLocal)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(isLocal ? "L" : "W", btnSize))
            m_gizmoMode = isLocal ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(isLocal ? "Local Space (L)" : "World Space (L)");
        if (isLocal)
            ImGui::PopStyleColor();

        ImGui::SameLine();

        // Snap toggle
        if (m_useSnap)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button("Sn", btnSize))
            m_useSnap = !m_useSnap;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip(m_useSnap ? "Snap: ON" : "Snap: OFF");
        if (m_useSnap)
            ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
}

// ============================================================================
// Viewport Picking
// ============================================================================

bool GXModelViewerApp::ComputeEntityAABB(const SceneEntity& entity, XMFLOAT3& outMin, XMFLOAT3& outMax)
{
    if (!entity.model) return false;

    outMin = { 1e30f, 1e30f, 1e30f };
    outMax = { -1e30f, -1e30f, -1e30f };

    auto updateAABB = [&](float px, float py, float pz) {
        outMin.x = (std::min)(outMin.x, px); outMin.y = (std::min)(outMin.y, py); outMin.z = (std::min)(outMin.z, pz);
        outMax.x = (std::max)(outMax.x, px); outMax.y = (std::max)(outMax.y, py); outMax.z = (std::max)(outMax.z, pz);
    };

    // Skinned model with active animator: CPU skinning for accurate AABB
    if (entity.model->HasSkeleton() && entity.animator)
    {
        const auto& gt = entity.animator->GetGlobalTransforms();
        if (gt.empty()) return false;

        const auto* cpuData = entity.model->GetCPUData();
        if (!cpuData || cpuData->skinnedVertices.empty()) return false;

        // Compute bone matrices (globalTransform * inverseBindMatrix)
        XMFLOAT4X4 boneMatrices[GX::BoneConstants::k_MaxBones];
        entity.model->GetSkeleton()->ComputeBoneMatrices(gt.data(), boneMatrices);

        // CPU skinning: transform each vertex by its weighted bone matrices
        for (const auto& v : cpuData->skinnedVertices)
        {
            XMVECTOR pos = XMLoadFloat3(&v.position);
            XMVECTOR skinned = XMVectorZero();
            for (int i = 0; i < 4; ++i)
            {
                float w = (&v.weights.x)[i];
                if (w <= 0.0f) continue;
                uint32_t bi = (&v.joints.x)[i];
                XMMATRIX bone = XMLoadFloat4x4(&boneMatrices[bi]);
                skinned += XMVector3Transform(pos, bone) * w;
            }
            XMFLOAT3 sp;
            XMStoreFloat3(&sp, skinned);
            updateAABB(sp.x, sp.y, sp.z);
        }
        return true;
    }

    // Static model: use vertex positions
    const auto* cpuData = entity.model->GetCPUData();
    if (!cpuData) return false;

    if (!cpuData->staticVertices.empty())
        for (const auto& v : cpuData->staticVertices)
            updateAABB(v.position.x, v.position.y, v.position.z);
    else if (!cpuData->skinnedVertices.empty())
        for (const auto& v : cpuData->skinnedVertices)
            updateAABB(v.position.x, v.position.y, v.position.z);

    return (outMin.x < outMax.x);
}

void GXModelViewerApp::HandleViewportPicking()
{
    // Only pick on left click in viewport, not when interacting with gizmo or toolbar
    if (!m_viewportHovered) return;
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;
    if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) return;

    // Compute mouse position in viewport-local UV [0,1]
    ImVec2 mousePos = ImGui::GetMousePos();
    float u = (mousePos.x - m_viewportImageMin.x) / m_viewportImageSize.x;
    float v = (mousePos.y - m_viewportImageMin.y) / m_viewportImageSize.y;
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return;

    // Convert to NDC
    float ndcX =  u * 2.0f - 1.0f;
    float ndcY = -(v * 2.0f - 1.0f); // flip Y

    // Unproject near and far points
    XMMATRIX viewProj = m_camera.GetViewMatrix() * m_camera.GetProjectionMatrix();
    XMVECTOR det;
    XMMATRIX vpInverse = XMMatrixInverse(&det, viewProj);

    XMVECTOR nearPt = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 0.0f, 1.0f), vpInverse);
    XMVECTOR farPt  = XMVector3TransformCoord(XMVectorSet(ndcX, ndcY, 1.0f, 1.0f), vpInverse);

    XMFLOAT3 rayOrigin, rayDir3;
    XMStoreFloat3(&rayOrigin, nearPt);
    XMVECTOR dir = XMVector3Normalize(farPt - nearPt);
    XMStoreFloat3(&rayDir3, dir);

    GX::Ray ray(GX::Vector3(rayOrigin.x, rayOrigin.y, rayOrigin.z),
                GX::Vector3(rayDir3.x, rayDir3.y, rayDir3.z));

    int bestEntity = -1;
    float bestT = 1e30f;

    for (int ei = 0; ei < m_sceneGraph.GetEntityCount(); ++ei)
    {
        SceneEntity* entity = m_sceneGraph.GetEntity(ei);
        if (!entity || !entity->model || !entity->visible) continue;

        // Compute local AABB
        XMFLOAT3 localMin, localMax;
        if (!ComputeEntityAABB(*entity, localMin, localMax)) continue;

        // Transform ray into entity's local space (= OBB test without inflation)
        XMMATRIX worldMat = entity->transform.GetWorldMatrix();
        XMVECTOR det2;
        XMMATRIX invWorld = XMMatrixInverse(&det2, worldMat);

        // Transform ray origin as point, direction as vector
        XMVECTOR localOrigin = XMVector3TransformCoord(nearPt, invWorld);
        XMVECTOR localFar    = XMVector3TransformCoord(farPt, invWorld);
        XMVECTOR localDir    = XMVector3Normalize(localFar - localOrigin);

        XMFLOAT3 lo, ld;
        XMStoreFloat3(&lo, localOrigin);
        XMStoreFloat3(&ld, localDir);

        GX::Ray localRay(GX::Vector3(lo.x, lo.y, lo.z),
                         GX::Vector3(ld.x, ld.y, ld.z));
        GX::AABB3D localAABB(
            GX::Vector3(localMin.x, localMin.y, localMin.z),
            GX::Vector3(localMax.x, localMax.y, localMax.z));

        float hitT = 0.0f;
        if (GX::Collision3D::RaycastAABB(localRay, localAABB, hitT))
        {
            // Convert local hit distance to world distance for comparison
            XMVECTOR localHitPt = localOrigin + localDir * hitT;
            XMVECTOR worldHitPt = XMVector3TransformCoord(localHitPt, worldMat);
            float worldDist = XMVectorGetX(XMVector3Length(worldHitPt - nearPt));
            if (worldDist < bestT)
            {
                bestT = worldDist;
                bestEntity = ei;
            }
        }
    }

    m_sceneGraph.selectedEntity = bestEntity;
}

// ============================================================================
// Shutdown
// ============================================================================

void GXModelViewerApp::Shutdown()
{
    m_commandQueue.Flush();
    ShutdownImGui();
    m_app.Shutdown();

#ifdef _DEBUG
    GX::GraphicsDevice::ReportLiveObjects();
#endif
}
