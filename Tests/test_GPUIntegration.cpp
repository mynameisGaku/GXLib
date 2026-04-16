/// @file test_GPUIntegration.cpp
/// @brief GPU integration tests using WARP software adapter for headless D3D12 testing

#include "pch.h"
#include <gtest/gtest.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// ============================================================================
// GPU Integration Test Fixture (WARP software adapter)
// ============================================================================

class GPUIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_available = TryCreateWARPDevice();
    }

    void TearDown() override
    {
        m_device.Reset();
        m_factory.Reset();
    }

    bool TryCreateWARPDevice()
    {
        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_factory));
        if (FAILED(hr)) return false;

        ComPtr<IDXGIAdapter> warpAdapter;
        hr = m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
        if (FAILED(hr)) return false;

        hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0,
                               IID_PPV_ARGS(&m_device));
        if (FAILED(hr)) return false;

        return true;
    }

    bool m_available = false;
    ComPtr<ID3D12Device> m_device;
    ComPtr<IDXGIFactory4> m_factory;
};

// ============================================================================
// Device Creation
// ============================================================================

TEST_F(GPUIntegrationTest, CreateWARPDevice)
{
    if (!m_available) GTEST_SKIP();
    EXPECT_NE(m_device.Get(), nullptr);
}

// ============================================================================
// Command Queue
// ============================================================================

TEST_F(GPUIntegrationTest, CreateCommandQueue)
{
    if (!m_available) GTEST_SKIP();

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    ComPtr<ID3D12CommandQueue> queue;
    HRESULT hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(queue.Get(), nullptr);
}

// ============================================================================
// Command Allocator
// ============================================================================

TEST_F(GPUIntegrationTest, CreateCommandAllocator)
{
    if (!m_available) GTEST_SKIP();

    ComPtr<ID3D12CommandAllocator> allocator;
    HRESULT hr = m_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(allocator.Get(), nullptr);
}

// ============================================================================
// Command List
// ============================================================================

TEST_F(GPUIntegrationTest, CreateCommandList)
{
    if (!m_available) GTEST_SKIP();

    ComPtr<ID3D12CommandAllocator> allocator;
    HRESULT hr = m_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
    ASSERT_TRUE(SUCCEEDED(hr));

    ComPtr<ID3D12GraphicsCommandList> cmdList;
    hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     allocator.Get(), nullptr,
                                     IID_PPV_ARGS(&cmdList));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(cmdList.Get(), nullptr);

    cmdList->Close();
}

// ============================================================================
// Fence
// ============================================================================

TEST_F(GPUIntegrationTest, CreateFence)
{
    if (!m_available) GTEST_SKIP();

    ComPtr<ID3D12Fence> fence;
    HRESULT hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(fence.Get(), nullptr);
    EXPECT_EQ(fence->GetCompletedValue(), 0u);
}

// ============================================================================
// Descriptor Heap
// ============================================================================

TEST_F(GPUIntegrationTest, CreateDescriptorHeapCBVSRVUAV)
{
    if (!m_available) GTEST_SKIP();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 16;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ComPtr<ID3D12DescriptorHeap> heap;
    HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(heap.Get(), nullptr);
}

TEST_F(GPUIntegrationTest, CreateDescriptorHeapRTV)
{
    if (!m_available) GTEST_SKIP();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 4;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ComPtr<ID3D12DescriptorHeap> heap;
    HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(heap.Get(), nullptr);
}

// ============================================================================
// Root Signature
// ============================================================================

TEST_F(GPUIntegrationTest, CreateEmptyRootSignature)
{
    if (!m_available) GTEST_SKIP();

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> serialized;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              &serialized, &errorBlob);
    ASSERT_TRUE(SUCCEEDED(hr));

    ComPtr<ID3D12RootSignature> rootSig;
    hr = m_device->CreateRootSignature(0, serialized->GetBufferPointer(),
                                        serialized->GetBufferSize(),
                                        IID_PPV_ARGS(&rootSig));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(rootSig.Get(), nullptr);
}

// ============================================================================
// Upload Buffer
// ============================================================================

TEST_F(GPUIntegrationTest, CreateUploadBuffer)
{
    if (!m_available) GTEST_SKIP();

    const UINT bufferSize = 1024;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = bufferSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> uploadBuffer;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&uploadBuffer));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(uploadBuffer.Get(), nullptr);
}

// ============================================================================
// Default Buffer
// ============================================================================

TEST_F(GPUIntegrationTest, CreateDefaultBuffer)
{
    if (!m_available) GTEST_SKIP();

    const UINT bufferSize = 4096;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = bufferSize;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> defaultBuffer;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &resDesc, D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&defaultBuffer));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(defaultBuffer.Get(), nullptr);
}

// ============================================================================
// Buffer Upload + Readback
// ============================================================================

