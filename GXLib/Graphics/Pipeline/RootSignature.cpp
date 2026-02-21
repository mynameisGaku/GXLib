/// @file RootSignature.cpp
/// @brief ルートシグネチャビルダーの実装
#include "pch.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Core/Logger.h"

namespace GX
{

RootSignatureBuilder& RootSignatureBuilder::AddCBV(uint32_t shaderRegister, uint32_t space,
                                                     D3D12_SHADER_VISIBILITY visibility)
{
    // ルートディスクリプタとしてCBVを1つ登録（GPU仮想アドレスを直接渡す方式）
    D3D12_ROOT_PARAMETER1 param = {};
    param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace  = space;
    param.Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
    param.ShaderVisibility          = visibility;
    m_parameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddSRV(uint32_t shaderRegister, uint32_t space,
                                                     D3D12_SHADER_VISIBILITY visibility)
{
    // ルートSRVはGPU仮想アドレスで直接バインド。Texture2D.Sample()には使えない点に注意。
    D3D12_ROOT_PARAMETER1 param = {};
    param.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace  = space;
    param.Descriptor.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
    param.ShaderVisibility          = visibility;
    m_parameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE type,
                                                                 uint32_t shaderRegister,
                                                                 uint32_t numDescriptors,
                                                                 uint32_t space,
                                                                 D3D12_SHADER_VISIBILITY visibility,
                                                                 D3D12_DESCRIPTOR_RANGE_FLAGS rangeFlags)
{
    // unique_ptrで確保してポインタの安定性を保証。
    // vectorが再確保されてもポインタが無効化されないようにするため。
    auto range = std::make_unique<D3D12_DESCRIPTOR_RANGE1>();
    range->RangeType                         = type;
    range->NumDescriptors                    = numDescriptors;
    range->BaseShaderRegister                = shaderRegister;
    range->RegisterSpace                     = space;
    range->Flags                             = rangeFlags;
    range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER1 param = {};
    param.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges   = range.get();
    param.ShaderVisibility                    = visibility;

    m_descriptorRanges.push_back(std::move(range));
    m_parameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddStaticSampler(uint32_t shaderRegister, uint32_t space,
                                                               D3D12_FILTER filter)
{
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter           = filter;
    sampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias       = 0.0f;
    sampler.MaxAnisotropy    = 16;
    sampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD           = 0.0f;
    sampler.MaxLOD           = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister   = shaderRegister;
    sampler.RegisterSpace    = space;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    m_staticSamplers.push_back(sampler);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddStaticSampler(uint32_t shaderRegister,
                                                               D3D12_FILTER filter,
                                                               D3D12_TEXTURE_ADDRESS_MODE addressMode,
                                                               D3D12_COMPARISON_FUNC comparisonFunc,
                                                               uint32_t space)
{
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter           = filter;
    sampler.AddressU         = addressMode;
    sampler.AddressV         = addressMode;
    sampler.AddressW         = addressMode;
    sampler.MipLODBias       = 0.0f;
    sampler.MaxAnisotropy    = 1;
    sampler.ComparisonFunc   = comparisonFunc;
    sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD           = 0.0f;
    sampler.MaxLOD           = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister   = shaderRegister;
    sampler.RegisterSpace    = space;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    m_staticSamplers.push_back(sampler);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
    m_flags = flags;
    return *this;
}

ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(ID3D12Device* device)
{
    // Version 1.1を使用（1.0より最適化の余地が広い）
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
    desc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
    desc.Desc_1_1.NumParameters     = static_cast<UINT>(m_parameters.size());
    desc.Desc_1_1.pParameters       = m_parameters.empty() ? nullptr : m_parameters.data();
    desc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(m_staticSamplers.size());
    desc.Desc_1_1.pStaticSamplers   = m_staticSamplers.empty() ? nullptr : m_staticSamplers.data();
    desc.Desc_1_1.Flags             = m_flags;

    // ルートシグネチャをバイト列にシリアライズしてからデバイスに渡す（DX12の2段階生成）
    ComPtr<ID3DBlob> serializedBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &serializedBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            GX_LOG_ERROR("Root signature serialization error: %s",
                static_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        return nullptr;
    }

    ComPtr<ID3D12RootSignature> rootSignature;
    hr = device->CreateRootSignature(
        0,
        serializedBlob->GetBufferPointer(),
        serializedBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)
    );

    if (FAILED(hr))
    {
        GX_LOG_ERROR("Failed to create root signature (HRESULT: 0x%08X)", hr);
        return nullptr;
    }

    GX_LOG_INFO("Root Signature created (%d parameters, %d samplers)",
        static_cast<int>(m_parameters.size()), static_cast<int>(m_staticSamplers.size()));
    return rootSignature;
}

} // namespace GX
