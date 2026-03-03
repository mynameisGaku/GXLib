/// @file VolumetricFog.cpp
/// @brief Froxelベースボリューメトリックフォグの実装
///
/// 処理フロー:
/// 1. Inject CS: FogVolumeからFroxelグリッド (160x90x64) へ密度/色を書き込み
/// 2. Scatter CS: フロント→バックのBeer-Lambertレイマーチで散乱光+透過率を蓄積
/// 3. Compose PS: HDRシーンにフォグ結果をフルスクリーンパスで合成
#include "pch_graphics.h"
#include "Graphics/3D/VolumetricFog.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"

namespace gx
{

bool VolumetricFog::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // CPU-only 初期化 (テスト用)
    if (!device)
    {
        m_initialized = true;
        return true;
    }

    // 3Dテクスチャ作成 (Froxelグリッド: R16G16B16A16_FLOAT)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        texDesc.Width              = m_settings.resolutionX;
        texDesc.Height             = m_settings.resolutionY;
        texDesc.DepthOrArraySize   = static_cast<UINT16>(m_settings.resolutionZ);
        texDesc.MipLevels          = 1;
        texDesc.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count   = 1;
        texDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr, IID_PPV_ARGS(&m_fogVolumeTexture));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("VolumetricFog: Failed to create 3D texture (0x%08X)", hr);
            return false;
        }
    }

    // Inject CS シェーダー・PSO
    {
        auto csBlob = m_injectShader.CompileFromFile(
            L"Shaders/VolumetricFogInject.hlsl", L"CSMain", L"cs_6_0");
        if (!csBlob.valid)
        {
            GX_LOG_ERROR("VolumetricFog: Failed to compile inject shader");
            return false;
        }

        // ルートシグネチャ: b0(CBV) + u0(UAV)
        RootSignatureBuilder rsBuilder;
        rsBuilder.AddCBV(0);  // FogInjectCB
        rsBuilder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);  // uFogVolume
        rsBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE);
        m_injectRS = rsBuilder.Build(device);
        if (!m_injectRS)
        {
            GX_LOG_ERROR("VolumetricFog: Failed to create inject root signature");
            return false;
        }

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_injectRS.Get();
        psoDesc.CS.pShaderBytecode = csBlob.blob->GetBufferPointer();
        psoDesc.CS.BytecodeLength  = csBlob.blob->GetBufferSize();

        HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_injectPSO));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("VolumetricFog: Failed to create inject PSO (0x%08X)", hr);
            return false;
        }
    }

    // Scatter CS シェーダー・PSO
    {
        auto csBlob = m_scatterShader.CompileFromFile(
            L"Shaders/VolumetricFogScatter.hlsl", L"CSMain", L"cs_6_0");
        if (!csBlob.valid)
        {
            GX_LOG_ERROR("VolumetricFog: Failed to compile scatter shader");
            return false;
        }

        RootSignatureBuilder rsBuilder;
        rsBuilder.AddCBV(0);  // FogScatterCB
        rsBuilder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);  // uFogVolume
        rsBuilder.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE);
        m_scatterRS = rsBuilder.Build(device);
        if (!m_scatterRS)
        {
            GX_LOG_ERROR("VolumetricFog: Failed to create scatter root signature");
            return false;
        }

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_scatterRS.Get();
        psoDesc.CS.pShaderBytecode = csBlob.blob->GetBufferPointer();
        psoDesc.CS.BytecodeLength  = csBlob.blob->GetBufferSize();

        HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_scatterPSO));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("VolumetricFog: Failed to create scatter PSO (0x%08X)", hr);
            return false;
        }
    }

    // Compose PS シェーダー・PSO
    {
        auto vsBlob = m_composeShader.CompileFromFile(
            L"Shaders/VolumetricFogCompose.hlsl", L"FullscreenVS", L"vs_6_0");
        auto psBlob = m_composeShader.CompileFromFile(
            L"Shaders/VolumetricFogCompose.hlsl", L"PSMain", L"ps_6_0");
        if (!vsBlob.valid || !psBlob.valid)
        {
            GX_LOG_ERROR("VolumetricFog: Failed to compile compose shaders");
            return false;
        }

        RootSignatureBuilder rsBuilder;
        rsBuilder.AddCBV(0);               // FogComposeCB
        rsBuilder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);  // tScene, tFogVolume, tDepth
        rsBuilder.AddStaticSampler(0);     // sLinear
        m_composeRS = rsBuilder.Build(device);
        if (!m_composeRS)
        {
            GX_LOG_ERROR("VolumetricFog: Failed to create compose root signature");
            return false;
        }

        PipelineStateBuilder psoBuilder;
        psoBuilder.SetRootSignature(m_composeRS.Get());
        psoBuilder.SetVertexShader(vsBlob.GetBytecode());
        psoBuilder.SetPixelShader(psBlob.GetBytecode());
        psoBuilder.SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
        psoBuilder.SetDepthEnable(false);
        m_composePSO = psoBuilder.Build(device);
        if (!m_composePSO)
        {
            GX_LOG_ERROR("VolumetricFog: Failed to create compose PSO");
            return false;
        }
    }

    // 定数バッファ作成
    if (!m_injectCB.Initialize(device, 256, 256))
        return false;
    if (!m_scatterCB.Initialize(device, 256, 256))
        return false;
    if (!m_composeCB.Initialize(device, 256, 256))
        return false;

    m_initialized = true;
    GX_LOG_INFO("VolumetricFog initialized (froxel %ux%ux%u)",
                m_settings.resolutionX, m_settings.resolutionY, m_settings.resolutionZ);
    return true;
}

