/// @file MeshPipeline.cpp
/// @brief メッシュシェーダパイプラインの実装
#include "pch_graphics.h"
#include "Graphics/Pipeline/MeshPipeline.h"
#include "Core/Logger.h"

namespace gx
{

// --- ストリームベースPSO用の構造体定義 ---
// D3D12のメッシュシェーダPSOはD3D12_PIPELINE_STATE_STREAM_DESCで作成する。
// 各フィールドはアラインメント付きサブオブジェクトとして順に並べる。

struct alignas(void*) PSO_RootSignature
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
    ID3D12RootSignature* rootSignature = nullptr;
};

struct alignas(void*) PSO_AS
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS;
    D3D12_SHADER_BYTECODE bytecode = {};
};

struct alignas(void*) PSO_MS
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS;
    D3D12_SHADER_BYTECODE bytecode = {};
};

struct alignas(void*) PSO_PS
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
    D3D12_SHADER_BYTECODE bytecode = {};
};

struct alignas(void*) PSO_RasterizerState
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
    D3D12_RASTERIZER_DESC desc = {};
};

struct alignas(void*) PSO_BlendState
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
    D3D12_BLEND_DESC desc = {};
};

struct alignas(void*) PSO_DepthStencilState
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
    D3D12_DEPTH_STENCIL_DESC desc = {};
};

struct alignas(void*) PSO_RTVFormats
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
    D3D12_RT_FORMAT_ARRAY formats = {};
};

struct alignas(void*) PSO_DSVFormat
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};

struct alignas(void*) PSO_SampleDesc
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
    DXGI_SAMPLE_DESC desc = { 1, 0 };
};

struct alignas(void*) PSO_SampleMask
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK;
    UINT mask = UINT_MAX;
};

/// @brief メッシュシェーダPSO用のストリーム構造体
struct MeshPSOStream
{
    PSO_RootSignature    rootSignature;
    PSO_AS               as;
    PSO_MS               ms;
    PSO_PS               ps;
    PSO_RasterizerState  rasterizer;
    PSO_BlendState       blend;
    PSO_DepthStencilState depthStencil;
    PSO_RTVFormats       rtvFormats;
    PSO_DSVFormat        dsvFormat;
    PSO_SampleDesc       sampleDesc;
    PSO_SampleMask       sampleMask;
};

MeshShaderTier MeshPipeline::DetectTier(ID3D12Device* device)
{
    if (!device)
        return MeshShaderTier::None;

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    HRESULT hr = device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

    if (SUCCEEDED(hr) && options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
    {
        GX_LOG_INFO("Mesh Shader Tier 1 supported");
        return MeshShaderTier::Tier1;
    }

    GX_LOG_INFO("Mesh Shader not supported on this GPU");
    return MeshShaderTier::None;
}

ComPtr<ID3D12PipelineState> MeshPipeline::Build(ID3D12Device2* device,
                                                 const MeshPipelineDesc& desc)
{
    if (!device)
    {
        GX_LOG_ERROR("MeshPipeline::Build: device is null");
        return nullptr;
    }

    if (!desc.msBlob || desc.msBlobSize == 0)
    {
        GX_LOG_ERROR("MeshPipeline::Build: Mesh Shader bytecode is required");
        return nullptr;
    }

    if (!desc.rootSignature)
    {
        GX_LOG_ERROR("MeshPipeline::Build: Root Signature is required");
        return nullptr;
    }

    // ストリームベースPSOを構築
    MeshPSOStream stream = {};

    // ルートシグネチャ
    stream.rootSignature.rootSignature = desc.rootSignature;

    // Amplification Shader (オプション)
    if (desc.asBlob && desc.asBlobSize > 0)
    {
        stream.as.bytecode.pShaderBytecode = desc.asBlob;
        stream.as.bytecode.BytecodeLength  = desc.asBlobSize;
    }

    // Mesh Shader (必須)
    stream.ms.bytecode.pShaderBytecode = desc.msBlob;
    stream.ms.bytecode.BytecodeLength  = desc.msBlobSize;

    // Pixel Shader (オプション)
    if (desc.psBlob && desc.psBlobSize > 0)
    {
        stream.ps.bytecode.pShaderBytecode = desc.psBlob;
        stream.ps.bytecode.BytecodeLength  = desc.psBlobSize;
    }

    // ラスタライザステート
    stream.rasterizer.desc.FillMode = D3D12_FILL_MODE_SOLID;
    stream.rasterizer.desc.CullMode = D3D12_CULL_MODE_BACK;
    stream.rasterizer.desc.FrontCounterClockwise = FALSE;
    stream.rasterizer.desc.DepthBias = 0;
    stream.rasterizer.desc.DepthBiasClamp = 0.0f;
    stream.rasterizer.desc.SlopeScaledDepthBias = 0.0f;
    stream.rasterizer.desc.DepthClipEnable = TRUE;

    // ブレンドステート（不透明描画用デフォルト）
    stream.blend.desc.AlphaToCoverageEnable = FALSE;
    stream.blend.desc.IndependentBlendEnable = FALSE;
    stream.blend.desc.RenderTarget[0].BlendEnable = FALSE;
    stream.blend.desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // 深度ステンシルステート
    stream.depthStencil.desc.DepthEnable = TRUE;
    stream.depthStencil.desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    stream.depthStencil.desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    stream.depthStencil.desc.StencilEnable = FALSE;

    // レンダーターゲットフォーマット
    stream.rtvFormats.formats.NumRenderTargets = desc.numRenderTargets;
    for (uint32_t i = 0; i < desc.numRenderTargets; ++i)
    {
        stream.rtvFormats.formats.RTFormats[i] = desc.rtvFormat;
    }

    // 深度フォーマット
    stream.dsvFormat.format = desc.dsvFormat;

    // PSO作成
    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
    streamDesc.SizeInBytes                      = sizeof(stream);
    streamDesc.pPipelineStateSubobjectStream    = &stream;

    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create Mesh Shader PSO (HRESULT: 0x%08X)", hr);
        return nullptr;
    }

    GX_LOG_INFO("Mesh Shader PSO created successfully");
    return pso;
}

void MeshPipeline::DispatchMesh(ID3D12GraphicsCommandList6* cmdList,
                                 uint32_t x, uint32_t y, uint32_t z)
{
    if (!cmdList)
        return;

    cmdList->DispatchMesh(x, y, z);
}

} // namespace gx
