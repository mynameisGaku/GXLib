/// @file InfiniteGrid.cpp
/// @brief シェーダーベース無限グリッドの実装
///
/// InfiniteGrid.hlslのVS/PSをフルスクリーン三角形(Draw(3,1,0,0))で描画する。
/// アルファブレンド+深度書き込みありで、シーンのオブジェクトと正しく交差する。

#include "InfiniteGrid.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

bool InfiniteGrid::Initialize(ID3D12Device* device, GX::Shader& shader)
{
    // Compile shaders
    auto vs = shader.CompileFromFile(L"Shaders/InfiniteGrid.hlsl", L"FullscreenVS", L"vs_6_0");
    auto ps = shader.CompileFromFile(L"Shaders/InfiniteGrid.hlsl", L"GridPS", L"ps_6_0");
    if (!vs.valid || !ps.valid)
    {
        GX_LOG_ERROR("InfiniteGrid: shader compilation failed: %s", shader.GetLastError().c_str());
        return false;
    }

    // Root signature: CBV at b0
    GX::RootSignatureBuilder rsBuilder;
    m_rootSig = rsBuilder
        .AddCBV(0) // b0 = GridCB
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE) // no input assembler
        .Build(device);
    if (!m_rootSig)
    {
        GX_LOG_ERROR("InfiniteGrid: root signature creation failed");
        return false;
    }

    // PSO: fullscreen triangle, alpha blend, depth write
    GX::PipelineStateBuilder psoBuilder;
    m_pso = psoBuilder
        .SetRootSignature(m_rootSig.Get())
        .SetVertexShader(vs.GetBytecode())
        .SetPixelShader(ps.GetBytecode())
        .SetInputLayout(nullptr, 0) // no vertex buffer
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0) // HDR RT
        .SetRenderTargetCount(1)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ALL)
        .SetDepthComparisonFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
        .SetAlphaBlend()
        .Build(device);
    if (!m_pso)
    {
        GX_LOG_ERROR("InfiniteGrid: PSO creation failed");
        return false;
    }

    // Constant buffer (double-buffered)
    if (!m_cbuffer.Initialize(device, sizeof(GridCBData), sizeof(GridCBData)))
    {
        GX_LOG_ERROR("InfiniteGrid: cbuffer creation failed");
        return false;
    }

    return true;
}

void InfiniteGrid::Draw(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                         const GX::Camera3D& camera)
{
    // Update constant buffer
    XMMATRIX view = camera.GetViewMatrix();
    XMMATRIX proj = camera.GetProjectionMatrix();
    XMMATRIX vp   = view * proj;

    XMVECTOR det;
    XMMATRIX vpInverse = XMMatrixInverse(&det, vp);

    GridCBData cbData = {};
    XMStoreFloat4x4(&cbData.viewProjectionInverse, XMMatrixTranspose(vpInverse));
    XMStoreFloat4x4(&cbData.viewProjection, XMMatrixTranspose(vp));
    cbData.cameraPos    = camera.GetPosition();
    cbData.gridScale    = gridScale;
    cbData.fadeDistance  = fadeDistance;
    cbData.majorLineEvery = majorLineEvery;

    void* mapped = m_cbuffer.Map(frameIndex);
    if (mapped)
    {
        memcpy(mapped, &cbData, sizeof(GridCBData));
        m_cbuffer.Unmap(frameIndex);
    }

    // Draw fullscreen triangle
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSig.Get());
    cmdList->SetGraphicsRootConstantBufferView(0, m_cbuffer.GetGPUVirtualAddress(frameIndex));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}
