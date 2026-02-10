/// @file Renderer3D.cpp
/// @brief 3Dレンダラーの実装
#include "pch.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Graphics/Device/BarrierBatch.h"
#include "Core/Logger.h"

namespace GX
{

bool Renderer3D::Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                              uint32_t screenWidth, uint32_t screenHeight)
{
    m_device       = device;
    m_screenWidth  = screenWidth;
    m_screenHeight = screenHeight;

    // 深度バッファ（SSAO用に自前SRV付き）
    if (!m_depthBuffer.CreateWithOwnSRV(device, screenWidth, screenHeight))
    {
        GX_LOG_ERROR("Renderer3D: Failed to create depth buffer");
        return false;
    }

    // テクスチャマネージャー
    if (!m_textureManager.Initialize(device, cmdQueue))
    {
        GX_LOG_ERROR("Renderer3D: Failed to create texture manager");
        return false;
    }

    // デフォルトテクスチャ（1x1）
    {
        const uint32_t white = 0xFFFFFFFF;
        const uint32_t black = 0xFF000000;
        const uint32_t normal = 0xFF8080FF; // (0.5, 0.5, 1.0)
        m_defaultWhiteTex = m_textureManager.CreateTextureFromMemory(&white, 1, 1);
        m_defaultBlackTex = m_textureManager.CreateTextureFromMemory(&black, 1, 1);
        m_defaultNormalTex = m_textureManager.CreateTextureFromMemory(&normal, 1, 1);
    }

    // 定数バッファ（256アラインメント）- リングバッファ化
    if (!m_objectCB.Initialize(device, 256 * k_MaxObjectsPerFrame, 256))
        return false;

    // FrameConstants: 増大したので適切なサイズ計算
    uint32_t frameCBSize = ((sizeof(FrameConstants) + 255) / 256) * 256;
    if (!m_frameCB.Initialize(device, frameCBSize, frameCBSize))
        return false;

    // LightConstants（ライト定数バッファ）
    uint32_t lightCBSize = ((sizeof(LightConstants) + 255) / 256) * 256;
    if (!m_lightCB.Initialize(device, lightCBSize, lightCBSize))
        return false;

    // MaterialConstants (リングバッファ化: objectCBと同じくk_MaxObjectsPerFrame個)
    if (!m_materialCB.Initialize(device, 256 * k_MaxObjectsPerFrame, 256))
        return false;

    // BoneConstants（ボーン定数バッファ）
    uint32_t boneCBSize = ((sizeof(BoneConstants) + 255) / 256) * 256;
    if (!m_boneCB.Initialize(device, boneCBSize, boneCBSize))
        return false;

    // シャドウパス用CB（各カスケード/面ごとのLightVP、256アライン×11枠: CSM4 + Spot1 + Point6）
    // 初学者向け: 影用のビュー行列をまとめてGPUへ渡すためのバッファです。
    static constexpr uint32_t k_ShadowCBSlots = 4 + 1 + 6; // CSM + Spot + Point
    if (!m_shadowPassCB.Initialize(device, 256 * k_ShadowCBSlots,
                                    256 * k_ShadowCBSlots))
        return false;

    // シェーダーコンパイラ
    if (!m_shaderCompiler.Initialize())
        return false;

    // CSM初期化（TextureManagerのSRVヒープにSRVを配置）
    // 連続6スロットをTextureManagerのSRVヒープから確保（CSM 4 + Spot 1 + Point 1）
    auto& srvHeap = m_textureManager.GetSRVHeap();
    uint32_t shadowSrvStart = srvHeap.AllocateIndex();
    for (uint32_t i = 1; i < 6; ++i)
        srvHeap.AllocateIndex();

    if (!m_csm.Initialize(device, &srvHeap, shadowSrvStart))
    {
        GX_LOG_ERROR("Renderer3D: Failed to initialize CSM");
        return false;
    }

    // スポットシャドウマップ（SRV index = shadowSrvStart + 4 → t12）
    if (!m_spotShadowMap.Create(device, k_SpotShadowMapSize, &srvHeap, shadowSrvStart + 4))
    {
        GX_LOG_ERROR("Renderer3D: Failed to create spot shadow map");
        return false;
    }

    // ポイントシャドウマップ（SRV index = shadowSrvStart + 5 → t13）
    if (!m_pointShadowMap.Create(device, &srvHeap, shadowSrvStart + 5))
    {
        GX_LOG_ERROR("Renderer3D: Failed to create point shadow map");
        return false;
    }

    // メインPSO
    if (!CreatePipelineState(device))
        return false;

    // シャドウPSO
    if (!CreateShadowPipelineState(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/PBR.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelineState(dev); }
    );
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/ShadowDepth.hlsl",
        [this](ID3D12Device* dev) { return CreateShadowPipelineState(dev); }
    );

    // スカイボックス
    if (!m_skybox.Initialize(device))
    {
        GX_LOG_ERROR("Renderer3D: Failed to initialize Skybox");
        return false;
    }

    // 3Dプリミティブバッチ
    if (!m_primitiveBatch3D.Initialize(device))
    {
        GX_LOG_ERROR("Renderer3D: Failed to initialize PrimitiveBatch3D");
        return false;
    }

    // デフォルトライト（Directional）
    m_currentLights.lights[0] = Light::CreateDirectional(
        { 0.3f, -1.0f, 0.5f }, { 1.0f, 1.0f, 1.0f }, 3.0f);
    m_currentLights.numLights = 1;
    m_currentLights.ambientColor = { 0.03f, 0.03f, 0.03f };

    GX_LOG_INFO("Renderer3D initialized (%dx%d) with CSM shadows", screenWidth, screenHeight);
    return true;
}

