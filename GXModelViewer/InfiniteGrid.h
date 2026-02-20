#pragma once
/// @file InfiniteGrid.h
/// @brief Shader-based infinite grid on Y=0 plane

#include "pch.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/3D/Camera3D.h"

/// @brief Renders an infinite grid on the Y=0 plane using a fullscreen triangle
class InfiniteGrid
{
public:
    bool Initialize(ID3D12Device* device, GX::Shader& shader);

    /// Draw the grid. Call after Renderer3D.End() but before PostEffect.EndScene().
    /// Assumes HDR render target + depth buffer are already bound.
    void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
              const GX::Camera3D& camera);

    float gridScale     = 1.0f;    // minor grid spacing in world units
    float fadeDistance   = 100.0f;  // distance at which grid fades out
    float majorLineEvery = 10.0f;  // major grid line every N minor lines

private:
    struct alignas(256) GridCBData
    {
        XMFLOAT4X4 viewProjectionInverse;
        XMFLOAT4X4 viewProjection;
        XMFLOAT3   cameraPos;
        float      gridScale;
        float      fadeDistance;
        float      majorLineEvery;
        float      _pad0;
        float      _pad1;
    };

    ComPtr<ID3D12RootSignature> m_rootSig;
    ComPtr<ID3D12PipelineState> m_pso;
    GX::DynamicBuffer           m_cbuffer;
};
