/// @file ClothSimulation.cpp
/// @brief GPU布シミュレーション実装（Verlet積分 + PBD制約ソルバー）
///
/// CPU-onlyモード（device=nullptr）とGPU Compute Shaderモードの双方をサポート。
/// GPU モードでは ClothUpdate.hlsl の CSVerlet / CSConstraint / CSCollision を使用し、
/// パーティクルデータをGPU上で更新した後、READBACK バッファ経由でCPU側に読み戻す。

#include "pch_graphics.h"
#include "Graphics/3D/ClothSimulation.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstring>

namespace gx
{

// -----------------------------------------------------------------------
// ヘルパー: Vector3 演算
// -----------------------------------------------------------------------
namespace
{

inline Vector3 Add(const Vector3& a, const Vector3& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vector3 Sub(const Vector3& a, const Vector3& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline Vector3 Mul(const Vector3& v, float s)
{
    return { v.x * s, v.y * s, v.z * s };
}

inline float Length(const Vector3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vector3 Normalize(const Vector3& v)
{
    float len = Length(v);
    if (len < 1e-8f) return { 0.0f, 0.0f, 0.0f };
    return Mul(v, 1.0f / len);
}

inline float Distance(const Vector3& a, const Vector3& b)
{
    return Length(Sub(a, b));
}

} // anonymous namespace

// =======================================================================
// Initialize / Shutdown
// =======================================================================

bool ClothSimulation::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    if (m_initialized) Shutdown();

    m_width  = width;
    m_height = height;

    if (!device)
    {
        m_cpuMode     = true;
        m_gpuReady    = false;
        m_initialized = true;
        return true;
    }

    // GPU mode
    m_cpuMode = false;
    m_device  = device;

    if (!m_shader.Initialize())
    {
        GX_LOG_ERROR("ClothSimulation: shader compiler init failed");
        m_cpuMode  = true;
        m_gpuReady = false;
        m_initialized = true;
        return true;
    }

    if (!CreateGPUPipeline(device))
    {
        GX_LOG_WARN("ClothSimulation: GPU pipeline creation failed, using CPU fallback");
        m_cpuMode  = true;
        m_gpuReady = false;
        m_initialized = true;
        return true;
    }

    uint32_t cbSize = (sizeof(ClothCBData) + 255) & ~255;
    if (!m_clothCB.Initialize(device, cbSize, cbSize))
    {
        GX_LOG_WARN("ClothSimulation: constant buffer init failed, using CPU fallback");
        m_cpuMode  = true;
        m_gpuReady = false;
        m_initialized = true;
        return true;
    }

    m_gpuReady    = true;
    m_initialized = true;
    GX_LOG_INFO("ClothSimulation: GPU pipeline initialized (%ux%u grid)", width, height);
    return true;
}

void ClothSimulation::Shutdown()
{
    m_particles.clear();
    m_constraints.clear();
    m_colliders.clear();

    m_rootSignature.Reset();
    m_verletPSO.Reset();
    m_constraintPSO.Reset();
    m_collisionPSO.Reset();
    m_readbackBuffer.Reset();

    m_width              = 0;
    m_height             = 0;
    m_cpuMode            = true;
    m_initialized        = false;
    m_gpuReady           = false;
    m_constraintsDirty   = true;
    m_collidersDirty     = true;
    m_nextColliderId     = 0;
    m_device             = nullptr;
    m_gpuMaxParticles    = 0;
    m_gpuMaxConstraints  = 0;
    m_gpuMaxColliders    = 0;
}

// =======================================================================
// GPU パイプライン構築
// =======================================================================

bool ClothSimulation::CreateGPUPipeline(ID3D12Device* device)
{
    // ルートシグネチャ
    //   [0] CBV b0
    //   [1] Descriptor Table: UAV u0 (particles)
    //   [2] Descriptor Table: SRV t0..t1 (constraints, colliders)
    m_rootSignature = RootSignatureBuilder()
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2)
        .Build(device);

    if (!m_rootSignature)
    {
        GX_LOG_ERROR("ClothSimulation: root signature creation failed");
        return false;
    }

    ShaderBlob verletCS = m_shader.CompileFromFile(
        L"Shaders/ClothUpdate.hlsl", L"CSVerlet", L"cs_6_0");
    if (!verletCS.valid)
    {
        GX_LOG_ERROR("ClothSimulation: CSVerlet compile failed: %s",
                     m_shader.GetLastError().c_str());
        return false;
    }

    ShaderBlob constraintCS = m_shader.CompileFromFile(
        L"Shaders/ClothUpdate.hlsl", L"CSConstraint", L"cs_6_0");
    if (!constraintCS.valid)
    {
        GX_LOG_ERROR("ClothSimulation: CSConstraint compile failed: %s",
                     m_shader.GetLastError().c_str());
        return false;
    }

    ShaderBlob collisionCS = m_shader.CompileFromFile(
        L"Shaders/ClothUpdate.hlsl", L"CSCollision", L"cs_6_0");
    if (!collisionCS.valid)
    {
        GX_LOG_ERROR("ClothSimulation: CSCollision compile failed: %s",
                     m_shader.GetLastError().c_str());
        return false;
    }

    auto createComputePSO = [&](const ShaderBlob& cs, ComPtr<ID3D12PipelineState>& outPSO,
                                const char* name) -> bool
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_rootSignature.Get();
        desc.CS = cs.GetBytecode();

        HRESULT hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&outPSO));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("ClothSimulation: %s PSO creation failed (0x%08X)", name, hr);
            return false;
        }
        return true;
    };

    if (!createComputePSO(verletCS, m_verletPSO, "Verlet"))
        return false;
    if (!createComputePSO(constraintCS, m_constraintPSO, "Constraint"))
        return false;
    if (!createComputePSO(collisionCS, m_collisionPSO, "Collision"))
        return false;

    return true;
}

