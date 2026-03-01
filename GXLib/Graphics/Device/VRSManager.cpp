/// @file VRSManager.cpp
/// @brief Variable Rate Shading (VRS) 管理の実装
#include "pch_graphics.h"
#include "Graphics/Device/VRSManager.h"
#include "Core/Logger.h"

namespace gx
{

bool VRSManager::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    if (!device)
    {
        GX_LOG_ERROR("VRSManager::Initialize: device is null");
        return false;
    }

    m_tier = VRSTier::None;
    m_tileSize = 16;

    // VRS対応レベルを検出 (D3D12_FEATURE_DATA_D3D12_OPTIONS6)
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    HRESULT hr = device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));

    if (SUCCEEDED(hr))
    {
        if (options6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
        {
            m_tier = VRSTier::Tier2;
            m_tileSize = options6.ShadingRateImageTileSize;
            GX_LOG_INFO("VRS Tier 2 supported (tile size: %u pixels)", m_tileSize);

            // Tier 2ではSRI画像を作成する
            if (!CreateShadingRateImage(device, width, height))
            {
                GX_LOG_WARN("Failed to create initial SRI -- VRS Image mode unavailable");
            }

            // --- コンピュートパイプラインセットアップ (SRI自動生成用) ---
            m_screenWidth  = width;
            m_screenHeight = height;

            if (m_shader.Initialize())
            {
                auto csBlob = m_shader.CompileFromFile(L"Shaders/VRSGenerate.hlsl",
                                                        L"CSMain", L"cs_6_0");
                if (csBlob.valid)
                {
                    // コンピュートルートシグネチャ:
                    //   param[0]: b0 = 32bit constants (8 DWORDs)
                    //   param[1]: t0+t1 SRV descriptor table
                    //   param[2]: u0 UAV descriptor table
                    D3D12_ROOT_PARAMETER cParams[3] = {};

                    cParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                    cParams[0].Constants.ShaderRegister = 0;
                    cParams[0].Constants.RegisterSpace  = 0;
                    cParams[0].Constants.Num32BitValues = 8;
                    cParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                    D3D12_DESCRIPTOR_RANGE srvRanges = {};
                    srvRanges.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    srvRanges.NumDescriptors     = 2; // t0 + t1
                    srvRanges.BaseShaderRegister = 0;
                    srvRanges.RegisterSpace      = 0;
                    srvRanges.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                    cParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    cParams[1].DescriptorTable.NumDescriptorRanges = 1;
                    cParams[1].DescriptorTable.pDescriptorRanges   = &srvRanges;
                    cParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                    D3D12_DESCRIPTOR_RANGE uavRange = {};
                    uavRange.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    uavRange.NumDescriptors     = 1; // u0
                    uavRange.BaseShaderRegister = 0;
                    uavRange.RegisterSpace      = 0;
                    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                    cParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    cParams[2].DescriptorTable.NumDescriptorRanges = 1;
                    cParams[2].DescriptorTable.pDescriptorRanges   = &uavRange;
                    cParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                    D3D12_ROOT_SIGNATURE_DESC crsDesc = {};
                    crsDesc.NumParameters = 3;
                    crsDesc.pParameters   = cParams;

                    ComPtr<ID3DBlob> sigBlob, errBlob;
                    HRESULT rsHr = D3D12SerializeRootSignature(&crsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                                                &sigBlob, &errBlob);
                    if (SUCCEEDED(rsHr))
                    {
                        rsHr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(),
                                                            sigBlob->GetBufferSize(),
                                                            IID_PPV_ARGS(&m_computeRootSig));
                    }

                    if (SUCCEEDED(rsHr))
                    {
                        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
                        psoDesc.pRootSignature = m_computeRootSig.Get();
                        psoDesc.CS = csBlob.GetBytecode();

                        rsHr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_computePSO));
                    }

                    if (SUCCEEDED(rsHr))
                    {
                        // ディスクリプタヒープ (3 descriptors: t0 SRV, t1 SRV, u0 UAV)
                        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
                        heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                        heapDesc.NumDescriptors = 3;
                        heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

                        rsHr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));
                    }

                    if (SUCCEEDED(rsHr))
                    {
                        m_computeReady = true;
                        GX_LOG_INFO("VRS: Compute pipeline for SRI auto-generation ready");
                    }
                    else
                    {
                        GX_LOG_WARN("VRS: Failed to create compute pipeline for SRI generation");
                    }
                }
                else
                {
                    GX_LOG_WARN("VRS: VRSGenerate.hlsl compilation failed (shader may not exist yet)");
                }
            }
            else
            {
                GX_LOG_WARN("VRS: DXC compiler initialization failed -- SRI auto-generation unavailable");
            }
        }
        else if (options6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1)
        {
            m_tier = VRSTier::Tier1;
            GX_LOG_INFO("VRS Tier 1 supported (per-draw shading rate only)");
        }
        else
        {
            GX_LOG_INFO("VRS not supported on this GPU");
        }
    }
    else
    {
        GX_LOG_INFO("VRS feature query failed -- VRS not available");
    }

    return true;
}

