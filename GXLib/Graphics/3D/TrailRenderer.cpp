/// @file TrailRenderer.cpp
/// @brief トレイル（軌跡）レンダラーの実装
#include "pch.h"
#include "Graphics/3D/TrailRenderer.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace GX
{

using namespace DirectX;

bool TrailRenderer::Initialize(ID3D12Device* device, uint32_t maxPoints)
{
    m_device = device;
    m_maxPoints = maxPoints;
    m_points.resize(m_maxPoints);
    m_head = 0;
    m_pointCount = 0;
    m_elapsedTime = 0.0f;

    if (!m_shader.Initialize())
        return false;

    // 頂点バッファ: 各ポイントから左右2頂点を生成するので最大頂点数は maxPoints * 2
    uint32_t vbSize = m_maxPoints * 2 * sizeof(TrailVertex);
    if (!m_vertexBuffer.Initialize(device, vbSize, sizeof(TrailVertex)))
        return false;

    // 定数バッファ: ViewProjection行列 (64B) を256Bアライメントで格納
    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    if (!CreatePipelineState(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/Trail.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelineState(dev); }
    );

    GX_LOG_INFO("TrailRenderer initialized (max %u points)", m_maxPoints);
    return true;
}

bool TrailRenderer::CreatePipelineState(ID3D12Device* device)
{
    auto vsBlob = m_shader.CompileFromFile(L"Shaders/Trail.hlsl", L"VSMain", L"vs_6_0");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/Trail.hlsl", L"PSMain", L"ps_6_0");
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("TrailRenderer: Failed to compile shaders");
        return false;
    }

    // ルートシグネチャ:
    // root param 0: b0 = ViewProjection定数バッファ (CBV)
    // root param 1: t0 = テクスチャ (SRV descriptor table)
    // static sampler s0: linear clamp
    RootSignatureBuilder rsBuilder;
    m_rootSignature = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        .AddCBV(0)                                                              // b0: ViewProjection
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)                      // t0: テクスチャ
        .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                          D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                          D3D12_COMPARISON_FUNC_NEVER)                          // s0: linear clamp
        .Build(device);

    if (!m_rootSignature)
        return false;

    // 頂点入力レイアウト: POSITION(float3) + TEXCOORD(float2) + COLOR(float4)
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    PipelineStateBuilder psoBuilder;
    m_pso = psoBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(inputLayout, _countof(inputLayout))
        .SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)  // HDR RT
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)  // 深度書き込みOFF（半透明）
        .SetCullMode(D3D12_CULL_MODE_NONE)                // 両面描画
        .SetAlphaBlend()
        .Build(device);

    return m_pso != nullptr;
}

void TrailRenderer::AddPoint(const XMFLOAT3& position, const XMFLOAT3& up,
                              float width, const Color& color)
{
    TrailPoint& pt = m_points[m_head];
    pt.position = position;
    pt.up = up;
    pt.width = width;
    pt.color = color;
    pt.time = m_elapsedTime;

    m_head = (m_head + 1) % m_maxPoints;
    if (m_pointCount < m_maxPoints)
        m_pointCount++;
}

void TrailRenderer::Update(float deltaTime)
{
    m_elapsedTime += deltaTime;

    // テール側（最も古いポイント）から寿命切れを削除
    while (m_pointCount > 0)
    {
        // テールのインデックス: ヘッドからpointCount分戻った位置
        uint32_t tail = (m_head + m_maxPoints - m_pointCount) % m_maxPoints;
        float age = m_elapsedTime - m_points[tail].time;
        if (age > lifetime)
            m_pointCount--;
        else
            break;
    }
}

