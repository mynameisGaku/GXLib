/// @file PrimitiveBatch.cpp
/// @brief プリミティブバッチの実装
#include "pch.h"
#include "Graphics/Rendering/PrimitiveBatch.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GX
{

XMFLOAT4 PrimitiveBatch::ColorToFloat4(uint32_t color)
{
    // 0xAARRGGBB形式（DXLib互換）
    float a = ((color >> 24) & 0xFF) / 255.0f;
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8)  & 0xFF) / 255.0f;
    float b = ((color >> 0)  & 0xFF) / 255.0f;
    // アルファが0の場合は完全不透明として扱う（DXLib互換: 色指定でアルファ省略時）
    if (a == 0.0f) a = 1.0f;
    return { r, g, b, a };
}

bool PrimitiveBatch::Initialize(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight)
{
    m_device       = device;
    m_screenWidth  = screenWidth;
    m_screenHeight = screenHeight;

    // 三角形用頂点バッファ
    uint32_t triBufferSize = k_MaxTriangleVertices * sizeof(PrimitiveVertex);
    if (!m_triangleVertexBuffer.Initialize(device, triBufferSize, sizeof(PrimitiveVertex)))
        return false;

    // 線分用頂点バッファ
    uint32_t lineBufferSize = k_MaxLineVertices * sizeof(PrimitiveVertex);
    if (!m_lineVertexBuffer.Initialize(device, lineBufferSize, sizeof(PrimitiveVertex)))
        return false;

    // 定数バッファ（正射影行列）
    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    // シェーダーコンパイル
    if (!m_shaderCompiler.Initialize())
        return false;

    if (!CreatePipelineStates(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/Primitive.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelineStates(dev); }
    );

    GX_LOG_INFO("PrimitiveBatch initialized (%dx%d)", screenWidth, screenHeight);
    return true;
}

bool PrimitiveBatch::CreatePipelineStates(ID3D12Device* device)
{
    auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/Primitive.hlsl", L"VSMain", L"vs_6_0");
    auto psBlob = m_shaderCompiler.CompileFromFile(L"Shaders/Primitive.hlsl", L"PSMain", L"ps_6_0");
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("Failed to compile primitive shaders");
        return false;
    }

    // ルートシグネチャ: b0（正射影行列）のみ
    RootSignatureBuilder rsBuilder;
    m_rootSignature = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        .AddCBV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX)
        .Build(device);

    if (!m_rootSignature)
        return false;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // 三角形PSO
    PipelineStateBuilder triBuilder;
    m_trianglePSO = triBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(inputLayout, _countof(inputLayout))
        .SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .SetAlphaBlend()
        .Build(device);

    if (!m_trianglePSO)
        return false;

    // 線分PSO
    PipelineStateBuilder lineBuilder;
    m_linePSO = lineBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(inputLayout, _countof(inputLayout))
        .SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .SetAlphaBlend()
        .Build(device);

    if (!m_linePSO)
        return false;

    return true;
}

void PrimitiveBatch::Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)
{
    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;

    m_mappedTriVertices  = static_cast<PrimitiveVertex*>(m_triangleVertexBuffer.Map(frameIndex));
    m_mappedLineVertices = static_cast<PrimitiveVertex*>(m_lineVertexBuffer.Map(frameIndex));
    m_triVertexCount  = 0;
    m_lineVertexCount = 0;

    // 正射影行列を定数バッファに書き込み
    void* cbData = m_constantBuffer.Map(frameIndex);
    if (cbData)
    {
        XMMATRIX proj = m_useCustomProjection ? m_projectionMatrix
                                               : XMMatrixOrthographicOffCenterLH(
                                                     0.0f, static_cast<float>(m_screenWidth),
                                                     static_cast<float>(m_screenHeight), 0.0f,
                                                     0.0f, 1.0f);
        memcpy(cbData, &proj, sizeof(XMMATRIX));
        m_constantBuffer.Unmap(frameIndex);
    }
}

void PrimitiveBatch::FlushTriangles()
{
    if (m_triVertexCount == 0)
        return;

    m_cmdList->SetPipelineState(m_trianglePSO.Get());
    m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(m_frameIndex));
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    uint32_t vertexSize = m_triVertexCount * sizeof(PrimitiveVertex);
    auto vbv = m_triangleVertexBuffer.GetVertexBufferView(m_frameIndex, vertexSize);
    m_cmdList->IASetVertexBuffers(0, 1, &vbv);
    m_cmdList->DrawInstanced(m_triVertexCount, 1, 0, 0);

    m_triVertexCount = 0;
}

void PrimitiveBatch::FlushLines()
{
    if (m_lineVertexCount == 0)
        return;

    m_cmdList->SetPipelineState(m_linePSO.Get());
    m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(m_frameIndex));
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    uint32_t vertexSize = m_lineVertexCount * sizeof(PrimitiveVertex);
    auto vbv = m_lineVertexBuffer.GetVertexBufferView(m_frameIndex, vertexSize);
    m_cmdList->IASetVertexBuffers(0, 1, &vbv);
    m_cmdList->DrawInstanced(m_lineVertexCount, 1, 0, 0);

    m_lineVertexCount = 0;
}

