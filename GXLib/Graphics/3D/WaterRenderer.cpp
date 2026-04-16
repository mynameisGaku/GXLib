/// @file WaterRenderer.cpp
/// @brief 水面レンダリングの実装
///
/// XZ平面上にグリッドメッシュを生成し、頂点シェーダーでGerstner波アニメーションを適用する。
/// ピクセルシェーダーでフレネルベースの反射/屈折を処理する。
#include "pch_graphics.h"
#include "Graphics/3D/WaterRenderer.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"
#include "Math/MathConvert.h"

namespace gx
{

/// @brief 水面頂点レイアウト（位置 + UV）
struct WaterVertex
{
    Vector3 position;
    Vector2 uv;
};

bool WaterRenderer::Initialize(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight)
{
    m_device       = device;
    m_screenWidth  = screenWidth;
    m_screenHeight = screenHeight;

    // 専用SRVヒープ: 深度テクスチャ1枚 x 2フレーム = 2スロット
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true))
        return false;

    if (!m_shader.Initialize())
        return false;

    // CBサイズ: 224バイト、CBアライメントのため256にパディング
    if (!m_cb.Initialize(device, 256, 256))
        return false;

    // ルートシグネチャ: [0] CBV(b0), [1] ディスクリプタテーブル(t0 深度SRV), s0(リニアクランプ)
    {
        RootSignatureBuilder rsb;
        m_rootSignature = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_ALL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
            .Build(device);
        if (!m_rootSignature) return false;
    }

    if (!CreatePipelines(device))
        return false;

    if (!CreateGridMesh(device))
        return false;

    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/Water.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("WaterRenderer initialized (%dx%d grid, plane=%.0f)", m_gridResolution, m_gridResolution, m_planeSize);
    return true;
}

bool WaterRenderer::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/Water.hlsl", L"VS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/Water.hlsl", L"PS", L"ps_6_0");
    if (!ps.valid) return false;

    // 入力レイアウト: POSITION (float3) + TEXCOORD (float2)
    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // 水面の透過用アルファブレンドステート
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable    = TRUE;
    blendDesc.RenderTarget[0].SrcBlend       = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend      = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp        = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha   = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    PipelineStateBuilder b;
    m_pso = b.SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vs.GetBytecode())
        .SetPixelShader(ps.GetBytecode())
        .SetInputLayout(inputElements, _countof(inputElements))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO) // 読み取り専用深度テスト
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .SetBlendState(blendDesc)
        .Build(device);
    if (!m_pso) return false;

    return true;
}

bool WaterRenderer::CreateGridMesh(ID3D12Device* device)
{
    int res = m_gridResolution;
    int vertexCount = (res + 1) * (res + 1);
    int quadCount   = res * res;
    m_indexCount    = static_cast<uint32_t>(quadCount * 6);

    // 頂点を生成
    gx::Vector<WaterVertex> vertices(vertexCount);
    float halfSize = m_planeSize * 0.5f;

    for (int z = 0; z <= res; ++z)
    {
        for (int x = 0; x <= res; ++x)
        {
            int idx = z * (res + 1) + x;
            float fx = -halfSize + (static_cast<float>(x) / res) * m_planeSize;
            float fz = -halfSize + (static_cast<float>(z) / res) * m_planeSize;

            vertices[idx].position = { fx, 0.0f, fz };
            vertices[idx].uv = {
                static_cast<float>(x) / res,
                static_cast<float>(z) / res
            };
        }
    }

    // インデックスを生成（クアッドあたり2三角形）
    gx::Vector<uint32_t> indices(m_indexCount);
    uint32_t ii = 0;
    for (int z = 0; z < res; ++z)
    {
        for (int x = 0; x < res; ++x)
        {
            uint32_t topLeft     = static_cast<uint32_t>(z * (res + 1) + x);
            uint32_t topRight    = topLeft + 1;
            uint32_t bottomLeft  = static_cast<uint32_t>((z + 1) * (res + 1) + x);
            uint32_t bottomRight = bottomLeft + 1;

            indices[ii++] = topLeft;
            indices[ii++] = bottomLeft;
            indices[ii++] = topRight;

            indices[ii++] = topRight;
            indices[ii++] = bottomLeft;
            indices[ii++] = bottomRight;
        }
    }

    // アップロードヒープ上に頂点バッファを作成
    uint32_t vbSize = static_cast<uint32_t>(vertices.size() * sizeof(WaterVertex));
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width            = vbSize;
        resDesc.Height           = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels        = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_vertexBuffer));
        if (FAILED(hr)) return false;

        void* mapped = nullptr;
        m_vertexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, vertices.data(), vbSize);
        m_vertexBuffer->Unmap(0, nullptr);
    }

    m_vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vbView.SizeInBytes    = vbSize;
    m_vbView.StrideInBytes  = sizeof(WaterVertex);

    // アップロードヒープ上にインデックスバッファを作成
    uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        resDesc.Width            = ibSize;
        resDesc.Height           = 1;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels        = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_indexBuffer));
        if (FAILED(hr)) return false;

        void* mapped = nullptr;
        m_indexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, indices.data(), ibSize);
        m_indexBuffer->Unmap(0, nullptr);
    }

    m_ibView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_ibView.SizeInBytes    = ibSize;
    m_ibView.Format         = DXGI_FORMAT_R32_UINT;

    return true;
}

