/// @file test_GPUDebugLayer.cpp
/// @brief GPUDebugLayerのテスト — HRESULT変換、DREDデフォルト値、ステート

#include <gtest/gtest.h>
#include "Graphics/Device/GPUDebugLayer.h"

using namespace gx;

// ============================================================================
// TranslateDeviceRemovedReason
// ============================================================================

TEST(GPUDebugLayerTest, TranslateDeviceHung)
{
    std::string msg = GPUDebugLayer::TranslateDeviceRemovedReason(DXGI_ERROR_DEVICE_HUNG);
    EXPECT_FALSE(msg.empty());
    // "HUNG"を含むはず（HRESULT名の一部）
    EXPECT_NE(msg.find("HUNG"), std::string::npos);
}

TEST(GPUDebugLayerTest, TranslateDeviceRemoved)
{
    std::string msg = GPUDebugLayer::TranslateDeviceRemovedReason(DXGI_ERROR_DEVICE_REMOVED);
    EXPECT_FALSE(msg.empty());
}

TEST(GPUDebugLayerTest, TranslateOK)
{
    std::string msg = GPUDebugLayer::TranslateDeviceRemovedReason(S_OK);
    EXPECT_FALSE(msg.empty());
}

TEST(GPUDebugLayerTest, TranslateUnknown)
{
    std::string msg = GPUDebugLayer::TranslateDeviceRemovedReason(E_FAIL);
    EXPECT_FALSE(msg.empty());
}

// ============================================================================
// DREDレポートのデフォルト
// ============================================================================

TEST(GPUDebugLayerTest, DREDReportNullDevice)
{
    DREDReport report = GPUDebugLayer::GetDREDReport(nullptr);
    EXPECT_FALSE(report.available);
}

// ============================================================================
// nullデバイスでのConfigureInfoQueue
// ============================================================================

TEST(GPUDebugLayerTest, ConfigureInfoQueueNullDevice)
{
    // nullデバイスでクラッシュしないこと
    GPUDebugLayer::ConfigureInfoQueue(nullptr, true, true);
}
