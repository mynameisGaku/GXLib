#pragma once
/// @file SpriteBatch.h
/// @brief スプライトバッチ — 2Dスプライト描画エンジン
///
/// 【初学者向け解説】
/// SpriteBatchは、2D画像（スプライト）を効率よく描画するためのクラスです。
/// DXLibのDrawGraph/DrawRotaGraph等に相当する機能を提供します。
///
/// 「バッチ」とは、複数のスプライトをまとめて1回のDrawCallで描画する仕組みです。
/// GPUへのDrawCall回数が少ないほどパフォーマンスが良くなります。
///
/// 使い方：
/// 1. Begin() — バッチ開始
/// 2. DrawGraph() / DrawRotaGraph() 等 — スプライトを登録
/// 3. End() — バッチ終了＆実際の描画

#include "pch.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief ブレンドモード
enum class BlendMode
{
    Alpha,      ///< アルファブレンド（半透明）
    Add,        ///< 加算ブレンド（発光効果）
    Sub,        ///< 減算ブレンド
    Mul,        ///< 乗算ブレンド
    Screen,     ///< スクリーンブレンド
    None,       ///< ブレンドなし（不透明）
    Count
};

/// @brief 2Dスプライト描画バッチクラス
class SpriteBatch
{
public:
    /// @brief 1バッチあたりの最大スプライト数
    static constexpr uint32_t k_MaxSprites = 4096;

    SpriteBatch() = default;
    ~SpriteBatch() = default;

    /// @brief スプライトバッチを初期化する
    /// @param device D3D12デバイスへのポインタ
    /// @param cmdQueue コマンドキューへのポインタ
    /// @param screenWidth スクリーン幅（ピクセル）
    /// @param screenHeight スクリーン高さ（ピクセル）
    /// @return 初期化に成功した場合true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    uint32_t screenWidth, uint32_t screenHeight);

    /// @brief バッチ描画を開始する
    /// @param cmdList グラフィックスコマンドリスト
    /// @param frameIndex 現在のフレームインデックス
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// @brief テクスチャを描画する（DXLib DrawGraph互換）
    /// @param x 描画先X座標
    /// @param y 描画先Y座標
    /// @param handle テクスチャハンドル
    /// @param transFlag 透過処理を有効にするかどうか
    void DrawGraph(float x, float y, int handle, bool transFlag = true);

    /// @brief 回転拡大描画する（DXLib DrawRotaGraph互換）
    /// @param cx 回転中心X座標
    /// @param cy 回転中心Y座標
    /// @param extRate 拡大率
    /// @param angle 回転角度（ラジアン）
    /// @param handle テクスチャハンドル
    /// @param transFlag 透過処理を有効にするかどうか
    void DrawRotaGraph(float cx, float cy, float extRate, float angle,
                       int handle, bool transFlag = true);

    /// @brief 矩形切り出し描画する（DXLib DrawRectGraph互換）
    /// @param x 描画先X座標
    /// @param y 描画先Y座標
    /// @param srcX 転送元矩形のX座標
    /// @param srcY 転送元矩形のY座標
    /// @param w 転送元矩形の幅
    /// @param h 転送元矩形の高さ
    /// @param handle テクスチャハンドル
    /// @param transFlag 透過処理を有効にするかどうか
    void DrawRectGraph(float x, float y, int srcX, int srcY, int w, int h,
                       int handle, bool transFlag = true);

    /// @brief 矩形切り出し＋拡大縮小描画する
    /// @param dstX 描画先X座標
    /// @param dstY 描画先Y座標
    /// @param dstW 描画先の幅
    /// @param dstH 描画先の高さ
    /// @param srcX 転送元矩形のX座標
    /// @param srcY 転送元矩形のY座標
    /// @param srcW 転送元矩形の幅
    /// @param srcH 転送元矩形の高さ
    /// @param handle テクスチャハンドル
    /// @param transFlag 透過処理を有効にするかどうか
    void DrawRectExtendGraph(float dstX, float dstY, float dstW, float dstH,
                             int srcX, int srcY, int srcW, int srcH,
                             int handle, bool transFlag = true);

    /// @brief 拡大縮小描画する（DXLib DrawExtendGraph互換）
    /// @param x1 描画先矩形の左上X座標
    /// @param y1 描画先矩形の左上Y座標
    /// @param x2 描画先矩形の右下X座標
    /// @param y2 描画先矩形の右下Y座標
    /// @param handle テクスチャハンドル
    /// @param transFlag 透過処理を有効にするかどうか
    void DrawExtendGraph(float x1, float y1, float x2, float y2,
                         int handle, bool transFlag = true);

