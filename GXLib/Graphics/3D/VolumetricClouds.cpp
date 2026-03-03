/// @file VolumetricClouds.cpp
/// @brief Volumetric Clouds implementation
///
/// Follows the same 2-SRV dedicated heap pattern as VolumetricLight.
#include "pch_graphics.h"
#include "Graphics/3D/VolumetricClouds.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"

namespace gx
{

bool VolumetricClouds::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // Dedicated SRV heap: 2 textures x 2 frames = 4 slots
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4, true))
        return false;

    if (!m_shader.Initialize())
        return false;

    if (!m_cb.Initialize(device, 256, 256))
        return false;

    // Root signature: [0] CBV(b0), [1] DescTable(t0,t1), s0(linear)
    {
        RootSignatureBuilder rsb;
        m_rootSignature = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_rootSignature) return false;
    }

    if (!CreatePipelines(device))
        return false;

    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/VolumetricClouds.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("VolumetricClouds initialized (%dx%d)", width, height);
    return true;
}

bool VolumetricClouds::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/VolumetricClouds.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_shader.CompileFromFile(L"Shaders/VolumetricClouds.hlsl", L"PSMain", L"ps_6_0");
    if (!ps.valid) return false;

    PipelineStateBuilder b;
    m_pso = b.SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vs.GetBytecode())
        .SetPixelShader(ps.GetBytecode())
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .Build(device);
    if (!m_pso) return false;

    return true;
}

void VolumetricClouds::UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex)
{
    uint32_t base = frameIndex * 2;

    // [base+0] = scene HDR
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = srcHDR.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(srcHDR.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 0));
    }

    // [base+1] = depth
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 1));
    }
}

void VolumetricClouds::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                RenderTarget& srcHDR, RenderTarget& destHDR,
                                DepthBuffer& depth, const Camera3D& camera,
                                float elapsedTime)
{
    // Resource transitions
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    UpdateSRVHeap(srcHDR, depth, frameIndex);

    auto destRTV = destHDR.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Build constant buffer
    XMMATRIX vp_mat = ToXMMATRIX(camera.GetViewProjectionMatrix());
    XMMATRIX invVP  = XMMatrixInverse(nullptr, vp_mat);

    // Normalise sun direction (towards sun, so negate light direction)
    XMVECTOR sunDir = XMVector3Normalize(XMVectorNegate(XMLoadFloat3(XM(&m_sunDirection))));
    Vector3 sunDirF;
    XMStoreFloat3(XM(&sunDirF), sunDir);

    CloudConstants cb = {};
    XMStoreFloat4x4(XM(&cb.invViewProjection), XMMatrixTranspose(invVP));
    cb.cameraPosition = camera.GetPosition();
    cb.time           = elapsedTime;
    cb.sunDirection   = sunDirF;
    cb.cloudBottom    = m_cloudBottom;
    cb.sunColor       = m_sunColor;
    cb.cloudTop       = m_cloudTop;
    cb.coverage       = m_coverage;
    cb.densityMul     = m_density;
    cb.windSpeed      = m_windSpeed;
    cb.silverLining   = m_silverLining;
    cb.windDirection  = m_windDirection;
    cb.marchSteps     = m_marchSteps;
    cb.lightSteps     = m_lightSteps;
    cb.screenDimensions = { static_cast<float>(m_width), static_cast<float>(m_height) };

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &cb, sizeof(cb));
        m_cb.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(frameIndex * 2));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void VolumetricClouds::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;
}

} // namespace gx
