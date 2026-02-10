/// @file SpriteBatch.cpp
/// @brief スプライトバッチの実装
#include "pch.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"
#include <cmath>

namespace GX
{

bool SpriteBatch::Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                              uint32_t screenWidth, uint32_t screenHeight)
{
    m_device       = device;
    m_screenWidth  = screenWidth;
    m_screenHeight = screenHeight;

    // テクスチャマネージャ初期化
    if (!m_textureManager.Initialize(device, cmdQueue))
        return false;

    // 頂点バッファ（4頂点 × 最大スプライト数 × 32バイト/頂点）
    uint32_t vertexBufferSize = k_MaxSprites * 4 * sizeof(SpriteVertex);
    if (!m_vertexBuffer.Initialize(device, vertexBufferSize, sizeof(SpriteVertex)))
        return false;

    // インデックスバッファ作成
    if (!CreateIndexBuffer(device))
        return false;

    // 定数バッファ（正射影行列 = 64バイト、256アラインメント）
    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    // シェーダーコンパイル
    if (!m_shaderCompiler.Initialize())
        return false;

    // PSO群を作成
    if (!CreatePipelineStates(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/Sprite.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelineStates(dev); }
    );

    // 正射影行列を初期化
    UpdateProjectionMatrix();

    GX_LOG_INFO("SpriteBatch initialized (%dx%d, max: %u sprites)", screenWidth, screenHeight, k_MaxSprites);
    return true;
}

bool SpriteBatch::CreateIndexBuffer(ID3D12Device* device)
{
    // 全スプライト分の共有インデックスバッファを作成
    // 各スプライトは4頂点、6インデックス（2三角形）
    std::vector<uint16_t> indices(k_MaxSprites * 6);
    for (uint32_t i = 0; i < k_MaxSprites; ++i)
    {
        uint16_t base = static_cast<uint16_t>(i * 4);
        indices[i * 6 + 0] = base + 0;
        indices[i * 6 + 1] = base + 1;
        indices[i * 6 + 2] = base + 2;
        indices[i * 6 + 3] = base + 2;
        indices[i * 6 + 4] = base + 1;
        indices[i * 6 + 5] = base + 3;
    }

    return m_indexBuffer.CreateIndexBuffer(device, indices.data(),
                                            static_cast<uint32_t>(indices.size() * sizeof(uint16_t)));
}

