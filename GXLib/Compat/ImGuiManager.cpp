/// @file ImGuiManager.cpp
/// @brief ImGuiライフサイクル管理の実装
#include "pch.h"
#include "Compat/ImGuiManager.h"
#include "Core/Window.h"
#include "Core/Logger.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

// imgui_impl_win32.cpp で定義される外部関数
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace gx
{

bool ImGuiManager::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue, Window& window)
{
    if (!device || !queue) return false;

    // ImGui 専用 shader-visible SRV ヒープ（64 エントリ）
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64, true))
    {
        GX_LOG_ERROR("ImGuiManager: Failed to create SRV heap");
        return false;
    }

    // ImGui コンテキスト作成
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // スタイル調整
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 4.0f;
    style.FrameRounding    = 2.0f;
    style.GrabRounding     = 2.0f;
    style.ScrollbarRounding = 3.0f;

    // Win32 バックエンド初期化
    ImGui_ImplWin32_Init(window.GetHWND());

    // DX12 バックエンド初期化
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device               = device;
    initInfo.CommandQueue          = queue;
    initInfo.NumFramesInFlight     = 2;  // SwapChain::k_BufferCount
    initInfo.RTVFormat             = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.SrvDescriptorHeap     = m_srvHeap.GetHeap();
    initInfo.UserData              = &m_srvHeap;

    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
                                       D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu,
                                       D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
    {
        auto* heap = static_cast<DescriptorHeap*>(info->UserData);
        uint32_t idx = heap->AllocateIndex();
        *out_cpu = heap->GetCPUHandle(idx);
        *out_gpu = heap->GetGPUHandle(idx);
    };

    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info,
                                      D3D12_CPU_DESCRIPTOR_HANDLE cpu,
                                      D3D12_GPU_DESCRIPTOR_HANDLE gpu)
    {
        (void)info; (void)cpu; (void)gpu;
    };

    ImGui_ImplDX12_Init(&initInfo);

    // Window にメッセージコールバックを登録して ImGui にイベントを転送
    window.AddMessageCallback([](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> bool {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        return false;  // GXLib 側でも処理を継続
    });

    m_initialized = true;
    GX_LOG_INFO("ImGuiManager: Initialized");
    return true;
}

void ImGuiManager::BeginFrame()
{
    if (!m_initialized) return;
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* cmdList)
{
    if (!m_initialized || !cmdList) return;
    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void ImGuiManager::Shutdown()
{
    if (!m_initialized) return;

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_initialized = false;
    GX_LOG_INFO("ImGuiManager: Shutdown");
}

} // namespace gx
