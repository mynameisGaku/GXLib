#include "pch_graphics.h"
/// @file GPUDebugLayer.cpp
/// @brief D3D12デバッグレイヤーユーティリティの実装

#include "Graphics/Device/GPUDebugLayer.h"
#include "Core/Logger.h"
#include <dxgidebug.h>

namespace gx
{

bool GPUDebugLayer::s_enabled = false;

// ============================================================================
// Enable
// ============================================================================

bool GPUDebugLayer::Enable(bool gpuValidation)
{
    // D3D12デバッグレイヤーを有効化する（デバイス作成前に行う必要がある）
    ComPtr<ID3D12Debug> debugController;
    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    if (FAILED(hr))
    {
        GX_LOG_WARN("GPUDebugLayer::Enable: D3D12GetDebugInterface failed (HRESULT: 0x%08X)", hr);
        return false;
    }

    debugController->EnableDebugLayer();
    GX_LOG_INFO("GPUDebugLayer: D3D12 debug layer enabled");

    // オプションでGPUベースバリデーションを有効化する（大幅に低速だがより多くのエラーを検出）
    if (gpuValidation)
    {
        ComPtr<ID3D12Debug1> debug1;
        hr = debugController.As(&debug1);
        if (SUCCEEDED(hr))
        {
            debug1->SetEnableGPUBasedValidation(TRUE);
            GX_LOG_INFO("GPUDebugLayer: GPU-based validation enabled (performance will be reduced)");
        }
        else
        {
            GX_LOG_WARN("GPUDebugLayer: ID3D12Debug1 not available, GPU-based validation skipped");
        }
    }

    // DRED (Device Removed Extended Data) の自動ブレッドクラムとページフォルト報告を有効化
    ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings));
    if (SUCCEEDED(hr))
    {
        dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        GX_LOG_INFO("GPUDebugLayer: DRED auto-breadcrumbs and page fault reporting enabled");
    }
    else
    {
        GX_LOG_WARN("GPUDebugLayer: DRED settings not available (older Windows SDK)");
    }

    s_enabled = true;
    return true;
}

// ============================================================================
// ConfigureInfoQueue
// ============================================================================

void GPUDebugLayer::ConfigureInfoQueue(ID3D12Device* device,
                                        bool breakOnError,
                                        bool breakOnCorruption)
{
    if (!device)
        return;

    ComPtr<ID3D12InfoQueue> infoQueue;
    HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&infoQueue));
    if (FAILED(hr))
    {
        GX_LOG_WARN("GPUDebugLayer::ConfigureInfoQueue: ID3D12InfoQueue not available");
        return;
    }

    if (breakOnCorruption)
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    }

    if (breakOnError)
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }

    // 一般的な無害な警告を抑制
    D3D12_MESSAGE_ID denyIds[] = {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
    };

    D3D12_MESSAGE_SEVERITY denySeverities[] = {
        D3D12_MESSAGE_SEVERITY_INFO,
    };

    D3D12_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.NumIDs = _countof(denyIds);
    filter.DenyList.pIDList = denyIds;
    filter.DenyList.NumSeverities = _countof(denySeverities);
    filter.DenyList.pSeverityList = denySeverities;
    infoQueue->PushStorageFilter(&filter);

    GX_LOG_INFO("GPUDebugLayer: InfoQueue configured (breakOnError=%s, breakOnCorruption=%s)",
                breakOnError ? "true" : "false",
                breakOnCorruption ? "true" : "false");
}

// ============================================================================
// GetDREDReport
// ============================================================================