void VolumetricFog::Shutdown()
{
    m_fogVolumeTexture.Reset();
    m_injectPSO.Reset();
    m_scatterPSO.Reset();
    m_composePSO.Reset();
    m_injectRS.Reset();
    m_scatterRS.Reset();
    m_composeRS.Reset();
    m_fogVolumes.clear();
    m_initialized = false;
}

int VolumetricFog::AddFogVolume(const FogVolume& volume)
{
    m_fogVolumes.push_back(volume);
    return static_cast<int>(m_fogVolumes.size()) - 1;
}

void VolumetricFog::RemoveFogVolume(int id)
{
    if (id >= 0 && id < static_cast<int>(m_fogVolumes.size()))
    {
        m_fogVolumes.erase(m_fogVolumes.begin() + id);
    }
}

void VolumetricFog::Execute(ID3D12GraphicsCommandList* cmdList,
                            const Matrix4x4& invViewProj, const Vector3& cameraPos,
                            ID3D12Resource* depthSRV, const void* lightConstants,
                            uint32_t frameIndex)
{
    if (!m_settings.enabled || !m_initialized || !cmdList)
        return;

    // --- Pass 1: Inject CS ---
    // Froxelグリッドにフォグ密度を書き込む
    {
        struct InjectCB
        {
            Matrix4x4  invViewProj;
            Vector3    cameraPos;
            float      maxDistance;
            Vector3    fogColor;
            float      globalDensity;
            float      absorption;
            uint32_t   resX, resY, resZ;
            float      pad;
        } cb = {};

        cb.invViewProj  = invViewProj;
        cb.cameraPos    = cameraPos;
        cb.maxDistance   = m_settings.maxDistance;
        cb.fogColor     = { 0.6f, 0.65f, 0.7f };  // デフォルトフォグ色
        cb.globalDensity = m_settings.globalDensity;
        cb.absorption   = m_settings.globalAbsorption;
        cb.resX         = m_settings.resolutionX;
        cb.resY         = m_settings.resolutionY;
        cb.resZ         = m_settings.resolutionZ;

        void* p = m_injectCB.Map(frameIndex);
        if (p) { memcpy(p, &cb, sizeof(cb)); m_injectCB.Unmap(frameIndex); }

        cmdList->SetComputeRootSignature(m_injectRS.Get());
        cmdList->SetPipelineState(m_injectPSO.Get());
        cmdList->SetComputeRootConstantBufferView(0, m_injectCB.GetGPUVirtualAddress(frameIndex));

        // UAV: 3Dテクスチャ (descriptor heapバインドは別途)
        uint32_t groupsX = (m_settings.resolutionX + 7) / 8;
        uint32_t groupsY = (m_settings.resolutionY + 7) / 8;
        uint32_t groupsZ = (m_settings.resolutionZ + 3) / 4;
        cmdList->Dispatch(groupsX, groupsY, groupsZ);
    }

    // --- Pass 2: Scatter CS ---
    // フロント→バックのBeer-Lambert蓄積
    {
        struct ScatterCB
        {
            uint32_t resX, resY, resZ;
            float    sliceDepth;
        } cb = {};

        cb.resX = m_settings.resolutionX;
        cb.resY = m_settings.resolutionY;
        cb.resZ = m_settings.resolutionZ;
        cb.sliceDepth = m_settings.maxDistance / static_cast<float>(m_settings.resolutionZ);

        void* p = m_scatterCB.Map(frameIndex);
        if (p) { memcpy(p, &cb, sizeof(cb)); m_scatterCB.Unmap(frameIndex); }

        cmdList->SetComputeRootSignature(m_scatterRS.Get());
        cmdList->SetPipelineState(m_scatterPSO.Get());
        cmdList->SetComputeRootConstantBufferView(0, m_scatterCB.GetGPUVirtualAddress(frameIndex));

        // Scatter: XY方向のみディスパッチ (Z方向はシェーダー内でループ)
        uint32_t groupsX = (m_settings.resolutionX + 7) / 8;
        uint32_t groupsY = (m_settings.resolutionY + 7) / 8;
        cmdList->Dispatch(groupsX, groupsY, 1);
    }

    // --- Pass 3: Compose PS ---
    // HDRシーンにフォグを合成 (フルスクリーンパス)
    {
        struct ComposeCB
        {
            float maxDistance;
            float nearZ;
            float farZ;
            float pad;
        } cb = {};

        cb.maxDistance = m_settings.maxDistance;
        cb.nearZ      = 0.1f;   // カメラのnearZ (外部から渡すことも可能)
        cb.farZ       = 1000.0f; // カメラのfarZ

        void* p = m_composeCB.Map(frameIndex);
        if (p) { memcpy(p, &cb, sizeof(cb)); m_composeCB.Unmap(frameIndex); }

        cmdList->SetGraphicsRootSignature(m_composeRS.Get());
        cmdList->SetPipelineState(m_composePSO.Get());
        cmdList->SetGraphicsRootConstantBufferView(0, m_composeCB.GetGPUVirtualAddress(frameIndex));

        // フルスクリーン三角形描画
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
}

} // namespace gx