TEST_F(GPUIntegrationTest, BufferUploadReadback)
{
    if (!m_available) GTEST_SKIP();

    // Create command infrastructure
    ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue))));

    ComPtr<ID3D12CommandAllocator> allocator;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))));

    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
        IID_PPV_ARGS(&cmdList))));

    ComPtr<ID3D12Fence> fence;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))));

    // Test data
    const UINT dataSize = 256;
    gx::Vector<uint32_t> srcData(dataSize / sizeof(uint32_t));
    for (size_t i = 0; i < srcData.size(); ++i)
        srcData[i] = static_cast<uint32_t>(i * 42);

    // Create upload buffer
    D3D12_HEAP_PROPERTIES uploadProps = {};
    uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = dataSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> uploadBuf;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommittedResource(
        &uploadProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf))));

    // Map and write data
    void* mapped = nullptr;
    uploadBuf->Map(0, nullptr, &mapped);
    memcpy(mapped, srcData.data(), dataSize);
    uploadBuf->Unmap(0, nullptr);

    // Create default buffer
    D3D12_HEAP_PROPERTIES defaultProps = {};
    defaultProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> defaultBuf;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommittedResource(
        &defaultProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuf))));

    // Create readback buffer
    D3D12_HEAP_PROPERTIES readbackProps = {};
    readbackProps.Type = D3D12_HEAP_TYPE_READBACK;
    ComPtr<ID3D12Resource> readbackBuf;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommittedResource(
        &readbackProps, D3D12_HEAP_FLAG_NONE, &bufDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readbackBuf))));

    // Upload -> Default -> Readback
    cmdList->CopyBufferRegion(defaultBuf.Get(), 0, uploadBuf.Get(), 0, dataSize);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = defaultBuf.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->CopyBufferRegion(readbackBuf.Get(), 0, defaultBuf.Get(), 0, dataSize);

    cmdList->Close();

    ID3D12CommandList* lists[] = { cmdList.Get() };
    queue->ExecuteCommandLists(1, lists);
    queue->Signal(fence.Get(), 1);

    // Wait for GPU
    if (fence->GetCompletedValue() < 1)
    {
        HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, 5000);
        CloseHandle(event);
    }

    // Read back and verify
    void* readMapped = nullptr;
    D3D12_RANGE readRange = { 0, dataSize };
    readbackBuf->Map(0, &readRange, &readMapped);

    auto* readData = static_cast<uint32_t*>(readMapped);
    for (size_t i = 0; i < srcData.size(); ++i)
    {
        EXPECT_EQ(readData[i], srcData[i]);
    }

    D3D12_RANGE writeRange = { 0, 0 };
    readbackBuf->Unmap(0, &writeRange);
}

// ============================================================================
// Texture2D
// ============================================================================

TEST_F(GPUIntegrationTest, CreateTexture2D)
{
    if (!m_available) GTEST_SKIP();

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = 64;
    texDesc.Height = 64;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> texture;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &texDesc, D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&texture));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(texture.Get(), nullptr);
}

// ============================================================================
// Render Target
// ============================================================================

TEST_F(GPUIntegrationTest, CreateRenderTarget)
{
    if (!m_available) GTEST_SKIP();

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC rtDesc = {};
    rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rtDesc.Width = 128;
    rtDesc.Height = 128;
    rtDesc.DepthOrArraySize = 1;
    rtDesc.MipLevels = 1;
    rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtDesc.SampleDesc.Count = 1;
    rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearVal = {};
    clearVal.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clearVal.Color[0] = 0.0f;
    clearVal.Color[1] = 0.0f;
    clearVal.Color[2] = 0.0f;
    clearVal.Color[3] = 1.0f;

    ComPtr<ID3D12Resource> renderTarget;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clearVal, IID_PPV_ARGS(&renderTarget));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(renderTarget.Get(), nullptr);
}

// ============================================================================
// Clear Render Target
// ============================================================================

TEST_F(GPUIntegrationTest, ClearRenderTarget)
{
    if (!m_available) GTEST_SKIP();

    // Create RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap))));

    // Create render target
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rtDesc = {};
    rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rtDesc.Width = 64;
    rtDesc.Height = 64;
    rtDesc.DepthOrArraySize = 1;
    rtDesc.MipLevels = 1;
    rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtDesc.SampleDesc.Count = 1;
    rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearVal = {};
    clearVal.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    ComPtr<ID3D12Resource> rt;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &rtDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearVal, IID_PPV_ARGS(&rt))));

    // Create RTV
    m_device->CreateRenderTargetView(rt.Get(), nullptr, rtvHeap->GetCPUDescriptorHandleForHeapStart());

    // Create command infrastructure
    ComPtr<ID3D12CommandAllocator> allocator;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))));

    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ASSERT_TRUE(SUCCEEDED(m_device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
        IID_PPV_ARGS(&cmdList))));

    // Clear to red
    float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(rtvHeap->GetCPUDescriptorHandleForHeapStart(),
                                    clearColor, 0, nullptr);

    HRESULT hr = cmdList->Close();
    EXPECT_TRUE(SUCCEEDED(hr));
}

// ============================================================================
// GPU Timestamp Query
// ============================================================================

TEST_F(GPUIntegrationTest, GPUTimestampQuery)
{
    if (!m_available) GTEST_SKIP();

    // Create timestamp query heap
    D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
    queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    queryHeapDesc.Count = 2;

    ComPtr<ID3D12QueryHeap> queryHeap;
    HRESULT hr = m_device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&queryHeap));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(queryHeap.Get(), nullptr);
}

// ============================================================================
// Descriptor Heap Size Query
// ============================================================================

TEST_F(GPUIntegrationTest, DescriptorIncrementSize)
{
    if (!m_available) GTEST_SKIP();

    UINT cbvSrvUavSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    UINT rtvSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    UINT dsvSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    UINT samplerSize = m_device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    EXPECT_GT(cbvSrvUavSize, 0u);
    EXPECT_GT(rtvSize, 0u);
    EXPECT_GT(dsvSize, 0u);
    EXPECT_GT(samplerSize, 0u);
}