bool SpriteBatch::CreatePipelineStates(ID3D12Device* device)
{
    // シェーダーコンパイル
    auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/Sprite.hlsl", L"VSMain", L"vs_6_0");
    auto psBlob = m_shaderCompiler.CompileFromFile(L"Shaders/Sprite.hlsl", L"PSMain", L"ps_6_0");
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("Failed to compile sprite shaders");
        return false;
    }

    // ルートシグネチャ作成
    // b0: TransformCB（正射影行列）
    // t0: テクスチャ（SRVディスクリプタテーブル）
    // s0: サンプラー
    RootSignatureBuilder rsBuilder;
    m_rootSignature = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)
        .AddCBV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX)                           // b0: 正射影行列
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL)                       // t0: テクスチャ
        .AddStaticSampler(0)                                                      // s0: リニアサンプラー
        .Build(device);

    if (!m_rootSignature)
        return false;

    // 頂点入力レイアウト
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 8,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // 各ブレンドモードのPSOを作成
    auto createPSO = [&](BlendMode mode) -> ComPtr<ID3D12PipelineState>
    {
        PipelineStateBuilder builder;
        builder
            .SetRootSignature(m_rootSignature.Get())
            .SetVertexShader(vsBlob.GetBytecode())
            .SetPixelShader(psBlob.GetBytecode())
            .SetInputLayout(inputLayout, _countof(inputLayout))
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE);

        switch (mode)
        {
        case BlendMode::Alpha:  builder.SetAlphaBlend();      break;
        case BlendMode::Add:    builder.SetAdditiveBlend();    break;
        case BlendMode::Sub:    builder.SetSubtractiveBlend(); break;
        case BlendMode::Mul:
        {
            D3D12_BLEND_DESC blendDesc = {};
            auto& rt = blendDesc.RenderTarget[0];
            rt.BlendEnable           = TRUE;
            rt.SrcBlend              = D3D12_BLEND_ZERO;
            rt.DestBlend             = D3D12_BLEND_SRC_COLOR;
            rt.BlendOp               = D3D12_BLEND_OP_ADD;
            rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
            rt.DestBlendAlpha        = D3D12_BLEND_ZERO;
            rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
            rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            builder.SetBlendState(blendDesc);
            break;
        }
        case BlendMode::Screen:
        {
            D3D12_BLEND_DESC blendDesc = {};
            auto& rt = blendDesc.RenderTarget[0];
            rt.BlendEnable           = TRUE;
            rt.SrcBlend              = D3D12_BLEND_INV_DEST_COLOR;
            rt.DestBlend             = D3D12_BLEND_ONE;
            rt.BlendOp               = D3D12_BLEND_OP_ADD;
            rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
            rt.DestBlendAlpha        = D3D12_BLEND_ZERO;
            rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
            rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            builder.SetBlendState(blendDesc);
            break;
        }
        case BlendMode::None:
        default:
            // デフォルト（不透明）
            break;
        }

        return builder.Build(device);
    };

    for (int i = 0; i < static_cast<int>(BlendMode::Count); ++i)
    {
        m_pipelineStates[i] = createPSO(static_cast<BlendMode>(i));
        if (!m_pipelineStates[i])
        {
            GX_LOG_ERROR("Failed to create PSO for blend mode %d", i);
            return false;
        }
    }

    return true;
}

void SpriteBatch::UpdateProjectionMatrix()
{
    // 2D正射影行列: 左上原点、Y軸下向き
    // (0,0) が左上、(screenWidth, screenHeight) が右下
    m_projectionMatrix = XMMatrixOrthographicOffCenterLH(
        0.0f, static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight), 0.0f,
        0.0f, 1.0f
    );
}

void SpriteBatch::Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)
{
    m_cmdList    = cmdList;
    m_frameIndex = frameIndex;

    m_mappedVertices    = static_cast<SpriteVertex*>(m_vertexBuffer.Map(frameIndex));
    m_spriteCount       = 0;
    m_currentTexture    = -1;
    m_lastBoundBlend    = BlendMode::Count;  // 新フレームでPSO再バインドを強制

    // フレームが変わった時だけオフセットをリセット（同一フレーム内の複数Begin/Endサイクルでは累積）
    if (m_lastFrameIndex != frameIndex)
    {
        m_vertexWriteOffset = 0;
        m_lastFrameIndex = frameIndex;
    }

    // 正射影行列を定数バッファに書き込み
    void* cbData = m_constantBuffer.Map(frameIndex);
    if (cbData)
    {
        XMMATRIX proj = m_useCustomProjection ? m_projectionMatrix
                                               : XMMatrixOrthographicOffCenterLH(
                                                     0.0f, static_cast<float>(m_screenWidth),
                                                     static_cast<float>(m_screenHeight), 0.0f,
                                                     0.0f, 1.0f);
        memcpy(cbData, &proj, sizeof(XMMATRIX));
        m_constantBuffer.Unmap(frameIndex);
    }

    // ディスクリプタヒープを設定
    ID3D12DescriptorHeap* heaps[] = { m_textureManager.GetSRVHeap().GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);
}

