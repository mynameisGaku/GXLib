/// @file LightProbeSystem.cpp
/// @brief DDGI方式ライトプローブシステムの実装
///
/// アトラステクスチャとして格納される放射照度/深度プローブの3Dグリッドを作成する。
/// Computeシェーダーがフレームごとに時間的ヒステリシスでプローブデータを更新する。
/// ComputeSkinningと同じComputeディスパッチパターンに従う。
#include "pch_graphics.h"
#include "Graphics/3D/LightProbeSystem.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"
#include <cmath>

namespace gx
{

uint32_t LightProbeSystem::GetProbeCount() const
{
    return m_config.dimensions.x * m_config.dimensions.y * m_config.dimensions.z;
}

bool LightProbeSystem::Initialize(ID3D12Device* device, const ProbeGridConfig& config)
{
    if (!device) return false;
    m_device = device;
    m_config = config;

    uint32_t probeCount = GetProbeCount();
    if (probeCount == 0)
    {
        GX_LOG_ERROR("LightProbeSystem: Grid dimensions are zero");
        return false;
    }

    // UAVヒープ: 2スロット（放射照度UAV + 深度UAV）
    if (!m_uavHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4, true))
        return false;

    // SRVヒープ: 2スロット（放射照度SRV + 深度SRV）ライティングシェーダーでのサンプリング用
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true))
        return false;

    if (!m_shader.Initialize())
    {
        GX_LOG_WARN("LightProbeSystem: DXC compiler initialization failed");
        return true;
    }

    // CB: 64バイト、CBアライメントのため256にパディング
    if (!m_updateCB.Initialize(device, 256, 256))
        return false;

    if (!CreateAtlasTextures(device))
        return false;

    if (!CreatePipelines(device))
        return false;

    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/LightProbeUpdate.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    m_initialized = true;
    GX_LOG_INFO("LightProbeSystem initialized (%dx%dx%d grid, %d probes)",
                config.dimensions.x, config.dimensions.y, config.dimensions.z, probeCount);
    return true;
}

bool LightProbeSystem::CreateAtlasTextures(ID3D12Device* device)
{
    uint32_t probeCount = GetProbeCount();

    // アトラスレイアウトの計算: プローブを行に配置
    int probesPerRow = static_cast<int>(ceilf(sqrtf(static_cast<float>(probeCount))));
    int rows = (static_cast<int>(probeCount) + probesPerRow - 1) / probesPerRow;

    // 放射照度アトラス
    m_irradianceAtlasWidth  = probesPerRow * k_IrradianceProbeSize;
    m_irradianceAtlasHeight = rows * k_IrradianceProbeSize;

    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width              = static_cast<UINT64>(m_irradianceAtlasWidth);
        texDesc.Height             = static_cast<UINT>(m_irradianceAtlasHeight);
        texDesc.DepthOrArraySize   = 1;
        texDesc.MipLevels          = 1;
        texDesc.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count   = 1;
        texDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &texDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr, IID_PPV_ARGS(&m_irradianceAtlas));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("LightProbeSystem: Failed to create irradiance atlas");
            return false;
        }
    }

    // 深度アトラス
    m_depthAtlasWidth  = probesPerRow * k_DepthProbeSize;
    m_depthAtlasHeight = rows * k_DepthProbeSize;

    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width              = static_cast<UINT64>(m_depthAtlasWidth);
        texDesc.Height             = static_cast<UINT>(m_depthAtlasHeight);
        texDesc.DepthOrArraySize   = 1;
        texDesc.MipLevels          = 1;
        texDesc.Format             = DXGI_FORMAT_R16G16_FLOAT;
        texDesc.SampleDesc.Count   = 1;
        texDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE,
            &texDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr, IID_PPV_ARGS(&m_depthAtlas));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("LightProbeSystem: Failed to create depth atlas");
            return false;
        }
    }

    // UAVディスクリプタ作成
    // スロット0: 放射照度UAV
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format               = DXGI_FORMAT_R16G16B16A16_FLOAT;
        uavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice   = 0;
        device->CreateUnorderedAccessView(m_irradianceAtlas.Get(), nullptr, &uavDesc,
                                           m_uavHeap.GetCPUHandle(0));
    }

    // スロット1: 深度UAV
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format               = DXGI_FORMAT_R16G16_FLOAT;
        uavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice   = 0;
        device->CreateUnorderedAccessView(m_depthAtlas.Get(), nullptr, &uavDesc,
                                           m_uavHeap.GetCPUHandle(1));
    }

    // サンプリング用SRVディスクリプタ作成
    // スロット0: 放射照度SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R16G16B16A16_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        device->CreateShaderResourceView(m_irradianceAtlas.Get(), &srvDesc,
                                          m_srvHeap.GetCPUHandle(0));
    }

    // スロット1: 深度SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R16G16_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        device->CreateShaderResourceView(m_depthAtlas.Get(), &srvDesc,
                                          m_srvHeap.GetCPUHandle(1));
    }

    return true;
}

