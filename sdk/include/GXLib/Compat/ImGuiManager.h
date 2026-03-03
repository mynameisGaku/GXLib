#pragma once
/// @file ImGuiManager.h
/// @brief ImGuiライフサイクル管理
///
/// CompatAPI と GameFramework の両方で使用する ImGui の
/// 初期化・フレーム制御・描画・破棄を管理するラッパー。

#include "Graphics/Device/DescriptorHeap.h"

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList;

namespace gx
{
class Window;

/// @brief ImGui の初期化・フレーム・描画・終了を管理する
class ImGuiManager
{
public:
    /// @brief ImGui を初期化する
    /// @param device D3D12 デバイス
    /// @param queue コマンドキュー
    /// @param window ウィンドウ（HWND取得用）
    /// @return 成功時 true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue, Window& window);

    /// @brief フレーム開始（NewFrame）
    void BeginFrame();

    /// @brief フレーム終了（Render + RenderDrawData）
    /// @param cmdList 描画コマンドリスト
    void EndFrame(ID3D12GraphicsCommandList* cmdList);

    /// @brief ImGui を終了する
    void Shutdown();

    /// @brief 初期化済みかどうか
    bool IsInitialized() const { return m_initialized; }

private:
    DescriptorHeap m_srvHeap;       ///< ImGui 専用 shader-visible SRV ヒープ
    bool           m_initialized = false;
};

} // namespace gx