void SpriteBatch::AddQuad(const SpriteVertex& v0, const SpriteVertex& v1,
                           const SpriteVertex& v2, const SpriteVertex& v3,
                           int textureHandle)
{
    // テクスチャが変わったらフラッシュ
    Texture* tex = m_textureManager.GetTexture(textureHandle);
    if (!tex) return;

    // テクスチャのSRVハンドルで比較（リージョンハンドルでも同じテクスチャならバッチ可能）
    int actualTexHandle = textureHandle;
    const auto& region = m_textureManager.GetRegion(textureHandle);
    if (region.textureHandle >= 0 && region.textureHandle != textureHandle)
        actualTexHandle = region.textureHandle;

    if (m_currentTexture != -1 && m_currentTexture != actualTexHandle)
        Flush();

    if (m_vertexWriteOffset + m_spriteCount >= k_MaxSprites)
        Flush();

    m_currentTexture = actualTexHandle;

    uint32_t base = (m_vertexWriteOffset + m_spriteCount) * 4;
    m_mappedVertices[base + 0] = v0;
    m_mappedVertices[base + 1] = v1;
    m_mappedVertices[base + 2] = v2;
    m_mappedVertices[base + 3] = v3;

    m_spriteCount++;
}

void SpriteBatch::Flush()
{
    if (m_spriteCount == 0)
        return;

    Texture* tex = m_textureManager.GetTexture(m_currentTexture);
    if (!tex)
    {
        m_spriteCount = 0;
        return;
    }

    // パイプライン設定（ブレンドモードが変わった場合のみ再バインド）
    if (m_lastBoundBlend != m_blendMode)
    {
        m_cmdList->SetPipelineState(m_pipelineStates[static_cast<int>(m_blendMode)].Get());
        m_cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        m_lastBoundBlend = m_blendMode;
    }

    // 定数バッファ（正射影行列）
    m_cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(m_frameIndex));

    // テクスチャ SRV
    m_cmdList->SetGraphicsRootDescriptorTable(1, tex->GetSRVGPUHandle());

    // トポロジ
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 頂点バッファ（オフセット付き — 前回のFlush分を飛ばす）
    uint32_t vertexOffset = m_vertexWriteOffset * 4 * sizeof(SpriteVertex);
    uint32_t vertexSize   = m_spriteCount * 4 * sizeof(SpriteVertex);

    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = m_vertexBuffer.GetGPUVirtualAddress(m_frameIndex) + vertexOffset;
    vbv.SizeInBytes    = vertexSize;
    vbv.StrideInBytes  = sizeof(SpriteVertex);
    m_cmdList->IASetVertexBuffers(0, 1, &vbv);

    // インデックスバッファ
    auto ibv = m_indexBuffer.GetIndexBufferView();
    m_cmdList->IASetIndexBuffer(&ibv);

    // 描画
    m_cmdList->DrawIndexedInstanced(m_spriteCount * 6, 1, 0, 0, 0);

    m_vertexWriteOffset += m_spriteCount;
    m_spriteCount = 0;
}

void SpriteBatch::DrawGraph(float x, float y, int handle, bool transFlag)
{
    Texture* tex = m_textureManager.GetTexture(handle);
    if (!tex) return;

    const auto& region = m_textureManager.GetRegion(handle);

    float w, h;
    if (region.textureHandle >= 0 && region.textureHandle != handle)
    {
        // リージョンハンドル: UV矩形からサイズを計算
        w = (region.u1 - region.u0) * tex->GetWidth();
        h = (region.v1 - region.v0) * tex->GetHeight();
    }
    else
    {
        w = static_cast<float>(tex->GetWidth());
        h = static_cast<float>(tex->GetHeight());
    }

    SpriteVertex v0 = { { x,     y     }, { region.u0, region.v0 }, m_drawColor };
    SpriteVertex v1 = { { x + w, y     }, { region.u1, region.v0 }, m_drawColor };
    SpriteVertex v2 = { { x,     y + h }, { region.u0, region.v1 }, m_drawColor };
    SpriteVertex v3 = { { x + w, y + h }, { region.u1, region.v1 }, m_drawColor };

    AddQuad(v0, v1, v2, v3, handle);
}

