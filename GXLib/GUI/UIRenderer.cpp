#include "pch.h"
#include "GUI/UIRenderer.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Graphics/Rendering/FontManager.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"

namespace GX { namespace GUI {

// ============================================================================
// 初期化
// ============================================================================

bool UIRenderer::Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                            uint32_t screenWidth, uint32_t screenHeight,
                            SpriteBatch* spriteBatch, TextRenderer* textRenderer,
                            FontManager* fontManager)
{
    m_device = device;
    m_spriteBatch = spriteBatch;
    m_textRenderer = textRenderer;
    m_fontManager = fontManager;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    UpdateProjectionMatrix();

    if (!CreateRectPipeline(device))
        return false;

    // UIRect用の頂点バッファ（1矩形 = 4頂点、最大512矩形）
    static constexpr uint32_t k_MaxRects = 512;
    if (!m_rectVertexBuffer.Initialize(device, k_MaxRects * 4 * sizeof(UIRectVertex), sizeof(UIRectVertex)))
        return false;

    // UIRect用の定数バッファ（256バイトアラインメント × 最大512矩形）
    static constexpr uint32_t k_CBAligned = (sizeof(UIRectConstants) + 255) & ~255;
    if (!m_rectConstantBuffer.Initialize(device, k_MaxRects * k_CBAligned, k_CBAligned))
        return false;

    // 1矩形用のインデックスバッファ（6インデックス: 2三角形）
    uint16_t indices[] = { 0, 1, 2, 2, 1, 3 };
    if (!m_rectIndexBuffer.CreateIndexBuffer(device, indices, sizeof(indices)))
        return false;

    m_fullScreen = { 0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight) };

    return true;
}

// ============================================================================
// パイプライン作成
// ============================================================================

bool UIRenderer::CreateRectPipeline(ID3D12Device* device)
{
    if (!m_rectShader.Initialize())
        return false;

    auto vs = m_rectShader.CompileFromFile(L"Shaders/UIRect.hlsl", L"VSMain", L"vs_6_0");
    auto ps = m_rectShader.CompileFromFile(L"Shaders/UIRect.hlsl", L"PSMain", L"ps_6_0");
    if (!vs.valid || !ps.valid)
        return false;

    // ルートシグネチャ: CBV(b0)
    RootSignatureBuilder rsBuilder;
    m_rectRootSignature = rsBuilder
        .AddCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL)
        .AddStaticSampler(0)
        .Build(device);
    if (!m_rectRootSignature)
        return false;

    // 入力レイアウト
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // PSO: アルファブレンド、デプスなし、RGBA8出力
    PipelineStateBuilder psoBuilder;
    m_rectPSO = psoBuilder
        .SetRootSignature(m_rectRootSignature.Get())
        .SetVertexShader(vs.GetBytecode())
        .SetPixelShader(ps.GetBytecode())
        .SetInputLayout(inputLayout, _countof(inputLayout))
        .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .SetAlphaBlend()
        .Build(device);
    if (!m_rectPSO)
        return false;

    return true;
}

// ============================================================================
// 正射影行列
// ============================================================================

void UIRenderer::UpdateProjectionMatrix()
{
    float w = static_cast<float>(m_screenWidth);
    float h = static_cast<float>(m_screenHeight);

    XMMATRIX proj = XMMatrixOrthographicOffCenterLH(0.0f, w, h, 0.0f, 0.0f, 1.0f);
    XMStoreFloat4x4(&m_projectionMatrix, XMMatrixTranspose(proj));
}

// ============================================================================
// Begin / End
// ============================================================================

void UIRenderer::Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)
{
    m_cmdList = cmdList;
    m_frameIndex = frameIndex;
    m_rectDrawCount = 0;
    m_scissorStack.clear();
    m_spriteBatchActive = false;
}

void UIRenderer::End()
{
    // SpriteBatchが開始中なら終了
    if (m_spriteBatchActive)
    {
        m_spriteBatch->End();
        m_spriteBatchActive = false;
    }

    // ビューポートとシザーをフルスクリーンに復元
    // （SpriteBatchは自分でviewport/scissorをセットしないため、
    //   UIRect描画が変更した状態が後続の描画に影響しないようにする）
    if (m_cmdList)
    {
        D3D12_VIEWPORT viewport = {
            0.0f, 0.0f,
            static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight),
            0.0f, 1.0f
        };
        m_cmdList->RSSetViewports(1, &viewport);

        D3D12_RECT scissor = {
            0, 0,
            static_cast<LONG>(m_screenWidth),
            static_cast<LONG>(m_screenHeight)
        };
        m_cmdList->RSSetScissorRects(1, &scissor);
    }

    m_cmdList = nullptr;
}

// ============================================================================
// 矩形描画
// ============================================================================

