/// @file test_DX12Advanced.cpp
/// @brief DirectStorage、SamplerFeedback、HDRDisplayManagerの単体テスト（GPU不要）

#include <gtest/gtest.h>
#include "IO/DirectStorageManager.h"
#include "Graphics/Resource/SamplerFeedback.h"
#include "Graphics/Device/HDRDisplayManager.h"

using namespace gx;

// ============================================================================
// DirectStorageManagerテスト
// ============================================================================

TEST(DirectStorage, NotAvailableByDefault)
{
    DirectStorageManager mgr;
    EXPECT_FALSE(mgr.IsAvailable());
}

TEST(DirectStorage, PendingCountStartsAtZero)
{
    DirectStorageManager mgr;
    EXPECT_EQ(mgr.GetPendingRequestCount(), 0u);
}

TEST(DirectStorage, BandwidthStartsAtZero)
{
    DirectStorageManager mgr;
    EXPECT_FLOAT_EQ(mgr.GetBandwidthMBps(), 0.0f);
}

TEST(DirectStorage, EnqueueReadReturnsIncrementingIds)
{
    DirectStorageManager mgr;
    DirectStorageRequest req;
    req.filePath = L"test.bin";
    req.size = 100;

    uint32_t id1 = mgr.EnqueueRead(req);
    uint32_t id2 = mgr.EnqueueRead(req);
    EXPECT_GT(id1, 0u);
    EXPECT_GT(id2, id1);
    EXPECT_EQ(mgr.GetPendingRequestCount(), 2u);
}

TEST(DirectStorage, FlushClearsPendingRequests)
{
    DirectStorageManager mgr;
    DirectStorageRequest req;
    req.filePath = L"nonexistent_test_file.bin";
    req.size = 0;
    req.destination = nullptr;

    mgr.EnqueueRead(req);
    EXPECT_EQ(mgr.GetPendingRequestCount(), 1u);

    mgr.Flush();
    EXPECT_EQ(mgr.GetPendingRequestCount(), 0u);
}

TEST(DirectStorage, StatusEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(DirectStorageStatus::Pending), 0u);
    EXPECT_EQ(static_cast<uint32_t>(DirectStorageStatus::InProgress), 1u);
    EXPECT_EQ(static_cast<uint32_t>(DirectStorageStatus::Completed), 2u);
    EXPECT_EQ(static_cast<uint32_t>(DirectStorageStatus::Failed), 3u);
}

// ============================================================================
// SamplerFeedbackテスト
// ============================================================================

TEST(SamplerFeedback, DefaultTierIsNone)
{
    SamplerFeedback fb;
    EXPECT_EQ(fb.GetSupportedTier(), SamplerFeedbackTier::None);
}

TEST(SamplerFeedback, NotAvailableByDefault)
{
    SamplerFeedback fb;
    EXPECT_FALSE(fb.IsAvailable());
}

TEST(SamplerFeedback, InitializeWithoutDeviceReturnsFalse)
{
    SamplerFeedback fb;
    EXPECT_FALSE(fb.Initialize(nullptr, 16));
}

TEST(SamplerFeedback, EmptyMipRequestsByDefault)
{
    SamplerFeedback fb;
    EXPECT_TRUE(fb.GetMipRequests().empty());
}

TEST(SamplerFeedback, TierEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(SamplerFeedbackTier::None), 0u);
    EXPECT_EQ(static_cast<uint32_t>(SamplerFeedbackTier::Tier0_9), 1u);
    EXPECT_EQ(static_cast<uint32_t>(SamplerFeedbackTier::Tier1_0), 2u);
}

TEST(SamplerFeedback, TrackedCountDefaultIsZero)
{
    SamplerFeedback fb;
    EXPECT_EQ(fb.GetTrackedTextureCount(), 0u);
}

// ============================================================================
// HDRDisplayManagerテスト
// ============================================================================

TEST(HDRDisplay, DefaultNotSupported)
{
    HDRDisplayManager mgr;
    EXPECT_FALSE(mgr.IsHDRSupported());
}

TEST(HDRDisplay, DefaultModeIsSDR)
{
    HDRDisplayManager mgr;
    EXPECT_EQ(mgr.GetActiveMode(), HDRMode::SDR);
}

TEST(HDRDisplay, HDRModeEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(HDRMode::SDR), 0u);
    EXPECT_EQ(static_cast<uint32_t>(HDRMode::HDR10), 1u);
    EXPECT_EQ(static_cast<uint32_t>(HDRMode::scRGB), 2u);
}

TEST(HDRDisplay, DefaultLuminanceValues)
{
    HDRDisplayManager mgr;
    EXPECT_FLOAT_EQ(mgr.GetMaxLuminance(), 80.0f);
    EXPECT_FLOAT_EQ(mgr.GetMinLuminance(), 0.0f);
}

TEST(HDRDisplay, RecommendedFormatForSDR)
{
    HDRDisplayManager mgr;
    // 初期化なしではHDR非対応のため、SDRフォーマットが期待される
    EXPECT_EQ(mgr.GetRecommendedFormat(), DXGI_FORMAT_R8G8B8A8_UNORM);
}

TEST(HDRDisplay, RecommendedColorSpaceForSDR)
{
    HDRDisplayManager mgr;
    // 初期化なしではsRGB色空間を返すはず
    EXPECT_EQ(mgr.GetRecommendedColorSpace(), DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
}
