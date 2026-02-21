/// @file MaskScreen.cpp
/// @brief DXLib互換マスクスクリーンの実装
#include "pch.h"
#include "Graphics/Layer/MaskScreen.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace GX
{

bool MaskScreen::Create(ID3D12Device* device, uint32_t w, uint32_t h)
{
    m_device = device;
    m_width  = w;
    m_height = h;

    // マスクレイヤー (R8G8B8A8_UNORM — RenderLayerの標準フォーマット)
    // Rチャンネルのみをマスク値として使用
    if (!m_maskLayer.Create(device, "_Mask", -1, w, h))
        return false;

    // シェーダー
    if (!m_shader.Initialize())
        return false;

    // 定数バッファ (256Bアライン)
    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    // 頂点バッファ (矩形=6頂点*8byte=48, 円=64seg*3vert*8byte=1536 → 2048バイト)
    if (!m_vertexBuffer.Initialize(device, 2048, sizeof(float) * 2))
        return false;

    // ルートシグネチャ: [0]=CBV(b0)
    {
        RootSignatureBuilder rsb;
        rsb.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        m_rootSignature = rsb.AddCBV(0).Build(device);
        if (!m_rootSignature) return false;
    }

    if (!CreatePipelines(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/MaskDraw.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("MaskScreen created (%dx%d)", w, h);
    return true;
}

bool MaskScreen::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/MaskDraw.hlsl", L"VSMask", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/MaskDraw.hlsl", L"PSMask", L"ps_6_0");
    if (!ps.valid) return false;

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    PipelineStateBuilder b;
    m_fillPSO = b.SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vs.GetBytecode())
        .SetPixelShader(ps.GetBytecode())
        .SetInputLayout(inputLayout, _countof(inputLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .Build(device);
    if (!m_fillPSO) return false;

    return true;
}

void MaskScreen::Clear(ID3D12GraphicsCommandList* cmdList, uint8_t fill)
{
    float v = static_cast<float>(fill) / 255.0f;
    m_maskLayer.Clear(cmdList, v, 0.0f, 0.0f, 1.0f);
}

void MaskScreen::SetupPipeline(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, float maskValue)
{
    // RT → RENDER_TARGET
    m_maskLayer.GetRenderTarget().TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto rtvHandle = m_maskLayer.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width    = static_cast<float>(m_width);
    vp.Height   = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_fillPSO.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // 正射影行列 (ピクセル座標系)
    MaskConstants mc = {};
    XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(
        0.0f, static_cast<float>(m_width),
        static_cast<float>(m_height), 0.0f,
        0.0f, 1.0f);
    XMStoreFloat4x4(&mc.projection, XMMatrixTranspose(ortho));
    mc.maskValue = maskValue;

    void* p = m_constantBuffer.Map(frameIndex);
    if (p)
    {
        memcpy(p, &mc, sizeof(mc));
        m_constantBuffer.Unmap(frameIndex);
    }
    cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(frameIndex));
}

void MaskScreen::DrawFillRect(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                float x, float y, float w, float h, float value)
{
    SetupPipeline(cmdList, frameIndex, value);

    // 矩形 = 2三角形 (6頂点)
    struct Vertex { float x, y; };
    Vertex verts[6] = {
        { x,     y     },
        { x + w, y     },
        { x,     y + h },
        { x + w, y     },
        { x + w, y + h },
        { x,     y + h },
    };

    void* p = m_vertexBuffer.Map(frameIndex);
    if (p)
    {
        memcpy(p, verts, sizeof(verts));
        m_vertexBuffer.Unmap(frameIndex);
    }

    auto vbv = m_vertexBuffer.GetVertexBufferView(frameIndex, sizeof(verts));
    cmdList->IASetVertexBuffers(0, 1, &vbv);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(6, 1, 0, 0);
}

void MaskScreen::DrawCircle(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                              float cx, float cy, float radius, float value)
{
    SetupPipeline(cmdList, frameIndex, value);

    // 円 = 中心頂点から放射状に三角形を並べるファン方式 (64セグメント)
    static constexpr int k_Segments = 64;
    struct Vertex { float x, y; };
    Vertex verts[k_Segments * 3];

    for (int i = 0; i < k_Segments; ++i)
    {
        float a0 = XM_2PI * static_cast<float>(i)     / static_cast<float>(k_Segments);
        float a1 = XM_2PI * static_cast<float>(i + 1) / static_cast<float>(k_Segments);
        verts[i * 3 + 0] = { cx, cy };
        verts[i * 3 + 1] = { cx + radius * cosf(a0), cy + radius * sinf(a0) };
        verts[i * 3 + 2] = { cx + radius * cosf(a1), cy + radius * sinf(a1) };
    }

    void* p = m_vertexBuffer.Map(frameIndex);
    if (p)
    {
        memcpy(p, verts, sizeof(verts));
        m_vertexBuffer.Unmap(frameIndex);
    }

    auto vbv = m_vertexBuffer.GetVertexBufferView(frameIndex, sizeof(verts));
    cmdList->IASetVertexBuffers(0, 1, &vbv);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(k_Segments * 3, 1, 0, 0);
}

void MaskScreen::OnResize(ID3D12Device* device, uint32_t w, uint32_t h)
{
    m_width  = w;
    m_height = h;
    m_maskLayer.OnResize(device, w, h);
}

} // namespace GX
