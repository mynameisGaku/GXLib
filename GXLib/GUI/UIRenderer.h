#pragma once
/// @file UIRenderer.h
/// @brief GUI描画エンジン
///
/// ウィジェットの描画を担当するクラス。角丸矩形(SDF)、テキスト、画像の描画に加え、
/// シザー矩形、Transform/Opacityスタック、遅延描画（ドロップダウンのポップアップ等）を提供する。
/// 内部ではSpriteBatch、TextRenderer、専用のUIRect PSOを使い分ける。

#include "pch_graphics.h"
#include "GUI/Style.h"
#include "GUI/Widget.h"
#include "Math/Transform2D.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace gx
{
/// @addtogroup grp_gui
/// @{
class SpriteBatch;
class TextRenderer;
class FontManager;
}

namespace gx { namespace GUI {

// ---------------------------------------------------------------------------
// Scissor（描画をクリップする矩形。デザイン座標で管理する）
// ---------------------------------------------------------------------------

/// @brief シザー矩形（描画クリッピング用）
struct ScissorRect
{
    float left = 0, top = 0, right = 0, bottom = 0; ///< 矩形の各辺（デザイン座標）

    /// @brief 他のシザー矩形との交差領域を返す
    /// @param other 交差対象の矩形
    /// @return 交差した結果の矩形
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

// ---------------------------------------------------------------------------
// UIRect Batch（SDF角丸矩形の描画に使う頂点・定数バッファ定義）
// ---------------------------------------------------------------------------

/// @brief UIRect用の頂点データ
struct UIRectVertex
{
    XMFLOAT2 position;  ///< スクリーン座標
    XMFLOAT2 localUV;   ///< ローカルUV（SDF計算用、0~1）
};

/// @brief UIRect用のGPU定数バッファ（b0にバインドされる）
/// 角丸、ボーダー、影、グラデーション、エフェクト情報を全て1回のドローコールで処理する。
struct UIRectConstants
{
    XMFLOAT4X4 projection;         ///< 射影行列
    XMFLOAT2   rectSize;           ///< 矩形サイズ（ピクセル）
    float      cornerRadius;       ///< 角丸半径（ピクセル）
    float      borderWidth;        ///< ボーダー幅（ピクセル）
    XMFLOAT4   fillColor;          ///< 塗りつぶし色（RGBA）
    XMFLOAT4   borderColor;        ///< ボーダー色（RGBA）
    XMFLOAT2   shadowOffset;       ///< 影オフセット（ピクセル）
    float      shadowBlur;         ///< 影ぼかし半径（ピクセル）
    float      shadowAlpha;        ///< 影の不透明度
    float      opacity;            ///< 全体の不透明度
    float      _pad[3];            ///< パディング
    XMFLOAT4   gradientColor;      ///< グラデーション終了色（RGBA）
    XMFLOAT2   gradientDir;        ///< グラデーション方向ベクトル
    float      gradientEnabled;    ///< グラデーション有効フラグ（0/1）
    float      _pad2;              ///< パディング
    XMFLOAT2   effectCenter;       ///< エフェクト中心（UV, 0~1）
    float      effectTime;         ///< エフェクト経過時間（秒）
    float      effectDuration;     ///< エフェクト持続時間（秒）
    float      effectStrength;     ///< エフェクト強度（0~1）
    float      effectWidth;        ///< エフェクト幅（0~1）
    float      effectType;         ///< エフェクト種別（0=None, 1=Ripple）
    float      _pad3;              ///< パディング
};                                 // 208 bytes

/// @brief UIRect エフェクトの種類
enum class UIRectEffectType
{
    None = 0,    ///< エフェクトなし
    Ripple = 1   ///< リップル（波紋）エフェクト
};

/// @brief UIRect エフェクト情報
struct UIRectEffect
{
    UIRectEffectType type = UIRectEffectType::None; ///< エフェクト種別
    float centerX = 0.5f;   ///< エフェクト中心X (0~1)
    float centerY = 0.5f;   ///< エフェクト中心Y (0~1)
    float time = 0.0f;      ///< 経過時間（秒）
    float duration = 0.8f;  ///< 持続時間（秒）
    float strength = 0.4f;  ///< 強度 (0~1)
    float width = 0.08f;    ///< 幅 (0~1)
};

// ---------------------------------------------------------------------------
// UIRenderer
// ---------------------------------------------------------------------------

/// @brief GUI描画エンジン
/// SpriteBatch/TextRendererとSDF角丸矩形パイプラインを使い分けて描画する。
/// デザイン解像度によるスケーリング、シザー矩形スタック、Transform/Opacityスタックを管理する。
class UIRenderer
{
public:
    UIRenderer() = default;
    ~UIRenderer() = default;

