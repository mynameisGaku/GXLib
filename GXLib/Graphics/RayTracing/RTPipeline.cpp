/// @file RTPipeline.cpp
/// @brief DXR State Object + Shader Table の実装
#include "pch.h"
#include "Graphics/RayTracing/RTPipeline.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Core/Logger.h"

namespace GX
{

// シェーダーテーブルのアライメント定数
static constexpr uint32_t k_ShaderTableAlignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT; // 64
static constexpr uint32_t k_ShaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT; // 32

/// アライメントヘルパー
static uint32_t AlignUp(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

bool RTPipeline::Initialize(ID3D12Device5* device,
                             const wchar_t* shaderPath,
                             const wchar_t* rayGenExport,
                             const wchar_t* closestHitExport,
                             const wchar_t* missExport,
                             const wchar_t* shadowMissExport,
                             const wchar_t* hitGroupName)
{
    m_device = device;
    m_shaderPath        = shaderPath;
    m_rayGenExport      = rayGenExport;
    m_closestHitExport  = closestHitExport;
    m_missExport        = missExport;
    m_shadowMissExport  = shadowMissExport;
    m_hitGroupName      = hitGroupName;

    if (!m_shader.Initialize())
        return false;

    if (!CreateGlobalRootSignature(device))
        return false;

    if (!CreateLocalRootSignatures(device))
        return false;

    if (!CreateStateObject(device))
        return false;

    if (!CreateShaderTable(device))
        return false;

    GX_LOG_INFO("RTPipeline initialized");
    return true;
}

bool RTPipeline::CreateGlobalRootSignature(ID3D12Device5* device)
{
    // Global Root Signature:
    // [0] CBV  b0     — RTReflectionConstants
    // [1] Root SRV t0 — TLAS (GPU VA直接バインド)
    // [2] DescTable   — t1, t2, t3 (Scene HDR + Depth + Normal)
    // [3] DescTable   — u0     (Output UAV)
    // [4] CBV  b1     — InstanceData (per-instance albedo/metallic/roughness/geomIdx)
    // [5] DescTable   — t0..t31 in space1 (VB/IB ByteAddressBuffers, 最大16ジオメトリ×2)
    // [6] DescTable   — t0..t31 in space2 (Albedo Texture2D配列)
    // [7] CBV  b2     — LightConstants (PBR.hlsl b2と同一構造体)
    // s0: Linear Clamp
    // s1: Point Clamp

    constexpr auto kVolatile = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    RootSignatureBuilder rsb;
    m_globalRS = rsb
        .AddCBV(0)
        .AddSRV(0) // Root SRV for TLAS
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0,
                            D3D12_SHADER_VISIBILITY_ALL, kVolatile) // t1, t2, t3
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0,
                            D3D12_SHADER_VISIBILITY_ALL, kVolatile) // u0
        .AddCBV(1) // Instance data (albedo/metallic/roughness/geomIdx)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 32, 1,
                            D3D12_SHADER_VISIBILITY_ALL, kVolatile) // t0..t31 in space1 (geometry VB/IB)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 32, 2,
                            D3D12_SHADER_VISIBILITY_ALL, kVolatile) // t0..t31 in space2 (albedo textures)
        .AddCBV(2) // LightConstants (PBR.hlsl b2と同一)
        .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                          D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                          D3D12_COMPARISON_FUNC_NEVER)
        .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
                          D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                          D3D12_COMPARISON_FUNC_NEVER)
        .Build(device);

    if (!m_globalRS)
    {
        GX_LOG_ERROR("RTPipeline: Failed to create global root signature");
        return false;
    }
    return true;
}

bool RTPipeline::CreateLocalRootSignatures(ID3D12Device5* device)
{
    // 空の Local RS を 2 つ作成 (DXR spec推奨: 全シェーダーに明示的 Local RS association)
    // RayGen 用
    {
        RootSignatureBuilder rsb;
        m_rayGenLocalRS = rsb
            .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)
            .Build(device);
        if (!m_rayGenLocalRS)
        {
            GX_LOG_ERROR("RTPipeline: Failed to create RayGen local root signature");
            return false;
        }
    }

    // Hit/Miss 用
    {
        RootSignatureBuilder rsb;
        m_hitMissLocalRS = rsb
            .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)
            .Build(device);
        if (!m_hitMissLocalRS)
        {
            GX_LOG_ERROR("RTPipeline: Failed to create Hit/Miss local root signature");
            return false;
        }
    }

    GX_LOG_INFO("RTPipeline: Local root signatures created");
    return true;
}