void VRSManager::SetShadingRate(ID3D12GraphicsCommandList5* cmdList, D3D12_SHADING_RATE rate)
{
    if (!cmdList || m_tier == VRSTier::None)
        return;

    D3D12_SHADING_RATE_COMBINER combiners[2] = {
        D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,
        D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,
    };

    cmdList->RSSetShadingRate(rate, combiners);
}

void VRSManager::SetShadingRateCombiner(ID3D12GraphicsCommandList5* cmdList,
                                         D3D12_SHADING_RATE_COMBINER combiner0,
                                         D3D12_SHADING_RATE_COMBINER combiner1)
{
    if (!cmdList || m_tier == VRSTier::None)
        return;

    D3D12_SHADING_RATE_COMBINER combiners[2] = { combiner0, combiner1 };
    cmdList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
}

bool VRSManager::CreateShadingRateImage(ID3D12Device* device, uint32_t width, uint32_t height)
{
    if (!device || m_tier < VRSTier::Tier2)
    {
        return false;
    }

    if (m_tileSize == 0)
    {
        GX_LOG_ERROR("VRSManager: tile size is 0");
        return false;
    }

    // SRI画像のサイズはタイル数（画面サイズをタイルサイズで割る、切り上げ）
    m_sriWidth  = (width  + m_tileSize - 1) / m_tileSize;
    m_sriHeight = (height + m_tileSize - 1) / m_tileSize;

    // 既存のSRI画像を解放
    m_shadingRateImage.Reset();

    // SRI画像はR8_UINT形式で、各ピクセルがD3D12_SHADING_RATE値を格納する
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width              = m_sriWidth;
    desc.Height             = m_sriHeight;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_R8_UINT;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
        nullptr,
        IID_PPV_ARGS(&m_shadingRateImage)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create SRI texture (HRESULT: 0x%08X)", hr);
        return false;
    }

    m_shadingRateImage->SetName(L"VRS_ShadingRateImage");
    GX_LOG_INFO("VRS SRI created: %ux%u tiles (tile size: %u)", m_sriWidth, m_sriHeight, m_tileSize);
    return true;
}