bool ClothSimulation::CreateGPUBuffers(ID3D12Device* device)
{
    uint32_t particleCount   = static_cast<uint32_t>(m_particles.size());
    uint32_t constraintCount = static_cast<uint32_t>(m_constraints.size());
    uint32_t colliderCount   = std::max(static_cast<uint32_t>(m_colliders.size()), 1u);

    uint64_t particleSize   = static_cast<uint64_t>(particleCount) * sizeof(GPUClothParticle);
    uint64_t constraintSize = static_cast<uint64_t>(constraintCount) * sizeof(GPUClothConstraint);
    uint64_t colliderSize   = static_cast<uint64_t>(colliderCount) * sizeof(GPUClothCollider);

    // パーティクルバッファ（DEFAULT heap, UAV）
    if (!m_gpuParticleBuffer.CreateDefaultBuffer(device, particleSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
    {
        GX_LOG_ERROR("ClothSimulation: particle buffer creation failed");
        return false;
    }

    // パーティクルアップロードバッファ
    if (!m_particleUpload.CreateUploadBufferEmpty(device, particleSize))
    {
        GX_LOG_ERROR("ClothSimulation: particle upload buffer creation failed");
        return false;
    }

    // パーティクルリードバックバッファ（READBACK heap）
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width              = particleSize;
        resourceDesc.Height             = 1;
        resourceDesc.DepthOrArraySize   = 1;
        resourceDesc.MipLevels          = 1;
        resourceDesc.SampleDesc.Count   = 1;
        resourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_readbackBuffer));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("ClothSimulation: readback buffer creation failed (0x%08X)", hr);
            return false;
        }
    }

    // 制約バッファ（DEFAULT heap, SRV）
    if (constraintCount > 0)
    {
        if (!m_gpuConstraintBuffer.CreateDefaultBuffer(device, constraintSize,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))
        {
            GX_LOG_ERROR("ClothSimulation: constraint buffer creation failed");
            return false;
        }

        if (!m_constraintUpload.CreateUploadBufferEmpty(device, constraintSize))
        {
            GX_LOG_ERROR("ClothSimulation: constraint upload buffer creation failed");
            return false;
        }
    }

    // コライダーバッファ（DEFAULT heap, SRV）
    if (!m_gpuColliderBuffer.CreateDefaultBuffer(device, colliderSize,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))
    {
        GX_LOG_ERROR("ClothSimulation: collider buffer creation failed");
        return false;
    }
    if (!m_colliderUpload.CreateUploadBufferEmpty(device, colliderSize))
    {
        GX_LOG_ERROR("ClothSimulation: collider upload buffer creation failed");
        return false;
    }

    m_gpuMaxParticles   = particleCount;
    m_gpuMaxConstraints = constraintCount;
    m_gpuMaxColliders   = colliderCount;

    return true;
}

