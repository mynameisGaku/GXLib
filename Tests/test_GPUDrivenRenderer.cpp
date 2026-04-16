/// @file test_GPUDrivenRenderer.cpp
/// @brief GPU-Driven Renderer completion tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/GPUDrivenRenderer.h"
#include "Math/MathConvert.h"

using namespace gx;

TEST(GPUDrivenRendererTest, DefaultState)
{
    GPUDrivenRenderer renderer;
    EXPECT_FALSE(renderer.IsAvailable());
    EXPECT_EQ(renderer.GetMeshletCount(), 0u);
    EXPECT_FALSE(renderer.IsIndirectReady());
    EXPECT_EQ(renderer.GetVisibleCount(), 0u);
}

TEST(GPUDrivenRendererTest, InitializeNullDevice)
{
    GPUDrivenRenderer renderer;
    EXPECT_FALSE(renderer.Initialize(nullptr, nullptr));
}

TEST(GPUDrivenRendererTest, CreateIndirectPipelineNullDevice)
{
    GPUDrivenRenderer renderer;
    renderer.CreateIndirectPipeline(nullptr);
    EXPECT_FALSE(renderer.IsIndirectReady());
}

TEST(GPUDrivenRendererTest, UploadMeshletBoundsNullParams)
{
    GPUDrivenRenderer renderer;
    renderer.UploadMeshletBounds(nullptr, nullptr, 0);
    // Should not crash
}

TEST(GPUDrivenRendererTest, CullAndDrawNotReady)
{
    GPUDrivenRenderer renderer;
    Matrix4x4 identity;
    XMStoreFloat4x4(XM(&identity), XMMatrixIdentity());
    renderer.CullAndDraw(nullptr, identity, 10);
    EXPECT_EQ(renderer.GetVisibleCount(), 0u);
}

TEST(GPUDrivenRendererTest, GPUDrawCommandLayout)
{
    GPUDrivenRenderer::GPUDrawCommand cmd = {};
    cmd.drawArgs.IndexCountPerInstance = 36;
    cmd.drawArgs.InstanceCount = 1;
    cmd.meshletIndex = 5;
    cmd.objectIndex = 10;

    EXPECT_EQ(cmd.drawArgs.IndexCountPerInstance, 36u);
    EXPECT_EQ(cmd.meshletIndex, 5u);
    EXPECT_EQ(cmd.objectIndex, 10u);
}

TEST(GPUDrivenRendererTest, MeshletBoundsLayout)
{
    GPUDrivenRenderer::MeshletBounds bounds;
    bounds.center = { 1.0f, 2.0f, 3.0f };
    bounds.radius = 5.0f;

    EXPECT_FLOAT_EQ(bounds.center.x, 1.0f);
    EXPECT_FLOAT_EQ(bounds.center.y, 2.0f);
    EXPECT_FLOAT_EQ(bounds.center.z, 3.0f);
    EXPECT_FLOAT_EQ(bounds.radius, 5.0f);
    EXPECT_EQ(sizeof(GPUDrivenRenderer::MeshletBounds), 16u);
}
