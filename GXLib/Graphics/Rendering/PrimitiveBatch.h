#pragma once
/// @file PrimitiveBatch.h
/// @brief 基本図形描画バッチ
///
/// DxLibの DrawLine / DrawBox / DrawCircle / DrawTriangle 等に相当する図形描画を提供する。
/// 三角形バッファと線分バッファを個別に持ち、End() 時にそれぞれフラッシュして描画する。

#include "pch.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief 基本図形描画バッチクラス（DxLibの DrawLine / DrawBox / DrawCircle 等に相当）
class PrimitiveBatch
{
public:
    static constexpr uint32_t k_MaxTriangleVertices = 4096 * 3; ///< 1バッチの三角形頂点上限
    static constexpr uint32_t k_MaxLineVertices     = 4096 * 2; ///< 1バッチの線分頂点上限

    PrimitiveBatch() = default;
    ~PrimitiveBatch() = default;

    /// @brief プリミティブバッチを初期化する
    /// @param device D3D12デバイスへのポインタ
    /// @param screenWidth スクリーン幅（ピクセル）
    /// @param screenHeight スクリーン高さ（ピクセル）
    /// @return 初期化に成功した場合 true
    bool Initialize(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight);

    /// @brief バッチ描画を開始する。End() までに呼んだ Draw 系が蓄積される
    /// @param cmdList グラフィックスコマンドリスト
    /// @param frameIndex 現在のフレームインデックス（ダブルバッファ用）
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// @brief 線分を描画する。DxLibの DrawLine に相当
    /// @param x1 始点X座標
    /// @param y1 始点Y座標
    /// @param x2 終点X座標
    /// @param y2 終点Y座標
    /// @param color 色（0xAARRGGBB形式、アルファ0は不透明扱い）
    /// @param thickness 線の太さ（現在は1固定）
    void DrawLine(float x1, float y1, float x2, float y2, uint32_t color, int thickness = 1);

    /// @brief 矩形を描画する。DxLibの DrawBox に相当
    /// @param x1 左上X座標
    /// @param y1 左上Y座標
    /// @param x2 右下X座標
    /// @param y2 右下Y座標
    /// @param color 色（0xAARRGGBB形式）
    /// @param fillFlag true で塗りつぶし、false で枠線のみ
    void DrawBox(float x1, float y1, float x2, float y2, uint32_t color, bool fillFlag);

    /// @brief 円を描画する。DxLibの DrawCircle に相当
    /// @param cx 中心X座標
    /// @param cy 中心Y座標
    /// @param r 半径
    /// @param color 色（0xAARRGGBB形式）
    /// @param fillFlag true で塗りつぶし、false で枠線のみ
    /// @param segments 円を近似する多角形の辺数（多いほど滑らか）
    void DrawCircle(float cx, float cy, float r, uint32_t color, bool fillFlag, int segments = 32);

    /// @brief 三角形を描画する。DxLibの DrawTriangle に相当
    /// @param x1 頂点1のX座標
    /// @param y1 頂点1のY座標
    /// @param x2 頂点2のX座標
    /// @param y2 頂点2のY座標
    /// @param x3 頂点3のX座標
    /// @param y3 頂点3のY座標
    /// @param color 色（0xAARRGGBB形式）
    /// @param fillFlag true で塗りつぶし、false で枠線のみ
    void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                      uint32_t color, bool fillFlag);

    /// @brief 楕円を描画する。DxLibの DrawOval に相当
    /// @param cx 中心X座標
    /// @param cy 中心Y座標
    /// @param rx X方向半径
    /// @param ry Y方向半径
    /// @param color 色（0xAARRGGBB形式）
    /// @param fillFlag true で塗りつぶし、false で枠線のみ
    /// @param segments 楕円を近似する多角形の辺数
    void DrawOval(float cx, float cy, float rx, float ry, uint32_t color,
                  bool fillFlag, int segments = 32);

    /// @brief 1ピクセルを描画する。DxLibの DrawPixel に相当
    /// @param x X座標
    /// @param y Y座標
    /// @param color 色（0xAARRGGBB形式）
    void DrawPixel(float x, float y, uint32_t color);

    /// @brief バッチ描画を終了し、蓄積した図形をフラッシュする
    void End();

    /// @brief スクリーンサイズを更新する
    /// @param width 新しいスクリーン幅（ピクセル）
    /// @param height 新しいスクリーン高さ（ピクセル）
    void SetScreenSize(uint32_t width, uint32_t height);

    /// @brief カスタム正射影行列を設定する（Camera2Dとの連携用）
    /// @param matrix 正射影行列
    void SetProjectionMatrix(const XMMATRIX& matrix);

    /// @brief デフォルトの正射影行列にリセットする
    void ResetProjectionMatrix();

private:
    /// プリミティブ頂点（24バイト: 位置2D + カラーRGBA）
    struct PrimitiveVertex
    {
        XMFLOAT2 position;  ///< スクリーン座標
        XMFLOAT4 color;     ///< 頂点カラー (RGBA)
    };

    /// 0xAARRGGBB形式の色を XMFLOAT4 に変換（アルファ0は不透明として扱う）
    static XMFLOAT4 ColorToFloat4(uint32_t color);

    /// 蓄積した三角形頂点をGPUに発行
    void FlushTriangles();

    /// 蓄積した線分頂点をGPUに発行
    void FlushLines();

    /// 三角形PSO・線分PSOをまとめて作成
    bool CreatePipelineStates(ID3D12Device* device);

    ID3D12Device*              m_device   = nullptr;
    ID3D12GraphicsCommandList* m_cmdList  = nullptr;
    uint32_t                   m_frameIndex = 0;

    // --- バッファ ---
    DynamicBuffer m_triangleVertexBuffer;  ///< 塗りつぶし図形用の頂点バッファ
    DynamicBuffer m_lineVertexBuffer;      ///< 線分用の頂点バッファ
    DynamicBuffer m_constantBuffer;        ///< 正射影行列の定数バッファ

    // --- シェーダー ---
    Shader m_shaderCompiler;

    // --- パイプライン ---
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_trianglePSO;  ///< TRIANGLE_LIST トポロジ用PSO
    ComPtr<ID3D12PipelineState> m_linePSO;       ///< LINE_LIST トポロジ用PSO

    // --- バッチ状態 ---
    PrimitiveVertex* m_mappedTriVertices = nullptr;   ///< Map済み三角形頂点ポインタ
    PrimitiveVertex* m_mappedLineVertices = nullptr;  ///< Map済み線分頂点ポインタ
    uint32_t m_triVertexCount  = 0;  ///< 現在の三角形頂点数
    uint32_t m_lineVertexCount = 0;  ///< 現在の線分頂点数

    // --- スクリーン ---
    uint32_t m_screenWidth  = 1280;
    uint32_t m_screenHeight = 720;
    XMMATRIX m_projectionMatrix;
    bool     m_useCustomProjection = false;
};

} // namespace GX