void ClothSimulation::CreateGPUDescriptors(ID3D12Device* device)
{
    m_srvUavHeap.Initialize(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        8, true);

    m_particleUAVSlot   = m_srvUavHeap.AllocateIndex();
    m_constraintSRVSlot = m_srvUavHeap.AllocateIndex();
    m_colliderSRVSlot   = m_srvUavHeap.AllocateIndex();

    uint32_t particleCount   = static_cast<uint32_t>(m_particles.size());
    uint32_t constraintCount = static_cast<uint32_t>(m_constraints.size());
    uint32_t colliderCount   = std::max(static_cast<uint32_t>(m_colliders.size()), 1u);

    // パーティクルバッファ UAV (u0)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format                      = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.FirstElement         = 0;
        uavDesc.Buffer.NumElements          = particleCount;
        uavDesc.Buffer.StructureByteStride  = sizeof(GPUClothParticle);

        device->CreateUnorderedAccessView(
            m_gpuParticleBuffer.GetResource(), nullptr, &uavDesc,
            m_srvUavHeap.GetCPUHandle(m_particleUAVSlot));
    }

    // 制約バッファ SRV (t0)
    if (constraintCount > 0)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension               = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping      = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format                       = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement          = 0;
        srvDesc.Buffer.NumElements           = constraintCount;
        srvDesc.Buffer.StructureByteStride   = sizeof(GPUClothConstraint);

        device->CreateShaderResourceView(
            m_gpuConstraintBuffer.GetResource(), &srvDesc,
            m_srvUavHeap.GetCPUHandle(m_constraintSRVSlot));
    }

    // コライダーバッファ SRV (t1)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension               = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping      = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format                       = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement          = 0;
        srvDesc.Buffer.NumElements           = colliderCount;
        srvDesc.Buffer.StructureByteStride   = sizeof(GPUClothCollider);

        device->CreateShaderResourceView(
            m_gpuColliderBuffer.GetResource(), &srvDesc,
            m_srvUavHeap.GetCPUHandle(m_colliderSRVSlot));
    }
}

// =======================================================================
// GPU データ転送
// =======================================================================

void ClothSimulation::UploadParticlesToGPU(ID3D12GraphicsCommandList* cmdList)
{
    uint32_t count = static_cast<uint32_t>(m_particles.size());
    if (count == 0) return;

    void* mapped = m_particleUpload.Map();
    if (!mapped) return;

    GPUClothParticle* dst = static_cast<GPUClothParticle*>(mapped);
    for (uint32_t i = 0; i < count; ++i)
    {
        const ClothParticle& src = m_particles[i];
        dst[i].position[0]     = src.position.x;
        dst[i].position[1]     = src.position.y;
        dst[i].position[2]     = src.position.z;
        dst[i].inverseMass     = src.inverseMass;
        dst[i].prevPosition[0] = src.prevPosition.x;
        dst[i].prevPosition[1] = src.prevPosition.y;
        dst[i].prevPosition[2] = src.prevPosition.z;
        dst[i]._pad0           = 0.0f;
        dst[i].acceleration[0] = src.acceleration.x;
        dst[i].acceleration[1] = src.acceleration.y;
        dst[i].acceleration[2] = src.acceleration.z;
        dst[i]._pad1           = 0.0f;
    }
    m_particleUpload.Unmap();

    // UAV -> COPY_DEST
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_gpuParticleBuffer.GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    uint64_t byteSize = static_cast<uint64_t>(count) * sizeof(GPUClothParticle);
    cmdList->CopyBufferRegion(m_gpuParticleBuffer.GetResource(), 0,
                               m_particleUpload.GetResource(), 0, byteSize);

    // COPY_DEST -> UAV
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_gpuParticleBuffer.GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }
}