void WaterRenderer::Render(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                            DepthBuffer& depth, const Camera3D& camera, float time)
{
    if (!m_enabled) return;

    // VS/PSでの読み取り用に深度をシェーダーリソースに遷移
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_READ);

    // 専用ヒープ内の深度SRVを更新
    {
        uint32_t slot = frameIndex;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(slot));
    }

    // 定数バッファを構築
    XMMATRIX vpMat = ToXMMATRIX(camera.GetViewProjectionMatrix());

    // ワールド行列: 水面高さに平行移動
    XMMATRIX worldMat = XMMatrixTranslation(0.0f, m_waterLevel, 0.0f);

    // 太陽方向を正規化
    XMVECTOR sunDir = XMVector3Normalize(XMLoadFloat3(XM(&m_sunDirection)));
    Vector3 sunDirF;
    XMStoreFloat3(XM(&sunDirF), sunDir);

    WaterConstants cb = {};
    XMStoreFloat4x4(XM(&cb.viewProjection), XMMatrixTranspose(vpMat));
    XMStoreFloat4x4(XM(&cb.world), XMMatrixTranspose(worldMat));
    cb.cameraPosition = camera.GetPosition();
    cb.time           = time;
    cb.sunDirection   = sunDirF;
    cb.waterLevel     = m_waterLevel;
    cb.sunColor       = m_sunColor;
    cb.planeSize      = m_planeSize;
    cb.waterColor     = m_waterColor;
    cb.windDirection  = m_windDirection;
    cb.waveAmplitude  = m_waveAmplitude;
    cb.waveFrequency  = m_waveFrequency;
    cb.specularPower  = m_specularPower;
    cb.fresnelBias    = m_fresnelBias;
    cb.fresnelPower   = m_fresnelPower;

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &cb, sizeof(cb));
        m_cb.Unmap(frameIndex);
    }

    // パイプラインステートを設定
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(frameIndex));

    // ビューポートとシザーを設定
    D3D12_VIEWPORT vp = {};
    vp.Width    = static_cast<float>(m_screenWidth);
    vp.Height   = static_cast<float>(m_screenHeight);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_screenWidth);
    sc.bottom = static_cast<LONG>(m_screenHeight);
    cmdList->RSSetScissorRects(1, &sc);

    // ジオメトリをバインドして描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 1, &m_vbView);
    cmdList->IASetIndexBuffer(&m_ibView);

    cmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void WaterRenderer::OnResize(ID3D12Device* /*device*/, uint32_t width, uint32_t height)
{
    m_screenWidth  = width;
    m_screenHeight = height;
}

} // namespace gx
