/// @file Renderer3D.cpp
/// @brief 3Dレンダラーの実装
#include "pch.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

bool Renderer3D::Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                              uint32_t screenWidth, uint32_t screenHeight)
{
    m_device       = device;
    m_screenWidth  = screenWidth;
    m_screenHeight = screenHeight;

    // 深度バッファ
    if (!m_depthBuffer.Create(device, screenWidth, screenHeight))
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

    // 定数バッファ（256アラインメント）- リングバッファ化
    if (!m_objectCB.Initialize(device, 256 * k_MaxObjectsPerFrame, 256))
        return false;

    // FrameConstants: 増大したので適切なサイズ計算
    uint32_t frameCBSize = ((sizeof(FrameConstants) + 255) / 256) * 256;
    if (!m_frameCB.Initialize(device, frameCBSize, frameCBSize))
        return false;

    // LightConstants
    uint32_t lightCBSize = ((sizeof(LightConstants) + 255) / 256) * 256;
    if (!m_lightCB.Initialize(device, lightCBSize, lightCBSize))
        return false;

    // MaterialConstants
    if (!m_materialCB.Initialize(device, 256, 256))
        return false;

    // BoneConstants
    uint32_t boneCBSize = ((sizeof(BoneConstants) + 255) / 256) * 256;
    if (!m_boneCB.Initialize(device, boneCBSize, boneCBSize))
        return false;

    // Shadow pass CB (lightVP matrix per cascade, 256-aligned × 4 cascades)
    if (!m_shadowPassCB.Initialize(device, 256 * CascadedShadowMap::k_NumCascades,
                                    256 * CascadedShadowMap::k_NumCascades))
        return false;

    // シェーダーコンパイラ
    if (!m_shaderCompiler.Initialize())
        return false;

    // CSM初期化（TextureManagerのSRVヒープにSRVを配置）
    // 連続4スロットをTextureManagerのSRVヒープから確保
    auto& srvHeap = m_textureManager.GetSRVHeap();
    uint32_t shadowSrvStart = srvHeap.AllocateIndex();
    for (uint32_t i = 1; i < CascadedShadowMap::k_NumCascades; ++i)
        srvHeap.AllocateIndex();

    if (!m_csm.Initialize(device, &srvHeap, shadowSrvStart))
    {
        GX_LOG_ERROR("Renderer3D: Failed to initialize CSM");
        return false;
    }

    // メインPSO
    if (!CreatePipelineState(device))
        return false;

    // シャドウPSO
    if (!CreateShadowPipelineState(device))
        return false;

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
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("Renderer3D: Failed to compile PBR shaders");
        return false;
    }

    // ルートシグネチャ
    // Root Param 0: b0 ObjectConstants (CBV)
    // Root Param 1: b1 FrameConstants  (CBV)
    // Root Param 2: b2 LightConstants  (CBV)
    // Root Param 3: b3 MaterialConstants (CBV)
    // Root Param 4: Descriptor table for textures (t0-t4)
    // Root Param 5: Descriptor table for shadow maps (t8-t11)
    // s0: linear wrap sampler
    // s2: comparison sampler for PCF
    RootSignatureBuilder rsBuilder;
    m_rootSignature = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        .AddCBV(0)  // b0: ObjectConstants
        .AddCBV(1)  // b1: FrameConstants
        .AddCBV(2)  // b2: LightConstants
        .AddCBV(3)  // b3: MaterialConstants
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)  // t0-t4
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 4, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)  // t8-t11 shadow maps
        .AddStaticSampler(0)  // s0: linear wrap
        .AddStaticSampler(2, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
                          D3D12_TEXTURE_ADDRESS_MODE_BORDER,
                          D3D12_COMPARISON_FUNC_LESS_EQUAL)  // s2: shadow comparison
        .Build(device);

    if (!m_rootSignature)
        return false;

    PipelineStateBuilder psoBuilder;
    m_pso = psoBuilder
        .SetRootSignature(m_rootSignature.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(k_Vertex3DPBRLayout, _countof(k_Vertex3DPBRLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)  // HDR RT
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetCullMode(D3D12_CULL_MODE_BACK)
        .Build(device);

    return m_pso != nullptr;
}

bool Renderer3D::CreateShadowPipelineState(ID3D12Device* device)
{
    auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/ShadowDepth.hlsl", L"VSMain", L"vs_6_0");
    if (!vsBlob.valid)
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
        .SetCullMode(D3D12_CULL_MODE_FRONT)  // フロントフェイスカリング（シャドウアクネ対策）
        .SetDepthBias(1500, 0.0f, 1.5f)      // 深度バイアス
        .SetRenderTargetCount(0)               // カラー出力なし
        .Build(device);

    return m_shadowPso != nullptr;
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
    for (uint32_t i = 0; i < m_currentLights.numLights; ++i)
    {
        if (m_currentLights.lights[i].type == static_cast<uint32_t>(LightType::Directional))
        {
            lightDir = m_currentLights.lights[i].direction;
            break;
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
    cmdList->SetGraphicsRootSignature(m_shadowRootSignature.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 最初のカスケードで全lightVPを一括書き込み + オブジェクトCBリセット
    if (cascadeIndex == 0)
    {
        void* cbData = m_shadowPassCB.Map(frameIndex);
        if (cbData)
        {
            const auto& sc = m_csm.GetShadowConstants();
            for (uint32_t i = 0; i < CascadedShadowMap::k_NumCascades; ++i)
                memcpy(static_cast<uint8_t*>(cbData) + i * 256, &sc.lightVP[i], sizeof(XMFLOAT4X4));
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
        XMStoreFloat4x4(&fc.projection, XMMatrixTranspose(camera.GetProjectionMatrix()));
        XMStoreFloat4x4(&fc.viewProjection, XMMatrixTranspose(camera.GetViewProjectionMatrix()));
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

    // パイプライン設定
    m_cmdList->SetPipelineState(m_pso.Get());
    m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // ディスクリプタヒープ（テクスチャ + シャドウマップは同一ヒープ）
    ID3D12DescriptorHeap* heaps[] = { m_textureManager.GetSRVHeap().GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);

    // フレーム・ライト定数バッファをバインド
    m_cmdList->SetGraphicsRootConstantBufferView(1, m_frameCB.GetGPUVirtualAddress(frameIndex));
    m_cmdList->SetGraphicsRootConstantBufferView(2, m_lightCB.GetGPUVirtualAddress(frameIndex));

    // シャドウマップSRVバインド（Root Param 5: t8-t11）
    if (m_shadowEnabled)
    {
        m_cmdList->SetGraphicsRootDescriptorTable(5, m_csm.GetSRVGPUHandle());
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

    // マテリアル定数バッファ更新
    void* cbData = m_materialCB.Map(m_frameIndex);
    if (cbData)
    {
        memcpy(cbData, &material.constants, sizeof(MaterialConstants));
        m_materialCB.Unmap(m_frameIndex);
    }
    m_cmdList->SetGraphicsRootConstantBufferView(3, m_materialCB.GetGPUVirtualAddress(m_frameIndex));

    // テクスチャバインド（t0-t4）
    if (material.albedoMapHandle >= 0)
    {
        Texture* tex = m_textureManager.GetTexture(material.albedoMapHandle);
        if (tex)
            m_cmdList->SetGraphicsRootDescriptorTable(4, tex->GetSRVGPUHandle());
    }
}

void Renderer3D::DrawMesh(const GPUMesh& mesh, const Transform3D& transform)
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

    auto vbv = mesh.vertexBuffer.GetVertexBufferView();
    auto ibv = mesh.indexBuffer.GetIndexBufferView();
    m_cmdList->IASetVertexBuffers(0, 1, &vbv);
    m_cmdList->IASetIndexBuffer(&ibv);
    m_cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
}

void Renderer3D::DrawTerrain(const Terrain& terrain, const Transform3D& transform)
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

    for (const auto& sub : model.GetMesh().GetSubMeshes())
    {
        if (!m_inShadowPass)
        {
            if (sub.materialHandle >= 0)
            {
                Material* mat = m_materialManager.GetMaterial(sub.materialHandle);
                if (mat)
                    SetMaterial(*mat);
            }
        }

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
        m_depthBuffer.Create(m_device, width, height);
}

} // namespace GX
