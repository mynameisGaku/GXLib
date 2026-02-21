/// @file PipelineState.cpp
/// @brief パイプラインステートオブジェクト（PSO）ビルダーの実装
#include "pch.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

PipelineStateBuilder::PipelineStateBuilder()
{
    // 全フィールドをゼロ初期化してから妥当なデフォルト値を設定する
    ZeroMemory(&m_desc, sizeof(m_desc));

    // ラスタライザ: ソリッド描画、背面カリング、反時計回りが表面
    m_desc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
    m_desc.RasterizerState.CullMode              = D3D12_CULL_MODE_BACK;
    m_desc.RasterizerState.FrontCounterClockwise = TRUE;
    m_desc.RasterizerState.DepthClipEnable       = TRUE;

    // ブレンド: 不透明描画（ブレンド無効、全チャンネル書き込み）
    m_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // 深度テスト有効、手前のピクセルのみ描画（LESS）
    m_desc.DepthStencilState.DepthEnable    = TRUE;
    m_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    m_desc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;

    // MSAA無効（1サンプル）
    m_desc.SampleMask            = UINT_MAX;
    m_desc.SampleDesc.Count      = 1;
    m_desc.SampleDesc.Quality    = 0;

    m_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // デフォルトはLDR形式。HDRパイプラインではR16G16B16A16_FLOATに変更が必要
    m_desc.NumRenderTargets = 1;
    m_desc.RTVFormats[0]    = DXGI_FORMAT_R8G8B8A8_UNORM;
}

PipelineStateBuilder& PipelineStateBuilder::SetRootSignature(ID3D12RootSignature* rootSignature)
{
    m_desc.pRootSignature = rootSignature;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetVertexShader(D3D12_SHADER_BYTECODE bytecode)
{
    m_desc.VS = bytecode;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetPixelShader(D3D12_SHADER_BYTECODE bytecode)
{
    m_desc.PS = bytecode;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* elements, uint32_t count)
{
    m_desc.InputLayout.pInputElementDescs = elements;
    m_desc.InputLayout.NumElements        = count;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetRenderTargetFormat(DXGI_FORMAT format, uint32_t index)
{
    m_desc.RTVFormats[index] = format;
    m_desc.BlendState.RenderTarget[index].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    if (index + 1 > m_desc.NumRenderTargets)
        m_desc.NumRenderTargets = index + 1;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthFormat(DXGI_FORMAT format)
{
    m_desc.DSVFormat = format;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
    m_desc.PrimitiveTopologyType = type;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetCullMode(D3D12_CULL_MODE mode)
{
    m_desc.RasterizerState.CullMode = mode;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthEnable(bool enable)
{
    m_desc.DepthStencilState.DepthEnable = enable ? TRUE : FALSE;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask)
{
    m_desc.DepthStencilState.DepthWriteMask = mask;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthBias(int bias, float clamp, float slopeScaledBias)
{
    m_desc.RasterizerState.DepthBias = bias;
    m_desc.RasterizerState.DepthBiasClamp = clamp;
    m_desc.RasterizerState.SlopeScaledDepthBias = slopeScaledBias;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetDepthComparisonFunc(D3D12_COMPARISON_FUNC func)
{
    m_desc.DepthStencilState.DepthFunc = func;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetRenderTargetCount(uint32_t count)
{
    m_desc.NumRenderTargets = count;
    if (count == 0)
    {
        // 深度のみ描画（シャドウマップ等）ではRT数0にし、フォーマットをクリアする
        for (int i = 0; i < 8; ++i)
            m_desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    }
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetFillMode(D3D12_FILL_MODE mode)
{
    m_desc.RasterizerState.FillMode = mode;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetBlendState(const D3D12_BLEND_DESC& blendDesc)
{
    m_desc.BlendState = blendDesc;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetAlphaBlend()
{
    // Result = Src.rgb * Src.a + Dest.rgb * (1 - Src.a)
    auto& rt = m_desc.BlendState.RenderTarget[0];
    rt.BlendEnable           = TRUE;
    rt.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOp               = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
    rt.DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetAdditiveBlend()
{
    // Result = Src.rgb * Src.a + Dest.rgb（光の重ね合わせ、パーティクル等）
    auto& rt = m_desc.BlendState.RenderTarget[0];
    rt.BlendEnable           = TRUE;
    rt.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend             = D3D12_BLEND_ONE;
    rt.BlendOp               = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
    rt.DestBlendAlpha        = D3D12_BLEND_ZERO;
    rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetSubtractiveBlend()
{
    // Result = Dest.rgb - Src.rgb * Src.a（REV_SUBTRACTなので Dest - Src の順）
    auto& rt = m_desc.BlendState.RenderTarget[0];
    rt.BlendEnable           = TRUE;
    rt.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend             = D3D12_BLEND_ONE;
    rt.BlendOp               = D3D12_BLEND_OP_REV_SUBTRACT;
    rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
    rt.DestBlendAlpha        = D3D12_BLEND_ZERO;
    rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return *this;
}

PipelineStateBuilder& PipelineStateBuilder::SetMultiplyBlend()
{
    // Result = Dest.rgb * Src.rgb（0*Src + Dest*SrcColor で乗算を実現）
    auto& rt = m_desc.BlendState.RenderTarget[0];
    rt.BlendEnable           = TRUE;
    rt.SrcBlend              = D3D12_BLEND_ZERO;
    rt.DestBlend             = D3D12_BLEND_SRC_COLOR;
    rt.BlendOp               = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
    rt.DestBlendAlpha        = D3D12_BLEND_ZERO;
    rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return *this;
}

ComPtr<ID3D12PipelineState> PipelineStateBuilder::Build(ID3D12Device* device)
{
    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device->CreateGraphicsPipelineState(&m_desc, IID_PPV_ARGS(&pso));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create pipeline state (HRESULT: 0x%08X)", hr);
        return nullptr;
    }

    GX_LOG_INFO("Pipeline State Object created");
    return pso;
}

} // namespace GX
