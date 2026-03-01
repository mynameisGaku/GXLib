/// @file GPUDrivenRenderer.cpp
/// @brief GPUドリブンレンダリングの実装
#include "pch_graphics.h"
#include "Graphics/3D/GPUDrivenRenderer.h"
#include "Core/Logger.h"
#include <cstring>

namespace gx
{

bool GPUDrivenRenderer::Initialize(ID3D12Device* device, ID3D12Device2* device2)
{
    if (!device)
    {
        GX_LOG_ERROR("GPUDrivenRenderer::Initialize: device is null");
        return false;
    }

    m_available = false;
    m_tier = MeshShaderTier::None;
    m_meshletCount = 0;

    // メッシュシェーダの対応レベルを検出
    m_tier = MeshPipeline::DetectTier(device);

    if (m_tier == MeshShaderTier::None)
    {
        GX_LOG_INFO("GPUDrivenRenderer: Mesh Shader not available -- GPU-driven rendering disabled");
        return true; // 非対応でも初期化自体は成功
    }

    if (!device2)
    {
        GX_LOG_WARN("GPUDrivenRenderer: ID3D12Device2 not available -- cannot create stream PSO");
        return true;
    }

    m_available = true;
    m_device2 = device2;
    GX_LOG_INFO("GPUDrivenRenderer initialized -- Mesh Shader GPU-driven rendering available");

    // --- シェーダコンパイル + PSO作成 ---
    if (!m_shader.Initialize())
    {
        GX_LOG_WARN("GPUDrivenRenderer: DXC compiler initialization failed -- PSO not created");
        return true;
    }

    auto asBlob = m_shader.CompileFromFile(L"Shaders/MeshCull.hlsl", L"ASMain", L"as_6_5");
    auto msBlob = m_shader.CompileFromFile(L"Shaders/MeshCull.hlsl", L"MSMain", L"ms_6_5");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/MeshCull.hlsl", L"PSMain", L"ps_6_0");

    if (!asBlob.valid || !msBlob.valid || !psBlob.valid)
    {
        GX_LOG_WARN("GPUDrivenRenderer: Shader compilation failed (shader may not exist yet)");
        return true; // シェーダがなくても初期化は成功
    }

    // ルートシグネチャ: b0 = 32bit定数(20 DWORDs: viewProj 16 + meshletCount 1 + padding 3), t0 = SRVテーブル
    D3D12_ROOT_PARAMETER rootParams[2] = {};

    // param[0]: 32ビット定数 (b0) — 20 DWORD
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParams[0].Constants.ShaderRegister = 0;
    rootParams[0].Constants.RegisterSpace  = 0;
    rootParams[0].Constants.Num32BitValues = 20;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // param[1]: SRVディスクリプタテーブル (t0) — メッシュレットバッファ
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors     = 1;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace      = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges   = &srvRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 2;
    rsDesc.pParameters   = rootParams;
    rsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              &sigBlob, &errBlob);
    if (FAILED(hr))
    {
        GX_LOG_WARN("GPUDrivenRenderer: Failed to serialize root signature (0x%08X)", hr);
        return true;
    }

    hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        GX_LOG_WARN("GPUDrivenRenderer: Failed to create root signature (0x%08X)", hr);
        return true;
    }

    // MeshPipelineDescを構築してPSOを作成
    auto asBytecode = asBlob.GetBytecode();
    auto msBytecode = msBlob.GetBytecode();
    auto psBytecode = psBlob.GetBytecode();

    MeshPipelineDesc meshDesc;
    meshDesc.asBlob         = asBytecode.pShaderBytecode;
    meshDesc.asBlobSize     = static_cast<uint32_t>(asBytecode.BytecodeLength);
    meshDesc.msBlob         = msBytecode.pShaderBytecode;
    meshDesc.msBlobSize     = static_cast<uint32_t>(msBytecode.BytecodeLength);
    meshDesc.psBlob         = psBytecode.pShaderBytecode;
    meshDesc.psBlobSize     = static_cast<uint32_t>(psBytecode.BytecodeLength);
    meshDesc.rootSignature  = m_rootSignature.Get();

    m_meshPSO = m_meshPipeline.Build(device2, meshDesc);
    if (!m_meshPSO)
    {
        GX_LOG_WARN("GPUDrivenRenderer: Failed to build mesh shader PSO");
        return true;
    }

    m_psoReady = true;
    GX_LOG_INFO("GPUDrivenRenderer: Mesh shader PSO created successfully");

    return true;
}

void GPUDrivenRenderer::UploadMeshlets(ID3D12Device* device,
                                        const MeshletData* data, uint32_t count)
{
    if (!m_available)
    {
        GX_LOG_WARN("GPUDrivenRenderer::UploadMeshlets: GPU-driven rendering not available");
        return;
    }

    if (!device || !data || count == 0)
    {
        GX_LOG_ERROR("GPUDrivenRenderer::UploadMeshlets: invalid parameters");
        return;
    }

    // メッシュレットデータを構造化バッファとしてアップロード
    uint32_t bufferSize = count * static_cast<uint32_t>(sizeof(MeshletData));

    // UPLOADヒープに頂点バッファとして作成（構造化バッファとして使う）
    // 本来はDEFAULTヒープ + コピーが望ましいが、簡略化のためUPLOADを使用
    bool result = m_meshletBuffer.CreateVertexBuffer(
        device, data, bufferSize, static_cast<uint32_t>(sizeof(MeshletData)));

    if (!result)
    {
        GX_LOG_ERROR("Failed to create meshlet buffer");
        return;
    }

    m_meshletCount = count;
    GX_LOG_INFO("Uploaded %u meshlets (%u bytes)", count, bufferSize);
}

