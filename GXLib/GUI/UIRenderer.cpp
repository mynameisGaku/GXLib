#include "pch.h"
#include "GUI/UIRenderer.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Graphics/Rendering/FontManager.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"

namespace GX { namespace GUI {

static bool IsIdentityTransform(const Transform2D& t)
{
    return t.a == 1.0f && t.b == 0.0f &&
           t.c == 0.0f && t.d == 1.0f &&
           t.tx == 0.0f && t.ty == 0.0f;
}

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

    UpdateGuiMetrics();
    UpdateProjectionMatrix();

    if (!CreateRectPipeline(device))
        return false;

    // ホットリロード用PSO Rebuilder登録
    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/UIRect.hlsl",
        [this](ID3D12Device* dev) { return CreateRectPipeline(dev); }
    );

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

    // fullScreen はデザイン座標（scissor stack用）
    m_fullScreen = { 0.0f, 0.0f,
                     static_cast<float>(m_designWidth > 0 ? m_designWidth : screenWidth),
                     static_cast<float>(m_designHeight > 0 ? m_designHeight : screenHeight) };

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
// 正射影行列（常にスクリーン解像度ベース）
// ============================================================================

void UIRenderer::UpdateProjectionMatrix()
{
    float w = static_cast<float>(m_screenWidth);
    float h = static_cast<float>(m_screenHeight);

    XMMATRIX proj = XMMatrixOrthographicOffCenterLH(0.0f, w, h, 0.0f, 0.0f, 1.0f);
    XMStoreFloat4x4(&m_projectionMatrix, XMMatrixTranspose(proj));
}

// ============================================================================
// GUI スケーリング
// ============================================================================

void UIRenderer::SetDesignResolution(uint32_t designWidth, uint32_t designHeight)
{
    m_designWidth = designWidth;
    m_designHeight = designHeight;
    UpdateGuiMetrics();

    // scissor の fullscreen をデザイン解像度に合わせる
    m_fullScreen = { 0.0f, 0.0f,
                     static_cast<float>(m_designWidth > 0 ? m_designWidth : m_screenWidth),
                     static_cast<float>(m_designHeight > 0 ? m_designHeight : m_screenHeight) };
}

void UIRenderer::UpdateGuiMetrics()
{
    if (m_designWidth > 0 && m_designHeight > 0)
    {
        float sx = static_cast<float>(m_screenWidth) / static_cast<float>(m_designWidth);
        float sy = static_cast<float>(m_screenHeight) / static_cast<float>(m_designHeight);
        m_guiScale = (std::min)(sx, sy);
        // レターボックスオフセット（中央揃え）
        m_guiOffsetX = (static_cast<float>(m_screenWidth) - m_designWidth * m_guiScale) * 0.5f;
        m_guiOffsetY = (static_cast<float>(m_screenHeight) - m_designHeight * m_guiScale) * 0.5f;
    }
    else
    {
        m_guiScale = 1.0f;
        m_guiOffsetX = 0.0f;
        m_guiOffsetY = 0.0f;
    }
}

void UIRenderer::ApplyGuiViewport()
{
    D3D12_VIEWPORT viewport = {
        0.0f, 0.0f,
        static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight),
        0.0f, 1.0f
    };
    m_cmdList->RSSetViewports(1, &viewport);
}

// ============================================================================
// Begin / End（開始/終了）
// ============================================================================

void UIRenderer::Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)
{
    m_cmdList = cmdList;
    m_frameIndex = frameIndex;
    m_rectDrawCount = 0;
    m_scissorStack.clear();
    m_spriteBatchActive = false;
    m_deferredDraws.clear();
    m_transformStack.clear();
    m_transformStack.push_back(Transform2D::Identity());
    m_opacityStack.clear();
    m_opacityStack.push_back(1.0f);

    // SpriteBatch にデザイン解像度の射影行列を設定
    // ※ SpriteBatch CB は upload heap のため、同フレーム内で後から上書きされると
    //   全ドローコールに影響する。そのためカスタム射影は使わず、
    //   座標変換（design→screen）を UIRenderer 側で行う。

    // フルスクリーンビューポート + デフォルトシザー
    ApplyGuiViewport();
    ApplyScissor();
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

void UIRenderer::DrawRect(const LayoutRect& rect, const Style& style, float opacity, const UIRectEffect* effect)
{
    float finalOpacity = opacity * GetOpacity();
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
        finalOpacity,
        nullptr,
        { 0.0f, 1.0f },
        effect
    );
}

void UIRenderer::DrawSolidRect(float x, float y, float w, float h, const StyleColor& color)
{
    float finalOpacity = GetOpacity();
    DrawRectInternal(x, y, w, h, color, 0.0f, 0.0f, {}, 0.0f, 0.0f, 0.0f, 0.0f,
                     finalOpacity, nullptr, { 0.0f, 1.0f }, nullptr);
}