    /// @brief 初期化。UIRect用のPSO、頂点/定数/インデックスバッファを作成する
    /// @param device D3D12デバイス
    /// @param cmdQueue コマンドキュー
    /// @param screenWidth スクリーン幅
    /// @param screenHeight スクリーン高さ
    /// @param spriteBatch テキスト・画像描画に使うSpriteBatch
    /// @param textRenderer テキスト描画用
    /// @param fontManager フォント管理用
    /// @return 成功なら true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    uint32_t screenWidth, uint32_t screenHeight,
                    SpriteBatch* spriteBatch, TextRenderer* textRenderer,
                    FontManager* fontManager);

    /// @brief フレームの描画開始。スタックを初期化しビューポートを設定する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファリングのフレームインデックス
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// @brief フレームの描画終了。SpriteBatchの終了とビューポート復元を行う
    void End();

    // --- Rect ---

    /// @brief スタイル情報から角丸矩形を描画する（背景、ボーダー、影を一括処理）
    /// @param rect 描画位置・サイズ（デザイン座標）
    /// @param style 描画に使うスタイル
    /// @param opacity 追加の不透明度
    /// @param effect リップル等のエフェクト情報（不要なら nullptr）
    void DrawRect(const LayoutRect& rect, const Style& style,
                  float opacity = 1.0f,
                  const UIRectEffect* effect = nullptr);

    /// @brief 単色の矩形を描画する
    /// @param x X座標（デザイン座標）
    /// @param y Y座標（デザイン座標）
    /// @param w 幅
    /// @param h 高さ
    /// @param color 塗りつぶし色
    void DrawSolidRect(float x, float y, float w, float h, const StyleColor& color);

    /// @brief グラデーション矩形を描画する
    /// @param x X座標（デザイン座標）
    /// @param y Y座標（デザイン座標）
    /// @param w 幅
    /// @param h 高さ
    /// @param startColor グラデーション開始色
    /// @param endColor グラデーション終了色
    /// @param dirX グラデーション方向X（0=水平なし）
    /// @param dirY グラデーション方向Y（1=上→下）
    /// @param cornerRadius 角丸半径
    /// @param opacity 不透明度
    void DrawGradientRect(float x, float y, float w, float h,
                          const StyleColor& startColor, const StyleColor& endColor,
                          float dirX = 0.0f, float dirY = 1.0f,
                          float cornerRadius = 0.0f,
                          float opacity = 1.0f);

    // --- Text ---

    /// @brief テキストを描画する
    /// @param x X座標（デザイン座標）
    /// @param y Y座標（デザイン座標）
    /// @param fontHandle フォントハンドル（FontManagerで取得）
    /// @param text 描画するテキスト
    /// @param color テキスト色
    /// @param opacity 不透明度
    void DrawText(float x, float y, int fontHandle, const std::wstring& text,
                  const StyleColor& color, float opacity = 1.0f);

    /// @brief テキストの描画幅を取得する
    /// @param fontHandle フォントハンドル
    /// @param text 計測するテキスト
    /// @return テキスト幅（ピクセル）
    int GetTextWidth(int fontHandle, const std::wstring& text);