bool Renderer3D::CreatePipelineState(ID3D12Device* device)
{
    auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/PBR.hlsl", L"VSMain", L"vs_6_0");
    auto psBlob = m_shaderCompiler.CompileFromFile(L"Shaders/PBR.hlsl", L"PSMain", L"ps_6_0");
    std::vector<std::pair<std::wstring, std::wstring>> skinnedDefines = { { L"SKINNED", L"1" } };
    auto vsSkinned = m_shaderCompiler.CompileFromFile(L"Shaders/PBR.hlsl", L"VSMain", L"vs_6_0", skinnedDefines);
    auto psSkinned = m_shaderCompiler.CompileFromFile(L"Shaders/PBR.hlsl", L"PSMain", L"ps_6_0", skinnedDefines);
    if (!vsBlob.valid || !psBlob.valid || !vsSkinned.valid || !psSkinned.valid)
    {
        GX_LOG_ERROR("Renderer3D: Failed to compile PBR shaders");
        return false;
    }

    // ルートシグネチャ
    // ルートパラメータ0: b0 ObjectConstants (CBV)
    // ルートパラメータ1: b1 FrameConstants  (CBV)
    // ルートパラメータ2: b2 LightConstants  (CBV)
    // ルートパラメータ3: b3 MaterialConstants (CBV)
    // ルートパラメータ4: テクスチャ用ディスクリプタテーブル (t0-t4)
    // ルートパラメータ5: シャドウマップ用ディスクリプタテーブル (t8-t11)
    // s0: リニアWrapサンプラ
    // s2: PCF用の比較サンプラ（影のソフト化）
    RootSignatureBuilder rsBuilder;
    m_rootSignature = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        .AddCBV(0)  // b0: ObjectConstants
        .AddCBV(1)  // b1: FrameConstants
        .AddCBV(2)  // b2: LightConstants
        .AddCBV(3)  // b3: MaterialConstants
        .AddCBV(4)  // b4: BoneConstants
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)  // t0: albedo
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)  // t1: normal
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)  // t2: met/rough
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)  // t3: AO
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)  // t4: emissive
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 6, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)  // t8-t13: シャドウマップ（CSM + Spot + Point）
        .AddStaticSampler(0)  // s0: リニアWrapサンプラ
        .AddStaticSampler(2, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
                          D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                          D3D12_COMPARISON_FUNC_LESS_EQUAL)  // s2: 影比較サンプラ
        .Build(device);

    if (!m_rootSignature)
        return false;

    PipelineStateBuilder psoBuilder;
    m_pso = psoBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(k_Vertex3DPBRLayout, _countof(k_Vertex3DPBRLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)  // HDR用RT
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_BACK)
        .Build(device);

    PipelineStateBuilder psoSkinnedBuilder;
    m_psoSkinned = psoSkinnedBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsSkinned.GetBytecode())
        .SetPixelShader(psSkinned.GetBytecode())
        .SetInputLayout(k_Vertex3DSkinnedLayout, _countof(k_Vertex3DSkinnedLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_BACK)
        .Build(device);

    m_currentPso = nullptr;
    return m_pso != nullptr && m_psoSkinned != nullptr;
}

