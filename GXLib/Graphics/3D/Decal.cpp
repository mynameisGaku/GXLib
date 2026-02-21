#include "pch.h"
/// @file Decal.cpp
/// @brief デカールシステム（Deferred Box Projection）の実装

#include "Graphics/3D/Decal.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"

namespace GX
{

// ============================================================================
// ユニットキューブ頂点データ（位置のみ、float3 x 8頂点）
// ============================================================================
struct DecalVertex
{
    XMFLOAT3 position;
};

// ユニットキューブ: -0.5 〜 +0.5 の範囲
static const DecalVertex k_CubeVertices[] =
{
    // 前面 (z = +0.5)
    { { -0.5f, -0.5f,  0.5f } },  // 0: 左下前
    { {  0.5f, -0.5f,  0.5f } },  // 1: 右下前
    { {  0.5f,  0.5f,  0.5f } },  // 2: 右上前
    { { -0.5f,  0.5f,  0.5f } },  // 3: 左上前
    // 背面 (z = -0.5)
    { { -0.5f, -0.5f, -0.5f } },  // 4: 左下後
    { {  0.5f, -0.5f, -0.5f } },  // 5: 右下後
    { {  0.5f,  0.5f, -0.5f } },  // 6: 右上後
    { { -0.5f,  0.5f, -0.5f } },  // 7: 左上後
};

// 36 indices (12 triangles, 6 faces)
static const uint16_t k_CubeIndices[] =
{
    // 前面 (+Z)
    0, 1, 2,  0, 2, 3,
    // 背面 (-Z)
    5, 4, 7,  5, 7, 6,
    // 上面 (+Y)
    3, 2, 6,  3, 6, 7,
    // 下面 (-Y)
    4, 5, 1,  4, 1, 0,
    // 右面 (+X)
    1, 5, 6,  1, 6, 2,
    // 左面 (-X)
    4, 0, 3,  4, 3, 7,
};

// ============================================================================
// 初期化
// ============================================================================
bool DecalSystem::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    if (!device || width == 0 || height == 0)
        return false;

    m_width = width;
    m_height = height;

    // ユニットキューブメッシュ作成
    if (!CreateCubeMesh(device))
        return false;

    // PSO作成
    if (!CreatePSO(device))
        return false;

    // デカール用定数バッファ（256バイト x ダブルバッファリング）
    if (!m_cb.Initialize(device, sizeof(DecalCB), sizeof(DecalCB)))
        return false;

    // SRVヒープ: 深度テクスチャ + デカールテクスチャ（2スロット）
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true))
        return false;

    m_initialized = true;
    return true;
}

// ============================================================================
// キューブメッシュ作成
// ============================================================================
bool DecalSystem::CreateCubeMesh(ID3D12Device* device)
{
    if (!m_cubeVB.CreateVertexBuffer(device, k_CubeVertices,
        sizeof(k_CubeVertices), sizeof(DecalVertex)))
        return false;

    if (!m_cubeIB.CreateIndexBuffer(device, k_CubeIndices,
        sizeof(k_CubeIndices), DXGI_FORMAT_R16_UINT))
        return false;

    return true;
}

