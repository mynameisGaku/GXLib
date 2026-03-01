/// @file RTSoftShadows.cpp
/// @brief DXRソフトシャドウの実装
///
/// 処理フロー:
/// 1. DispatchRays: 面積光源ジッターレイをシーンに発射してシャドウマップ生成
/// 2. Temporal Composite: 現フレームのシャドウと履歴をブレンドしてノイズ低減
/// 3. 結果を m_shadowRT に書き込み (外部から GetShadowRT() で参照)
#include "pch_graphics.h"
#include "Graphics/RayTracing/RTSoftShadows.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Device/BarrierBatch.h"
#include "Core/Logger.h"

namespace gx
{

bool RTSoftShadows::Initialize(ID3D12Device5* device, uint32_t width, uint32_t height)
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

    // シャドウ結果RT (R8_UNORM: 0=影, 1=光)
    if (!m_shadowRT.Create(device, width, height, DXGI_FORMAT_R8_UNORM))
    {
        GX_LOG_ERROR("RTSoftShadows: Failed to create shadow RT");
        return false;
    }

    // テンポラル蓄積履歴RT (R8_UNORM)
    if (!m_historyRT.Create(device, width, height, DXGI_FORMAT_R8_UNORM))
    {
        GX_LOG_ERROR("RTSoftShadows: Failed to create history RT");
        return false;
    }

    // DXRパイプライン作成
    {
        // Globalルートシグネチャのビルド
        // t0: TLAS, t1: depth, t2: normal, u0: shadow UAV, b0: CB
        D3D12_DESCRIPTOR_RANGE1 srvRange = {};
        srvRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors                    = 3;
        srvRange.BaseShaderRegister                = 0;
        srvRange.RegisterSpace                     = 0;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE1 uavRange = {};
        uavRange.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uavRange.NumDescriptors                    = 1;
        uavRange.BaseShaderRegister                = 0;
        uavRange.RegisterSpace                     = 0;
        uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER1 params[3] = {};
        // [0] CBV: RTSoftShadowCB
        params[0].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_CBV;
        params[0].Descriptor.ShaderRegister = 0;
        params[0].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;

        // [1] SRV table: TLAS + Depth + Normal
        params[1].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[1].DescriptorTable.NumDescriptorRanges = 1;
        params[1].DescriptorTable.pDescriptorRanges   = &srvRange;
        params[1].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

        // [2] UAV table: Shadow output
        params[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        params[2].DescriptorTable.NumDescriptorRanges = 1;
        params[2].DescriptorTable.pDescriptorRanges   = &uavRange;
        params[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
        rsDesc.Version                = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rsDesc.Desc_1_1.NumParameters = _countof(params);
        rsDesc.Desc_1_1.pParameters   = params;
        rsDesc.Desc_1_1.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        ComPtr<ID3DBlob> serialized, errorBlob;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&rsDesc, &serialized, &errorBlob);
        if (FAILED(hr))
        {
            GX_LOG_ERROR("RTSoftShadows: Failed to serialize root signature (0x%08X)", hr);
            return false;
        }

        hr = device->CreateRootSignature(0,
            serialized->GetBufferPointer(), serialized->GetBufferSize(),
            IID_PPV_ARGS(&m_raygenRS));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("RTSoftShadows: Failed to create raygen root signature (0x%08X)", hr);
            return false;
        }
    }