void SpriteBatch::DrawRotaGraph(float cx, float cy, float extRate, float angle,
                                 int handle, bool transFlag)
{
    Texture* tex = m_textureManager.GetTexture(handle);
    if (!tex) return;

    const auto& region = m_textureManager.GetRegion(handle);

    float w, h;
    if (region.textureHandle >= 0 && region.textureHandle != handle)
    {
        w = (region.u1 - region.u0) * tex->GetWidth();
        h = (region.v1 - region.v0) * tex->GetHeight();
    }
    else
    {
        w = static_cast<float>(tex->GetWidth());
        h = static_cast<float>(tex->GetHeight());
    }

    float hw = w * 0.5f * extRate;
    float hh = h * 0.5f * extRate;

    float cosA = cosf(angle);
    float sinA = sinf(angle);

    // 回転変換: 中心からの相対座標を回転
    auto rotate = [&](float rx, float ry) -> XMFLOAT2
    {
        return { cx + rx * cosA - ry * sinA,
                 cy + rx * sinA + ry * cosA };
    };

    SpriteVertex v0 = { rotate(-hw, -hh), { region.u0, region.v0 }, m_drawColor };
    SpriteVertex v1 = { rotate( hw, -hh), { region.u1, region.v0 }, m_drawColor };
    SpriteVertex v2 = { rotate(-hw,  hh), { region.u0, region.v1 }, m_drawColor };
    SpriteVertex v3 = { rotate( hw,  hh), { region.u1, region.v1 }, m_drawColor };

    AddQuad(v0, v1, v2, v3, handle);
}

void SpriteBatch::DrawRectGraph(float x, float y, int srcX, int srcY, int w, int h,
                                 int handle, bool transFlag)
{
    Texture* tex = m_textureManager.GetTexture(handle);
    if (!tex) return;

    float texW = static_cast<float>(tex->GetWidth());
    float texH = static_cast<float>(tex->GetHeight());

    float u0 = srcX / texW;
    float v0 = srcY / texH;
    float u1 = (srcX + w) / texW;
    float v1 = (srcY + h) / texH;

    float fw = static_cast<float>(w);
    float fh = static_cast<float>(h);

    SpriteVertex sv0 = { { x,      y      }, { u0, v0 }, m_drawColor };
    SpriteVertex sv1 = { { x + fw, y      }, { u1, v0 }, m_drawColor };
    SpriteVertex sv2 = { { x,      y + fh }, { u0, v1 }, m_drawColor };
    SpriteVertex sv3 = { { x + fw, y + fh }, { u1, v1 }, m_drawColor };

    AddQuad(sv0, sv1, sv2, sv3, handle);
}

void SpriteBatch::DrawRectExtendGraph(float dstX, float dstY, float dstW, float dstH,
                                       int srcX, int srcY, int srcW, int srcH,
                                       int handle, bool transFlag)
{
    Texture* tex = m_textureManager.GetTexture(handle);
    if (!tex) return;

    float texW = static_cast<float>(tex->GetWidth());
    float texH = static_cast<float>(tex->GetHeight());

    float u0 = srcX / texW;
    float v0 = srcY / texH;
    float u1 = (srcX + srcW) / texW;
    float v1 = (srcY + srcH) / texH;

    SpriteVertex sv0 = { { dstX,        dstY        }, { u0, v0 }, m_drawColor };
    SpriteVertex sv1 = { { dstX + dstW, dstY        }, { u1, v0 }, m_drawColor };
    SpriteVertex sv2 = { { dstX,        dstY + dstH }, { u0, v1 }, m_drawColor };
    SpriteVertex sv3 = { { dstX + dstW, dstY + dstH }, { u1, v1 }, m_drawColor };

    AddQuad(sv0, sv1, sv2, sv3, handle);
}