bool LightProbeSystem::CreatePipelines(ID3D12Device* device)
{
    // ルートシグネチャ: b0 (CBV) + u0 (放射照度UAV) + u1 (深度UAV)
    D3D12_ROOT_PARAMETER params[3] = {};

    // [0] b0のCBV
    params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].Descriptor.RegisterSpace  = 0;
    params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

    // [1] ディスクリプタテーブル: u0（放射照度UAV）
    D3D12_DESCRIPTOR_RANGE uavRange0 = {};
    uavRange0.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange0.NumDescriptors     = 1;
    uavRange0.BaseShaderRegister = 0;

    params[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges   = &uavRange0;
    params[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    // [2] ディスクリプタテーブル: u1（深度UAV）
    D3D12_DESCRIPTOR_RANGE uavRange1 = {};
    uavRange1.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange1.NumDescriptors     = 1;
    uavRange1.BaseShaderRegister = 1;

    params[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 1;
    params[2].DescriptorTable.pDescriptorRanges   = &uavRange1;
    params[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 3;
    rsDesc.pParameters   = params;

    ComPtr<ID3DBlob> signature, error;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              &signature, &error);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("LightProbeSystem: Failed to serialize root signature");
        return false;
    }

    hr = device->CreateRootSignature(0, signature->GetBufferPointer(),
                                      signature->GetBufferSize(),
                                      IID_PPV_ARGS(&m_updateRS));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("LightProbeSystem: Failed to create root signature");
        return false;
    }

    // Computeシェーダーのコンパイル
    auto csBlob = m_shader.CompileFromFile(L"Shaders/LightProbeUpdate.hlsl", L"CS", L"cs_6_0");
    if (!csBlob.valid)
    {
        GX_LOG_WARN("LightProbeSystem: Shader compilation failed (shader may not exist yet)");
        m_initialized = false;
        return true; // Non-fatal: shader may not exist in test environment
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_updateRS.Get();
    psoDesc.CS = csBlob.GetBytecode();

    hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_updatePSO));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("LightProbeSystem: Failed to create compute PSO");
        return false;
    }

    return true;
}

void LightProbeSystem::Update(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)
{
    if (!m_enabled || !m_initialized || !m_updatePSO) return;

    uint32_t probeCount = GetProbeCount();

    // 定数バッファの書き込み
    LightProbeUpdateConstants cb = {};
    cb.gridOrigin         = m_config.origin;
    cb.gridSpacing        = m_config.spacing;
    cb.hysteresis         = m_hysteresis;
    cb.gridDimensions     = m_config.dimensions;
    cb.raysPerProbe       = m_raysPerProbe;
    cb.probeCount         = static_cast<int>(probeCount);
    cb.irradianceTexWidth = m_irradianceAtlasWidth;
    cb.depthTexWidth      = m_depthAtlasWidth;

    void* p = m_updateCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &cb, sizeof(cb));
        m_updateCB.Unmap(frameIndex);
    }

    // Computeパイプラインの設定
    cmdList->SetComputeRootSignature(m_updateRS.Get());
    cmdList->SetPipelineState(m_updatePSO.Get());

    ID3D12DescriptorHeap* heaps[] = { m_uavHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetComputeRootConstantBufferView(0, m_updateCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetComputeRootDescriptorTable(1, m_uavHeap.GetGPUHandle(0));
    cmdList->SetComputeRootDescriptorTable(2, m_uavHeap.GetGPUHandle(1));

    // ディスパッチ: グループあたり64スレッド（[numthreads(64,1,1)]に対応）
    uint32_t groupCount = (probeCount + 63) / 64;
    cmdList->Dispatch(groupCount, 1, 1);

    // サンプリング前に書き込み完了を保証するUAVバリア
    D3D12_RESOURCE_BARRIER barriers[2] = {};

    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource = m_irradianceAtlas.Get();

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[1].UAV.pResource = m_depthAtlas.Get();

    cmdList->ResourceBarrier(2, barriers);
}

void LightProbeSystem::BindForSampling(ID3D12GraphicsCommandList* cmdList, uint32_t rootParamIndex)
{
    if (!m_initialized) return;

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootDescriptorTable(rootParamIndex, m_srvHeap.GetGPUHandle(0));
}

void LightProbeSystem::OnResize(ID3D12Device* /*device*/, uint32_t /*width*/, uint32_t /*height*/)
{
    // プローブシステムは解像度非依存なため、リサイズ時は何もしない
}

void LightProbeSystem::Shutdown()
{
    m_updatePSO.Reset();
    m_updateRS.Reset();
    m_irradianceAtlas.Reset();
    m_depthAtlas.Reset();
    m_device = nullptr;
    m_initialized = false;
    m_enabled = false;
}

} // namespace gx