void UIRenderer::DrawRect(const LayoutRect& rect, const Style& style, float opacity)
{
    // 元の矩形サイズと影の拡張量を分けて渡す
    DrawRectInternal(
        rect.x, rect.y, rect.width, rect.height,
        style.backgroundColor,
        style.cornerRadius,
        style.borderWidth,
        style.borderColor,
        style.shadowOffsetX,
        style.shadowOffsetY,
        style.shadowBlur,
        style.shadowColor.a,
        opacity
    );
}

void UIRenderer::DrawSolidRect(float x, float y, float w, float h, const StyleColor& color)
{
    DrawRectInternal(x, y, w, h, color, 0.0f, 0.0f, {}, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

void UIRenderer::DrawRectInternal(float x, float y, float w, float h,
                                   const StyleColor& fillColor, float cornerRadius,
                                   float borderWidth, const StyleColor& borderColor,
                                   float shadowOffsetX, float shadowOffsetY,
                                   float shadowBlur, float shadowAlpha,
                                   float opacity)
{
    if (w <= 0.0f || h <= 0.0f) return;
    if (fillColor.a <= 0.0f && borderWidth <= 0.0f && shadowAlpha <= 0.0f) return;

    // SpriteBatchが開始中ならいったん終了（PSO切り替えのため）
    if (m_spriteBatchActive)
    {
        m_spriteBatch->End();
        m_spriteBatchActive = false;
    }

    // 影の分だけ描画領域を拡張（SDFの rectSize は元のサイズを維持）
    float shadowExtend = shadowBlur + (std::max)(std::abs(shadowOffsetX), std::abs(shadowOffsetY));
    float drawX = x - shadowExtend;
    float drawY = y - shadowExtend;
    float drawW = w + shadowExtend * 2.0f;
    float drawH = h + shadowExtend * 2.0f;

    // 矩形インデックス（フレーム内でのオフセット計算用）
    uint32_t rectIdx = m_rectDrawCount++;
    static constexpr uint32_t k_VertexStride = 4 * sizeof(UIRectVertex);
    static constexpr uint32_t k_CBAligned = (sizeof(UIRectConstants) + 255) & ~255;

    // 定数バッファを書き込み（オフセット付き）
    auto* baseCB = static_cast<uint8_t*>(m_rectConstantBuffer.Map(m_frameIndex));
    if (!baseCB) return;
    auto* cb = reinterpret_cast<UIRectConstants*>(baseCB + rectIdx * k_CBAligned);

    cb->projection = m_projectionMatrix;
    cb->rectSize = { w, h };                 // 元の矩形サイズ（拡張前）
    cb->cornerRadius = cornerRadius;
    cb->borderWidth = borderWidth;
    cb->fillColor = { fillColor.r, fillColor.g, fillColor.b, fillColor.a };
    cb->borderColor = { borderColor.r, borderColor.g, borderColor.b, borderColor.a };
    cb->shadowOffset = { shadowOffsetX, shadowOffsetY };
    cb->shadowBlur = shadowBlur;
    cb->shadowAlpha = shadowAlpha;
    cb->opacity = opacity;

    m_rectConstantBuffer.Unmap(m_frameIndex);

    // 頂点データ: 描画領域は拡張、localUV は元の矩形基準（オフセット付き）
    auto* baseVerts = static_cast<uint8_t*>(m_rectVertexBuffer.Map(m_frameIndex));
    if (!baseVerts) return;
    auto* verts = reinterpret_cast<UIRectVertex*>(baseVerts + rectIdx * k_VertexStride);

    // 左上（影の領域を含む）
    verts[0].position = { drawX,         drawY };
    verts[0].localUV  = { -shadowExtend, -shadowExtend };
    // 右上
    verts[1].position = { drawX + drawW, drawY };
    verts[1].localUV  = { w + shadowExtend, -shadowExtend };
    // 左下
    verts[2].position = { drawX,         drawY + drawH };
    verts[2].localUV  = { -shadowExtend, h + shadowExtend };
    // 右下
    verts[3].position = { drawX + drawW, drawY + drawH };
    verts[3].localUV  = { w + shadowExtend, h + shadowExtend };

    m_rectVertexBuffer.Unmap(m_frameIndex);

    // パイプライン設定
    m_cmdList->SetGraphicsRootSignature(m_rectRootSignature.Get());
    m_cmdList->SetPipelineState(m_rectPSO.Get());

    // 定数バッファをバインド（オフセット付き）
    D3D12_GPU_VIRTUAL_ADDRESS cbAddr = m_rectConstantBuffer.GetGPUVirtualAddress(m_frameIndex)
                                       + rectIdx * k_CBAligned;
    m_cmdList->SetGraphicsRootConstantBufferView(0, cbAddr);

    // 頂点バッファ（オフセット付き）& インデックスバッファ
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = m_rectVertexBuffer.GetGPUVirtualAddress(m_frameIndex)
                         + rectIdx * k_VertexStride;
    vbv.SizeInBytes    = k_VertexStride;
    vbv.StrideInBytes  = sizeof(UIRectVertex);
    m_cmdList->IASetVertexBuffers(0, 1, &vbv);

    auto ibv = m_rectIndexBuffer.GetIndexBufferView();
    m_cmdList->IASetIndexBuffer(&ibv);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ビューポートとシザー
    D3D12_VIEWPORT viewport = {
        0.0f, 0.0f,
        static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight),
        0.0f, 1.0f
    };
    m_cmdList->RSSetViewports(1, &viewport);
    ApplyScissor();

    // 描画
    m_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

// ============================================================================
// テキスト描画
// ============================================================================

void UIRenderer::DrawText(float x, float y, int fontHandle, const std::wstring& text,
                           const StyleColor& color, float opacity)
{
    if (text.empty() || !m_textRenderer) return;

    // SpriteBatchが開始していなければ開始
    if (!m_spriteBatchActive)
    {
        m_spriteBatch->Begin(m_cmdList, m_frameIndex);
        m_spriteBatchActive = true;
    }

    // ARGB色に変換
    uint8_t a = static_cast<uint8_t>((std::min)(color.a * opacity, 1.0f) * 255.0f);
    uint8_t r = static_cast<uint8_t>((std::min)(color.r, 1.0f) * 255.0f);
    uint8_t g = static_cast<uint8_t>((std::min)(color.g, 1.0f) * 255.0f);
    uint8_t b = static_cast<uint8_t>((std::min)(color.b, 1.0f) * 255.0f);
    uint32_t argb = (a << 24) | (r << 16) | (g << 8) | b;

    m_textRenderer->DrawString(fontHandle, x, y, text, argb);
}

int UIRenderer::GetTextWidth(int fontHandle, const std::wstring& text)
{
    if (!m_textRenderer) return 0;
    return m_textRenderer->GetStringWidth(fontHandle, text);
}

int UIRenderer::GetLineHeight(int fontHandle)
{
    if (!m_fontManager) return 16;
    return m_fontManager->GetLineHeight(fontHandle);
}

// ============================================================================
// テクスチャ描画
// ============================================================================

void UIRenderer::DrawImage(float x, float y, float w, float h, int textureHandle, float opacity)
{
    if (!m_spriteBatch) return;

    // SpriteBatchが開始していなければ開始
    if (!m_spriteBatchActive)
    {
        m_spriteBatch->Begin(m_cmdList, m_frameIndex);
        m_spriteBatchActive = true;
    }

    m_spriteBatch->SetDrawColor(1.0f, 1.0f, 1.0f, opacity);
    m_spriteBatch->DrawExtendGraph(x, y, x + w, y + h, textureHandle);
    m_spriteBatch->SetDrawColor(1.0f, 1.0f, 1.0f, 1.0f);
}

// ============================================================================
// ScissorStack
// ============================================================================

void UIRenderer::PushScissor(const LayoutRect& rect)
{
    ScissorRect sr = {
        rect.x, rect.y,
        rect.x + rect.width, rect.y + rect.height
    };

    if (!m_scissorStack.empty())
    {
        sr = sr.Intersect(m_scissorStack.back());
    }
    else
    {
        sr = sr.Intersect(m_fullScreen);
    }

    m_scissorStack.push_back(sr);

    // SpriteBatchが開始中なら終了してシザー適用
    if (m_spriteBatchActive)
    {
        m_spriteBatch->End();
        m_spriteBatchActive = false;
    }
    ApplyScissor();
}

void UIRenderer::PopScissor()
{
    if (!m_scissorStack.empty())
    {
        m_scissorStack.pop_back();
    }

    if (m_spriteBatchActive)
    {
        m_spriteBatch->End();
        m_spriteBatchActive = false;
    }
    ApplyScissor();
}

void UIRenderer::ApplyScissor()
{
    ScissorRect sr = m_fullScreen;
    if (!m_scissorStack.empty())
    {
        sr = m_scissorStack.back();
    }

    D3D12_RECT d3dRect;
    d3dRect.left   = static_cast<LONG>((std::max)(sr.left, 0.0f));
    d3dRect.top    = static_cast<LONG>((std::max)(sr.top, 0.0f));
    d3dRect.right  = static_cast<LONG>((std::max)(sr.right, 0.0f));
    d3dRect.bottom = static_cast<LONG>((std::max)(sr.bottom, 0.0f));
    m_cmdList->RSSetScissorRects(1, &d3dRect);
}

// ============================================================================
// リサイズ
// ============================================================================

void UIRenderer::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    m_fullScreen = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
    UpdateProjectionMatrix();
}

}} // namespace GX::GUI