bool Renderer3D::CreateShadowPipelineState(ID3D12Device* device)
{
    auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/ShadowDepth.hlsl", L"VSMain", L"vs_6_0");
    std::vector<std::pair<std::wstring, std::wstring>> skinnedDefines = { { L"SKINNED", L"1" } };
    auto vsSkinned = m_shaderCompiler.CompileFromFile(L"Shaders/ShadowDepth.hlsl", L"VSMain", L"vs_6_0", skinnedDefines);
    if (!vsBlob.valid || !vsSkinned.valid)
    {
        GX_LOG_ERROR("Renderer3D: Failed to compile ShadowDepth VS");
        return false;
    }

    // シャドウ用ルートシグネチャ（シンプル: b0=Object, b1=LightVP）
    RootSignatureBuilder rsBuilder;
    m_shadowRootSignature = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        .AddCBV(0)  // b0: ObjectConstants
        .AddCBV(1)  // b1: ShadowPassConstants (lightVP)
        .AddCBV(4)  // b4: BoneConstants
        .Build(device);

    if (!m_shadowRootSignature)
        return false;

    PipelineStateBuilder psoBuilder;
    m_shadowPso = psoBuilder
        .SetRootSignature(m_shadowRootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetInputLayout(k_Vertex3DPBRLayout, _countof(k_Vertex3DPBRLayout))
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_NONE)    // 両面描画（単面ジオメトリのライトリーク防止）
        .SetDepthBias(200, 0.0f, 2.0f)        // 深度バイアス（自己シャドウ防止）
        .SetRenderTargetCount(0)               // カラー出力なし
        .Build(device);

    PipelineStateBuilder psoSkinnedBuilder;
    m_shadowPsoSkinned = psoSkinnedBuilder
        .SetRootSignature(m_shadowRootSignature.Get())
        .SetVertexShader(vsSkinned.GetBytecode())
        .SetInputLayout(k_Vertex3DSkinnedLayout, _countof(k_Vertex3DSkinnedLayout))
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .SetDepthBias(200, 0.0f, 2.0f)
        .SetRenderTargetCount(0)
        .Build(device);

    return m_shadowPso != nullptr && m_shadowPsoSkinned != nullptr;
}

int Renderer3D::CreateMaterialShader(const ShaderProgramDesc& desc)
{
    if (!m_device || !m_rootSignature)
        return -1;
    if (desc.vsPath.empty() || desc.psPath.empty())
        return -1;

    auto vsBlob = m_shaderCompiler.CompileFromFile(desc.vsPath, desc.vsEntry, L"vs_6_0", desc.defines);
    auto psBlob = m_shaderCompiler.CompileFromFile(desc.psPath, desc.psEntry, L"ps_6_0", desc.defines);

    auto skinnedDefines = desc.defines;
    skinnedDefines.push_back({ L"SKINNED", L"1" });
    auto vsSkinned = m_shaderCompiler.CompileFromFile(desc.vsPath, desc.vsEntry, L"vs_6_0", skinnedDefines);
    auto psSkinned = m_shaderCompiler.CompileFromFile(desc.psPath, desc.psEntry, L"ps_6_0", skinnedDefines);

    if (!vsBlob.valid || !psBlob.valid || !vsSkinned.valid || !psSkinned.valid)
        return -1;

    ShaderPipeline pipeline;
    pipeline.desc = desc;

    PipelineStateBuilder psoBuilder;
    pipeline.pso = psoBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(k_Vertex3DPBRLayout, _countof(k_Vertex3DPBRLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_BACK)
        .Build(m_device);

    PipelineStateBuilder psoSkinnedBuilder;
    pipeline.psoSkinned = psoSkinnedBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsSkinned.GetBytecode())
        .SetPixelShader(psSkinned.GetBytecode())
        .SetInputLayout(k_Vertex3DSkinnedLayout, _countof(k_Vertex3DSkinnedLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_BACK)
        .Build(m_device);

    if (!pipeline.pso || !pipeline.psoSkinned)
        return -1;

    int handle = m_nextShaderHandle++;
    m_customShaders[handle] = std::move(pipeline);
    return handle;
}

void Renderer3D::BindPipeline(bool skinned, int shaderHandle)
{
    ID3D12PipelineState* target = nullptr;
    if (m_inShadowPass)
    {
        target = skinned ? m_shadowPsoSkinned.Get() : m_shadowPso.Get();
    }
    else if (shaderHandle >= 0)
    {
        auto it = m_customShaders.find(shaderHandle);
        if (it != m_customShaders.end())
            target = skinned ? it->second.psoSkinned.Get() : it->second.pso.Get();
    }

    if (!target)
        target = skinned ? m_psoSkinned.Get() : m_pso.Get();

    if (target && target != m_currentPso)
    {
        m_cmdList->SetPipelineState(target);
        m_currentPso = target;
    }
}

GPUMesh Renderer3D::CreateGPUMesh(const MeshData& meshData)
{
    GPUMesh mesh;
    uint32_t vbSize = static_cast<uint32_t>(meshData.vertices.size() * sizeof(Vertex3D_PBR));
    mesh.vertexBuffer.CreateVertexBuffer(m_device, meshData.vertices.data(), vbSize, sizeof(Vertex3D_PBR));

    uint32_t ibSize = static_cast<uint32_t>(meshData.indices.size() * sizeof(uint32_t));
    mesh.indexBuffer.CreateIndexBuffer(m_device, meshData.indices.data(), ibSize, DXGI_FORMAT_R32_UINT);

    mesh.indexCount = static_cast<uint32_t>(meshData.indices.size());
    return mesh;
}

void Renderer3D::UpdateShadow(const Camera3D& camera)
{
    if (!m_shadowEnabled) return;

    // 最初のDirectionalライトをシャドウキャスターとして使用
    XMFLOAT3 lightDir = { 0.3f, -1.0f, 0.5f };
    m_spotShadowLightIndex = -1;
    m_pointShadowLightIndex = -1;

    for (uint32_t i = 0; i < m_currentLights.numLights; ++i)
    {
        auto type = static_cast<LightType>(m_currentLights.lights[i].type);
        if (type == LightType::Directional)
        {
            lightDir = m_currentLights.lights[i].direction;
        }
        else if (type == LightType::Spot && m_spotShadowLightIndex < 0)
        {
            m_spotShadowLightIndex = static_cast<int32_t>(i);
            const auto& light = m_currentLights.lights[i];

            // スポットライトVP行列計算
            XMVECTOR spotPos = XMLoadFloat3(&light.position);
            XMVECTOR spotDir = XMVector3Normalize(XMLoadFloat3(&light.direction));
            XMVECTOR up = XMVectorSet(0, 1, 0, 0);
            if (XMVectorGetX(XMVector3LengthSq(XMVector3Cross(spotDir, up))) < 0.001f)
                up = XMVectorSet(0, 0, 1, 0);

            float fov = acosf(light.spotAngle) * 2.0f * 1.2f;
            fov = (std::min)(fov, XM_PI * 0.95f);

            XMMATRIX view = XMMatrixLookAtLH(spotPos, XMVectorAdd(spotPos, spotDir), up);
            XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, 1.0f, 0.1f, light.range);
            XMStoreFloat4x4(&m_spotLightVP, XMMatrixTranspose(view * proj));
        }
        else if (type == LightType::Point && m_pointShadowLightIndex < 0)
        {
            m_pointShadowLightIndex = static_cast<int32_t>(i);
            const auto& light = m_currentLights.lights[i];
            m_pointShadowMap.Update(light.position, light.range);
        }
    }

    m_csm.Update(camera, lightDir);
}

