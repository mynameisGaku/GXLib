/// @file ClusteredLighting.cpp
/// @brief クラスタードライティング実装

#include "pch_graphics.h"
#include "Graphics/3D/ClusteredLighting.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Core/Logger.h"

namespace gx
{

// -----------------------------------------------------------------------
// Initialize / Shutdown
// -----------------------------------------------------------------------

bool ClusteredLighting::Initialize(ID3D12Device* device)
{
    if (m_initialized) Shutdown();

    m_device = device;
    if (!device) return false;

    const uint32_t totalClusters = GetTotalClusters();

    // Create cluster info buffer (ClusterInfo per cluster)
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = totalClusters * sizeof(ClusterInfo);
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.SampleDesc.Count   = 1;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(&m_clusterInfoBuffer));
        if (FAILED(hr)) return false;
    }

    // Create light index buffer
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = static_cast<uint64_t>(totalClusters) * k_MaxLightsPerCluster * sizeof(uint32_t);
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.SampleDesc.Count   = 1;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(&m_lightIndexBuffer));
        if (FAILED(hr)) return false;
    }

    // Create light data upload buffer
    {
        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = LightConstants::k_MaxLights * sizeof(LightData);
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.SampleDesc.Count   = 1;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_lightDataBuffer));
        if (FAILED(hr)) return false;
    }

    // Create constant buffer (256-byte aligned)
    {
        const uint32_t cbSize = (sizeof(Matrix4x4) * 2 + 64 + 255) & ~255u;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width              = cbSize;
        desc.Height             = 1;
        desc.DepthOrArraySize   = 1;
        desc.MipLevels          = 1;
        desc.SampleDesc.Count   = 1;
        desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_constantBuffer));
        if (FAILED(hr)) return false;
    }

    // Create descriptor heap for SRV/UAV/CBV (4 slots, shader-visible)
    // Layout:
    //   Slot 0: LightData SRV (StructuredBuffer<LightData>, t0)
    //   Slot 1: ClusterInfo UAV (RWStructuredBuffer<ClusterInfo>, u0)
    //   Slot 2: LightIndices UAV (RWStructuredBuffer<uint>, u1)
    //   Slot 3: ClusterCB CBV (b0)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 4;
        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));
        if (FAILED(hr)) return false;
    }

    // Create descriptor views
    {
        const uint32_t descriptorSize = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

        // Slot 0: LightData SRV (StructuredBuffer<LightData>)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                  = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.FirstElement     = 0;
            srvDesc.Buffer.NumElements      = LightConstants::k_MaxLights;
            srvDesc.Buffer.StructureByteStride = sizeof(LightData);

            device->CreateShaderResourceView(m_lightDataBuffer.Get(), &srvDesc, cpuHandle);
        }

        // Slot 1: ClusterInfo UAV (RWStructuredBuffer<ClusterInfo>)
        cpuHandle.ptr += descriptorSize;
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format                = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension         = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement   = 0;
            uavDesc.Buffer.NumElements    = totalClusters;
            uavDesc.Buffer.StructureByteStride = sizeof(ClusterInfo);

            device->CreateUnorderedAccessView(m_clusterInfoBuffer.Get(), nullptr, &uavDesc, cpuHandle);
        }

        // Slot 2: LightIndices UAV (RWStructuredBuffer<uint>)
        cpuHandle.ptr += descriptorSize;
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format                = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension         = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement   = 0;
            uavDesc.Buffer.NumElements    = totalClusters * k_MaxLightsPerCluster;
            uavDesc.Buffer.StructureByteStride = sizeof(uint32_t);

            device->CreateUnorderedAccessView(m_lightIndexBuffer.Get(), nullptr, &uavDesc, cpuHandle);
        }

        // Slot 3: ClusterCB CBV
        cpuHandle.ptr += descriptorSize;
        {
            const uint32_t cbSize = (sizeof(Matrix4x4) * 2 + 64 + 255) & ~255u;

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes    = cbSize;

            device->CreateConstantBufferView(&cbvDesc, cpuHandle);
        }
    }

    // Create root signature
    // [0] CBV (b0) - constant buffer (root CBV for direct GPU VA binding)
    // [1] SRV descriptor table (t0) - light data
    // [2] UAV descriptor table (u0, u1) - cluster info + light indices
    m_rootSignature = RootSignatureBuilder()
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2)
        .Build(device);

    if (!m_rootSignature)
    {
        GX_LOG_WARN("ClusteredLighting: Failed to create root signature");
        m_initialized = true;
        return true;
    }

    // Compile compute shader
    Shader shaderCompiler;
    if (!shaderCompiler.Initialize())
    {
        GX_LOG_WARN("ClusteredLighting: DXC compiler initialization failed -- PSO not created");
        m_initialized = true;
        return true;
    }

    ShaderBlob csBlob = shaderCompiler.CompileFromFile(
        L"Shaders/ClusteredLightAssign.hlsl", L"CSMain", L"cs_6_0");

    if (!csBlob.valid)
    {
        GX_LOG_WARN("ClusteredLighting: CS compile failed: %s", shaderCompiler.GetLastError().c_str());
        m_initialized = true;
        return true;
    }

    // Create compute pipeline state
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS             = csBlob.GetBytecode();

    HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_assignPSO));
    if (FAILED(hr))
    {
        GX_LOG_WARN("ClusteredLighting: Failed to create compute PSO (0x%08X)", hr);
        m_rootSignature.Reset();
        m_initialized = true;
        return true;
    }

    GX_LOG_INFO("ClusteredLighting: Initialized with %ux%ux%u clusters, PSO ready",
                m_settings.clusterCountX, m_settings.clusterCountY, m_settings.clusterCountZ);

    m_initialized = true;
    return true;
}