void UIRenderer::DrawGradientRect(float x, float y, float w, float h,
                                  const StyleColor& startColor, const StyleColor& endColor,
                                  float dirX, float dirY, float cornerRadius, float opacity)
{
    XMFLOAT2 dir = { dirX, dirY };
    DrawRectInternal(x, y, w, h, startColor, cornerRadius,
                     0.0f, {}, 0.0f, 0.0f, 0.0f, 0.0f,
                     opacity * GetOpacity(), &endColor, dir, nullptr);
}

void UIRenderer::DrawRectInternal(float x, float y, float w, float h,
                                   const StyleColor& fillColor, float cornerRadius,
                                   float borderWidth, const StyleColor& borderColor,
                                   float shadowOffsetX, float shadowOffsetY,
                                   float shadowBlur, float shadowAlpha,
                                   float opacity,
                                   const StyleColor* gradientColor,
                                   const XMFLOAT2& gradientDir,
                                   const UIRectEffect* effect)
{
    if (w <= 0.0f || h <= 0.0f) return;
    if (opacity <= 0.0f) return;
    if (fillColor.a <= 0.0f && borderWidth <= 0.0f && shadowAlpha <= 0.0f) return;

    // デザイン座標 → スクリーン座標に変換
    x = x * m_guiScale + m_guiOffsetX;
    y = y * m_guiScale + m_guiOffsetY;
    w *= m_guiScale;
    h *= m_guiScale;
    cornerRadius *= m_guiScale;
    borderWidth  *= m_guiScale;
    shadowOffsetX *= m_guiScale;
    shadowOffsetY *= m_guiScale;
    shadowBlur    *= m_guiScale;

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
    cb->rectSize = { w, h };                 // スケーリング後のスクリーン座標サイズ
    cb->cornerRadius = cornerRadius;
    cb->borderWidth = borderWidth;
    cb->fillColor = { fillColor.r, fillColor.g, fillColor.b, fillColor.a };
    cb->borderColor = { borderColor.r, borderColor.g, borderColor.b, borderColor.a };
    cb->shadowOffset = { shadowOffsetX, shadowOffsetY };
    cb->shadowBlur = shadowBlur;
    cb->shadowAlpha = shadowAlpha;
    cb->opacity = opacity;
    if (gradientColor)
    {
        cb->gradientColor = { gradientColor->r, gradientColor->g, gradientColor->b, gradientColor->a };
        cb->gradientEnabled = 1.0f;
        cb->gradientDir = gradientDir;
    }
    else
    {
        cb->gradientColor = { fillColor.r, fillColor.g, fillColor.b, fillColor.a };
        cb->gradientEnabled = 0.0f;
        cb->gradientDir = { 0.0f, 1.0f };
    }
    cb->_pad2 = 0.0f;

    if (effect && effect->type != UIRectEffectType::None)
    {
        cb->effectCenter = { effect->centerX, effect->centerY };
        cb->effectTime = effect->time;
        cb->effectDuration = effect->duration;
        cb->effectStrength = effect->strength;
        cb->effectWidth = effect->width;
        cb->effectType = static_cast<float>(effect->type);
    }
    else
    {
        cb->effectCenter = { 0.5f, 0.5f };
        cb->effectTime = 0.0f;
        cb->effectDuration = 0.0f;
        cb->effectStrength = 0.0f;
        cb->effectWidth = 0.0f;
        cb->effectType = 0.0f;
    }
    cb->_pad3 = 0.0f;

    m_rectConstantBuffer.Unmap(m_frameIndex);

    // 頂点データ: 描画領域は拡張、localUV は元の矩形基準（オフセット付き）
    auto* baseVerts = static_cast<uint8_t*>(m_rectVertexBuffer.Map(m_frameIndex));
    if (!baseVerts) return;
    auto* verts = reinterpret_cast<UIRectVertex*>(baseVerts + rectIdx * k_VertexStride);

    // 左上（影の領域を含む）
    XMFLOAT2 p0 = { drawX,         drawY };
    XMFLOAT2 p1 = { drawX + drawW, drawY };
    XMFLOAT2 p2 = { drawX,         drawY + drawH };
    XMFLOAT2 p3 = { drawX + drawW, drawY + drawH };

    const Transform2D& t = GetTransform();
    if (!IsIdentityTransform(t))
    {
        p0 = TransformPoint(t, p0.x, p0.y);
        p1 = TransformPoint(t, p1.x, p1.y);
        p2 = TransformPoint(t, p2.x, p2.y);
        p3 = TransformPoint(t, p3.x, p3.y);
    }

    verts[0].position = p0;
    verts[0].localUV  = { -shadowExtend, -shadowExtend };
    // 右上
    verts[1].position = p1;
    verts[1].localUV  = { w + shadowExtend, -shadowExtend };
    // 左下
    verts[2].position = p2;
    verts[2].localUV  = { -shadowExtend, h + shadowExtend };
    // 右下
    verts[3].position = p3;
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
    ApplyGuiViewport();
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
    if (text.empty() || !m_spriteBatch || !m_fontManager) return;

    int atlasHandle = m_fontManager->GetAtlasTextureHandle(fontHandle);
    if (atlasHandle < 0) return;

    float finalOpacity = opacity * GetOpacity();

    // SpriteBatchが開始していなければ開始
    if (!m_spriteBatchActive)
    {
        m_spriteBatch->Begin(m_cmdList, m_frameIndex);
        m_spriteBatchActive = true;
    }

    // 描画色を設定
    float fa = (std::min)(color.a * finalOpacity, 1.0f);
    float fr = (std::min)(color.r, 1.0f);
    float fg = (std::min)(color.g, 1.0f);
    float fb = (std::min)(color.b, 1.0f);
    m_spriteBatch->SetDrawColor(fr, fg, fb, fa);

    float scale = m_guiScale;
    float cursorX = x;  // デザイン座標
    const Transform2D& t = GetTransform();
    bool useTransform = !IsIdentityTransform(t);

    for (wchar_t ch : text)
    {
        if (ch == L'\n')
        {
            cursorX = x;
            y += static_cast<float>(m_fontManager->GetLineHeight(fontHandle));
            continue;
        }

        const GlyphInfo* glyph = m_fontManager->GetGlyphInfo(fontHandle, ch);
        if (!glyph) continue;

        if (ch == L' ')
        {
            cursorX += glyph->advance;
            continue;
        }

        // デザイン座標での描画位置
        float drawX = cursorX + glyph->offsetX;
        float drawY = y + glyph->offsetY;

        // デザイン座標 → スクリーン座標に変換（位置とサイズ両方）
        float sx = drawX * scale + m_guiOffsetX;
        float sy = drawY * scale + m_guiOffsetY;
        float sw = glyph->width * scale;
        float sh = glyph->height * scale;

        // アトラス上のソース矩形
        int srcX = static_cast<int>(glyph->u0 * FontManager::k_AtlasSize);
        int srcY = static_cast<int>(glyph->v0 * FontManager::k_AtlasSize);

        if (useTransform)
        {
            XMFLOAT2 p0 = TransformPoint(t, sx, sy);
            XMFLOAT2 p1 = TransformPoint(t, sx + sw, sy);
            XMFLOAT2 p2 = TransformPoint(t, sx + sw, sy + sh);
            XMFLOAT2 p3 = TransformPoint(t, sx, sy + sh);
            m_spriteBatch->DrawRectModiGraph(p0.x, p0.y, p1.x, p1.y,
                                             p2.x, p2.y, p3.x, p3.y,
                                             static_cast<float>(srcX), static_cast<float>(srcY),
                                             static_cast<float>(glyph->width), static_cast<float>(glyph->height),
                                             atlasHandle, true);
        }
        else
        {
            m_spriteBatch->DrawRectExtendGraph(sx, sy, sw, sh,
                                               srcX, srcY, glyph->width, glyph->height,
                                               atlasHandle, true);
        }

        cursorX += glyph->advance;
    }

    // 描画色をリセット
    m_spriteBatch->SetDrawColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void UIRenderer::DrawImageUV(float x, float y, float w, float h, int textureHandle,
                             float u0, float v0, float u1, float v1,
                             float opacity)
{
    if (!m_spriteBatch) return;

    if (!m_spriteBatchActive)
    {
        m_spriteBatch->Begin(m_cmdList, m_frameIndex);
        m_spriteBatchActive = true;
    }

    float finalOpacity = opacity * GetOpacity();

    float sx = x * m_guiScale + m_guiOffsetX;
    float sy = y * m_guiScale + m_guiOffsetY;
    float sw = w * m_guiScale;
    float sh = h * m_guiScale;

    XMFLOAT2 p0 = { sx,      sy };
    XMFLOAT2 p1 = { sx + sw, sy };
    XMFLOAT2 p2 = { sx + sw, sy + sh };
    XMFLOAT2 p3 = { sx,      sy + sh };

    const Transform2D& t = GetTransform();
    if (!IsIdentityTransform(t))
    {
        p0 = TransformPoint(t, p0.x, p0.y);
        p1 = TransformPoint(t, p1.x, p1.y);
        p2 = TransformPoint(t, p2.x, p2.y);
        p3 = TransformPoint(t, p3.x, p3.y);
    }

    TextureManager& tm = m_spriteBatch->GetTextureManager();
    Texture* tex = tm.GetTexture(textureHandle);
    if (!tex) return;

    float texW = static_cast<float>(tex->GetWidth());
    float texH = static_cast<float>(tex->GetHeight());
    float srcX = u0 * texW;
    float srcY = v0 * texH;
    float srcW = (u1 - u0) * texW;
    float srcH = (v1 - v0) * texH;

    m_spriteBatch->SetDrawColor(1.0f, 1.0f, 1.0f, finalOpacity);
    m_spriteBatch->DrawRectModiGraph(p0.x, p0.y, p1.x, p1.y,
                                     p2.x, p2.y, p3.x, p3.y,
                                     srcX, srcY, srcW, srcH,
                                     textureHandle, true);
    m_spriteBatch->SetDrawColor(1.0f, 1.0f, 1.0f, 1.0f);
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

float UIRenderer::GetFontCapOffset(int fontHandle)
{
    if (!m_fontManager) return 0.0f;
    return m_fontManager->GetCapOffset(fontHandle);
}

// ============================================================================
// テクスチャ描画
// ============================================================================

void UIRenderer::DrawImage(float x, float y, float w, float h, int textureHandle, float opacity)
{
    if (!m_spriteBatch) return;

    DrawImageUV(x, y, w, h, textureHandle, 0.0f, 0.0f, 1.0f, 1.0f, opacity);
    return;

    // SpriteBatchが開始していなければ開始
    if (!m_spriteBatchActive)
    {
        m_spriteBatch->Begin(m_cmdList, m_frameIndex);
        m_spriteBatchActive = true;
    }

    // デザイン座標 → スクリーン座標に変換
    float sx = x * m_guiScale + m_guiOffsetX;
    float sy = y * m_guiScale + m_guiOffsetY;
    float sw = w * m_guiScale;
    float sh = h * m_guiScale;

    m_spriteBatch->SetDrawColor(1.0f, 1.0f, 1.0f, opacity);
    m_spriteBatch->DrawExtendGraph(sx, sy, sx + sw, sy + sh, textureHandle);
    m_spriteBatch->SetDrawColor(1.0f, 1.0f, 1.0f, 1.0f);
}

// ============================================================================
// ScissorStack（デザイン座標で管理、ApplyScissorでスクリーン座標に変換）
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

void UIRenderer::PushTransform(const Transform2D& local)
{
    if (m_transformStack.empty())
        m_transformStack.push_back(Transform2D::Identity());

    const Transform2D& parent = m_transformStack.back();
    m_transformStack.push_back(Multiply(parent, local));
}

void UIRenderer::PopTransform()
{
    if (m_transformStack.size() > 1)
        m_transformStack.pop_back();
}

void UIRenderer::PushOpacity(float opacity)
{
    if (m_opacityStack.empty())
        m_opacityStack.push_back(1.0f);

    float parent = m_opacityStack.back();
    m_opacityStack.push_back(parent * opacity);
}

void UIRenderer::PopOpacity()
{
    if (m_opacityStack.size() > 1)
        m_opacityStack.pop_back();
}

void UIRenderer::ApplyScissor()
{
    ScissorRect sr = m_fullScreen;
    if (!m_scissorStack.empty())
    {
        sr = m_scissorStack.back();
    }

    // デザイン座標 → スクリーン座標に変換
    D3D12_RECT d3dRect;
    d3dRect.left   = static_cast<LONG>((std::max)(sr.left * m_guiScale + m_guiOffsetX, 0.0f));
    d3dRect.top    = static_cast<LONG>((std::max)(sr.top * m_guiScale + m_guiOffsetY, 0.0f));
    d3dRect.right  = static_cast<LONG>((std::max)(sr.right * m_guiScale + m_guiOffsetX, 0.0f));
    d3dRect.bottom = static_cast<LONG>((std::max)(sr.bottom * m_guiScale + m_guiOffsetY, 0.0f));

    // inside-out rect を防止 (Intersect で画面外になった場合)
    if (d3dRect.right < d3dRect.left) d3dRect.right = d3dRect.left;
    if (d3dRect.bottom < d3dRect.top) d3dRect.bottom = d3dRect.top;

    m_cmdList->RSSetScissorRects(1, &d3dRect);
}

// ============================================================================
// 遅延描画（オーバーレイ）
// ============================================================================

void UIRenderer::DeferDraw(std::function<void()> fn)
{
    m_deferredDraws.push_back(std::move(fn));
}

void UIRenderer::FlushDeferredDraws()
{
    for (auto& fn : m_deferredDraws)
        fn();
    m_deferredDraws.clear();
}

// ============================================================================
// リサイズ
// ============================================================================

void UIRenderer::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    UpdateGuiMetrics();
    UpdateProjectionMatrix();

    m_fullScreen = { 0.0f, 0.0f,
                     static_cast<float>(m_designWidth > 0 ? m_designWidth : width),
                     static_cast<float>(m_designHeight > 0 ? m_designHeight : height) };
}

}} // namespace GX::GUI