void Renderer3D::BeginShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                   uint32_t cascadeIndex)
{
    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;
    m_inShadowPass = true;

    auto& shadowMap = m_csm.GetShadowMap(cascadeIndex);

    // リソースバリア: → DEPTH_WRITE
    if (shadowMap.GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = shadowMap.GetResource();
        barrier.Transition.StateBefore = shadowMap.GetCurrentState();
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        shadowMap.SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    // 深度クリア
    auto dsvHandle = shadowMap.GetDSVHandle();
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // レンダーターゲット: 深度のみ
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

    // ビューポート & シザー
    uint32_t size = shadowMap.GetSize();
    D3D12_VIEWPORT viewport = {};
    viewport.Width    = static_cast<float>(size);
    viewport.Height   = static_cast<float>(size);
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = {};
    scissor.right  = static_cast<LONG>(size);
    scissor.bottom = static_cast<LONG>(size);
    cmdList->RSSetScissorRects(1, &scissor);

    // シャドウPSO設定
    cmdList->SetPipelineState(m_shadowPso.Get());
    m_currentPso = m_shadowPso.Get();
    cmdList->SetGraphicsRootSignature(m_shadowRootSignature.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 最初のカスケードで全lightVPを一括書き込み + オブジェクトCBリセット
    if (cascadeIndex == 0)
    {
        void* cbData = m_shadowPassCB.Map(frameIndex);
        if (cbData)
        {
            auto* base = static_cast<uint8_t*>(cbData);
            const auto& sc = m_csm.GetShadowConstants();
            // CSMカスケード（スロット0-3）
            for (uint32_t i = 0; i < CascadedShadowMap::k_NumCascades; ++i)
                memcpy(base + i * 256, &sc.lightVP[i], sizeof(XMFLOAT4X4));
            // スポットシャドウ（スロット4）
            memcpy(base + 4 * 256, &m_spotLightVP, sizeof(XMFLOAT4X4));
            // ポイントシャドウ各面（スロット5-10）
            for (uint32_t i = 0; i < PointShadowMap::k_NumFaces; ++i)
                memcpy(base + (5 + i) * 256, &m_pointShadowMap.GetFaceVP(i), sizeof(XMFLOAT4X4));
            m_shadowPassCB.Unmap(frameIndex);
        }

        m_objectCBMapped = static_cast<uint8_t*>(m_objectCB.Map(frameIndex));
        m_objectCBOffset = 0;
    }

    // 各カスケードは自分のオフセットでlightVPを参照
    uint32_t cbOffset = cascadeIndex * 256;
    cmdList->SetGraphicsRootConstantBufferView(1,
        m_shadowPassCB.GetGPUVirtualAddress(frameIndex) + cbOffset);
}

void Renderer3D::EndShadowPass(uint32_t cascadeIndex)
{
    m_inShadowPass = false;

    auto& shadowMap = m_csm.GetShadowMap(cascadeIndex);

    // リソースバリア: → PIXEL_SHADER_RESOURCE
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = shadowMap.GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);
    shadowMap.SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer3D::BeginSpotShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)
{
    if (m_spotShadowLightIndex < 0) return;

    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;
    m_inShadowPass = true;

    // DEPTH_WRITEへ遷移
    // 初学者向け: 描画前に「書き込み可能な状態」へ戻す必要があります。
    if (m_spotShadowMap.GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_spotShadowMap.GetResource();
        barrier.Transition.StateBefore = m_spotShadowMap.GetCurrentState();
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        m_spotShadowMap.SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    // 深度クリア
    auto dsvHandle = m_spotShadowMap.GetDSVHandle();
    cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

    // ビューポートとシザー
    D3D12_VIEWPORT viewport = {};
    viewport.Width    = static_cast<float>(k_SpotShadowMapSize);
    viewport.Height   = static_cast<float>(k_SpotShadowMapSize);
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = {};
    scissor.right  = static_cast<LONG>(k_SpotShadowMapSize);
    scissor.bottom = static_cast<LONG>(k_SpotShadowMapSize);
    cmdList->RSSetScissorRects(1, &scissor);

    // シャドウPSO
    cmdList->SetPipelineState(m_shadowPso.Get());
    m_currentPso = m_shadowPso.Get();
    cmdList->SetGraphicsRootSignature(m_shadowRootSignature.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // スポットライトのVPをバインド（シャドウCBのスロット4）
    cmdList->SetGraphicsRootConstantBufferView(1,
        m_shadowPassCB.GetGPUVirtualAddress(frameIndex) + 4 * 256);
}

void Renderer3D::EndSpotShadowPass()
{
    if (m_spotShadowLightIndex < 0) return;

    m_inShadowPass = false;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_spotShadowMap.GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);
    m_spotShadowMap.SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer3D::BeginPointShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                        uint32_t face)
{
    if (m_pointShadowLightIndex < 0) return;

    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;
    m_inShadowPass = true;

    // 最初の面ではリソース全体をDEPTH_WRITEへ遷移し、全6面をクリア
    if (face == 0)
    {
        if (m_pointShadowMap.GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource   = m_pointShadowMap.GetResource();
            barrier.Transition.StateBefore = m_pointShadowMap.GetCurrentState();
            barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &barrier);
            m_pointShadowMap.SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
        }

        // 6面すべてをクリア
        for (uint32_t f = 0; f < PointShadowMap::k_NumFaces; ++f)
        {
            auto dsv = m_pointShadowMap.GetFaceDSVHandle(f);
            cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        }
    }

    // 現在の面のDSVを設定
    auto dsvHandle = m_pointShadowMap.GetFaceDSVHandle(face);
    cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

    // ビューポートとシザー
    D3D12_VIEWPORT viewport = {};
    viewport.Width    = static_cast<float>(PointShadowMap::k_Size);
    viewport.Height   = static_cast<float>(PointShadowMap::k_Size);
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = {};
    scissor.right  = static_cast<LONG>(PointShadowMap::k_Size);
    scissor.bottom = static_cast<LONG>(PointShadowMap::k_Size);
    cmdList->RSSetScissorRects(1, &scissor);

    // シャドウPSO
    cmdList->SetPipelineState(m_shadowPso.Get());
    m_currentPso = m_shadowPso.Get();
    cmdList->SetGraphicsRootSignature(m_shadowRootSignature.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 面ごとのVPをバインド（シャドウCBのスロット5-10）
    cmdList->SetGraphicsRootConstantBufferView(1,
        m_shadowPassCB.GetGPUVirtualAddress(frameIndex) + (5 + face) * 256);
}

void Renderer3D::EndPointShadowPass(uint32_t face)
{
    if (m_pointShadowLightIndex < 0) return;

    m_inShadowPass = false;

    // 最後の面でPIXEL_SHADER_RESOURCEへ遷移
    if (face == PointShadowMap::k_NumFaces - 1)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_pointShadowMap.GetResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_cmdList->ResourceBarrier(1, &barrier);
        m_pointShadowMap.SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

void Renderer3D::Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                        const Camera3D& camera, float time)
{
    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;
    m_inShadowPass = false;

    // フレーム定数バッファ（シャドウ情報含む）
    void* cbData = m_frameCB.Map(frameIndex);
    if (cbData)
    {
        FrameConstants fc = {};
        XMStoreFloat4x4(&fc.view, XMMatrixTranspose(camera.GetViewMatrix()));
        XMStoreFloat4x4(&fc.projection, XMMatrixTranspose(camera.GetJitteredProjectionMatrix()));
        XMMATRIX jitteredVP = camera.GetViewMatrix() * camera.GetJitteredProjectionMatrix();
        XMStoreFloat4x4(&fc.viewProjection, XMMatrixTranspose(jitteredVP));
        fc.cameraPosition = camera.GetPosition();
        fc.time = time;

        // シャドウ情報
        if (m_shadowEnabled)
        {
            const auto& sc = m_csm.GetShadowConstants();
            for (uint32_t i = 0; i < ShadowConstants::k_NumCascades; ++i)
            {
                fc.lightVP[i]       = sc.lightVP[i];
                fc.cascadeSplits[i] = sc.cascadeSplits[i];
            }
            fc.shadowMapSize = sc.shadowMapSize;
            fc.shadowEnabled = 1;
        }
        else
        {
            fc.shadowEnabled = 0;
        }

        // シャドウデバッグモード
        fc.shadowDebugMode = m_shadowDebugMode;

        // 初回フレームでシャドウ定数をログ出力
        static bool s_shadowLogOnce = true;
        if (s_shadowLogOnce && m_shadowEnabled)
        {
            GX_LOG_INFO("Shadow: enabled=%u mapSize=%.0f splits=[%.1f,%.1f,%.1f,%.1f]",
                fc.shadowEnabled, fc.shadowMapSize,
                fc.cascadeSplits[0], fc.cascadeSplits[1], fc.cascadeSplits[2], fc.cascadeSplits[3]);
            GX_LOG_INFO("LightVP[0]: [%.4f, %.4f, %.4f, %.4f | %.4f, %.4f, %.4f, %.4f ...]",
                fc.lightVP[0]._11, fc.lightVP[0]._12, fc.lightVP[0]._13, fc.lightVP[0]._14,
                fc.lightVP[0]._21, fc.lightVP[0]._22, fc.lightVP[0]._23, fc.lightVP[0]._24);
            s_shadowLogOnce = false;
        }

        // フォグ情報
        fc.fogColor   = m_fogConstants.fogColor;
        fc.fogStart   = m_fogConstants.fogStart;
        fc.fogEnd     = m_fogConstants.fogEnd;
        fc.fogDensity = m_fogConstants.fogDensity;
        fc.fogMode    = m_fogConstants.fogMode;

        // スポットシャドウ情報
        fc.spotLightVP = m_spotLightVP;
        fc.spotShadowMapSize = static_cast<float>(k_SpotShadowMapSize);
        fc.spotShadowLightIndex = m_spotShadowLightIndex;

        // ポイントシャドウ情報
        for (uint32_t i = 0; i < PointShadowMap::k_NumFaces; ++i)
            fc.pointLightVP[i] = m_pointShadowMap.GetFaceVP(i);
        fc.pointShadowMapSize = static_cast<float>(PointShadowMap::k_Size);
        fc.pointShadowLightIndex = m_pointShadowLightIndex;

        memcpy(cbData, &fc, sizeof(FrameConstants));
        m_frameCB.Unmap(frameIndex);
    }

    // ライト定数バッファ
    cbData = m_lightCB.Map(frameIndex);
    if (cbData)
    {
        memcpy(cbData, &m_currentLights, sizeof(LightConstants));
        m_lightCB.Unmap(frameIndex);
    }

    // シャドウパスを経由していない場合、ここでオブジェクトCBをマップ
    if (!m_objectCBMapped)
    {
        m_objectCBMapped = static_cast<uint8_t*>(m_objectCB.Map(frameIndex));
        m_objectCBOffset = 0;
    }

    // 冗長バインド防止用リセット
    m_lastBoundVB = nullptr;
    m_lastBoundIB = nullptr;

    // マテリアルCBをマップ（リングバッファ方式）
    m_materialCBMapped = static_cast<uint8_t*>(m_materialCB.Map(frameIndex));
    m_materialCBOffset = 0;

    // パイプライン設定
    m_cmdList->SetPipelineState(m_pso.Get());
    m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_currentPso = m_pso.Get();

    // ディスクリプタヒープ（テクスチャ + シャドウマップは同一ヒープ）
    ID3D12DescriptorHeap* heaps[] = { m_textureManager.GetSRVHeap().GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);

    // フレーム・ライト定数バッファをバインド
    m_cmdList->SetGraphicsRootConstantBufferView(1, m_frameCB.GetGPUVirtualAddress(frameIndex));
    m_cmdList->SetGraphicsRootConstantBufferView(2, m_lightCB.GetGPUVirtualAddress(frameIndex));

    // シャドウマップSRVバインド（Root Param 5: t8-t11）
    if (m_shadowEnabled)
    {
        m_cmdList->SetGraphicsRootDescriptorTable(10, m_csm.GetSRVGPUHandle());
    }

    // トポロジ
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // デフォルトマテリアル適用
    SetMaterial(m_defaultMaterial);
}

void Renderer3D::SetLights(const LightData* lights, uint32_t count, const XMFLOAT3& ambient)
{
    m_currentLights = {};
    uint32_t n = (std::min)(count, LightConstants::k_MaxLights);
    for (uint32_t i = 0; i < n; ++i)
        m_currentLights.lights[i] = lights[i];
    m_currentLights.numLights = n;
    m_currentLights.ambientColor = ambient;
}

void Renderer3D::SetMaterial(const Material& material)
{
    if (m_inShadowPass) return;  // シャドウパスではマテリアル不要

    // マテリアル定数バッファ更新（リングバッファ方式）
    if (m_materialCBMapped)
    {
        memcpy(m_materialCBMapped + m_materialCBOffset, &material.constants, sizeof(MaterialConstants));
    }
    m_cmdList->SetGraphicsRootConstantBufferView(
        3, m_materialCB.GetGPUVirtualAddress(m_frameIndex) + m_materialCBOffset);
    m_materialCBOffset += 256;

    // テクスチャバインド（t0-t4）
    auto bindTex = [this](uint32_t rootIndex, int handle, int fallback)
    {
        int useHandle = (handle >= 0) ? handle : fallback;
        if (useHandle < 0) return;
        Texture* tex = m_textureManager.GetTexture(useHandle);
        if (tex)
            m_cmdList->SetGraphicsRootDescriptorTable(rootIndex, tex->GetSRVGPUHandle());
    };

    bindTex(5, material.albedoMapHandle, m_defaultWhiteTex);
    bindTex(6, material.normalMapHandle, m_defaultNormalTex);
    bindTex(7, material.metRoughMapHandle, m_defaultWhiteTex);
    bindTex(8, material.aoMapHandle, m_defaultWhiteTex);
    bindTex(9, material.emissiveMapHandle, m_defaultBlackTex);
}

void Renderer3D::DrawMesh(const GPUMesh& mesh, const Transform3D& transform)
{
    BindPipeline(false, -1);
    // オブジェクト定数バッファ更新（リングバッファ方式）
    if (m_objectCBMapped)
    {
        ObjectConstants oc;
        XMStoreFloat4x4(&oc.world, XMMatrixTranspose(transform.GetWorldMatrix()));
        XMStoreFloat4x4(&oc.worldInverseTranspose, XMMatrixTranspose(transform.GetWorldInverseTranspose()));
        memcpy(m_objectCBMapped + m_objectCBOffset, &oc, sizeof(ObjectConstants));
    }
    m_cmdList->SetGraphicsRootConstantBufferView(
        0, m_objectCB.GetGPUVirtualAddress(m_frameIndex) + m_objectCBOffset);
    m_objectCBOffset += 256;

    // VB/IB は前回と同じメッシュなら再バインドをスキップ
    ID3D12Resource* vbRes = mesh.vertexBuffer.GetResource();
    ID3D12Resource* ibRes = mesh.indexBuffer.GetResource();
    if (m_lastBoundVB != vbRes || m_lastBoundIB != ibRes)
    {
        auto vbv = mesh.vertexBuffer.GetVertexBufferView();
        auto ibv = mesh.indexBuffer.GetIndexBufferView();
        m_cmdList->IASetVertexBuffers(0, 1, &vbv);
        m_cmdList->IASetIndexBuffer(&ibv);
        m_lastBoundVB = vbRes;
        m_lastBoundIB = ibRes;
    }
    m_cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
}

void Renderer3D::DrawMesh(const GPUMesh& mesh, const XMMATRIX& worldMatrix)
{
    BindPipeline(false, -1);
    if (m_objectCBMapped)
    {
        ObjectConstants oc;
        XMStoreFloat4x4(&oc.world, XMMatrixTranspose(worldMatrix));
        XMMATRIX invTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));
        XMStoreFloat4x4(&oc.worldInverseTranspose, XMMatrixTranspose(invTranspose));
        memcpy(m_objectCBMapped + m_objectCBOffset, &oc, sizeof(ObjectConstants));
    }
    m_cmdList->SetGraphicsRootConstantBufferView(
        0, m_objectCB.GetGPUVirtualAddress(m_frameIndex) + m_objectCBOffset);
    m_objectCBOffset += 256;

    // VB/IB は前回と同じメッシュなら再バインドをスキップ
    ID3D12Resource* vbRes = mesh.vertexBuffer.GetResource();
    ID3D12Resource* ibRes = mesh.indexBuffer.GetResource();
    if (m_lastBoundVB != vbRes || m_lastBoundIB != ibRes)
    {
        auto vbv = mesh.vertexBuffer.GetVertexBufferView();
        auto ibv = mesh.indexBuffer.GetIndexBufferView();
        m_cmdList->IASetVertexBuffers(0, 1, &vbv);
        m_cmdList->IASetIndexBuffer(&ibv);
        m_lastBoundVB = vbRes;
        m_lastBoundIB = ibRes;
    }
    m_cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
}

