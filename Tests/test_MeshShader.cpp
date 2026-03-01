/// @file test_MeshShader.cpp
/// @brief MeshPipelineとGPUDrivenRendererの単体テスト（GPU不要）

#include <gtest/gtest.h>
#include "Graphics/Pipeline/MeshPipeline.h"
#include "Graphics/3D/GPUDrivenRenderer.h"

using namespace gx;

// ============================================================================
// MeshShaderTier列挙型テスト
// ============================================================================

TEST(MeshPipeline, MeshShaderTierEnumValues)
{
    EXPECT_EQ(static_cast<uint32_t>(MeshShaderTier::None), 0u);
    EXPECT_EQ(static_cast<uint32_t>(MeshShaderTier::Tier1), 1u);
}

// ============================================================================
// MeshPipelineテスト
// ============================================================================

TEST(MeshPipeline, DetectTierWithoutDevice)
{
    MeshShaderTier tier = MeshPipeline::DetectTier(nullptr);
    EXPECT_EQ(tier, MeshShaderTier::None);
}

TEST(MeshPipeline, MeshPipelineDescDefaults)
{
    MeshPipelineDesc desc;
    EXPECT_EQ(desc.asBlob, nullptr);
    EXPECT_EQ(desc.asBlobSize, 0u);
    EXPECT_EQ(desc.msBlob, nullptr);
    EXPECT_EQ(desc.msBlobSize, 0u);
    EXPECT_EQ(desc.psBlob, nullptr);
    EXPECT_EQ(desc.psBlobSize, 0u);
    EXPECT_EQ(desc.rootSignature, nullptr);
    EXPECT_EQ(desc.rtvFormat, DXGI_FORMAT_R16G16B16A16_FLOAT);
    EXPECT_EQ(desc.dsvFormat, DXGI_FORMAT_D32_FLOAT);
    EXPECT_EQ(desc.numRenderTargets, 1u);
}

// ============================================================================
// GPUDrivenRendererテスト
// ============================================================================

TEST(GPUDrivenRenderer, NotAvailableByDefault)
{
    GPUDrivenRenderer renderer;
    EXPECT_FALSE(renderer.IsAvailable());
}

TEST(GPUDrivenRenderer, InitializeWithoutDeviceReturnsFalse)
{
    GPUDrivenRenderer renderer;
    EXPECT_FALSE(renderer.Initialize(nullptr, nullptr));
}

TEST(GPUDrivenRenderer, MeshletCountDefaultIsZero)
{
    GPUDrivenRenderer renderer;
    EXPECT_EQ(renderer.GetMeshletCount(), 0u);
}

// ============================================================================
// MeshletData構造体テスト
// ============================================================================

TEST(MeshletData, StructLayout)
{
    MeshletData data{};
    data.vertexCount     = 64;
    data.vertexOffset    = 100;
    data.primitiveCount  = 126;
    data.primitiveOffset = 200;

    EXPECT_EQ(data.vertexCount, 64u);
    EXPECT_EQ(data.vertexOffset, 100u);
    EXPECT_EQ(data.primitiveCount, 126u);
    EXPECT_EQ(data.primitiveOffset, 200u);
}