    /// @brief 自由変形描画する（DXLib DrawModiGraph互換）
    /// @param x1 左上のX座標
    /// @param y1 左上のY座標
    /// @param x2 右上のX座標
    /// @param y2 右上のY座標
    /// @param x3 右下のX座標
    /// @param y3 右下のY座標
    /// @param x4 左下のX座標
    /// @param y4 左下のY座標
    /// @param handle テクスチャハンドル
    /// @param transFlag 透過処理を有効にするかどうか
    void DrawModiGraph(float x1, float y1, float x2, float y2,
                       float x3, float y3, float x4, float y4,
                       int handle, bool transFlag = true);

    /// @brief 任意四角形 + ソース矩形指定で描画（矩形の一部を変形して描画）
    /// @param x1,y1 左上  @param x2,y2 右上  @param x3,y3 右下  @param x4,y4 左下
    /// @param srcX,srcY ソース矩形左上（ピクセル）
    /// @param srcW,srcH ソース矩形サイズ（ピクセル）
    void DrawRectModiGraph(float x1, float y1, float x2, float y2,
                           float x3, float y3, float x4, float y4,
                           float srcX, float srcY, float srcW, float srcH,
                           int handle, bool transFlag = true);

    /// @brief ブレンドモードを設定する
    /// @param mode 設定するブレンドモード
    void SetBlendMode(BlendMode mode);

    /// @brief 描画色を設定する（以降の描画に乗算される）
    /// @param r 赤成分（0.0〜1.0）
    /// @param g 緑成分（0.0〜1.0）
    /// @param b 青成分（0.0〜1.0）
    /// @param a アルファ成分（0.0〜1.0）
    void SetDrawColor(float r, float g, float b, float a = 1.0f);

    /// @brief バッチ描画を終了し、蓄積したスプライトをフラッシュする
    void End();

    /// @brief スクリーンサイズを更新する
    /// @param width 新しいスクリーン幅（ピクセル）
    /// @param height 新しいスクリーン高さ（ピクセル）
    void SetScreenSize(uint32_t width, uint32_t height);

    /// @brief テクスチャマネージャへの参照を取得する
    /// @return TextureManagerへの参照
    TextureManager& GetTextureManager() { return m_textureManager; }

    /// @brief カスタム正射影行列を設定する（Camera2Dやスケーリング用）
    /// @param matrix 正射影行列
    void SetProjectionMatrix(const XMMATRIX& matrix);

    /// @brief デフォルトの正射影行列にリセットする
    void ResetProjectionMatrix();

private:
    /// スプライト頂点フォーマット（32バイト）
    struct SpriteVertex
    {
        XMFLOAT2 position;  ///< スクリーン座標
        XMFLOAT2 texcoord;  ///< UV座標
        XMFLOAT4 color;     ///< 頂点カラー
    };

    /// 内部バッチフラッシュ
    void Flush();

    /// 4頂点をバッチに追加
    void AddQuad(const SpriteVertex& v0, const SpriteVertex& v1,
                 const SpriteVertex& v2, const SpriteVertex& v3,
                 int textureHandle);

    /// PSO群を作成
    bool CreatePipelineStates(ID3D12Device* device);

    /// インデックスバッファを作成
    bool CreateIndexBuffer(ID3D12Device* device);

    /// 正射影行列を更新
    void UpdateProjectionMatrix();

    // デバイスとコンテキスト
    ID3D12Device*              m_device   = nullptr;
    ID3D12GraphicsCommandList* m_cmdList  = nullptr;
    uint32_t                   m_frameIndex = 0;

    // テクスチャ管理
    TextureManager m_textureManager;

    // バッファ
    DynamicBuffer m_vertexBuffer;
    Buffer        m_indexBuffer;

    // シェーダー
    Shader m_shaderCompiler;

    // パイプライン
    ComPtr<ID3D12RootSignature> m_rootSignature;
    std::array<ComPtr<ID3D12PipelineState>, static_cast<size_t>(BlendMode::Count)> m_pipelineStates;

    // 定数バッファ（正射影行列用）
    DynamicBuffer m_constantBuffer;

    // バッチ状態
    SpriteVertex*  m_mappedVertices    = nullptr;
    uint32_t       m_spriteCount      = 0;
    uint32_t       m_vertexWriteOffset = 0;  ///< フレーム内の累積書き込み位置（Flush済みスプライト数）
    int            m_currentTexture   = -1;
    BlendMode      m_blendMode      = BlendMode::Alpha;
    BlendMode      m_lastBoundBlend = BlendMode::Count; ///< 最後にバインドしたPSOのブレンドモード（冗長バインド防止）
    XMFLOAT4       m_drawColor      = { 1.0f, 1.0f, 1.0f, 1.0f };
    uint32_t       m_lastFrameIndex = UINT32_MAX; ///< 前回のフレーム番号（同一フレームでのリセット防止）

    // スクリーン設定
    uint32_t m_screenWidth  = 1280;
    uint32_t m_screenHeight = 720;
    XMMATRIX m_projectionMatrix;
    bool     m_useCustomProjection = false;
};

} // namespace GX
