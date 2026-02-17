/// @file GXModelViewerApp.cpp
/// @brief GXModelViewer application implementation

#include "GXModelViewerApp.h"
#include "ModelExporter.h"
#include "Scene/SceneSerializer.h"
#include "Core/Logger.h"
#include "Graphics/3D/Transform3D.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "implot.h"
#include "imnodes.h"
#include "ImGuiFileDialog.h"

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

    // Disable shadows for the viewer (no shadow pass)
    m_renderer3D.SetShadowEnabled(false);

    // PostEffectPipeline (HDR -> tonemap -> backbuffer)
    if (!m_postEffect.Initialize(device, width, height))
        return false;
    m_postEffect.SetTonemapMode(GX::TonemapMode::ACES);
    m_postEffect.SetFXAAEnabled(true);

    // Set up skybox
    m_renderer3D.GetSkybox().SetColors({ 0.4f, 0.55f, 0.8f }, { 0.7f, 0.75f, 0.85f });
    m_renderer3D.GetSkybox().SetSun({ 0.3f, -1.0f, 0.5f }, 3.0f);

    // Create a ground plane grid mesh
    m_gridMesh = m_renderer3D.CreateGPUMesh(GX::MeshGenerator::CreatePlane(20.0f, 20.0f, 20, 20));
    m_gridMaterial.constants.albedoFactor = { 0.35f, 0.35f, 0.37f, 1.0f };
    m_gridMaterial.constants.roughnessFactor = 0.9f;
    m_gridMaterial.constants.metallicFactor  = 0.0f;
    m_gridTransform.SetPosition(0.0f, 0.0f, 0.0f);

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
    // Note: Docking API is not available in this ImGui version (master branch).
    // Windows can be freely positioned and resized by the user.
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
            ImGui::MenuItem("Performance", nullptr, &m_showPerformance);
            ImGui::MenuItem("Log", nullptr, &m_showLog);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemoWindow);
            if (ImGui::MenuItem("Reset Camera"))
            {
                m_orbitYaw      = 0.0f;
                m_orbitPitch    = 0.5f;
                m_orbitDistance  = 8.0f;
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
    BuildMainMenuBar();
    BuildDockSpace();

    // Show ImGui demo window for reference
    if (m_showDemoWindow)
        ImGui::ShowDemoWindow(&m_showDemoWindow);

    // --- Editor Panels ---

    // Scene Hierarchy
    if (m_showSceneHierarchy)
        m_sceneHierarchyPanel.Draw(m_sceneGraph);

    // Properties
    if (m_showProperties)
        m_propertyPanel.Draw(m_sceneGraph, m_renderer3D.GetMaterialManager(), m_renderer3D.GetTextureManager());

    // Lighting
    if (m_showLighting)
        m_lightingPanel.Draw(m_renderer3D);

    // Post Effects
    if (m_showPostEffects)
        m_postEffectPanel.Draw(m_postEffect);

    // Skybox
    if (m_showSkybox)
        m_skyboxPanel.Draw(m_renderer3D.GetSkybox());

    // Terrain
    if (m_showTerrain)
        m_terrainPanel.Draw();

    // Timeline (no animator assigned yet)
    if (m_showTimeline)
        m_timelinePanel.Draw(nullptr, m_app.GetTimer().GetDeltaTime());

    // Animator (no state machine assigned yet)
    if (m_showAnimator)
        m_animatorPanel.Draw(nullptr);

    // Blend Tree (no blend tree assigned yet)
    if (m_showBlendTree)
        m_blendTreeEditor.Draw(nullptr);

    // Texture Browser
    if (m_showTextureBrowser)
        m_textureBrowser.Draw(m_textureManager);

    // Performance
    if (m_showPerformance)
        m_performancePanel.Draw(m_app.GetTimer().GetDeltaTime(), m_app.GetTimer().GetFPS());

    // Log
    if (m_showLog)
        m_logPanel.Draw();

    // Scene info panel (camera orbit controls)
    if (ImGui::Begin("Camera"))
    {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)",
            m_camera.GetPosition().x, m_camera.GetPosition().y, m_camera.GetPosition().z);
        if (ImGui::SliderFloat("Distance", &m_orbitDistance, 1.0f, 50.0f))
            UpdateOrbitCamera();
        if (ImGui::SliderFloat("Yaw", &m_orbitYaw, -XM_PI, XM_PI))
            UpdateOrbitCamera();
        if (ImGui::SliderFloat("Pitch", &m_orbitPitch, -XM_PIDIV2 + 0.01f, XM_PIDIV2 - 0.01f))
            UpdateOrbitCamera();
    }
    ImGui::End();

    // Handle orbit camera with mouse input (right-drag to rotate, scroll to zoom)
    HandleOrbitInput();

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

    // カメラ自動フィット: モデルAABBからオービット距離を計算
    const auto* cpuData = entity->model->GetCPUData();
    if (cpuData)
    {
        XMFLOAT3 aabbMin = { 1e30f, 1e30f, 1e30f };
        XMFLOAT3 aabbMax = { -1e30f, -1e30f, -1e30f };

        auto updateAABB = [&](float px, float py, float pz) {
            aabbMin.x = (std::min)(aabbMin.x, px);
            aabbMin.y = (std::min)(aabbMin.y, py);
            aabbMin.z = (std::min)(aabbMin.z, pz);
            aabbMax.x = (std::max)(aabbMax.x, px);
            aabbMax.y = (std::max)(aabbMax.y, py);
            aabbMax.z = (std::max)(aabbMax.z, pz);
        };

        if (!cpuData->skinnedVertices.empty())
        {
            for (const auto& v : cpuData->skinnedVertices)
                updateAABB(v.position.x, v.position.y, v.position.z);
        }
        else if (!cpuData->staticVertices.empty())
        {
            for (const auto& v : cpuData->staticVertices)
                updateAABB(v.position.x, v.position.y, v.position.z);
        }

        if (aabbMin.x < aabbMax.x) // valid AABB
        {
            m_orbitTarget = {
                (aabbMin.x + aabbMax.x) * 0.5f,
                (aabbMin.y + aabbMax.y) * 0.5f,
                (aabbMin.z + aabbMax.z) * 0.5f
            };
            float extentX = aabbMax.x - aabbMin.x;
            float extentY = aabbMax.y - aabbMin.y;
            float extentZ = aabbMax.z - aabbMin.z;
            float maxExtent = (std::max)({extentX, extentY, extentZ});
            m_orbitDistance = maxExtent * 1.5f;
            if (m_orbitDistance < 1.0f) m_orbitDistance = 5.0f;
            UpdateOrbitCamera();
        }
    }

    GX_LOG_INFO("Imported model: %s (%u submeshes)", entityName.c_str(),
                entity->model->GetSubMeshCount());
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
    if (ModelExporter::ExportToGxmd(*entity, m_renderer3D.GetMaterialManager(), outputPath))
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
    m_camera.SetTarget(m_orbitTarget);
}