void ClothSimulation::UploadConstraintsToGPU(ID3D12GraphicsCommandList* cmdList)
{
    uint32_t count = static_cast<uint32_t>(m_constraints.size());
    if (count == 0) return;

    void* mapped = m_constraintUpload.Map();
    if (!mapped) return;

    GPUClothConstraint* dst = static_cast<GPUClothConstraint*>(mapped);
    for (uint32_t i = 0; i < count; ++i)
    {
        const ClothConstraint& src = m_constraints[i];
        dst[i].particleA  = src.particleA;
        dst[i].particleB  = src.particleB;
        dst[i].restLength = src.restLength;
        dst[i].stiffness  = src.stiffness;
    }
    m_constraintUpload.Unmap();

    // SRV -> COPY_DEST
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_gpuConstraintBuffer.GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    uint64_t byteSize = static_cast<uint64_t>(count) * sizeof(GPUClothConstraint);
    cmdList->CopyBufferRegion(m_gpuConstraintBuffer.GetResource(), 0,
                               m_constraintUpload.GetResource(), 0, byteSize);

    // COPY_DEST -> SRV
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_gpuConstraintBuffer.GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    m_constraintsDirty = false;
}

void ClothSimulation::UploadCollidersToGPU(ID3D12GraphicsCommandList* cmdList)
{
    uint32_t count = static_cast<uint32_t>(m_colliders.size());
    if (count == 0) return;

    void* mapped = m_colliderUpload.Map();
    if (!mapped) return;

    GPUClothCollider* dst = static_cast<GPUClothCollider*>(mapped);
    for (uint32_t i = 0; i < count; ++i)
    {
        const ClothCollider& src = m_colliders[i];
        dst[i].shape          = static_cast<uint32_t>(src.shape);
        dst[i].position[0]    = src.position.x;
        dst[i].position[1]    = src.position.y;
        dst[i].position[2]    = src.position.z;
        dst[i].radius         = src.radius;
        dst[i].capsuleEnd[0]  = src.capsuleEnd.x;
        dst[i].capsuleEnd[1]  = src.capsuleEnd.y;
        dst[i].capsuleEnd[2]  = src.capsuleEnd.z;
        dst[i].capsuleRadius  = src.capsuleRadius;
    }
    m_colliderUpload.Unmap();

    // SRV -> COPY_DEST
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_gpuColliderBuffer.GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    uint64_t byteSize = static_cast<uint64_t>(count) * sizeof(GPUClothCollider);
    cmdList->CopyBufferRegion(m_gpuColliderBuffer.GetResource(), 0,
                               m_colliderUpload.GetResource(), 0, byteSize);

    // COPY_DEST -> SRV
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_gpuColliderBuffer.GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    m_collidersDirty = false;
}

void ClothSimulation::ReadbackParticlesFromGPU(ID3D12GraphicsCommandList* cmdList)
{
    uint32_t count = static_cast<uint32_t>(m_particles.size());
    if (count == 0 || !m_readbackBuffer) return;

    // UAV -> COPY_SOURCE
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_gpuParticleBuffer.GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    uint64_t byteSize = static_cast<uint64_t>(count) * sizeof(GPUClothParticle);
    cmdList->CopyBufferRegion(m_readbackBuffer.Get(), 0,
                               m_gpuParticleBuffer.GetResource(), 0, byteSize);

    // COPY_SOURCE -> UAV
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource    = m_gpuParticleBuffer.GetResource();
        barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }
}