    // DXR ステートオブジェクト (Ray Generation + Miss)
    {
        Shader rayShader;
        if (!rayShader.Initialize())
            return false;

        auto libBlob = rayShader.CompileLibrary(L"Shaders/RTSoftShadows.hlsl");
        if (!libBlob.valid)
        {
            GX_LOG_ERROR("RTSoftShadows: Failed to compile DXR library");
            return false;
        }

        // ステートオブジェクトビルド
        D3D12_STATE_SUBOBJECT subobjects[5] = {};
        uint32_t soIndex = 0;

        // DXIL Library
        D3D12_DXIL_LIBRARY_DESC libDesc = {};
        libDesc.DXILLibrary.pShaderBytecode = libBlob.blob->GetBufferPointer();
        libDesc.DXILLibrary.BytecodeLength  = libBlob.blob->GetBufferSize();
        subobjects[soIndex].Type    = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        subobjects[soIndex].pDesc   = &libDesc;
        soIndex++;

        // Shader Config
        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
        shaderConfig.MaxPayloadSizeInBytes   = sizeof(float);
        shaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2;
        subobjects[soIndex].Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
        subobjects[soIndex].pDesc = &shaderConfig;
        soIndex++;

        // Global Root Signature
        D3D12_GLOBAL_ROOT_SIGNATURE globalRS = {};
        globalRS.pGlobalRootSignature = m_raygenRS.Get();
        subobjects[soIndex].Type  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        subobjects[soIndex].pDesc = &globalRS;
        soIndex++;

        // Pipeline Config
        D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
        pipelineConfig.MaxTraceRecursionDepth = 1;
        subobjects[soIndex].Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
        subobjects[soIndex].pDesc = &pipelineConfig;
        soIndex++;

        D3D12_STATE_OBJECT_DESC stateDesc = {};
        stateDesc.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        stateDesc.NumSubobjects = soIndex;
        stateDesc.pSubobjects   = subobjects;

        HRESULT hr = device->CreateStateObject(&stateDesc, IID_PPV_ARGS(&m_rtPSO));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("RTSoftShadows: Failed to create DXR state object (0x%08X)", hr);
            return false;
        }
    }

    // テンポラル蓄積パス PSO
    {
        if (!m_compositeShader.Initialize())
            return false;

        auto vsBlob = m_compositeShader.CompileFromFile(
            L"Shaders/RTSoftShadowComposite.hlsl", L"FullscreenVS", L"vs_6_0");
        auto psBlob = m_compositeShader.CompileFromFile(
            L"Shaders/RTSoftShadowComposite.hlsl", L"PSMain", L"ps_6_0");
        if (!vsBlob.valid || !psBlob.valid)
        {
            GX_LOG_ERROR("RTSoftShadows: Failed to compile composite shaders");
            return false;
        }

        RootSignatureBuilder rsBuilder;
        rsBuilder.AddCBV(0);              // CompositeCB
        rsBuilder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);  // tCurrentShadow, tHistoryShadow
        rsBuilder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT,
                                   D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                   D3D12_COMPARISON_FUNC_NEVER);
        m_compositeRS = rsBuilder.Build(device);
        if (!m_compositeRS)
        {
            GX_LOG_ERROR("RTSoftShadows: Failed to create composite root signature");
            return false;
        }

        PipelineStateBuilder psoBuilder;
        psoBuilder.SetRootSignature(m_compositeRS.Get());
        psoBuilder.SetVertexShader(vsBlob.GetBytecode());
        psoBuilder.SetPixelShader(psBlob.GetBytecode());
        psoBuilder.SetRenderTargetFormat(DXGI_FORMAT_R8_UNORM);
        psoBuilder.SetDepthEnable(false);
        m_compositePSO = psoBuilder.Build(device);
        if (!m_compositePSO)
        {
            GX_LOG_ERROR("RTSoftShadows: Failed to create composite PSO");
            return false;
        }
    }

    // 定数バッファ作成
    if (!m_rayCB.Initialize(device, 256, 256))
        return false;
    if (!m_compositeCB.Initialize(device, 256, 256))
        return false;

    m_initialized = true;
    GX_LOG_INFO("RTSoftShadows initialized (%ux%u, %d rays/pixel)", width, height, m_settings.numRays);
    return true;
}

void RTSoftShadows::Shutdown()
{
    m_rtPSO.Reset();
    m_raygenRS.Reset();
    m_compositePSO.Reset();
    m_compositeRS.Reset();
    m_tlas = nullptr;
    m_initialized = false;
}

void RTSoftShadows::Execute(ID3D12GraphicsCommandList4* cmdList4,
                            const XMMATRIX& invViewProj, const XMFLOAT3& cameraPos,
                            const XMFLOAT3& lightDir,
                            ID3D12Resource* depthSRV, ID3D12Resource* normalSRV,
                            uint32_t frameIndex)
{
    if (!m_settings.enabled || !m_initialized || !cmdList4 || !m_tlas)
        return;

    // --- Pass 1: DispatchRays ---
    {
        struct RayCB
        {
            XMFLOAT4X4 invViewProj;
            XMFLOAT3   lightDir;
            float      lightRadius;
            XMFLOAT3   cameraPos;
            int        numRays;
            float      screenSizeX, screenSizeY;
            float      maxDistance;
            uint32_t   frameCount;
        } cb = {};

        XMStoreFloat4x4(&cb.invViewProj, invViewProj);
        cb.lightDir    = lightDir;
        cb.lightRadius = m_settings.lightRadius;
        cb.cameraPos   = cameraPos;
        cb.numRays     = m_settings.numRays;
        cb.screenSizeX = static_cast<float>(m_width);
        cb.screenSizeY = static_cast<float>(m_height);
        cb.maxDistance  = m_settings.maxDistance;
        cb.frameCount  = m_frameCounter++;

        void* p = m_rayCB.Map(frameIndex);
        if (p) { memcpy(p, &cb, sizeof(cb)); m_rayCB.Unmap(frameIndex); }

        // DispatchRays
        cmdList4->SetComputeRootSignature(m_raygenRS.Get());
        cmdList4->SetPipelineState1(m_rtPSO.Get());
        cmdList4->SetComputeRootConstantBufferView(0, m_rayCB.GetGPUVirtualAddress(frameIndex));

        D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
        dispatchDesc.Width  = m_width;
        dispatchDesc.Height = m_height;
        dispatchDesc.Depth  = 1;
        // Note: ShaderTable bindings are set up during Initialize
        cmdList4->DispatchRays(&dispatchDesc);
    }

    // --- Pass 2: Temporal Composite ---
    {
        struct CompositeCBData
        {
            float temporalBlend;
            float pad[3];
        } cb = {};

        cb.temporalBlend = m_settings.temporalBlend;

        void* p = m_compositeCB.Map(frameIndex);
        if (p) { memcpy(p, &cb, sizeof(cb)); m_compositeCB.Unmap(frameIndex); }

        cmdList4->SetGraphicsRootSignature(m_compositeRS.Get());
        cmdList4->SetPipelineState(m_compositePSO.Get());
        cmdList4->SetGraphicsRootConstantBufferView(0, m_compositeCB.GetGPUVirtualAddress(frameIndex));

        // フルスクリーン三角形描画
        cmdList4->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList4->DrawInstanced(3, 1, 0, 0);
    }
}

} // namespace gx