void ClusteredLighting::Shutdown()
{
    m_clusterInfoBuffer.Reset();
    m_lightIndexBuffer.Reset();
    m_lightDataBuffer.Reset();
    m_constantBuffer.Reset();
    m_rootSignature.Reset();
    m_assignPSO.Reset();
    m_descriptorHeap.Reset();

    m_device      = nullptr;
    m_initialized = false;
}

// -----------------------------------------------------------------------
// AssignLights
// -----------------------------------------------------------------------

void ClusteredLighting::AssignLights(
    ID3D12GraphicsCommandList* cmdList,
    const LightData* lights, uint32_t lightCount,
    const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix,
    float nearZ, float farZ,
    uint32_t screenWidth, uint32_t screenHeight,
    uint32_t frameIndex)
{
    if (!m_initialized || !cmdList || !m_settings.enabled) return;
    if (!lights || lightCount == 0) return;

    // Clamp light count
    lightCount = (lightCount > LightConstants::k_MaxLights) ? LightConstants::k_MaxLights : lightCount;

    // Upload light data
    if (m_lightDataBuffer)
    {
        void* mapped = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        if (SUCCEEDED(m_lightDataBuffer->Map(0, &readRange, &mapped)))
        {
            memcpy(mapped, lights, lightCount * sizeof(LightData));
            m_lightDataBuffer->Unmap(0, nullptr);
        }
    }

    // Upload constant buffer
    if (m_constantBuffer)
    {
        struct ClusterCB
        {
            Matrix4x4 view;
            Matrix4x4 projection;
            float       nearZ;
            float       farZ;
            uint32_t    numLights;
            uint32_t    clusterCountX;
            uint32_t    clusterCountY;
            uint32_t    clusterCountZ;
            float       screenWidth;
            float       screenHeight;
            uint32_t    maxLightsPerCluster;
            float       _pad;
        };

        ClusterCB cb = {};
        cb.view                = viewMatrix;
        cb.projection          = projMatrix;
        cb.nearZ               = nearZ;
        cb.farZ                = farZ;
        cb.numLights           = lightCount;
        cb.clusterCountX       = m_settings.clusterCountX;
        cb.clusterCountY       = m_settings.clusterCountY;
        cb.clusterCountZ       = m_settings.clusterCountZ;
        cb.screenWidth         = static_cast<float>(screenWidth);
        cb.screenHeight        = static_cast<float>(screenHeight);
        cb.maxLightsPerCluster = k_MaxLightsPerCluster;
        cb._pad                = 0.0f;

        void* mapped = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        if (SUCCEEDED(m_constantBuffer->Map(0, &readRange, &mapped)))
        {
            memcpy(mapped, &cb, sizeof(cb));
            m_constantBuffer->Unmap(0, nullptr);
        }
    }

    // Dispatch compute shader
    if (m_assignPSO && m_rootSignature)
    {
        cmdList->SetComputeRootSignature(m_rootSignature.Get());
        cmdList->SetPipelineState(m_assignPSO.Get());

        ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.Get() };
        cmdList->SetDescriptorHeaps(1, heaps);

        const uint32_t descriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_GPU_DESCRIPTOR_HANDLE gpuBase = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();

        // Root param [0]: CBV (b0) - constant buffer via root CBV (GPU virtual address)
        cmdList->SetComputeRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

        // Root param [1]: SRV descriptor table (t0) - light data at slot 0
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = gpuBase;
        cmdList->SetComputeRootDescriptorTable(1, srvHandle);

        // Root param [2]: UAV descriptor table (u0, u1) - cluster info + light indices at slots 1-2
        D3D12_GPU_DESCRIPTOR_HANDLE uavHandle = gpuBase;
        uavHandle.ptr += descriptorSize; // slot 1: clusterInfo UAV
        cmdList->SetComputeRootDescriptorTable(2, uavHandle);

        // Dispatch: one thread group per cluster
        cmdList->Dispatch(
            m_settings.clusterCountX,
            m_settings.clusterCountY,
            m_settings.clusterCountZ);
    }
}

} // namespace gx