void ClothSimulation::SyncParticlesFromReadback()
{
    uint32_t count = static_cast<uint32_t>(m_particles.size());
    if (count == 0 || !m_readbackBuffer) return;

    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(count) * sizeof(GPUClothParticle) };
    HRESULT hr = m_readbackBuffer->Map(0, &readRange, &mapped);
    if (FAILED(hr) || !mapped) return;

    const GPUClothParticle* src = static_cast<const GPUClothParticle*>(mapped);
    for (uint32_t i = 0; i < count; ++i)
    {
        m_particles[i].position.x     = src[i].position[0];
        m_particles[i].position.y     = src[i].position[1];
        m_particles[i].position.z     = src[i].position[2];
        m_particles[i].prevPosition.x = src[i].prevPosition[0];
        m_particles[i].prevPosition.y = src[i].prevPosition[1];
        m_particles[i].prevPosition.z = src[i].prevPosition[2];
        m_particles[i].acceleration.x = src[i].acceleration[0];
        m_particles[i].acceleration.y = src[i].acceleration[1];
        m_particles[i].acceleration.z = src[i].acceleration[2];
        m_particles[i].inverseMass    = src[i].inverseMass;
    }

    D3D12_RANGE writeRange = { 0, 0 };
    m_readbackBuffer->Unmap(0, &writeRange);
}

// =======================================================================
// DispatchGPU
// =======================================================================

void ClothSimulation::DispatchGPU(ID3D12GraphicsCommandList* cmdList,
                                   float deltaTime, uint32_t frameIndex)
{
    if (!m_initialized || !m_gpuReady || m_cpuMode || !cmdList) return;
    if (m_particles.empty()) return;

    uint32_t particleCount   = static_cast<uint32_t>(m_particles.size());
    uint32_t constraintCount = static_cast<uint32_t>(m_constraints.size());
    uint32_t colliderCount   = static_cast<uint32_t>(m_colliders.size());

    // GPU バッファが未作成またはサイズ不足なら再作成
    if (m_gpuMaxParticles < particleCount ||
        m_gpuMaxConstraints < constraintCount ||
        m_gpuMaxColliders < std::max(colliderCount, 1u))
    {
        if (!CreateGPUBuffers(m_device))
        {
            GX_LOG_ERROR("ClothSimulation: GPU buffer creation failed in DispatchGPU");
            return;
        }
        CreateGPUDescriptors(m_device);
        m_constraintsDirty = true;
        m_collidersDirty   = true;
    }

    // パーティクルデータをGPUにアップロード
    UploadParticlesToGPU(cmdList);

    // 制約データのアップロード（変更時のみ）
    if (m_constraintsDirty)
        UploadConstraintsToGPU(cmdList);

    // コライダーデータのアップロード（変更時のみ）
    if (m_collidersDirty)
        UploadCollidersToGPU(cmdList);

    // 定数バッファ更新
    float dt = (deltaTime > 0.0f) ? deltaTime : m_settings.timeStep;

    ClothCBData cbData = {};
    cbData.gravity[0]       = m_settings.gravity.x;
    cbData.gravity[1]       = m_settings.gravity.y;
    cbData.gravity[2]       = m_settings.gravity.z;
    cbData.damping          = m_settings.damping;
    cbData.wind[0]          = m_settings.windForce.x;
    cbData.wind[1]          = m_settings.windForce.y;
    cbData.wind[2]          = m_settings.windForce.z;
    cbData.deltaTime        = dt;
    cbData.particleCount    = particleCount;
    cbData.constraintCount  = constraintCount;
    cbData.solverIterations = m_settings.solverIterations;
    cbData.colliderCount    = colliderCount;

    void* cbMapped = m_clothCB.Map(frameIndex);
    if (cbMapped)
    {
        std::memcpy(cbMapped, &cbData, sizeof(ClothCBData));
        m_clothCB.Unmap(frameIndex);
    }

    // Shader-visible ヒープとルートシグネチャをセット
    ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetComputeRootSignature(m_rootSignature.Get());

    cmdList->SetComputeRootConstantBufferView(0,
        m_clothCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetComputeRootDescriptorTable(1,
        m_srvUavHeap.GetGPUHandle(m_particleUAVSlot));
    cmdList->SetComputeRootDescriptorTable(2,
        m_srvUavHeap.GetGPUHandle(m_constraintSRVSlot));

    uint32_t particleGroups = (particleCount + 255) / 256;

    // Pass 1: Verlet Integration
    cmdList->SetPipelineState(m_verletPSO.Get());
    cmdList->Dispatch(particleGroups, 1, 1);

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = m_gpuParticleBuffer.GetResource();
        cmdList->ResourceBarrier(1, &barrier);
    }

    // Pass 2: PBD Constraint Solving (反復)
    if (constraintCount > 0)
    {
        cmdList->SetPipelineState(m_constraintPSO.Get());
        uint32_t constraintGroups = (constraintCount + 255) / 256;

        for (uint32_t iter = 0; iter < m_settings.solverIterations; ++iter)
        {
            cmdList->Dispatch(constraintGroups, 1, 1);

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barrier.UAV.pResource = m_gpuParticleBuffer.GetResource();
            cmdList->ResourceBarrier(1, &barrier);
        }
    }

    // Pass 3: Collision Handling
    if (colliderCount > 0)
    {
        cmdList->SetPipelineState(m_collisionPSO.Get());
        cmdList->Dispatch(particleGroups, 1, 1);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = m_gpuParticleBuffer.GetResource();
        cmdList->ResourceBarrier(1, &barrier);
    }

    // パーティクルデータをREADBACKバッファにコピー
    ReadbackParticlesFromGPU(cmdList);
}

