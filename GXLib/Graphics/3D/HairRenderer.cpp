/// @file HairRenderer.cpp
/// @brief ヘアレンダラー (Marschner反射モデル) の実装
#include "pch_graphics.h"
#include "Graphics/3D/HairRenderer.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"

namespace gx
{

using namespace DirectX;

bool HairRenderer::Initialize(ID3D12Device* device)
{
    if (!device)
        return false;

    m_device = device;

    if (!m_shader.Initialize())
    {
        GX_LOG_ERROR("HairRenderer: Failed to initialize shader compiler");
        return false;
    }

    // 定数バッファ: 256Bアライメントの HairConstants
    if (!m_cb.Initialize(device, sizeof(HairConstants), sizeof(HairConstants)))
    {
        GX_LOG_ERROR("HairRenderer: Failed to create constant buffer");
        return false;
    }

    if (!CreatePipeline(device))
    {
        GX_LOG_ERROR("HairRenderer: Failed to create pipeline");
        return false;
    }

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/HairStrand.hlsl",
        [this](ID3D12Device* dev) { return CreatePipeline(dev); }
    );

    GX_LOG_INFO("HairRenderer initialized");
    return true;
}

void HairRenderer::Shutdown()
{
    m_pso.Reset();
    m_rootSignature.Reset();
    m_strandCount = 0;
    m_totalVertices = 0;
    m_psoReady = false;
    m_device = nullptr;
}

bool HairRenderer::CreatePipeline(ID3D12Device* device)
{
    auto vsBlob = m_shader.CompileFromFile(L"Shaders/HairStrand.hlsl", L"VSMain", L"vs_6_0");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/HairStrand.hlsl", L"PSMain", L"ps_6_0");
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("HairRenderer: Failed to compile shaders");
        return false;
    }

    // ルートシグネチャ:
    // root param 0: b0 = HairConstants定数バッファ (CBV)
    RootSignatureBuilder rsBuilder;
    m_rootSignature = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        .AddCBV(0)  // b0: HairConstants
        .Build(device);

    if (!m_rootSignature)
    {
        GX_LOG_ERROR("HairRenderer: Failed to create root signature");
        return false;
    }

    // 頂点入力レイアウト:
    // POSITION  (float3)  offset  0
    // TANGENT   (float3)  offset 12
    // TEXCOORD0 (float)   offset 24 (thickness)
    // TEXCOORD1 (float)   offset 28 (strandParam)
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT,       0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
        .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)  // 半透明パス
        .SetCullMode(D3D12_CULL_MODE_NONE)                // 両面
        .SetAlphaBlend()
        .Build(device);

    m_psoReady = (m_pso != nullptr);
    return m_psoReady;
}

void HairRenderer::SetStrands(const gx::Vector<HairStrand>& strands)
{
    BuildVertexBuffer(strands);
}

void HairRenderer::BuildVertexBuffer(const gx::Vector<HairStrand>& strands)
{
    m_strandCount = static_cast<uint32_t>(strands.size());
    m_totalVertices = 0;

    // まず総頂点数を算出 (各ストランドのセグメントあたり2頂点: line listなので)
    gx::Vector<HairVertex> vertices;
    for (const auto& strand : strands)
    {
        if (strand.controlPoints.size() < 2)
            continue;

        uint32_t cpCount = static_cast<uint32_t>(strand.controlPoints.size());
        for (uint32_t i = 0; i < cpCount - 1; ++i)
        {
            const auto& cp0 = strand.controlPoints[i];
            const auto& cp1 = strand.controlPoints[i + 1];

            // 接線: 隣接制御点間の差分
            Vector3 tangent;
            tangent.x = cp1.position.x - cp0.position.x;
            tangent.y = cp1.position.y - cp0.position.y;
            tangent.z = cp1.position.z - cp0.position.z;
            float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
            if (len > 1e-6f)
            {
                tangent.x /= len;
                tangent.y /= len;
                tangent.z /= len;
            }
            else
            {
                tangent = { 0.0f, 1.0f, 0.0f };
            }

            float param0 = (cpCount > 1)
                ? static_cast<float>(i) / static_cast<float>(cpCount - 1)
                : 0.0f;
            float param1 = (cpCount > 1)
                ? static_cast<float>(i + 1) / static_cast<float>(cpCount - 1)
                : 1.0f;

            HairVertex v0;
            v0.position = cp0.position;
            v0.tangent = tangent;
            v0.thickness = cp0.thickness;
            v0.strandParam = param0;
            vertices.push_back(v0);

            HairVertex v1;
            v1.position = cp1.position;
            v1.tangent = tangent;
            v1.thickness = cp1.thickness;
            v1.strandParam = param1;
            vertices.push_back(v1);
        }
    }

    m_totalVertices = static_cast<uint32_t>(vertices.size());

    if (m_totalVertices == 0 || !m_device)
        return;

    // 頂点バッファを作成
    uint32_t bufferSize = m_totalVertices * sizeof(HairVertex);
    m_vertexBuffer.CreateVertexBuffer(m_device, vertices.data(), bufferSize, sizeof(HairVertex));
}

void HairRenderer::Render(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                            const Matrix4x4& viewProj, const Vector3& cameraPos,
                            const Vector3& lightDir, const Vector3& lightColor)
{
    if (!m_psoReady || m_totalVertices == 0 || !cmdList)
        return;

    // 定数バッファを書き込み
    void* cbData = m_cb.Map(frameIndex);
    if (cbData)
    {
        HairConstants constants = {};

        // ViewProjection行列 (row-major float[16])
        std::memcpy(constants.viewProj, &viewProj, sizeof(float) * 16);

        // カメラ位置
        constants.cameraPos[0] = cameraPos.x;
        constants.cameraPos[1] = cameraPos.y;
        constants.cameraPos[2] = cameraPos.z;
        constants.cameraPos[3] = 0.0f;

        // ライト方向
        constants.lightDir[0] = lightDir.x;
        constants.lightDir[1] = lightDir.y;
        constants.lightDir[2] = lightDir.z;
        constants.lightDir[3] = 0.0f;

        // ライトカラー
        constants.lightColor[0] = lightColor.x;
        constants.lightColor[1] = lightColor.y;
        constants.lightColor[2] = lightColor.z;
        constants.lightColor[3] = 1.0f;

        // ベースカラー + roughnessR
        constants.baseColor[0] = m_material.baseColor.x;
        constants.baseColor[1] = m_material.baseColor.y;
        constants.baseColor[2] = m_material.baseColor.z;
        constants.baseColor[3] = m_material.roughnessR;

        // roughnessTT, roughnessTRT, shift, ior
        constants.roughness[0] = m_material.roughnessTT;
        constants.roughness[1] = m_material.roughnessTRT;
        constants.roughness[2] = m_material.shift;
        constants.roughness[3] = m_material.ior;

        // absorption, specularMultiplier, scatterStrength, pad
        constants.params[0] = m_material.absorption;
        constants.params[1] = m_material.specularMultiplier;
        constants.params[2] = m_material.scatterStrength;
        constants.params[3] = 0.0f;

        std::memcpy(cbData, &constants, sizeof(HairConstants));
        m_cb.Unmap(frameIndex);
    }

    // パイプライン設定
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // 定数バッファバインド (root param 0 = b0)
    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));

    // トポロジ: ラインリスト
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    // 頂点バッファバインド
    auto vbv = m_vertexBuffer.GetVertexBufferView();
    cmdList->IASetVertexBuffers(0, 1, &vbv);

    // 描画
    cmdList->DrawInstanced(m_totalVertices, 1, 0, 0);
}

} // namespace gx