void GXModelViewerApp::HandleOrbitInput()
{
    ImGuiIO& io = ImGui::GetIO();

    // Only process orbit input when ImGui doesn't want the mouse
    if (io.WantCaptureMouse)
        return;

    // Right-drag to orbit
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        ImVec2 delta = io.MouseDelta;
        m_orbitYaw   -= delta.x * 0.01f;
        m_orbitPitch += delta.y * 0.01f;

        // Clamp pitch to avoid gimbal lock
        m_orbitPitch = (std::max)(-XM_PIDIV2 + 0.01f, (std::min)(m_orbitPitch, XM_PIDIV2 - 0.01f));
        UpdateOrbitCamera();
    }

    // Middle-drag to pan
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
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

    // Scroll to zoom
    if (io.MouseWheel != 0.0f)
    {
        m_orbitDistance -= io.MouseWheel * m_orbitDistance * 0.15f;
        m_orbitDistance = (std::max)(0.5f, (std::min)(m_orbitDistance, 200.0f));
        UpdateOrbitCamera();
    }
}

// ============================================================================
// Rendering
// ============================================================================

void GXModelViewerApp::RenderFrame(float deltaTime)
{
    m_totalTime += deltaTime;

    m_frameIndex = m_swapChain.GetCurrentBackBufferIndex();
    m_commandQueue.GetFence().WaitForValue(m_frameFenceValues[m_frameIndex]);
    m_commandList.Reset(m_frameIndex, nullptr);
    auto* cmdList = m_commandList.Get();

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
    m_renderer3D.SetMaterial(m_gridMaterial);
    m_renderer3D.DrawMesh(m_gridMesh, m_gridTransform);

    // Draw scene entities (imported models)
    for (const auto& entity : m_sceneGraph.GetEntities())
    {
        if (entity.model && entity.visible)
            m_renderer3D.DrawModel(*entity.model, entity.transform);
    }

    m_renderer3D.End();

    // --- PostEffect EndScene ---
    m_postEffect.EndScene();

    // --- Transition backbuffer: PRESENT -> RENDER_TARGET ---
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_swapChain.GetCurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // --- PostEffect Resolve: HDR -> tonemap/FXAA -> backbuffer ---
    auto rtvHandle = m_swapChain.GetCurrentRTVHandle();
    m_postEffect.Resolve(rtvHandle, m_renderer3D.GetDepthBuffer(), m_camera, deltaTime);

    // --- Render ImGui on top of the tonemapped backbuffer ---
    // Re-bind backbuffer as render target for ImGui overlay
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

    m_commandQueue.Flush();
    m_swapChain.Resize(m_graphicsDevice.GetDevice(), width, height);
    m_renderer3D.OnResize(width, height);
    m_postEffect.OnResize(m_graphicsDevice.GetDevice(), width, height);
    m_camera.SetPerspective(m_camera.GetFovY(), static_cast<float>(width) / height,
                             m_camera.GetNearZ(), m_camera.GetFarZ());
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

        // Render 3D scene + ImGui
        RenderFrame(dt);
    }

    // Wait for GPU to finish before cleanup
    m_commandQueue.Flush();
    return 0;
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