// =======================================================================
// Grid creation
// =======================================================================

void ClothSimulation::CreateGrid(uint32_t width, uint32_t height, float spacing)
{
    m_width  = width;
    m_height = height;

    m_particles.clear();
    m_constraints.clear();
    m_constraintsDirty = true;
    m_collidersDirty   = true;

    // Generate particles in a grid layout (XZ plane at Y=0)
    m_particles.resize(static_cast<size_t>(width) * height);
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t idx = y * width + x;
            ClothParticle& p = m_particles[idx];
            p.position     = { static_cast<float>(x) * spacing, 0.0f, static_cast<float>(y) * spacing };
            p.prevPosition = p.position;
            p.acceleration = { 0.0f, 0.0f, 0.0f };
            p.inverseMass  = 1.0f;
        }
    }

    auto addConstraint = [&](uint32_t a, uint32_t b, ClothConstraintType type, float stiffness)
    {
        ClothConstraint c;
        c.type       = type;
        c.particleA  = a;
        c.particleB  = b;
        c.restLength = Distance(m_particles[a].position, m_particles[b].position);
        c.stiffness  = stiffness;
        m_constraints.push_back(c);
    };

    // Stretch constraints (horizontal + vertical neighbors)
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t idx = y * width + x;
            if (x + 1 < width)
                addConstraint(idx, idx + 1, ClothConstraintType::Stretch, 1.0f);
            if (y + 1 < height)
                addConstraint(idx, idx + width, ClothConstraintType::Stretch, 1.0f);
        }
    }

    // Shear constraints (diagonal neighbors)
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t idx = y * width + x;
            if (x + 1 < width && y + 1 < height)
                addConstraint(idx, (y + 1) * width + (x + 1), ClothConstraintType::Shear, 1.0f);
            if (x > 0 && y + 1 < height)
                addConstraint(idx, (y + 1) * width + (x - 1), ClothConstraintType::Shear, 1.0f);
        }
    }

    // Bend constraints (skip-one horizontal + vertical)
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t idx = y * width + x;
            if (x + 2 < width)
                addConstraint(idx, idx + 2, ClothConstraintType::Bend, 0.5f);
            if (y + 2 < height)
                addConstraint(idx, (y + 2) * width + x, ClothConstraintType::Bend, 0.5f);
        }
    }

    // GPU バッファの再作成が必要
    m_gpuMaxParticles   = 0;
    m_gpuMaxConstraints = 0;
}

// =======================================================================
// Pin / Unpin
// =======================================================================

void ClothSimulation::PinParticle(uint32_t index)
{
    assert(index < m_particles.size());
    m_particles[index].inverseMass = 0.0f;
}

void ClothSimulation::UnpinParticle(uint32_t index, float mass)
{
    assert(index < m_particles.size());
    m_particles[index].inverseMass = (mass > 0.0f) ? (1.0f / mass) : 1.0f;
}

bool ClothSimulation::IsParticlePinned(uint32_t index) const
{
    assert(index < m_particles.size());
    return m_particles[index].inverseMass <= 0.0f;
}

// =======================================================================
// Colliders
// =======================================================================