bool RTPipeline::CreateStateObject(ID3D12Device5* device)
{
    // DXIL ライブラリをコンパイル
    auto libBlob = m_shader.CompileLibrary(m_shaderPath.c_str());
    if (!libBlob.valid)
    {
        GX_LOG_ERROR("RTPipeline: Failed to compile RT shader library: %ls", m_shaderPath.c_str());
        return false;
    }

    // State Object: 10 サブオブジェクト構成
    // [0] DXILライブラリ → [1] HitGroup → [2-3] ShaderConfig+Association
    // → [4] PipelineConfig → [5] GlobalRS → [6-9] LocalRS+Association×2
    D3D12_STATE_SUBOBJECT subobjects[10] = {};

    // === [0] DXIL Library ===
    D3D12_DXIL_LIBRARY_DESC libDesc = {};
    libDesc.DXILLibrary = libBlob.GetBytecode();
    D3D12_EXPORT_DESC exports[] = {
        { m_rayGenExport.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE },
        { m_closestHitExport.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE },
        { m_missExport.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE },
        { m_shadowMissExport.c_str(), nullptr, D3D12_EXPORT_FLAG_NONE },
    };
    libDesc.NumExports   = _countof(exports);
    libDesc.pExports     = exports;

    subobjects[0].Type  = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[0].pDesc = &libDesc;

    // === [1] Hit Group ===
    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.HitGroupExport           = m_hitGroupName.c_str();
    hitGroupDesc.Type                     = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.ClosestHitShaderImport   = m_closestHitExport.c_str();
    hitGroupDesc.AnyHitShaderImport       = nullptr;
    hitGroupDesc.IntersectionShaderImport = nullptr;

    subobjects[1].Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    subobjects[1].pDesc = &hitGroupDesc;

    // === [2] Shader Config (payload + attribute sizes) ===
    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxPayloadSizeInBytes   = sizeof(float) * 4;  // ReflectionPayload: float4
    shaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2;  // BuiltInTriangleIntersectionAttributes

    subobjects[2].Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    subobjects[2].pDesc = &shaderConfig;

    // === [3] Shader Config Association (全シェーダーに適用) ===
    const WCHAR* shaderExports[] = {
        m_rayGenExport.c_str(), m_closestHitExport.c_str(),
        m_missExport.c_str(), m_shadowMissExport.c_str()
    };
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION configAssociation = {};
    configAssociation.pSubobjectToAssociate = &subobjects[2];
    configAssociation.NumExports = _countof(shaderExports);
    configAssociation.pExports   = shaderExports;

    subobjects[3].Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    subobjects[3].pDesc = &configAssociation;

    // === [4] Pipeline Config (MaxRecursion=2: 反射レイ + シャドウレイ) ===
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = 2;

    subobjects[4].Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    subobjects[4].pDesc = &pipelineConfig;

    // === [5] Global Root Signature ===
    D3D12_GLOBAL_ROOT_SIGNATURE globalRSDesc = {};
    globalRSDesc.pGlobalRootSignature = m_globalRS.Get();

    subobjects[5].Type  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    subobjects[5].pDesc = &globalRSDesc;

    // === [6] Local RS (RayGen) — 空 ===
    D3D12_LOCAL_ROOT_SIGNATURE rayGenLocalRSDesc = {};
    rayGenLocalRSDesc.pLocalRootSignature = m_rayGenLocalRS.Get();

    subobjects[6].Type  = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    subobjects[6].pDesc = &rayGenLocalRSDesc;

    // === [7] Association: [6] → {RayGen} ===
    const WCHAR* rayGenExports[] = { m_rayGenExport.c_str() };
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rayGenAssociation = {};
    rayGenAssociation.pSubobjectToAssociate = &subobjects[6];
    rayGenAssociation.NumExports = _countof(rayGenExports);
    rayGenAssociation.pExports   = rayGenExports;

    subobjects[7].Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    subobjects[7].pDesc = &rayGenAssociation;

    // === [8] Local RS (Hit/Miss) — 空 ===
    D3D12_LOCAL_ROOT_SIGNATURE hitMissLocalRSDesc = {};
    hitMissLocalRSDesc.pLocalRootSignature = m_hitMissLocalRS.Get();

    subobjects[8].Type  = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
    subobjects[8].pDesc = &hitMissLocalRSDesc;

    // === [9] Association: [8] → {Miss, ShadowMiss, ClosestHit} ===
    const WCHAR* hitMissExports[] = {
        m_missExport.c_str(), m_shadowMissExport.c_str(), m_closestHitExport.c_str()
    };
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION hitMissAssociation = {};
    hitMissAssociation.pSubobjectToAssociate = &subobjects[8];
    hitMissAssociation.NumExports = _countof(hitMissExports);
    hitMissAssociation.pExports   = hitMissExports;

    subobjects[9].Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    subobjects[9].pDesc = &hitMissAssociation;

    // State Object を作成
    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = _countof(subobjects);
    stateObjectDesc.pSubobjects   = subobjects;

    HRESULT hr = device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_stateObject));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("RTPipeline: Failed to create state object (HRESULT: 0x%08X)", hr);
        return false;
    }

    // StateObjectProperties を取得（シェーダーIDルックアップ用）
    hr = m_stateObject.As(&m_stateObjectProperties);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("RTPipeline: Failed to query state object properties");
        return false;
    }

    GX_LOG_INFO("RTPipeline: State object created (10 subobjects, 8 root params, 2 samplers)");
    return true;
}