void Renderer3D::DrawTerrain(const Terrain& terrain, const Transform3D& transform)
{
    BindPipeline(false, -1);
    // オブジェクト定数バッファ更新（リングバッファ方式）
    if (m_objectCBMapped)
    {
        ObjectConstants oc;
        XMStoreFloat4x4(&oc.world, XMMatrixTranspose(transform.GetWorldMatrix()));
        XMStoreFloat4x4(&oc.worldInverseTranspose, XMMatrixTranspose(transform.GetWorldInverseTranspose()));
        memcpy(m_objectCBMapped + m_objectCBOffset, &oc, sizeof(ObjectConstants));
    }
    m_cmdList->SetGraphicsRootConstantBufferView(
        0, m_objectCB.GetGPUVirtualAddress(m_frameIndex) + m_objectCBOffset);
    m_objectCBOffset += 256;

    auto vbv = terrain.GetVertexBuffer().GetVertexBufferView();
    auto ibv = terrain.GetIndexBuffer().GetIndexBufferView();
    m_cmdList->IASetVertexBuffers(0, 1, &vbv);
    m_cmdList->IASetIndexBuffer(&ibv);
    m_cmdList->DrawIndexedInstanced(terrain.GetIndexCount(), 1, 0, 0, 0);
}

void Renderer3D::DrawModel(const Model& model, const Transform3D& transform)
{
    // オブジェクト定数バッファ更新（リングバッファ方式）
    if (m_objectCBMapped)
    {
        ObjectConstants oc;
        XMStoreFloat4x4(&oc.world, XMMatrixTranspose(transform.GetWorldMatrix()));
        XMStoreFloat4x4(&oc.worldInverseTranspose, XMMatrixTranspose(transform.GetWorldInverseTranspose()));
        memcpy(m_objectCBMapped + m_objectCBOffset, &oc, sizeof(ObjectConstants));
    }
    m_cmdList->SetGraphicsRootConstantBufferView(
        0, m_objectCB.GetGPUVirtualAddress(m_frameIndex) + m_objectCBOffset);
    m_objectCBOffset += 256;

    auto vbv = model.GetMesh().GetVertexBuffer().GetVertexBufferView();
    auto ibv = model.GetMesh().GetIndexBuffer().GetIndexBufferView();
    m_cmdList->IASetVertexBuffers(0, 1, &vbv);
    m_cmdList->IASetIndexBuffer(&ibv);

    bool skinned = model.IsSkinned();
    for (const auto& sub : model.GetMesh().GetSubMeshes())
    {
        int shaderHandle = sub.shaderHandle;
        Material* mat = nullptr;
        if (!m_inShadowPass)
        {
            if (sub.materialHandle >= 0)
                mat = m_materialManager.GetMaterial(sub.materialHandle);
            if (shaderHandle < 0 && mat && mat->shaderHandle >= 0)
                shaderHandle = mat->shaderHandle;
        }

        BindPipeline(skinned, shaderHandle);

        if (!m_inShadowPass && mat)
            SetMaterial(*mat);

        m_cmdList->DrawIndexedInstanced(sub.indexCount, 1, sub.indexOffset, 0, 0);
    }
}