// ============================================================================
// PSO作成
// ============================================================================
bool DecalSystem::CreatePSO(ID3D12Device* device)
{
    // ルートシグネチャ:
    //   [0] CBV b0 — DecalCB
    //   [1] DescriptorTable SRV t0-t1 — 深度テクスチャ + デカールテクスチャ
    //   Static sampler s0 — LINEAR/CLAMP
    auto rs = RootSignatureBuilder()
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL,
                            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE)
        .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                          D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                          D3D12_COMPARISON_FUNC_NEVER)
        .Build(device);

    if (!rs)
        return false;

    m_rs = rs;

    // シェーダーコンパイル
    Shader shader;
    if (!shader.Initialize())
        return false;

    auto vsBlob = shader.CompileFromFile(L"Shaders/Decal.hlsl", L"VSMain", L"vs_6_0");
    auto psBlob = shader.CompileFromFile(L"Shaders/Decal.hlsl", L"PSMain", L"ps_6_0");

    if (!vsBlob.valid || !psBlob.valid)
        return false;

    // 入力レイアウト（位置のみ）
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // PSO:
    // - アルファブレンド有効（デカールは半透明合成）
    // - 深度テスト有効、深度書き込み無効
    // - FRONT面カリング（背面のみ描画 — カメラがキューブ内にいても描画される）
    // - HDR RTフォーマット
    auto pso = PipelineStateBuilder()
        .SetRootSignature(m_rs.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetInputLayout(inputLayout, _countof(inputLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthFormat(DXGI_FORMAT_D32_FLOAT)
        .SetDepthEnable(true)
        .SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO)
        .SetDepthComparisonFunc(D3D12_COMPARISON_FUNC_GREATER_EQUAL)
        .SetCullMode(D3D12_CULL_MODE_FRONT)
        .SetAlphaBlend()
        .Build(device);

    if (!pso)
        return false;

    m_pso = pso;
    return true;
}

// ============================================================================
// デカール追加
// ============================================================================
int DecalSystem::AddDecal(const DecalData& decal)
{
    // フリーリストから再利用
    if (!m_freeList.empty())
    {
        int handle = m_freeList.back();
        m_freeList.pop_back();
        m_decals[handle].data = decal;
        m_decals[handle].valid = true;
        return handle;
    }

    // 最大数チェック
    if (static_cast<uint32_t>(m_decals.size()) >= k_MaxDecals)
        return -1;

    // 新規追加
    int handle = static_cast<int>(m_decals.size());
    DecalEntry entry;
    entry.data = decal;
    entry.valid = true;
    m_decals.push_back(entry);
    return handle;
}

// ============================================================================
// デカール削除
// ============================================================================
void DecalSystem::RemoveDecal(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_decals.size()))
        return;
    if (!m_decals[handle].valid)
        return;

    m_decals[handle].valid = false;
    m_freeList.push_back(handle);
}

// ============================================================================
// 寿命管理の更新
// ============================================================================
void DecalSystem::Update(float deltaTime)
{
    for (int i = 0; i < static_cast<int>(m_decals.size()); ++i)
    {
        auto& entry = m_decals[i];
        if (!entry.valid)
            continue;

        // 永続デカール（lifetime < 0）はスキップ
        if (entry.data.lifetime < 0.0f)
            continue;

        entry.data.age += deltaTime;

        // 寿命超過で削除
        if (entry.data.age >= entry.data.lifetime)
        {
            RemoveDecal(i);
        }
    }
}