DREDReport GPUDebugLayer::GetDREDReport(ID3D12Device* device)
{
    DREDReport report;
    if (!device)
        return report;

    ComPtr<ID3D12DeviceRemovedExtendedData> dred;
    HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&dred));
    if (FAILED(hr))
    {
        GX_LOG_WARN("GPUDebugLayer::GetDREDReport: ID3D12DeviceRemovedExtendedData not available");
        return report;
    }

    // 自動ブレッドクラムを取得
    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbsOutput = {};
    hr = dred->GetAutoBreadcrumbsOutput(&breadcrumbsOutput);
    if (SUCCEEDED(hr) && breadcrumbsOutput.pHeadAutoBreadcrumbNode)
    {
        report.available = true;
        gx::String breadcrumbText;

        const D3D12_AUTO_BREADCRUMB_NODE* node = breadcrumbsOutput.pHeadAutoBreadcrumbNode;
        while (node)
        {
            // コマンドリスト名
            if (node->pCommandListDebugNameA)
            {
                breadcrumbText += "CommandList: ";
                breadcrumbText += node->pCommandListDebugNameA;
                breadcrumbText += "\n";
            }
            else if (node->pCommandListDebugNameW)
            {
                breadcrumbText += "CommandList: <wide string>\n";
            }

            // コマンドキュー名
            if (node->pCommandQueueDebugNameA)
            {
                breadcrumbText += "  Queue: ";
                breadcrumbText += node->pCommandQueueDebugNameA;
                breadcrumbText += "\n";
            }

            // ブレッドクラム操作
            if (node->pCommandHistory && node->BreadcrumbCount > 0)
            {
                uint32_t lastCompleted = (node->pLastBreadcrumbValue) ? *node->pLastBreadcrumbValue : 0;
                breadcrumbText += ("  Breadcrumbs (last completed: " + std::to_string(lastCompleted)
                               + " / " + std::to_string(node->BreadcrumbCount) + "):\n").c_str();

                for (uint32_t i = 0; i < node->BreadcrumbCount; ++i)
                {
                    const char* opName = "Unknown";
                    switch (node->pCommandHistory[i])
                    {
                    case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:        opName = "SetMarker"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:       opName = "BeginEvent"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:         opName = "EndEvent"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:    opName = "DrawInstanced"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED: opName = "DrawIndexedInstanced"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:  opName = "ExecuteIndirect"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:         opName = "Dispatch"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION: opName = "CopyBufferRegion"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION: opName = "CopyTextureRegion"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:     opName = "CopyResource"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE: opName = "ResolveSubresource"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW: opName = "ClearRTV"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW: opName = "ClearUAV"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW: opName = "ClearDSV"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:  opName = "ResourceBarrier"; break;
                    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:    opName = "ExecuteBundle"; break;
                    default: opName = "Other"; break;
                    }

                    char marker = (i < lastCompleted) ? '+' : (i == lastCompleted) ? '>' : ' ';
                    breadcrumbText += "    [";
                    breadcrumbText += marker;
                    breadcrumbText += "] ";
                    breadcrumbText += std::to_string(i).c_str();
                    breadcrumbText += ": ";
                    breadcrumbText += opName;
                    breadcrumbText += "\n";
                }
            }

            node = node->pNext;
        }

        report.breadcrumbs = breadcrumbText;
    }
    else
    {
        report.breadcrumbs = "(no auto-breadcrumb data available)";
    }

    // ページフォルト情報を取得
    D3D12_DRED_PAGE_FAULT_OUTPUT pageFaultOutput = {};
    hr = dred->GetPageFaultAllocationOutput(&pageFaultOutput);
    if (SUCCEEDED(hr))
    {
        report.available = true;
        gx::String pageFaultText;

        pageFaultText += ("Page Fault VA: 0x" +
            std::format("{:016X}", pageFaultOutput.PageFaultVA) + "\n").c_str();

        // アロケーションリストを走査して関連リソースを特定する
        const D3D12_DRED_ALLOCATION_NODE* allocNode = pageFaultOutput.pHeadExistingAllocationNode;
        if (allocNode)
        {
            pageFaultText += "Existing allocations near fault:\n";
            uint32_t count = 0;
            while (allocNode && count < 16)
            {
                if (allocNode->ObjectNameA)
                {
                    pageFaultText += ("  [" + std::to_string(count) + "] ").c_str();
                    pageFaultText += allocNode->ObjectNameA;
                    pageFaultText += "\n";
                }
                else if (allocNode->ObjectNameW)
                {
                    pageFaultText += ("  [" + std::to_string(count) + "] <wide name>\n").c_str();
                }
                else
                {
                    pageFaultText += ("  [" + std::to_string(count) + "] <unnamed>\n").c_str();
                }
                allocNode = allocNode->pNext;
                count++;
            }
        }

        const D3D12_DRED_ALLOCATION_NODE* freedNode = pageFaultOutput.pHeadRecentFreedAllocationNode;
        if (freedNode)
        {
            pageFaultText += "Recently freed allocations:\n";
            uint32_t count = 0;
            while (freedNode && count < 16)
            {
                if (freedNode->ObjectNameA)
                {
                    pageFaultText += ("  [" + std::to_string(count) + "] ").c_str();
                    pageFaultText += freedNode->ObjectNameA;
                    pageFaultText += "\n";
                }
                else if (freedNode->ObjectNameW)
                {
                    pageFaultText += ("  [" + std::to_string(count) + "] <wide name>\n").c_str();
                }
                else
                {
                    pageFaultText += ("  [" + std::to_string(count) + "] <unnamed>\n").c_str();
                }
                freedNode = freedNode->pNext;
                count++;
            }
        }

        report.pageFaultInfo = pageFaultText;
    }
    else
    {
        report.pageFaultInfo = "(no page fault data available)";
    }

    return report;
}

// ============================================================================
// ReportLiveObjects
// ============================================================================

void GPUDebugLayer::ReportLiveObjects()
{
    ComPtr<IDXGIDebug1> dxgiDebug;
    HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
    if (SUCCEEDED(hr))
    {
        GX_LOG_INFO("=== GPUDebugLayer: Live Objects Report ===");
        dxgiDebug->ReportLiveObjects(
            DXGI_DEBUG_ALL,
            static_cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        GX_LOG_INFO("=== End Live Objects Report ===");
    }
    else
    {
        GX_LOG_WARN("GPUDebugLayer::ReportLiveObjects: DXGIGetDebugInterface1 failed");
    }
}

// ============================================================================
// TranslateDeviceRemovedReason
// ============================================================================

gx::String GPUDebugLayer::TranslateDeviceRemovedReason(HRESULT hr)
{
    switch (hr)
    {
    case S_OK:
        return "S_OK: Device is functioning normally (not removed)";
    case DXGI_ERROR_DEVICE_HUNG:
        return "DXGI_ERROR_DEVICE_HUNG: The GPU took too long to execute commands. "
               "This may indicate an infinite loop in a shader or an excessively long draw call.";
    case DXGI_ERROR_DEVICE_REMOVED:
        return "DXGI_ERROR_DEVICE_REMOVED: The GPU has been physically removed, disabled, "
               "or a driver upgrade occurred while the application was running.";
    case DXGI_ERROR_DEVICE_RESET:
        return "DXGI_ERROR_DEVICE_RESET: The GPU stopped working due to a badly formed command. "
               "This is a design-time issue that should be investigated and fixed.";
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
        return "DXGI_ERROR_DRIVER_INTERNAL_ERROR: The GPU driver encountered an internal error "
               "and reset the device. This is typically a driver bug.";
    case DXGI_ERROR_INVALID_CALL:
        return "DXGI_ERROR_INVALID_CALL: The application provided invalid parameter data. "
               "Enable the debug layer for more details.";
    default:
        return "Unknown device removed reason (HRESULT: 0x" +
               std::format("{:08X}", static_cast<uint32_t>(hr)) + ")";
    }
}

} // namespace gx