void PrimitiveBatch::DrawLine(float x1, float y1, float x2, float y2, uint32_t color, int thickness)
{
    if (m_lineVertexCount + 2 > k_MaxLineVertices)
        FlushLines();

    XMFLOAT4 col = ColorToFloat4(color);
    m_mappedLineVertices[m_lineVertexCount++] = { { x1, y1 }, col };
    m_mappedLineVertices[m_lineVertexCount++] = { { x2, y2 }, col };
}

void PrimitiveBatch::DrawBox(float x1, float y1, float x2, float y2, uint32_t color, bool fillFlag)
{
    XMFLOAT4 col = ColorToFloat4(color);

    if (fillFlag)
    {
        if (m_triVertexCount + 6 > k_MaxTriangleVertices)
            FlushTriangles();

        // 2三角形で矩形を塗りつぶし
        m_mappedTriVertices[m_triVertexCount++] = { { x1, y1 }, col };
        m_mappedTriVertices[m_triVertexCount++] = { { x2, y1 }, col };
        m_mappedTriVertices[m_triVertexCount++] = { { x1, y2 }, col };
        m_mappedTriVertices[m_triVertexCount++] = { { x1, y2 }, col };
        m_mappedTriVertices[m_triVertexCount++] = { { x2, y1 }, col };
        m_mappedTriVertices[m_triVertexCount++] = { { x2, y2 }, col };
    }
    else
    {
        // 4線分で矩形のアウトライン
        DrawLine(x1, y1, x2, y1, color);
        DrawLine(x2, y1, x2, y2, color);
        DrawLine(x2, y2, x1, y2, color);
        DrawLine(x1, y2, x1, y1, color);
    }
}

void PrimitiveBatch::DrawCircle(float cx, float cy, float r, uint32_t color, bool fillFlag, int segments)
{
    DrawOval(cx, cy, r, r, color, fillFlag, segments);
}

void PrimitiveBatch::DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                                   uint32_t color, bool fillFlag)
{
    XMFLOAT4 col = ColorToFloat4(color);

    if (fillFlag)
    {
        if (m_triVertexCount + 3 > k_MaxTriangleVertices)
            FlushTriangles();

        m_mappedTriVertices[m_triVertexCount++] = { { x1, y1 }, col };
        m_mappedTriVertices[m_triVertexCount++] = { { x2, y2 }, col };
        m_mappedTriVertices[m_triVertexCount++] = { { x3, y3 }, col };
    }
    else
    {
        DrawLine(x1, y1, x2, y2, color);
        DrawLine(x2, y2, x3, y3, color);
        DrawLine(x3, y3, x1, y1, color);
    }
}

void PrimitiveBatch::DrawOval(float cx, float cy, float rx, float ry, uint32_t color,
                               bool fillFlag, int segments)
{
    XMFLOAT4 col = ColorToFloat4(color);
    float angleStep = 2.0f * static_cast<float>(M_PI) / segments;

    if (fillFlag)
    {
        // 扇形の三角形で塗りつぶし
        for (int i = 0; i < segments; ++i)
        {
            if (m_triVertexCount + 3 > k_MaxTriangleVertices)
                FlushTriangles();

            float a0 = angleStep * i;
            float a1 = angleStep * (i + 1);

            m_mappedTriVertices[m_triVertexCount++] = { { cx, cy }, col };
            m_mappedTriVertices[m_triVertexCount++] = { { cx + rx * cosf(a0), cy + ry * sinf(a0) }, col };
            m_mappedTriVertices[m_triVertexCount++] = { { cx + rx * cosf(a1), cy + ry * sinf(a1) }, col };
        }
    }
    else
    {
        // 線分でアウトライン
        for (int i = 0; i < segments; ++i)
        {
            float a0 = angleStep * i;
            float a1 = angleStep * (i + 1);

            float px0 = cx + rx * cosf(a0);
            float py0 = cy + ry * sinf(a0);
            float px1 = cx + rx * cosf(a1);
            float py1 = cy + ry * sinf(a1);

            if (m_lineVertexCount + 2 > k_MaxLineVertices)
                FlushLines();

            m_mappedLineVertices[m_lineVertexCount++] = { { px0, py0 }, col };
            m_mappedLineVertices[m_lineVertexCount++] = { { px1, py1 }, col };
        }
    }
}

void PrimitiveBatch::DrawPixel(float x, float y, uint32_t color)
{
    // 1ピクセル = 1x1の塗りつぶし矩形
    DrawBox(x, y, x + 1.0f, y + 1.0f, color, true);
}

void PrimitiveBatch::End()
{
    FlushTriangles();
    FlushLines();

    m_triangleVertexBuffer.Unmap(m_frameIndex);
    m_lineVertexBuffer.Unmap(m_frameIndex);
    m_mappedTriVertices  = nullptr;
    m_mappedLineVertices = nullptr;
    m_cmdList = nullptr;
}

void PrimitiveBatch::SetScreenSize(uint32_t width, uint32_t height)
{
    m_screenWidth  = width;
    m_screenHeight = height;
}

void PrimitiveBatch::SetProjectionMatrix(const XMMATRIX& matrix)
{
    m_projectionMatrix = matrix;
    m_useCustomProjection = true;
}

void PrimitiveBatch::ResetProjectionMatrix()
{
    m_useCustomProjection = false;
}

} // namespace GX
