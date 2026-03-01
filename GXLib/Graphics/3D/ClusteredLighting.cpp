/// @file ClusteredLighting.cpp
/// @brief クラスタードライティング実装

#include "pch_graphics.h"
#include "Graphics/3D/ClusteredLighting.h"

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

    // Create constant buffer
    {
        const uint32_t cbSize = (sizeof(XMFLOAT4X4) * 2 + 64 + 255) & ~255u;

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

    // Create descriptor heap for SRV/UAV
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 4; // lightData SRV, clusterInfo UAV, lightIndices UAV, CB CBV
        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));
        if (FAILED(hr)) return false;
    }

    // TODO: Create root signature and compile CS PSO from ClusteredLightAssign.hlsl
    // For now, the system is initialized but dispatch requires PSO setup

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
    const XMFLOAT4X4& viewMatrix, const XMFLOAT4X4& projMatrix,
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
            XMFLOAT4X4 view;
            XMFLOAT4X4 projection;
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

        // Set root parameters
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
        cmdList->SetComputeRootDescriptorTable(0, gpuHandle);

        // Dispatch: one thread group per cluster
        cmdList->Dispatch(
            m_settings.clusterCountX,
            m_settings.clusterCountY,
            m_settings.clusterCountZ);
    }
}

} // namespace gx
