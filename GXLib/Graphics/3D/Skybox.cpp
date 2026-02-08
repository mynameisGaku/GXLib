/// @file Skybox.cpp
/// @brief プロシージャルスカイボックスの実装
#include "pch.h"
#include "Graphics/3D/Skybox.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

bool Skybox::Initialize(ID3D12Device* device)
{
    if (!m_shader.Initialize())
        return false;

    // フルスクリーンキューブの頂点（位置のみ）
    struct SkyVertex { XMFLOAT3 position; };
    SkyVertex vertices[] = {
        // Front
        {{ -1, -1,  1 }}, {{  1, -1,  1 }}, {{  1,  1,  1 }}, {{ -1,  1,  1 }},
        // Back
        {{  1, -1, -1 }}, {{ -1, -1, -1 }}, {{ -1,  1, -1 }}, {{  1,  1, -1 }},
        // Left
        {{ -1, -1, -1 }}, {{ -1, -1,  1 }}, {{ -1,  1,  1 }}, {{ -1,  1, -1 }},
        // Right
        {{  1, -1,  1 }}, {{  1, -1, -1 }}, {{  1,  1, -1 }}, {{  1,  1,  1 }},
        // Top
        {{ -1,  1,  1 }}, {{  1,  1,  1 }}, {{  1,  1, -1 }}, {{ -1,  1, -1 }},
        // Bottom
        {{ -1, -1, -1 }}, {{  1, -1, -1 }}, {{  1, -1,  1 }}, {{ -1, -1,  1 }},
    };

    uint32_t indices[] = {
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    m_vertexBuffer.CreateVertexBuffer(device, vertices, sizeof(vertices), sizeof(SkyVertex));
    m_indexBuffer.CreateIndexBuffer(device, indices, sizeof(indices), DXGI_FORMAT_R32_UINT);

    // 定数バッファ
    uint32_t cbSize = ((sizeof(SkyboxConstants) + 255) / 256) * 256;
    if (!m_constantBuffer.Initialize(device, cbSize, cbSize))
        return false;

    if (!CreatePipelineState(device))
        return false;

    GX_LOG_INFO("Skybox initialized");
    return true;
}

bool Skybox::CreatePipelineState(ID3D12Device* device)
{
    auto vsBlob = m_shader.CompileFromFile(L"Shaders/Skybox.hlsl", L"VSMain", L"vs_6_0");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/Skybox.hlsl", L"PSMain", L"ps_6_0");
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("Skybox: Failed to compile shaders");
        return false;
    }

    RootSignatureBuilder rsBuilder;
    m_rootSignature = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        .AddCBV(0)
        .Build(device);

    if (!m_rootSignature)
        return false;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    PipelineStateBuilder psoBuilder;
    m_pso = psoBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(inputLayout, _countof(inputLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)  // HDR RT
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)  // 深度書き込みOFF
        .SetDepthComparisonFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL)  // z=1で描画
        .SetCullMode(D3D12_CULL_MODE_NONE)  // 裏面も描画
        .Build(device);

    return m_pso != nullptr;
}

void Skybox::SetColors(const XMFLOAT3& topColor, const XMFLOAT3& bottomColor)
{
    m_topColor    = topColor;
    m_bottomColor = bottomColor;
}

void Skybox::SetSun(const XMFLOAT3& direction, float intensity)
{
    m_sunDirection = direction;
    m_sunIntensity = intensity;
}

void Skybox::Draw(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                   const XMFLOAT4X4& viewProjection)
{
    // 定数バッファ更新
    void* cbData = m_constantBuffer.Map(frameIndex);
    if (cbData)
    {
        SkyboxConstants sc;
        sc.viewProjection = viewProjection;
        sc.topColor       = m_topColor;
        sc.bottomColor    = m_bottomColor;
        sc.sunDirection   = m_sunDirection;
        sc.sunIntensity   = m_sunIntensity;
        sc.padding1       = 0.0f;
        sc.padding2       = 0.0f;
        memcpy(cbData, &sc, sizeof(SkyboxConstants));
        m_constantBuffer.Unmap(frameIndex);
    }

    // パイプライン設定
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(frameIndex));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    auto vbv = m_vertexBuffer.GetVertexBufferView();
    auto ibv = m_indexBuffer.GetIndexBufferView();
    cmdList->IASetVertexBuffers(0, 1, &vbv);
    cmdList->IASetIndexBuffer(&ibv);
    cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

} // namespace GX