void GPUDrivenRenderer::Render(ID3D12GraphicsCommandList6* cmdList, uint32_t frameIndex,
                                const DirectX::XMFLOAT4X4& viewProj, uint32_t meshletCount)
{
    if (!m_available || !cmdList)
        return;

    if (meshletCount == 0 || m_meshletCount == 0)
        return;

    // メッシュレット数を実際のアップロード済み数にクランプ
    uint32_t dispatchCount = (meshletCount < m_meshletCount) ? meshletCount : m_meshletCount;

    (void)frameIndex;

    if (!m_psoReady)
    {
        // PSO未構築の場合はスタブログのみ
        GX_LOG_INFO("GPUDrivenRenderer: dispatch %u meshlets (stub -- PSO not ready)", dispatchCount);
        return;
    }

    // 1. PSO + ルートシグネチャをセット
    cmdList->SetPipelineState(m_meshPSO.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // 2. 32bit定数をセット: viewProj行列(16 floats) + meshletCount(1 uint) + padding(3)
    struct
    {
        float viewProj[16];
        uint32_t meshletCount;
        uint32_t pad[3];
    } constants = {};

    // XMFLOAT4X4は行優先4x4 = 16 floats
    std::memcpy(constants.viewProj, &viewProj, sizeof(float) * 16);
    constants.meshletCount = dispatchCount;

    cmdList->SetGraphicsRoot32BitConstants(0, 20, &constants, 0);

    // 3. メッシュレット1つにつき1スレッドグループとしてDispatchMesh
    MeshPipeline::DispatchMesh(cmdList, dispatchCount, 1, 1);
}

void GPUDrivenRenderer::CreateIndirectPipeline(ID3D12Device* device)
{
    if (!device)
    {
        GX_LOG_ERROR("GPUDrivenRenderer::CreateIndirectPipeline: device is null");
        return;
    }

    // Create command signature for ExecuteIndirect
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
    sigDesc.ByteStride = sizeof(GPUDrawCommand);
    sigDesc.NumArgumentDescs = 1;
    sigDesc.pArgumentDescs = &argDesc;

    HRESULT hr = device->CreateCommandSignature(&sigDesc, nullptr, IID_PPV_ARGS(&m_commandSignature));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUDrivenRenderer: Failed to create command signature (0x%08X)", hr);
        return;
    }

    // Create cull compute root signature: b0 = constants, t0 = bounds SRV, u0 = output UAV
    D3D12_ROOT_PARAMETER cullParams[3] = {};

    cullParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    cullParams[0].Constants.ShaderRegister = 0;
    cullParams[0].Constants.Num32BitValues = 20; // viewProj(16) + objectCount(1) + pad(3)
    cullParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    cullParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    cullParams[1].DescriptorTable.NumDescriptorRanges = 1;
    cullParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    cullParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = 0;
    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    cullParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    cullParams[2].DescriptorTable.NumDescriptorRanges = 1;
    cullParams[2].DescriptorTable.pDescriptorRanges = &uavRange;
    cullParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC cullRsDesc = {};
    cullRsDesc.NumParameters = 3;
    cullRsDesc.pParameters = cullParams;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    hr = D3D12SerializeRootSignature(&cullRsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    if (SUCCEEDED(hr))
    {
        device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_cullRootSignature));
    }

    m_indirectReady = (m_commandSignature != nullptr);
    if (m_indirectReady)
        GX_LOG_INFO("GPUDrivenRenderer: Indirect pipeline created successfully");
}

void GPUDrivenRenderer::UploadMeshletBounds(ID3D12Device* device, const MeshletBounds* bounds, uint32_t count)
{
    if (!device || !bounds || count == 0)
    {
        GX_LOG_ERROR("GPUDrivenRenderer::UploadMeshletBounds: invalid parameters");
        return;
    }

    uint32_t bufferSize = count * static_cast<uint32_t>(sizeof(MeshletBounds));
    m_boundsBuffer.CreateVertexBuffer(device, bounds, bufferSize, static_cast<uint32_t>(sizeof(MeshletBounds)));
    m_boundsCount = count;
    GX_LOG_INFO("Uploaded %u meshlet bounds", count);
}

void GPUDrivenRenderer::CullAndDraw(ID3D12GraphicsCommandList* cmdList,
                                      const XMFLOAT4X4& viewProj, uint32_t objectCount)
{
    if (!cmdList || !m_indirectReady || objectCount == 0)
        return;

    m_visibleCount = objectCount; // Without actual GPU cull, all pass

    if (!m_commandSignature || !m_indirectArgBuffer)
    {
        GX_LOG_INFO("GPUDrivenRenderer::CullAndDraw: %u objects (stub - no GPU resources)", objectCount);
        return;
    }

    // UAV barrier before execute
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_indirectArgBuffer.Get();
    cmdList->ResourceBarrier(1, &barrier);

    // ExecuteIndirect
    cmdList->ExecuteIndirect(m_commandSignature.Get(), objectCount,
                              m_indirectArgBuffer.Get(), 0, nullptr, 0);
}

} // namespace gx