bool RTPipeline::CreateShaderTable(ID3D12Device5* device)
{
    // StateObjectPropertiesからシェーダーID (各32バイト) を取得し、
    // テーブルバッファに書き込む。Local RSパラメータがないので各レコードは32バイト固定
    void* rayGenID     = m_stateObjectProperties->GetShaderIdentifier(m_rayGenExport.c_str());
    void* missID       = m_stateObjectProperties->GetShaderIdentifier(m_missExport.c_str());
    void* shadowMissID = m_stateObjectProperties->GetShaderIdentifier(m_shadowMissExport.c_str());
    void* hitGroupID   = m_stateObjectProperties->GetShaderIdentifier(m_hitGroupName.c_str());

    if (!rayGenID || !missID || !shadowMissID || !hitGroupID)
    {
        GX_LOG_ERROR("RTPipeline: Failed to get shader identifiers");
        return false;
    }

    uint32_t shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; // 32

    // レコードサイズ計算 (ID + ルートテーブルパラメータ — ここでは追加データなし)
    m_rayGenRecordSize   = AlignUp(shaderIDSize, k_ShaderRecordAlignment);
    m_missRecordSize     = AlignUp(shaderIDSize, k_ShaderRecordAlignment);
    m_hitGroupRecordSize = AlignUp(shaderIDSize, k_ShaderRecordAlignment);

    // RayGen テーブル (1レコード)
    uint32_t rayGenTableSize = AlignUp(m_rayGenRecordSize, k_ShaderTableAlignment);
    if (!m_rayGenShaderTable.CreateUploadBufferEmpty(device, rayGenTableSize))
        return false;

    {
        void* p = m_rayGenShaderTable.Map();
        memcpy(p, rayGenID, shaderIDSize);
        m_rayGenShaderTable.Unmap();
    }

    // Miss テーブル (2レコード: [0]=反射Miss, [1]=シャドウMiss)
    // TraceRay()のMissShaderIndex引数で選択される (0=反射, 1=シャドウ)
    uint32_t missTableSize = AlignUp(m_missRecordSize * 2, k_ShaderTableAlignment);
    if (!m_missShaderTable.CreateUploadBufferEmpty(device, missTableSize))
        return false;

    {
        void* p = m_missShaderTable.Map();
        memcpy(p, missID, shaderIDSize);
        memcpy(static_cast<uint8_t*>(p) + m_missRecordSize, shadowMissID, shaderIDSize);
        m_missShaderTable.Unmap();
    }

    // HitGroup テーブル (1レコード)
    uint32_t hitGroupTableSize = AlignUp(m_hitGroupRecordSize, k_ShaderTableAlignment);
    if (!m_hitGroupShaderTable.CreateUploadBufferEmpty(device, hitGroupTableSize))
        return false;

    {
        void* p = m_hitGroupShaderTable.Map();
        memcpy(p, hitGroupID, shaderIDSize);
        m_hitGroupShaderTable.Unmap();
    }

    GX_LOG_INFO("RTPipeline: Shader tables created (RayGen=%uB, Miss=%uB, HitGroup=%uB)",
                rayGenTableSize, missTableSize, hitGroupTableSize);
    return true;
}

void RTPipeline::DispatchRays(ID3D12GraphicsCommandList4* cmdList, uint32_t width, uint32_t height)
{
    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};

    // RayGen
    dispatchDesc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable.GetGPUVirtualAddress();
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes  = m_rayGenRecordSize;

    // Miss (2レコード: [0]=Miss, [1]=ShadowMiss)
    dispatchDesc.MissShaderTable.StartAddress  = m_missShaderTable.GetGPUVirtualAddress();
    dispatchDesc.MissShaderTable.SizeInBytes   = m_missRecordSize * 2;
    dispatchDesc.MissShaderTable.StrideInBytes = m_missRecordSize;

    // HitGroup
    dispatchDesc.HitGroupTable.StartAddress  = m_hitGroupShaderTable.GetGPUVirtualAddress();
    dispatchDesc.HitGroupTable.SizeInBytes   = m_hitGroupRecordSize;
    dispatchDesc.HitGroupTable.StrideInBytes = m_hitGroupRecordSize;

    // ディスパッチサイズ
    dispatchDesc.Width  = width;
    dispatchDesc.Height = height;
    dispatchDesc.Depth  = 1;

    cmdList->SetPipelineState1(m_stateObject.Get());
    cmdList->DispatchRays(&dispatchDesc);
}

} // namespace GX