void TrailRenderer::BuildVertices(std::vector<TrailVertex>& outVerts) const
{
    if (m_pointCount < 2)
        return;

    outVerts.reserve(m_pointCount * 2);

    // テール（最古）からヘッド（最新）へ順に走査
    uint32_t tail = (m_head + m_maxPoints - m_pointCount) % m_maxPoints;

    for (uint32_t i = 0; i < m_pointCount; ++i)
    {
        uint32_t idx = (tail + i) % m_maxPoints;
        const TrailPoint& pt = m_points[idx];

        // 正規化年齢: 0=最新、1=最古
        float age = m_elapsedTime - pt.time;
        float normalizedAge = (lifetime > 0.0f) ? (age / lifetime) : 0.0f;
        normalizedAge = (std::max)(0.0f, (std::min)(1.0f, normalizedAge));

        // UV.y: 0=最古（テール）, 1=最新（ヘッド）
        float v = (m_pointCount > 1)
                    ? static_cast<float>(i) / static_cast<float>(m_pointCount - 1)
                    : 0.0f;

        // カラー（フェード適用）
        XMFLOAT4 col = { pt.color.r, pt.color.g, pt.color.b, pt.color.a };
        if (fadeWithAge)
            col.w *= (1.0f - normalizedAge);

        // 幅方向のオフセット: position ± up * width
        XMVECTOR vPos = XMLoadFloat3(&pt.position);
        XMVECTOR vUp  = XMLoadFloat3(&pt.up);

        XMFLOAT3 leftPos, rightPos;
        XMStoreFloat3(&leftPos,  XMVectorAdd(vPos, XMVectorScale(vUp, pt.width)));
        XMStoreFloat3(&rightPos, XMVectorSubtract(vPos, XMVectorScale(vUp, pt.width)));

        // 左頂点 (u=0)
        TrailVertex leftVert;
        leftVert.position = leftPos;
        leftVert.uv = { 0.0f, v };
        leftVert.color = col;
        outVerts.push_back(leftVert);

        // 右頂点 (u=1)
        TrailVertex rightVert;
        rightVert.position = rightPos;
        rightVert.uv = { 1.0f, v };
        rightVert.color = col;
        outVerts.push_back(rightVert);
    }
}

void TrailRenderer::Draw(ID3D12GraphicsCommandList* cmdList,
                          const Camera3D& camera, uint32_t frameIndex,
                          TextureManager* texManager)
{
    if (m_pointCount < 2)
        return;

    // 頂点を構築
    std::vector<TrailVertex> vertices;
    BuildVertices(vertices);

    if (vertices.size() < 4)
        return;

    uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
    uint32_t usedSize = vertexCount * sizeof(TrailVertex);

    // 頂点バッファに書き込み
    void* mapped = m_vertexBuffer.Map(frameIndex);
    if (!mapped)
        return;
    memcpy(mapped, vertices.data(), usedSize);
    m_vertexBuffer.Unmap(frameIndex);

    // 定数バッファ: ViewProjection行列
    void* cbData = m_constantBuffer.Map(frameIndex);
    if (cbData)
    {
        XMMATRIX vp = camera.GetViewProjectionMatrix();
        memcpy(cbData, &vp, sizeof(XMMATRIX));
        m_constantBuffer.Unmap(frameIndex);
    }

    // パイプライン設定
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // 定数バッファバインド (root param 0 = b0)
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(frameIndex));

    // テクスチャバインド (root param 1 = t0)
    Texture* tex = nullptr;
    if (texManager && textureHandle >= 0)
        tex = texManager->GetTexture(textureHandle);

    if (tex)
    {
        ID3D12DescriptorHeap* heaps[] = { texManager->GetSRVHeap().GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootDescriptorTable(1, tex->GetSRVGPUHandle());
    }

    // トポロジ: トライアングルストリップ
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 頂点バッファバインド
    auto vbv = m_vertexBuffer.GetVertexBufferView(frameIndex, usedSize);
    cmdList->IASetVertexBuffers(0, 1, &vbv);

    // 描画
    cmdList->DrawInstanced(vertexCount, 1, 0, 0);
}

void TrailRenderer::Clear()
{
    m_head = 0;
    m_pointCount = 0;
}

} // namespace GX
