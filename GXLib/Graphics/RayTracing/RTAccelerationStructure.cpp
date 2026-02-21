/// @file RTAccelerationStructure.cpp
/// @brief DXR アクセラレーション構造の実装
#include "pch.h"
#include "Graphics/RayTracing/RTAccelerationStructure.h"
#include "Core/Logger.h"

namespace GX
{

bool RTAccelerationStructure::Initialize(ID3D12Device5* device)
{
    if (!device)
    {
        GX_LOG_ERROR("RTAccelerationStructure: device5 is null");
        return false;
    }
    m_device = device;
    m_instances.reserve(k_MaxInstances);

    // TLAS用バッファを事前確保（インスタンス記述子バッファ）
    uint64_t instanceDescSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * k_MaxInstances;
    for (uint32_t i = 0; i < k_BufferCount; ++i)
    {
        if (!m_instanceDescBuffer[i].CreateUploadBufferEmpty(device, instanceDescSize))
            return false;
    }

    GX_LOG_INFO("RTAccelerationStructure initialized (maxInstances=%u)", k_MaxInstances);
    return true;
}

int RTAccelerationStructure::BuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                                        ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                                        ID3D12Resource* ib, uint32_t indexCount,
                                        DXGI_FORMAT indexFormat)
{
    if (vertexStride < 12 || vertexCount == 0)
    {
        GX_LOG_ERROR("RTAccelerationStructure: invalid vertex params (stride=%u, count=%u)", vertexStride, vertexCount);
        return -1;
    }

    // ジオメトリ記述
    D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
    geomDesc.Type  = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geomDesc.Triangles.VertexBuffer.StartAddress  = vb->GetGPUVirtualAddress();
    geomDesc.Triangles.VertexBuffer.StrideInBytes  = vertexStride;
    geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT; // 頂点先頭12Bがfloat3 Position
    geomDesc.Triangles.VertexCount  = vertexCount;
    geomDesc.Triangles.IndexBuffer  = ib->GetGPUVirtualAddress();
    geomDesc.Triangles.IndexFormat  = indexFormat;
    geomDesc.Triangles.IndexCount   = indexCount;

    // ビルド入力
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.NumDescs       = 1;
    inputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.pGeometryDescs = &geomDesc;

    // プリビルド情報取得
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    // バッファ作成
    BLASEntry entry;
    if (!entry.result.CreateDefaultBuffer(m_device,
            prebuildInfo.ResultDataMaxSizeInBytes,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE))
        return -1;

    if (!entry.scratch.CreateDefaultBuffer(m_device,
            prebuildInfo.ScratchDataSizeInBytes,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON))
        return -1;

    // ビルド
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs                           = inputs;
    buildDesc.DestAccelerationStructureData    = entry.result.GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = entry.scratch.GetGPUVirtualAddress();

    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // UAVバリア（BLAS構築完了を保証）
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = entry.result.GetResource();
    cmdList->ResourceBarrier(1, &barrier);

    int index = static_cast<int>(m_blasCache.size());
    m_blasCache.push_back(std::move(entry));

    GX_LOG_INFO("BLAS built: index=%d, verts=%u, indices=%u", index, vertexCount, indexCount);
    return index;
}

void RTAccelerationStructure::BeginFrame()
{
    m_instances.clear();
}

void RTAccelerationStructure::AddInstance(int blasIndex, const XMMATRIX& worldMatrix,
                                           uint32_t instanceID, uint8_t mask,
                                           uint32_t instanceFlags)
{
    if (blasIndex < 0 || blasIndex >= static_cast<int>(m_blasCache.size()))
        return;
    if (m_instances.size() >= k_MaxInstances)
        return;

    D3D12_RAYTRACING_INSTANCE_DESC desc = {};

    // DirectXMath (行ベクトル v*M) → D3D12 TLAS (列ベクトル M*v) 変換
    // TLAS Transform は列ベクトル規約の3x4行列なので、Transposeが必要
    XMFLOAT4X4 mat;
    XMStoreFloat4x4(&mat, XMMatrixTranspose(worldMatrix));
    desc.Transform[0][0] = mat._11; desc.Transform[0][1] = mat._12; desc.Transform[0][2] = mat._13; desc.Transform[0][3] = mat._14;
    desc.Transform[1][0] = mat._21; desc.Transform[1][1] = mat._22; desc.Transform[1][2] = mat._23; desc.Transform[1][3] = mat._24;
    desc.Transform[2][0] = mat._31; desc.Transform[2][1] = mat._32; desc.Transform[2][2] = mat._33; desc.Transform[2][3] = mat._34;

    desc.InstanceID                          = instanceID;
    desc.InstanceMask                        = mask;
    desc.InstanceContributionToHitGroupIndex = 0;
    desc.Flags                               = instanceFlags;
    desc.AccelerationStructure               = m_blasCache[blasIndex].result.GetGPUVirtualAddress();

    m_instances.push_back(desc);
}

void RTAccelerationStructure::BuildTLAS(ID3D12GraphicsCommandList4* cmdList, uint32_t frameIndex)
{
    uint32_t bufIdx = frameIndex % k_BufferCount;
    m_lastBuiltBufIdx = bufIdx;
    uint32_t instanceCount = static_cast<uint32_t>(m_instances.size());

    if (instanceCount == 0) return;

    // インスタンス記述子をアップロード
    void* mapped = m_instanceDescBuffer[bufIdx].Map();
    if (mapped)
    {
        memcpy(mapped, m_instances.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount);
        m_instanceDescBuffer[bufIdx].Unmap();
    }

    // ビルド入力
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.Flags         = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    inputs.NumDescs      = instanceCount;
    inputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.InstanceDescs = m_instanceDescBuffer[bufIdx].GetGPUVirtualAddress();

    // プリビルド情報取得
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    // TLASバッファは毎フレーム再ビルドするが、サイズが足りている限り再利用する。
    // インスタンス数が増えてprebuildInfoが大きくなった場合のみ再作成
    if (!m_tlasResult[bufIdx].GetResource() ||
        m_tlasResult[bufIdx].GetResource()->GetDesc().Width < prebuildInfo.ResultDataMaxSizeInBytes)
    {
        m_tlasResult[bufIdx] = Buffer();
        m_tlasResult[bufIdx].CreateDefaultBuffer(m_device,
            prebuildInfo.ResultDataMaxSizeInBytes,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
    }

    if (!m_tlasScratch[bufIdx].GetResource() ||
        m_tlasScratch[bufIdx].GetResource()->GetDesc().Width < prebuildInfo.ScratchDataSizeInBytes)
    {
        m_tlasScratch[bufIdx] = Buffer();
        m_tlasScratch[bufIdx].CreateDefaultBuffer(m_device,
            prebuildInfo.ScratchDataSizeInBytes,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON);
    }

    // ビルド
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs                           = inputs;
    buildDesc.DestAccelerationStructureData    = m_tlasResult[bufIdx].GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = m_tlasScratch[bufIdx].GetGPUVirtualAddress();

    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // UAVバリア
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_tlasResult[bufIdx].GetResource();
    cmdList->ResourceBarrier(1, &barrier);
}

D3D12_GPU_VIRTUAL_ADDRESS RTAccelerationStructure::GetTLASAddress() const
{
    if (m_tlasResult[m_lastBuiltBufIdx].GetResource())
        return m_tlasResult[m_lastBuiltBufIdx].GetGPUVirtualAddress();
    return 0;
}

} // namespace GX
