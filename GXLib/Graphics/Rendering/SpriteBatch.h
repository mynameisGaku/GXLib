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
    static constexpr uint32_t k_MaxSprites = 4096;

    SpriteBatch() = default;
    ~SpriteBatch() = default;

    /// スプライトバッチを初期化
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    uint32_t screenWidth, uint32_t screenHeight);

    /// バッチ描画を開始
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// テクスチャを描画（DXLib DrawGraph互換）
    void DrawGraph(float x, float y, int handle, bool transFlag = true);

    /// 回転拡大描画（DXLib DrawRotaGraph互換）
    void DrawRotaGraph(float cx, float cy, float extRate, float angle,
                       int handle, bool transFlag = true);

    /// 矩形切り出し描画（DXLib DrawRectGraph互換）
    void DrawRectGraph(float x, float y, int srcX, int srcY, int w, int h,
                       int handle, bool transFlag = true);

    /// 拡大縮小描画（DXLib DrawExtendGraph互換）
    void DrawExtendGraph(float x1, float y1, float x2, float y2,
                         int handle, bool transFlag = true);

    /// 自由変形描画（DXLib DrawModiGraph互換）
    void DrawModiGraph(float x1, float y1, float x2, float y2,
                       float x3, float y3, float x4, float y4,
                       int handle, bool transFlag = true);

    /// ブレンドモードを設定
    void SetBlendMode(BlendMode mode);

    /// 描画色を設定（以降の描画に乗算される）
    void SetDrawColor(float r, float g, float b, float a = 1.0f);

    /// バッチ描画を終了（フラッシュ）
    void End();

    /// スクリーンサイズ更新
    void SetScreenSize(uint32_t width, uint32_t height);

    /// テクスチャマネージャへの参照を取得
    TextureManager& GetTextureManager() { return m_textureManager; }

    /// カスタム正射影行列を設定（Camera2Dから使用）
    void SetProjectionMatrix(const XMMATRIX& matrix);

    /// デフォルトの正射影行列にリセット
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
    XMFLOAT4       m_drawColor      = { 1.0f, 1.0f, 1.0f, 1.0f };
    uint32_t       m_lastFrameIndex = UINT32_MAX; ///< 前回のフレーム番号（同一フレームでのリセット防止）

    // スクリーン設定
    uint32_t m_screenWidth  = 1280;
    uint32_t m_screenHeight = 720;
    XMMATRIX m_projectionMatrix;
    bool     m_useCustomProjection = false;
};

} // namespace GX
