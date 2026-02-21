/// @file ParticleSystem3D.cpp
/// @brief 3Dパーティクルシステムの実装
#include "pch.h"
#include "Graphics/3D/ParticleSystem3D.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Core/Logger.h"

namespace GX
{

bool ParticleSystem3D::Initialize(ID3D12Device* device, TextureManager& textureManager)
{
    m_device = device;
    m_textureManager = &textureManager;

    // パーティクルデータ用のアップロードバッファ（StructuredBuffer SRV）
    uint32_t bufferSize = k_MaxTotalParticles * sizeof(ParticleVertex);
    if (!m_particleBuffer.Initialize(device, bufferSize, sizeof(ParticleVertex)))
    {
        GX_LOG_ERROR("ParticleSystem3D: particle buffer init failed");
        return false;
    }

    // 定数バッファ
    if (!m_constantBuffer.Initialize(device, sizeof(ParticleCB), sizeof(ParticleCB)))
    {
        GX_LOG_ERROR("ParticleSystem3D: constant buffer init failed");
        return false;
    }

    if (!CreateRootSignature(device)) return false;
    if (!CreatePipelineStates(device)) return false;

    GX_LOG_INFO("ParticleSystem3D initialized (max: %u particles)", k_MaxTotalParticles);
    return true;
}

bool ParticleSystem3D::CreateRootSignature(ID3D12Device* device)
{
    // Root Signature:
    //   [0] CBV b0 = ParticleCB（カメラ行列）
    //   [1] DescriptorTable SRV t0 = StructuredBuffer<ParticleVertex>

    D3D12_ROOT_PARAMETER1 params[2] = {};

    // [0] CBV b0
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0;
    params[0].Descriptor.RegisterSpace = 0;
    params[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // [1] DescriptorTable SRV t0 - パーティクルデータ
    auto range0 = std::make_unique<D3D12_DESCRIPTOR_RANGE1>();
    range0->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range0->NumDescriptors = 1;
    range0->BaseShaderRegister = 0;
    range0->RegisterSpace = 0;
    range0->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
    range0->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = range0.get();
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
    desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    desc.Desc_1_1.NumParameters = _countof(params);
    desc.Desc_1_1.pParameters = params;
    desc.Desc_1_1.NumStaticSamplers = 0;
    desc.Desc_1_1.pStaticSamplers = nullptr;
    desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> blob, error;
    HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, &blob, &error);
    if (FAILED(hr))
    {
        if (error)
            GX_LOG_ERROR("ParticleSystem3D RS: %s", static_cast<const char*>(error->GetBufferPointer()));
        return false;
    }

    hr = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("ParticleSystem3D: CreateRootSignature failed");
        return false;
    }

    return true;
}

bool ParticleSystem3D::CreatePipelineStates(ID3D12Device* device)
{
    // シェーダーコンパイラ初期化
    if (!m_shaderCompiler.Initialize())
    {
        GX_LOG_ERROR("ParticleSystem3D: shader compiler init failed");
        return false;
    }

    // パーティクルシェーダーをコンパイル
    ShaderBlob vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/Particle.hlsl", L"VSMain", L"vs_6_0");
    ShaderBlob psBlob = m_shaderCompiler.CompileFromFile(L"Shaders/Particle.hlsl", L"PSMain", L"ps_6_0");
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("ParticleSystem3D: shader compilation failed: %s",
                     m_shaderCompiler.GetLastError().c_str());
        return false;
    }

    // パーティクルは頂点バッファを使わない（SV_VertexIDベース）
    // InputLayoutは空

    auto createPSO = [&](D3D12_BLEND blendSrc, D3D12_BLEND blendDst,
                          ComPtr<ID3D12PipelineState>& outPSO)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = vsBlob.GetBytecode();
        psoDesc.PS = psBlob.GetBytecode();
        psoDesc.InputLayout = { nullptr, 0 };  // VBなし

        // ブレンド設定
        auto& rt = psoDesc.BlendState.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = blendSrc;
        rt.DestBlend = blendDst;
        rt.BlendOp = D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D12_BLEND_ONE;
        rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // ラスタライザ（裏面カリングなし）
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;

        // 深度テストあり、書き込みなし（パーティクルは半透明）
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;  // HDRパイプライン
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&outPSO));
        return SUCCEEDED(hr);
    };

    // アルファブレンドPSO
    if (!createPSO(D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, m_psoAlpha))
    {
        GX_LOG_ERROR("ParticleSystem3D: alpha blend PSO failed");
        return false;
    }

    // 加算ブレンドPSO
    if (!createPSO(D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_ONE, m_psoAdditive))
    {
        GX_LOG_ERROR("ParticleSystem3D: additive blend PSO failed");
        return false;
    }

    return true;
}

int ParticleSystem3D::AddEmitter(const ParticleEmitterConfig& config)
{
    int idx = static_cast<int>(m_emitters.size());
    m_emitters.emplace_back();
    m_emitters.back().Initialize(config);
    return idx;
}

void ParticleSystem3D::Update(float deltaTime)
{
    for (auto& emitter : m_emitters)
    {
        emitter.Update(deltaTime);
    }
}

uint32_t ParticleSystem3D::GetTotalParticleCount() const
{
    uint32_t total = 0;
    for (const auto& emitter : m_emitters)
    {
        total += emitter.GetParticleCount();
    }
    return total;
}

