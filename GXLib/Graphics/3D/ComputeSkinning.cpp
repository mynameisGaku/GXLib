/// @file ComputeSkinning.cpp
/// @brief Compute Shader スキニング実装
#include "pch_graphics.h"
#include "Graphics/3D/ComputeSkinning.h"
#include "Core/Logger.h"

namespace gx
{

bool ComputeSkinning::Initialize(ID3D12Device* device)
{
    if (!device) return false;
    m_device = device;

    // ルートシグネチャの作成
    // t0: InputVertices (SRV)
    // t1: BoneMatrices (SRV)
    // u0: OutputVertices (UAV)
    // b0: Constants (vertexCount, bonesPerVertex)
    D3D12_ROOT_PARAMETER params[4] = {};

    D3D12_DESCRIPTOR_RANGE srvRange0 = {};
    srvRange0.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange0.NumDescriptors = 1;
    srvRange0.BaseShaderRegister = 0;

    D3D12_DESCRIPTOR_RANGE srvRange1 = {};
    srvRange1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange1.NumDescriptors = 1;
    srvRange1.BaseShaderRegister = 1;

    D3D12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = 0;

    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[0].DescriptorTable.NumDescriptorRanges = 1;
    params[0].DescriptorTable.pDescriptorRanges = &srvRange0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &srvRange1;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 1;
    params[2].DescriptorTable.pDescriptorRanges = &uavRange;
    params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[3].Constants.ShaderRegister = 0;
    params[3].Constants.Num32BitValues = 2; // vertexCount, bonesPerVertex
    params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 4;
    rsDesc.pParameters = params;

    ComPtr<ID3DBlob> signature, error;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              &signature, &error);
    if (FAILED(hr))
    {
        Logger::Error("ComputeSkinning: Failed to serialize root signature");
        return false;
    }

    hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(),
                                        signature->GetBufferSize(),
                                        IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        Logger::Error("ComputeSkinning: Failed to create root signature");
        return false;
    }

    // Compute Shader のコンパイル
    if (!m_shader.Initialize())
    {
        Logger::Warn("ComputeSkinning: DXC compiler initialization failed");
        m_ready = false;
        return true;
    }

    auto csBlob = m_shader.CompileFromFile(L"Shaders/ComputeSkinning.hlsl",
                                            L"CSMain", L"cs_6_0");
    if (!csBlob.valid)
    {
        Logger::Warn("ComputeSkinning: Shader compilation failed (shader may not exist yet)");
        m_ready = false;
        return true; // シェーダーがなくても初期化は成功とする
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.CS = csBlob.GetBytecode();

    hr = m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pso));
    if (FAILED(hr))
    {
        Logger::Error("ComputeSkinning: Failed to create compute PSO");
        return false;
    }

    m_ready = true;
    return true;
}

void ComputeSkinning::Dispatch(ID3D12GraphicsCommandList* cmdList,
                                D3D12_GPU_DESCRIPTOR_HANDLE vertexSRV,
                                D3D12_GPU_DESCRIPTOR_HANDLE boneSRV,
                                D3D12_GPU_DESCRIPTOR_HANDLE outputUAV,
                                uint32_t vertexCount,
                                uint32_t bonesPerVertex)
{
    if (!m_ready || !m_enabled || !cmdList) return;

    cmdList->SetComputeRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_pso.Get());

    cmdList->SetComputeRootDescriptorTable(0, vertexSRV);
    cmdList->SetComputeRootDescriptorTable(1, boneSRV);
    cmdList->SetComputeRootDescriptorTable(2, outputUAV);

    uint32_t constants[2] = { vertexCount, bonesPerVertex };
    cmdList->SetComputeRoot32BitConstants(3, 2, constants, 0);

    uint32_t groupCount = (vertexCount + k_ThreadGroupSize - 1) / k_ThreadGroupSize;
    cmdList->Dispatch(groupCount, 1, 1);
}

void ComputeSkinning::Shutdown()
{
    m_pso.Reset();
    m_rootSignature.Reset();
    m_device = nullptr;
    m_ready = false;
}

} // namespace gx