void VRSManager::GenerateShadingRateImage(ID3D12GraphicsCommandList* cmdList,
                                           ID3D12Resource* hdrRT, ID3D12Resource* velocityRT)
{
    if (!cmdList || !m_shadingRateImage || m_tier < VRSTier::Tier2)
        return;

    if (!m_computeReady)
    {
        // コンピュートパイプライン未構築の場合は何もしない（後方互換）
        (void)hdrRT;
        (void)velocityRT;
        return;
    }

    // リソースバリア: SRI を SHADING_RATE_SOURCE → UNORDERED_ACCESS
    D3D12_RESOURCE_BARRIER barrierToUAV = {};
    barrierToUAV.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToUAV.Transition.pResource   = m_shadingRateImage.Get();
    barrierToUAV.Transition.StateBefore = D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    barrierToUAV.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrierToUAV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrierToUAV);

    // PSO + ルートシグネチャをセット
    cmdList->SetComputeRootSignature(m_computeRootSig.Get());
    cmdList->SetPipelineState(m_computePSO.Get());

    // ディスクリプタヒープをバインド
    ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.Get() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // SRV/UAV ディスクリプタを動的に作成
    // ディスクリプタヒープはInitializeで確保済み (3 descriptors: t0, t1, u0)
    // ここではComPtr<ID3D12Device>を取得してディスクリプタを更新する
    ComPtr<ID3D12Device> device;
    cmdList->GetDevice(IID_PPV_ARGS(&device));

    if (device)
    {
        UINT incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

        // t0: HDR render target SRV
        if (hdrRT)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                  = DXGI_FORMAT_R16G16B16A16_FLOAT;
            srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels     = 1;
            device->CreateShaderResourceView(hdrRT, &srvDesc, cpuHandle);
        }

        // t1: velocity buffer SRV
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle1 = cpuHandle;
        cpuHandle1.ptr += incrementSize;
        if (velocityRT)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                  = DXGI_FORMAT_R16G16_FLOAT;
            srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels     = 1;
            device->CreateShaderResourceView(velocityRT, &srvDesc, cpuHandle1);
        }
        else
        {
            // velocityRT が null の場合、null SRV を作成
            D3D12_SHADER_RESOURCE_VIEW_DESC nullSrv = {};
            nullSrv.Format                  = DXGI_FORMAT_R16G16_FLOAT;
            nullSrv.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
            nullSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            device->CreateShaderResourceView(nullptr, &nullSrv, cpuHandle1);
        }

        // u0: SRI UAV
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle2 = cpuHandle;
        cpuHandle2.ptr += incrementSize * 2;
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format               = DXGI_FORMAT_R8_UINT;
            uavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice   = 0;
            device->CreateUnorderedAccessView(m_shadingRateImage.Get(), nullptr, &uavDesc, cpuHandle2);
        }
    }

    // ディスクリプタテーブルをバインド
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    UINT incrementSize = 0;
    {
        ComPtr<ID3D12Device> dev;
        cmdList->GetDevice(IID_PPV_ARGS(&dev));
        if (dev)
            incrementSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // SRV table (t0, t1) at slot 1
    cmdList->SetComputeRootDescriptorTable(1, gpuHandle);

    // UAV table (u0) at slot 2
    D3D12_GPU_DESCRIPTOR_HANDLE uavGpuHandle = gpuHandle;
    uavGpuHandle.ptr += incrementSize * 2;
    cmdList->SetComputeRootDescriptorTable(2, uavGpuHandle);

    // 32bit定数をセット: tileSize, screenWidth, screenHeight, sriWidth, sriHeight + padding
    uint32_t constants[8] = {};
    constants[0] = m_tileSize;
    constants[1] = m_screenWidth;
    constants[2] = m_screenHeight;
    constants[3] = m_sriWidth;
    constants[4] = m_sriHeight;
    constants[5] = 0; // padding
    constants[6] = 0; // padding
    constants[7] = 0; // padding
    cmdList->SetComputeRoot32BitConstants(0, 8, constants, 0);

    // Dispatch: SRIの各タイルに対して1スレッド、8x8スレッドグループ
    uint32_t groupX = (m_sriWidth  + 7) / 8;
    uint32_t groupY = (m_sriHeight + 7) / 8;
    cmdList->Dispatch(groupX, groupY, 1);

    // リソースバリア: SRI を UNORDERED_ACCESS → SHADING_RATE_SOURCE
    D3D12_RESOURCE_BARRIER barrierToSRS = {};
    barrierToSRS.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToSRS.Transition.pResource   = m_shadingRateImage.Get();
    barrierToSRS.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrierToSRS.Transition.StateAfter  = D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    barrierToSRS.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrierToSRS);
}

void VRSManager::BindShadingRateImage(ID3D12GraphicsCommandList5* cmdList)
{
    if (!cmdList || !m_shadingRateImage || m_tier < VRSTier::Tier2)
        return;

    cmdList->RSSetShadingRateImage(m_shadingRateImage.Get());
}

void VRSManager::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    if (m_tier < VRSTier::Tier2)
        return;

    // 画面サイズを更新
    m_screenWidth  = width;
    m_screenHeight = height;

    uint32_t newSriWidth  = (width  + m_tileSize - 1) / m_tileSize;
    uint32_t newSriHeight = (height + m_tileSize - 1) / m_tileSize;

    // サイズが変わった場合のみ再作成
    if (newSriWidth != m_sriWidth || newSriHeight != m_sriHeight)
    {
        CreateShadingRateImage(device, width, height);
    }
}

} // namespace gx
