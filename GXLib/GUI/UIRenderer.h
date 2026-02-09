#pragma once
/// @file UIRenderer.h
/// @brief GUI 描画統合クラス
///
/// UIRectBatch（SDF角丸矩形）+ SpriteBatch（テクスチャ）+ TextRenderer（テキスト）
/// + ScissorStack（クリッピング）を統合する描画クラス。

#include "pch.h"
#include "GUI/Style.h"
#include "GUI/Widget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{
class SpriteBatch;
class TextRenderer;
class FontManager;
}

namespace GX { namespace GUI {

// ============================================================================
// ScissorStack — ネストしたクリッピング領域の管理
// ============================================================================

/// @brief スクリーン座標のクリッピング領域
struct ScissorRect
{
    float left = 0, top = 0, right = 0, bottom = 0;

    /// 2つの矩形の交差を計算
    ScissorRect Intersect(const ScissorRect& other) const
    {
        return {
            (std::max)(left,  other.left),
            (std::max)(top,   other.top),
            (std::min)(right, other.right),
            (std::min)(bottom, other.bottom)
        };
    }
};

// ============================================================================
// UIRectInstance — バッチ化された矩形データ
// ============================================================================

/// @brief UIRectBatch用頂点データ
struct UIRectVertex
{
    XMFLOAT2 position;   // スクリーン座標
    XMFLOAT2 localUV;    // 矩形内のローカル座標 (0-rectSize)
};

/// @brief UIRect定数バッファ
struct UIRectConstants
{
    XMFLOAT4X4 projection;         // 正射影行列 (64)
    XMFLOAT2   rectSize;           // 矩形サイズ (8)
    float      cornerRadius;       // 角丸半径 (4)
    float      borderWidth;        // 枠線幅 (4)
    XMFLOAT4   fillColor;          // 塗り色 (16)
    XMFLOAT4   borderColor;        // 枠色 (16)
    XMFLOAT2   shadowOffset;       // 影オフセット (8)
    float      shadowBlur;         // 影ぼかし (4)
    float      shadowAlpha;        // 影透明度 (4)
    float      opacity;            // 全体透明度 (4)
    float      _pad[3];            // パディング (12)
};                                 // 合計: 144 bytes

// ============================================================================
// UIRenderer
// ============================================================================

/// @brief GUI描画統合クラス
class UIRenderer
{
public:
    UIRenderer() = default;
    ~UIRenderer() = default;

    /// 初期化
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    uint32_t screenWidth, uint32_t screenHeight,
                    SpriteBatch* spriteBatch, TextRenderer* textRenderer,
                    FontManager* fontManager);

    /// フレーム描画開始
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// フレーム描画終了
    void End();

    // --- 矩形描画 ---

    /// スタイル付き矩形を描画（背景 + 枠線 + 影 + 角丸）
    void DrawRect(const LayoutRect& rect, const Style& style, float opacity = 1.0f);

    /// 単色矩形を描画（角丸なし、高速パス）
    void DrawSolidRect(float x, float y, float w, float h, const StyleColor& color);

    // --- テキスト描画 ---

    /// テキストを描画（フォントハンドル指定）
    void DrawText(float x, float y, int fontHandle, const std::wstring& text,
                  const StyleColor& color, float opacity = 1.0f);

    /// テキスト幅を計算
    int GetTextWidth(int fontHandle, const std::wstring& text);

    /// フォント行の高さを取得
    int GetLineHeight(int fontHandle);

    // --- テクスチャ描画 ---

    /// テクスチャを描画
    void DrawImage(float x, float y, float w, float h, int textureHandle, float opacity = 1.0f);

    // --- クリッピング ---

    /// クリッピング矩形をプッシュ（ネスト対応）
    void PushScissor(const LayoutRect& rect);

    /// クリッピング矩形をポップ
    void PopScissor();

    /// スクリーンサイズ更新
    void OnResize(uint32_t width, uint32_t height);

    /// フォントマネージャーを取得
    FontManager* GetFontManager() const { return m_fontManager; }

    /// SpriteBatchを取得
    SpriteBatch* GetSpriteBatch() const { return m_spriteBatch; }

    /// TextRendererを取得
    TextRenderer* GetTextRenderer() const { return m_textRenderer; }

private:
    /// UIRectBatch パイプライン作成
    bool CreateRectPipeline(ID3D12Device* device);

    /// 正射影行列を更新
    void UpdateProjectionMatrix();

    /// 1矩形を描画（内部）
    void DrawRectInternal(float x, float y, float w, float h,
                          const StyleColor& fillColor, float cornerRadius,
                          float borderWidth, const StyleColor& borderColor,
                          float shadowOffsetX, float shadowOffsetY,
                          float shadowBlur, float shadowAlpha,
                          float opacity);

    /// 現在のシザー矩形をGPUに適用
    void ApplyScissor();

    // デバイス
    ID3D12Device*              m_device = nullptr;
    ID3D12GraphicsCommandList* m_cmdList = nullptr;
    uint32_t                   m_frameIndex = 0;

    // 既存レンダラー参照
    SpriteBatch*  m_spriteBatch = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    FontManager*  m_fontManager = nullptr;

    // スクリーン設定
    uint32_t m_screenWidth  = 1280;
    uint32_t m_screenHeight = 720;
    XMFLOAT4X4 m_projectionMatrix;

    // UIRectBatch
    Shader                       m_rectShader;
    ComPtr<ID3D12RootSignature>  m_rectRootSignature;
    ComPtr<ID3D12PipelineState>  m_rectPSO;
    DynamicBuffer                m_rectVertexBuffer;
    DynamicBuffer                m_rectConstantBuffer;
    Buffer                       m_rectIndexBuffer;

    // 矩形描画カウンタ（フレーム内のオフセット計算用）
    uint32_t m_rectDrawCount = 0;

    // ScissorStack
    std::vector<ScissorRect> m_scissorStack;
    ScissorRect m_fullScreen;
    bool m_spriteBatchActive = false;
};

}} // namespace GX::GUI