    /// @brief フォントの行高さを取得する
    /// @param fontHandle フォントハンドル
    /// @return 行高さ（ピクセル）
    int GetLineHeight(int fontHandle);

    /// @brief フォントのキャップオフセットを取得する（上側余白の調整に使う）
    /// @param fontHandle フォントハンドル
    /// @return キャップオフセット（ピクセル）
    float GetFontCapOffset(int fontHandle);

    // --- Image ---

    /// @brief テクスチャ画像を描画する
    /// @param x X座標（デザイン座標）
    /// @param y Y座標（デザイン座標）
    /// @param w 描画幅
    /// @param h 描画高さ
    /// @param textureHandle テクスチャハンドル（TextureManagerで取得）
    /// @param opacity 不透明度
    void DrawImage(float x, float y, float w, float h, int textureHandle, float opacity = 1.0f);

    /// @brief UV範囲を指定してテクスチャ画像を描画する（トリミング/タイリング用）
    /// @param x X座標（デザイン座標）
    /// @param y Y座標（デザイン座標）
    /// @param w 描画幅
    /// @param h 描画高さ
    /// @param textureHandle テクスチャハンドル
    /// @param u0 UV左上U
    /// @param v0 UV左上V
    /// @param u1 UV右下U
    /// @param v1 UV右下V
    /// @param opacity 不透明度
    void DrawImageUV(float x, float y, float w, float h, int textureHandle,
                     float u0, float v0, float u1, float v1,
                     float opacity = 1.0f);

    // --- Scissor ---

    /// @brief シザー矩形をスタックに積む（描画範囲をクリップする）
    /// @param rect クリップ範囲（デザイン座標）
    void PushScissor(const LayoutRect& rect);

    /// @brief シザー矩形をスタックから取り除く
    void PopScissor();

    // --- Transform / Opacity ---

    /// @brief 2Dアフィン変換をスタックに積む（親の変換と合成される）
    /// @param local 追加するローカル変換
    void PushTransform(const Transform2D& local);

    /// @brief 変換スタックを1段戻す
    void PopTransform();

    /// @brief 現在の合成変換を取得する
    /// @return 現在のアフィン変換行列
    const Transform2D& GetTransform() const { return m_transformStack.back(); }

    /// @brief 不透明度をスタックに積む（親の不透明度と乗算される）
    /// @param opacity 追加する不透明度 (0.0~1.0)
    void PushOpacity(float opacity);

    /// @brief 不透明度スタックを1段戻す
    void PopOpacity();

    /// @brief 現在の合成不透明度を取得する
    /// @return 現在の不透明度
    float GetOpacity() const { return m_opacityStack.back(); }

    // --- Deferred ---

    /// @brief 遅延描画を登録する（ドロップダウンのポップアップなど全描画後に描画したいもの用）
    /// @param fn 描画時に呼ばれるコールバック
    void DeferDraw(std::function<void()> fn);

    /// @brief 登録された遅延描画を全て実行し、キューをクリアする
    void FlushDeferredDraws();

    // --- Resize ---

    /// @brief スクリーンサイズ変更を通知する
    /// @param width 新しいスクリーン幅
    /// @param height 新しいスクリーン高さ
    void OnResize(uint32_t width, uint32_t height);

    /// @brief デザイン解像度を設定する。GUI座標はこの解像度ベースで計算される
    /// @param designWidth デザイン幅（0で無効化）
    /// @param designHeight デザイン高さ（0で無効化）
    void SetDesignResolution(uint32_t designWidth, uint32_t designHeight);

    /// @brief GUIスケール係数を取得する（デザイン解像度/実解像度の比率）
    /// @return スケール値
    float GetGuiScale() const { return m_guiScale; }

    /// @brief レターボックスのX方向オフセットを取得する
    /// @return オフセット（ピクセル）
    float GetGuiOffsetX() const { return m_guiOffsetX; }