// ============================================================================
// デカール描画
// ============================================================================
void DecalSystem::Render(ID3D12GraphicsCommandList* cmdList,
                          const Camera3D& camera,
                          ID3D12Resource* depthSRV,
                          TextureManager& texManager,
                          uint32_t frameIndex)
{
    if (!m_initialized || !cmdList || !depthSRV)
        return;

    // アクティブなデカールがあるか確認
    bool hasActive = false;
    for (const auto& entry : m_decals)
    {
        if (entry.valid)
        {
            hasActive = true;
            break;
        }
    }
    if (!hasActive)
        return;

    // ビュープロジェクション行列と逆行列を計算
    XMMATRIX viewMat = camera.GetViewMatrix();
    XMMATRIX projMat = camera.GetProjectionMatrix();
    XMMATRIX viewProj = XMMatrixMultiply(viewMat, projMat);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    XMFLOAT4X4 invViewProjF;
    XMStoreFloat4x4(&invViewProjF, XMMatrixTranspose(invViewProj));

    // PSO設定
    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rs.Get());

    // プリミティブトポロジ
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // キューブメッシュのVB/IB
    auto vbv = m_cubeVB.GetVertexBufferView();
    auto ibv = m_cubeIB.GetIndexBufferView();
    cmdList->IASetVertexBuffers(0, 1, &vbv);
    cmdList->IASetIndexBuffer(&ibv);

    // 各デカールを描画
    for (const auto& entry : m_decals)
    {
        if (!entry.valid)
            continue;

        const DecalData& decal = entry.data;

        // テクスチャハンドルが無効ならスキップ
        if (decal.textureHandle < 0)
            continue;

        Texture* decalTex = texManager.GetTexture(decal.textureHandle);
        if (!decalTex)
            continue;

        // デカールのワールド行列と逆ワールド行列を計算
        XMMATRIX decalWorldMat = decal.transform.GetWorldMatrix();
        XMMATRIX decalInvWorldMat = XMMatrixInverse(nullptr, decalWorldMat);

        // 定数バッファを更新
        DecalCB cb = {};
        cb.invViewProj = invViewProjF;
        XMStoreFloat4x4(&cb.decalWorld, XMMatrixTranspose(XMMatrixMultiply(decalWorldMat, viewProj)));
        XMStoreFloat4x4(&cb.decalInvWorld, XMMatrixTranspose(decalInvWorldMat));
        cb.decalColor = decal.color.ToXMFLOAT4();

        // 寿命によるフェードアウト
        if (decal.lifetime > 0.0f)
        {
            float fadeStart = decal.lifetime * 0.8f;  // 残り20%でフェードアウト開始
            if (decal.age > fadeStart)
            {
                float fadeT = (decal.age - fadeStart) / (decal.lifetime - fadeStart);
                fadeT = (std::min)(fadeT, 1.0f);
                cb.decalColor.w *= (1.0f - fadeT);
            }
        }

        cb.fadeDistance = decal.fadeDistance;
        cb.normalThreshold = decal.normalThreshold;
        cb.screenSize = { static_cast<float>(m_width), static_cast<float>(m_height) };

        // CBにデータ書き込み
        void* mapped = m_cb.Map(frameIndex);
        if (mapped)
        {
            memcpy(mapped, &cb, sizeof(DecalCB));
            m_cb.Unmap(frameIndex);
        }

        // CBVバインド（ルートパラメータ0）
        cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));

        // SRVヒープを設定し、深度テクスチャとデカールテクスチャのSRVを作成
        ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        // 深度テクスチャSRV (t0)
        D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
        depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;  // D32_FLOATをR32_FLOATとして読む
        depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        depthSrvDesc.Texture2D.MipLevels = 1;

        // デバイスをリソースから取得
        ComPtr<ID3D12Device> pDevice;
        depthSRV->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(pDevice.GetAddressOf()));

        if (pDevice)
        {
            pDevice->CreateShaderResourceView(depthSRV, &depthSrvDesc,
                m_srvHeap.GetCPUHandle(0));

            // デカールテクスチャSRV (t1)
            D3D12_SHADER_RESOURCE_VIEW_DESC texSrvDesc = {};
            texSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            texSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            texSrvDesc.Texture2D.MipLevels = 1;

            pDevice->CreateShaderResourceView(decalTex->GetResource(), &texSrvDesc,
                m_srvHeap.GetCPUHandle(1));
        }

        // ディスクリプタテーブルバインド（ルートパラメータ1）
        cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(0));

        // キューブ描画（36インデックス）
        cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
    }
}

// ============================================================================
// デカール数取得
// ============================================================================
int DecalSystem::GetDecalCount() const
{
    int count = 0;
    for (const auto& entry : m_decals)
    {
        if (entry.valid)
            ++count;
    }
    return count;
}

// ============================================================================
// デカールデータ取得
// ============================================================================
DecalData* DecalSystem::GetDecal(int handle)
{
    if (handle < 0 || handle >= static_cast<int>(m_decals.size()))
        return nullptr;
    if (!m_decals[handle].valid)
        return nullptr;
    return &m_decals[handle].data;
}

// ============================================================================
// リソース解放
// ============================================================================
void DecalSystem::Shutdown()
{
    m_decals.clear();
    m_freeList.clear();
    m_pso.Reset();
    m_rs.Reset();
    m_initialized = false;
}

} // namespace GX