void ParticleSystem3D::Draw(ID3D12GraphicsCommandList* cmdList, const Camera3D& camera, uint32_t frameIndex)
{
    // 全エミッターの粒子をブレンドタイプ別に収集してバッチ描画する
    // まずアルファブレンド粒子、次に加算ブレンド粒子の順に描画

    uint32_t totalParticles = GetTotalParticleCount();
    if (totalParticles == 0) return;
    if (totalParticles > k_MaxTotalParticles)
        totalParticles = k_MaxTotalParticles;

    // 1. 定数バッファを更新
    ParticleCB cb = {};
    XMMATRIX vp = camera.GetViewProjectionMatrix();
    XMStoreFloat4x4(&cb.viewProj, XMMatrixTranspose(vp));
    cb.cameraRight = camera.GetRight();
    cb.cameraUp = camera.GetUp();

    void* cbMapped = m_constantBuffer.Map(frameIndex);
    if (!cbMapped) return;
    memcpy(cbMapped, &cb, sizeof(cb));
    m_constantBuffer.Unmap(frameIndex);

    // 2. 粒子データをGPUバッファに書き込む
    void* mapped = m_particleBuffer.Map(frameIndex);
    if (!mapped) return;
    auto* dst = static_cast<ParticleVertex*>(mapped);

    // アルファ粒子を先に、加算粒子を後に格納
    uint32_t alphaCount = 0;
    uint32_t additiveCount = 0;
    uint32_t alphaStart = 0;
    uint32_t additiveStart = 0;

    // まずカウント
    for (const auto& emitter : m_emitters)
    {
        uint32_t count = emitter.GetParticleCount();
        if (emitter.GetConfig().blend == ParticleBlend::Alpha)
            alphaCount += count;
        else
            additiveCount += count;
    }

    if (alphaCount + additiveCount > k_MaxTotalParticles)
    {
        // 上限超過時は比率で制限
        float ratio = static_cast<float>(k_MaxTotalParticles) / (alphaCount + additiveCount);
        alphaCount = static_cast<uint32_t>(alphaCount * ratio);
        additiveCount = k_MaxTotalParticles - alphaCount;
    }

    additiveStart = alphaCount;

    uint32_t alphaWritten = 0;
    uint32_t additiveWritten = 0;

    for (const auto& emitter : m_emitters)
    {
        const auto& particles = emitter.GetParticles();
        bool isAdditive = (emitter.GetConfig().blend == ParticleBlend::Additive);

        for (const auto& p : particles)
        {
            uint32_t idx;
            if (isAdditive)
            {
                if (additiveWritten >= additiveCount) break;
                idx = additiveStart + additiveWritten++;
            }
            else
            {
                if (alphaWritten >= alphaCount) break;
                idx = alphaStart + alphaWritten++;
            }

            dst[idx].position = p.position;
            dst[idx].size = p.size;
            dst[idx].color = p.color;
            dst[idx].rotation = p.rotation;
        }
    }

    m_particleBuffer.Unmap(frameIndex);

    // 3. SRVスロットをテクスチャマネージャーのSRVヒープから確保
    DescriptorHeap& srvHeap = m_textureManager->GetSRVHeap();
    if (!m_srvInitialized)
    {
        for (int i = 0; i < 2; ++i)
        {
            m_particleSRVSlot[i] = srvHeap.AllocateIndex();
        }
        m_srvInitialized = true;
    }

    // パーティクルデータのSRVを作成
    {
        ID3D12Resource* resource = m_particleBuffer.GetResource(frameIndex);
        if (!resource) return;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = alphaWritten + additiveWritten;
        srvDesc.Buffer.StructureByteStride = sizeof(ParticleVertex);

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
            srvHeap.GetCPUHandle(m_particleSRVSlot[frameIndex]);
        m_device->CreateShaderResourceView(resource, &srvDesc, cpuHandle);
    }

    // 4. 描画コマンドを発行
    // SRVヒープはRenderer3D::Beginで既にセット済みと仮定
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetGraphicsRootConstantBufferView(0,
        m_constantBuffer.GetGPUVirtualAddress(frameIndex));
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 0, nullptr);  // VBなし
    cmdList->IASetIndexBuffer(nullptr);           // IBなし

    D3D12_GPU_DESCRIPTOR_HANDLE particleSRV =
        srvHeap.GetGPUHandle(m_particleSRVSlot[frameIndex]);
    cmdList->SetGraphicsRootDescriptorTable(1, particleSRV);

    // アルファブレンド粒子を描画
    if (alphaWritten > 0)
    {
        cmdList->SetPipelineState(m_psoAlpha.Get());
        // 各パーティクルはクワッド（6頂点: 2三角形）
        cmdList->DrawInstanced(6 * alphaWritten, 1, 0, 0);
    }

    // 加算ブレンド粒子を描画
    if (additiveWritten > 0)
    {
        cmdList->SetPipelineState(m_psoAdditive.Get());
        cmdList->DrawInstanced(6 * additiveWritten, 1, 6 * alphaWritten, 0);
    }
}

void ParticleSystem3D::Shutdown()
{
    m_emitters.clear();
    m_psoAlpha.Reset();
    m_psoAdditive.Reset();
    m_rootSignature.Reset();
}

} // namespace GX
