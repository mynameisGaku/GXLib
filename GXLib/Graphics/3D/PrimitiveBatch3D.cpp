/// @file PrimitiveBatch3D.cpp
/// @brief 3Dプリミティブバッチの実装
#include "pch.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace GX
{

bool PrimitiveBatch3D::Initialize(ID3D12Device* device)
{
    if (!m_shader.Initialize())
        return false;

    uint32_t vbSize = k_MaxVertices * sizeof(LineVertex3D);
    if (!m_vertexBuffer.Initialize(device, vbSize, sizeof(LineVertex3D)))
        return false;

    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    if (!CreatePipelineState(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/Primitive3D.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelineState(dev); }
    );

    GX_LOG_INFO("PrimitiveBatch3D initialized (max %d vertices)", k_MaxVertices);
    return true;
}

bool PrimitiveBatch3D::CreatePipelineState(ID3D12Device* device)
{
    auto vsBlob = m_shader.CompileFromFile(L"Shaders/Primitive3D.hlsl", L"VSMain", L"vs_6_0");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/Primitive3D.hlsl", L"PSMain", L"ps_6_0");
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("PrimitiveBatch3D: Failed to compile shaders");
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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    PipelineStateBuilder psoBuilder;
    m_pso = psoBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(inputLayout, _countof(inputLayout))
        .SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)  // HDR RT
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)  // 深度書き込みOFF
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .SetAlphaBlend()
        .Build(device);

    return m_pso != nullptr;
}

void PrimitiveBatch3D::Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                              const XMFLOAT4X4& viewProjection)
{
    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;
    m_vertexCount = 0;

    m_mappedVertices = static_cast<LineVertex3D*>(m_vertexBuffer.Map(frameIndex));

    // 定数バッファ
    void* cbData = m_constantBuffer.Map(frameIndex);
    if (cbData)
    {
        memcpy(cbData, &viewProjection, sizeof(XMFLOAT4X4));
        m_constantBuffer.Unmap(frameIndex);
    }
}

void PrimitiveBatch3D::DrawLine(const XMFLOAT3& p0, const XMFLOAT3& p1, const XMFLOAT4& color)
{
    if (!m_mappedVertices || m_vertexCount + 2 > k_MaxVertices)
        return;

    m_mappedVertices[m_vertexCount++] = { p0, color };
    m_mappedVertices[m_vertexCount++] = { p1, color };
}

void PrimitiveBatch3D::DrawWireBox(const XMFLOAT3& center, const XMFLOAT3& extents, const XMFLOAT4& color)
{
    float cx = center.x, cy = center.y, cz = center.z;
    float ex = extents.x, ey = extents.y, ez = extents.z;

    // 8頂点
    XMFLOAT3 v[8] = {
        { cx - ex, cy - ey, cz - ez }, { cx + ex, cy - ey, cz - ez },
        { cx + ex, cy + ey, cz - ez }, { cx - ex, cy + ey, cz - ez },
        { cx - ex, cy - ey, cz + ez }, { cx + ex, cy - ey, cz + ez },
        { cx + ex, cy + ey, cz + ez }, { cx - ex, cy + ey, cz + ez },
    };

    // 12辺
    int edges[][2] = {
        {0,1},{1,2},{2,3},{3,0}, // front
        {4,5},{5,6},{6,7},{7,4}, // back
        {0,4},{1,5},{2,6},{3,7}, // sides
    };

    for (auto& e : edges)
        DrawLine(v[e[0]], v[e[1]], color);
}

void PrimitiveBatch3D::DrawWireSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color,
                                        uint32_t segments)
{
    float step = XM_2PI / static_cast<float>(segments);

    // XY平面
    for (uint32_t i = 0; i < segments; ++i)
    {
        float a0 = step * i, a1 = step * (i + 1);
        DrawLine(
            { center.x + cosf(a0) * radius, center.y + sinf(a0) * radius, center.z },
            { center.x + cosf(a1) * radius, center.y + sinf(a1) * radius, center.z }, color);
    }
    // XZ平面
    for (uint32_t i = 0; i < segments; ++i)
    {
        float a0 = step * i, a1 = step * (i + 1);
        DrawLine(
            { center.x + cosf(a0) * radius, center.y, center.z + sinf(a0) * radius },
            { center.x + cosf(a1) * radius, center.y, center.z + sinf(a1) * radius }, color);
    }
    // YZ平面
    for (uint32_t i = 0; i < segments; ++i)
    {
        float a0 = step * i, a1 = step * (i + 1);
        DrawLine(
            { center.x, center.y + cosf(a0) * radius, center.z + sinf(a0) * radius },
            { center.x, center.y + cosf(a1) * radius, center.z + sinf(a1) * radius }, color);
    }
}

void PrimitiveBatch3D::DrawGrid(float size, uint32_t divisions, const XMFLOAT4& color)
{
    float step = size / static_cast<float>(divisions);
    float half = size * 0.5f;

    for (uint32_t i = 0; i <= divisions; ++i)
    {
        float pos = -half + step * i;
        DrawLine({ pos, 0, -half }, { pos, 0, half }, color);
        DrawLine({ -half, 0, pos }, { half, 0, pos }, color);
    }
}

void PrimitiveBatch3D::End()
{
    if (!m_mappedVertices || m_vertexCount == 0)
    {
        if (m_mappedVertices)
            m_vertexBuffer.Unmap(m_frameIndex);
        m_mappedVertices = nullptr;
        return;
    }

    m_vertexBuffer.Unmap(m_frameIndex);
    m_mappedVertices = nullptr;

    // パイプライン設定
    m_cmdList->SetPipelineState(m_pso.Get());
    m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(m_frameIndex));

    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    auto vbv = m_vertexBuffer.GetVertexBufferView(m_frameIndex, m_vertexCount * sizeof(LineVertex3D));
    m_cmdList->IASetVertexBuffers(0, 1, &vbv);
    m_cmdList->DrawInstanced(m_vertexCount, 1, 0, 0);
}

} // namespace GX