int ClothSimulation::AddCollider(const ClothCollider& collider)
{
    m_colliders.push_back(collider);
    m_collidersDirty = true;
    return m_nextColliderId++;
}

void ClothSimulation::RemoveCollider(int id)
{
    if (id >= 0 && id < static_cast<int>(m_colliders.size()))
    {
        m_colliders.erase(m_colliders.begin() + id);
        m_collidersDirty = true;
    }
}

// =======================================================================
// Update (CPU simulation step)
// =======================================================================

void ClothSimulation::Update(float deltaTime)
{
    if (!m_initialized || m_particles.empty()) return;

    float dt = (deltaTime > 0.0f) ? deltaTime : m_settings.timeStep;

    ApplyWind();
    VerletIntegrate(dt);

    for (uint32_t iter = 0; iter < m_settings.solverIterations; ++iter)
        SolveConstraints();

    HandleCollisions();
}

// =======================================================================
// CPU simulation methods
// =======================================================================

void ClothSimulation::ApplyWind()
{
    for (auto& p : m_particles)
    {
        if (p.inverseMass > 0.0f)
        {
            p.acceleration = Add(m_settings.gravity, m_settings.windForce);
        }
    }
}

void ClothSimulation::VerletIntegrate(float dt)
{
    for (auto& p : m_particles)
    {
        if (p.inverseMass <= 0.0f) continue;

        Vector3 velocity = Mul(Sub(p.position, p.prevPosition), m_settings.damping);
        p.prevPosition = p.position;

        float dt2 = dt * dt;
        p.position = Add(p.position, Add(velocity, Mul(p.acceleration, dt2)));
    }
}

void ClothSimulation::SolveConstraints()
{
    for (auto& c : m_constraints)
    {
        ClothParticle& pA = m_particles[c.particleA];
        ClothParticle& pB = m_particles[c.particleB];

        Vector3 delta = Sub(pB.position, pA.position);
        float currentLength = Length(delta);

        if (currentLength < 1e-8f) continue;

        float diff = (currentLength - c.restLength) / currentLength;
        Vector3 correction = Mul(delta, diff * c.stiffness);

        float totalInvMass = pA.inverseMass + pB.inverseMass;
        if (totalInvMass <= 0.0f) continue;

        if (pA.inverseMass > 0.0f)
        {
            float weightA = pA.inverseMass / totalInvMass;
            pA.position = Add(pA.position, Mul(correction, weightA));
        }

        if (pB.inverseMass > 0.0f)
        {
            float weightB = pB.inverseMass / totalInvMass;
            pB.position = Sub(pB.position, Mul(correction, weightB));
        }
    }
}

void ClothSimulation::HandleCollisions()
{
    for (auto& p : m_particles)
    {
        if (p.inverseMass <= 0.0f) continue;

        for (const auto& collider : m_colliders)
        {
            if (collider.shape == ClothColliderShape::Sphere)
            {
                Vector3 delta = Sub(p.position, collider.position);
                float dist = Length(delta);

                if (dist < collider.radius && dist > 1e-8f)
                {
                    Vector3 normal = Normalize(delta);
                    p.position = Add(collider.position, Mul(normal, collider.radius));
                }
            }
            else if (collider.shape == ClothColliderShape::Capsule)
            {
                Vector3 ab = Sub(collider.capsuleEnd, collider.position);
                Vector3 ap = Sub(p.position, collider.position);
                float abLenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;

                float t = 0.0f;
                if (abLenSq > 1e-8f)
                {
                    t = (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / abLenSq;
                    t = std::clamp(t, 0.0f, 1.0f);
                }

                Vector3 closestOnAxis = Add(collider.position, Mul(ab, t));
                Vector3 delta = Sub(p.position, closestOnAxis);
                float dist = Length(delta);

                float capsuleR = collider.capsuleRadius > 0.0f ? collider.capsuleRadius : collider.radius;
                if (dist < capsuleR && dist > 1e-8f)
                {
                    Vector3 normal = Normalize(delta);
                    p.position = Add(closestOnAxis, Mul(normal, capsuleR));
                }
            }
        }
    }
}

} // namespace gx
