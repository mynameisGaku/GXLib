/// @file PointShadowMap.cpp
/// @brief ポイントライト用キューブシャドウマップの実装
#include "pch.h"
#include "Graphics/3D/PointShadowMap.h"
#include "Core/Logger.h"

namespace GX
{

bool PointShadowMap::Create(ID3D12Device* device, DescriptorHeap* srvHeap, uint32_t srvIndex)
{
    // Texture2DArray（6スライス）でキューブシャドウを表現（CubeMapではなくArrayを使う）
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width              = k_Size;
    desc.Height             = k_Size;
    desc.DepthOrArraySize   = k_NumFaces;
    desc.MipLevels          = 1;
    desc.Format             = DXGI_FORMAT_R32_TYPELESS;
    desc.SampleDesc.Count   = 1;
    desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format               = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth   = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&m_resource));

    if (FAILED(hr))
    {
        GX_LOG_ERROR("PointShadowMap: Failed to create resource (HRESULT: 0x%08X)", hr);
        return false;
    }

    // DSVヒープ（6面分、面ごとに1つ）
    if (!m_dsvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, k_NumFaces, false))
    {
        GX_LOG_ERROR("PointShadowMap: Failed to create DSV heap");
        return false;
    }

    for (uint32_t face = 0; face < k_NumFaces; ++face)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.FirstArraySlice = face;
        dsvDesc.Texture2DArray.ArraySize       = 1;
        dsvDesc.Texture2DArray.MipSlice        = 0;

        device->CreateDepthStencilView(m_resource.Get(), &dsvDesc,
                                       m_dsvHeap.GetCPUHandle(face));
    }

    // SRV（Texture2DArray, 6面まとめて）を外部ヒープに作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                        = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2DArray.MipLevels      = 1;
    srvDesc.Texture2DArray.ArraySize      = k_NumFaces;
    srvDesc.Texture2DArray.FirstArraySlice = 0;

    device->CreateShaderResourceView(m_resource.Get(), &srvDesc,
                                     srvHeap->GetCPUHandle(srvIndex));
    m_srvGPUHandle = srvHeap->GetGPUHandle(srvIndex);

    GX_LOG_INFO("PointShadowMap created (%dx%d x%d faces, SRV index %d)",
                k_Size, k_Size, k_NumFaces, srvIndex);
    return true;
}

void PointShadowMap::Update(const XMFLOAT3& lightPos, float range)
{
    XMVECTOR pos = XMLoadFloat3(&lightPos);

    // 6面の向き: +X, -X, +Y, -Y, +Z, -Z
    struct FaceInfo
    {
        XMVECTOR target;
        XMVECTOR up;
    };

    FaceInfo faces[k_NumFaces] =
    {
        { XMVectorSet( 1, 0, 0, 0), XMVectorSet(0, 1, 0, 0) }, // +X
        { XMVectorSet(-1, 0, 0, 0), XMVectorSet(0, 1, 0, 0) }, // -X
        { XMVectorSet( 0, 1, 0, 0), XMVectorSet(0, 0,-1, 0) }, // +Y
        { XMVectorSet( 0,-1, 0, 0), XMVectorSet(0, 0, 1, 0) }, // -Y
        { XMVectorSet( 0, 0, 1, 0), XMVectorSet(0, 1, 0, 0) }, // +Z
        { XMVectorSet( 0, 0,-1, 0), XMVectorSet(0, 1, 0, 0) }, // -Z
    };

    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, 0.1f, range);

    for (uint32_t face = 0; face < k_NumFaces; ++face)
    {
        XMVECTOR target = XMVectorAdd(pos, faces[face].target);
        XMMATRIX view = XMMatrixLookAtLH(pos, target, faces[face].up);
        XMMATRIX vp = view * proj;
        XMStoreFloat4x4(&m_faceVP[face], XMMatrixTranspose(vp));
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE PointShadowMap::GetFaceDSVHandle(uint32_t face) const
{
    return m_dsvHeap.GetCPUHandle(face);
}

} // namespace GX