void SpriteBatch::DrawExtendGraph(float x1, float y1, float x2, float y2,
                                   int handle, bool transFlag)
{
    Texture* tex = m_textureManager.GetTexture(handle);
    if (!tex) return;

    const auto& region = m_textureManager.GetRegion(handle);

    SpriteVertex v0 = { { x1, y1 }, { region.u0, region.v0 }, m_drawColor };
    SpriteVertex v1 = { { x2, y1 }, { region.u1, region.v0 }, m_drawColor };
    SpriteVertex v2 = { { x1, y2 }, { region.u0, region.v1 }, m_drawColor };
    SpriteVertex v3 = { { x2, y2 }, { region.u1, region.v1 }, m_drawColor };

    AddQuad(v0, v1, v2, v3, handle);
}

void SpriteBatch::DrawModiGraph(float x1, float y1, float x2, float y2,
                                 float x3, float y3, float x4, float y4,
                                 int handle, bool transFlag)
{
    Texture* tex = m_textureManager.GetTexture(handle);
    if (!tex) return;

    const auto& region = m_textureManager.GetRegion(handle);

    // 四隅を自由に指定: 左上, 右上, 右下, 左下
    SpriteVertex v0 = { { x1, y1 }, { region.u0, region.v0 }, m_drawColor }; // 左上
    SpriteVertex v1 = { { x2, y2 }, { region.u1, region.v0 }, m_drawColor }; // 右上
    SpriteVertex v2 = { { x4, y4 }, { region.u0, region.v1 }, m_drawColor }; // 左下
    SpriteVertex v3 = { { x3, y3 }, { region.u1, region.v1 }, m_drawColor }; // 右下

    AddQuad(v0, v1, v2, v3, handle);
}

void SpriteBatch::DrawRectModiGraph(float x1, float y1, float x2, float y2,
                                     float x3, float y3, float x4, float y4,
                                     float srcX, float srcY, float srcW, float srcH,
                                     int handle, bool transFlag)
{
    Texture* tex = m_textureManager.GetTexture(handle);
    if (!tex) return;

    float texW = static_cast<float>(tex->GetWidth());
    float texH = static_cast<float>(tex->GetHeight());
    if (texW <= 0.0f || texH <= 0.0f) return;

    float u0 = srcX / texW;
    float v0 = srcY / texH;
    float u1 = (srcX + srcW) / texW;
    float v1 = (srcY + srcH) / texH;

    // 左上・右上・右下・左下の順
    SpriteVertex v0 = { { x1, y1 }, { u0, v0 }, m_drawColor };
    SpriteVertex v1 = { { x2, y2 }, { u1, v0 }, m_drawColor };
    SpriteVertex v2 = { { x4, y4 }, { u0, v1 }, m_drawColor };
    SpriteVertex v3 = { { x3, y3 }, { u1, v1 }, m_drawColor };

    AddQuad(v0, v1, v2, v3, handle);
}

void SpriteBatch::SetBlendMode(BlendMode mode)
{
    if (m_blendMode != mode)
    {
        Flush();
        m_blendMode = mode;
    }
}

void SpriteBatch::SetDrawColor(float r, float g, float b, float a)
{
    m_drawColor = { r, g, b, a };
}

void SpriteBatch::End()
{
    Flush();

    m_vertexBuffer.Unmap(m_frameIndex);
    m_mappedVertices = nullptr;
    m_cmdList = nullptr;
}

void SpriteBatch::SetScreenSize(uint32_t width, uint32_t height)
{
    m_screenWidth  = width;
    m_screenHeight = height;
    if (!m_useCustomProjection)
        UpdateProjectionMatrix();
}

void SpriteBatch::SetProjectionMatrix(const XMMATRIX& matrix)
{
    m_projectionMatrix = matrix;
    m_useCustomProjection = true;
}

void SpriteBatch::ResetProjectionMatrix()
{
    m_useCustomProjection = false;
    UpdateProjectionMatrix();
}

} // namespace GX