    /// @brief レターボックスのY方向オフセットを取得する
    /// @return オフセット（ピクセル）
    float GetGuiOffsetY() const { return m_guiOffsetY; }

    /// @brief FontManagerを取得する
    /// @return FontManagerのポインタ
    FontManager* GetFontManager() const { return m_fontManager; }

    /// @brief SpriteBatchを取得する
    /// @return SpriteBatchのポインタ
    SpriteBatch* GetSpriteBatch() const { return m_spriteBatch; }

    /// @brief TextRendererを取得する
    /// @return TextRendererのポインタ
    TextRenderer* GetTextRenderer() const { return m_textRenderer; }

private:
    bool CreateRectPipeline(ID3D12Device* device);
    void UpdateProjectionMatrix();

    void DrawRectInternal(float x, float y, float w, float h,
                          const StyleColor& fillColor, float cornerRadius,
                          float borderWidth, const StyleColor& borderColor,
                          float shadowOffsetX, float shadowOffsetY,
                          float shadowBlur, float shadowAlpha,
                          float opacity,
                          const StyleColor* gradientColor,
                          const XMFLOAT2& gradientDir,
                          const UIRectEffect* effect);

    void ApplyScissor();
    void UpdateGuiMetrics();
    void ApplyGuiViewport();

    // Device
    ID3D12Device*              m_device = nullptr;       ///< D3D12デバイス
    ID3D12GraphicsCommandList* m_cmdList = nullptr;      ///< 現フレームのコマンドリスト
    uint32_t                   m_frameIndex = 0;         ///< ダブルバッファ用フレームインデックス

    // Renderers
    SpriteBatch*  m_spriteBatch = nullptr;               ///< テキスト・画像描画用SpriteBatch
    TextRenderer* m_textRenderer = nullptr;              ///< テキスト描画用レンダラー
    FontManager*  m_fontManager = nullptr;               ///< フォント管理

    // Screen
    uint32_t m_screenWidth  = 1280;                      ///< スクリーン幅（ピクセル）
    uint32_t m_screenHeight = 720;                       ///< スクリーン高さ（ピクセル）
    XMFLOAT4X4 m_projectionMatrix;                       ///< 2D正射影行列

    // Design resolution scaling
    uint32_t m_designWidth  = 0;                         ///< デザイン幅（0=無効）
    uint32_t m_designHeight = 0;                         ///< デザイン高さ（0=無効）
    float    m_guiScale   = 1.0f;                        ///< GUIスケール係数
    float    m_guiOffsetX = 0.0f;                        ///< レターボックスXオフセット（ピクセル）
    float    m_guiOffsetY = 0.0f;                        ///< レターボックスYオフセット（ピクセル）

    // UIRect batch
    Shader                       m_rectShader;           ///< SDF角丸矩形シェーダー
    ComPtr<ID3D12RootSignature>  m_rectRootSignature;    ///< UIRect用ルートシグネチャ
    ComPtr<ID3D12PipelineState>  m_rectPSO;              ///< UIRect用PSO
    DynamicBuffer                m_rectVertexBuffer;     ///< UIRect用頂点バッファ
    DynamicBuffer                m_rectConstantBuffer;   ///< UIRect用定数バッファ
    Buffer                       m_rectIndexBuffer;      ///< UIRect用インデックスバッファ
    uint32_t                     m_rectDrawCount = 0;    ///< 描画した矩形数

    // Scissor
    std::vector<ScissorRect> m_scissorStack;             ///< シザー矩形スタック
    ScissorRect m_fullScreen;                            ///< フルスクリーンシザー矩形
    bool m_spriteBatchActive = false;                    ///< SpriteBatchが描画中か

    // Deferred
    std::vector<std::function<void()>> m_deferredDraws;  ///< 遅延描画コールバックキュー

    // Transform/Opacity stacks
    std::vector<Transform2D> m_transformStack;           ///< アフィン変換スタック
    std::vector<float> m_opacityStack;                   ///< 不透明度スタック
};

/// @}
}} // namespace gx::GUI