void Renderer3D::DrawSkinnedModel(const Model& model, const Transform3D& transform,
                                    const AnimationPlayer& animPlayer)
{
    void* cbData = m_boneCB.Map(m_frameIndex);
    if (cbData)
    {
        memcpy(cbData, &animPlayer.GetBoneConstants(), sizeof(BoneConstants));
        m_boneCB.Unmap(m_frameIndex);
    }
    uint32_t boneRootIndex = m_inShadowPass ? 2 : 4;
    m_cmdList->SetGraphicsRootConstantBufferView(
        boneRootIndex, m_boneCB.GetGPUVirtualAddress(m_frameIndex));
    DrawModel(model, transform);
}

void Renderer3D::DrawSkinnedModel(const Model& model, const Transform3D& transform,
                                    const Animator& animator)
{
    void* cbData = m_boneCB.Map(m_frameIndex);
    if (cbData)
    {
        memcpy(cbData, &animator.GetBoneConstants(), sizeof(BoneConstants));
        m_boneCB.Unmap(m_frameIndex);
    }
    uint32_t boneRootIndex = m_inShadowPass ? 2 : 4;
    m_cmdList->SetGraphicsRootConstantBufferView(
        boneRootIndex, m_boneCB.GetGPUVirtualAddress(m_frameIndex));
    DrawModel(model, transform);
}

void Renderer3D::End()
{
    // オブジェクトCBアンマップ
    if (m_objectCBMapped)
    {
        m_objectCB.Unmap(m_frameIndex);
        m_objectCBMapped = nullptr;
    }
    // マテリアルCBアンマップ
    if (m_materialCBMapped)
    {
        m_materialCB.Unmap(m_frameIndex);
        m_materialCBMapped = nullptr;
    }
    m_cmdList = nullptr;
}

void Renderer3D::SetFog(FogMode mode, const XMFLOAT3& color, float start, float end, float density)
{
    m_fogConstants.fogMode    = static_cast<uint32_t>(mode);
    m_fogConstants.fogColor   = color;
    m_fogConstants.fogStart   = start;
    m_fogConstants.fogEnd     = end;
    m_fogConstants.fogDensity = density;
}

void Renderer3D::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth  = width;
    m_screenHeight = height;
    if (m_device)
        m_depthBuffer.CreateWithOwnSRV(m_device, width, height);
}

} // namespace GX
