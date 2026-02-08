#pragma once
/// @file PrimitiveBatch.h
/// @brief プリミティブバッチ — 基本図形描画エンジン
///
/// 【初学者向け解説】
/// PrimitiveBatchは、線分、矩形、円、三角形などの基本図形を描画するクラスです。
/// DXLibのDrawLine/DrawBox/DrawCircle等に相当する機能を提供します。
///
/// 内部では頂点データをバッファに溜め、End()時にまとめて描画します。

#include "pch.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief 基本図形描画バッチクラス
class PrimitiveBatch
{
public:
    static constexpr uint32_t k_MaxTriangleVertices = 4096 * 3; // 最大三角形頂点数
    static constexpr uint32_t k_MaxLineVertices     = 4096 * 2; // 最大線分頂点数

    PrimitiveBatch() = default;
    ~PrimitiveBatch() = default;

    /// 初期化
    bool Initialize(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight);

    /// バッチ開始
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// 線分を描画（DXLib DrawLine互換）
    void DrawLine(float x1, float y1, float x2, float y2, uint32_t color, int thickness = 1);

    /// 矩形を描画（DXLib DrawBox互換）
    void DrawBox(float x1, float y1, float x2, float y2, uint32_t color, bool fillFlag);

    /// 円を描画（DXLib DrawCircle互換）
    void DrawCircle(float cx, float cy, float r, uint32_t color, bool fillFlag, int segments = 32);

    /// 三角形を描画（DXLib DrawTriangle互換）
    void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                      uint32_t color, bool fillFlag);

    /// 楕円を描画（DXLib DrawOval互換）
    void DrawOval(float cx, float cy, float rx, float ry, uint32_t color,
                  bool fillFlag, int segments = 32);

    /// 1ピクセルを描画（DXLib DrawPixel互換）
    void DrawPixel(float x, float y, uint32_t color);

    /// バッチ終了（フラッシュ）
    void End();

    /// スクリーンサイズ更新
    void SetScreenSize(uint32_t width, uint32_t height);

    /// カスタム正射影行列を設定（Camera2Dから使用）
    void SetProjectionMatrix(const XMMATRIX& matrix);

    /// デフォルト正射影行列にリセット
    void ResetProjectionMatrix();

private:
    /// プリミティブ頂点（24バイト）
    struct PrimitiveVertex
    {
        XMFLOAT2 position;
        XMFLOAT4 color;
    };

    /// 色をuint32_t(0xAARRGGBB)からXMFLOAT4に変換
    static XMFLOAT4 ColorToFloat4(uint32_t color);

    /// 三角形用フラッシュ
    void FlushTriangles();

    /// 線分用フラッシュ
    void FlushLines();

    /// PSO作成
    bool CreatePipelineStates(ID3D12Device* device);

    ID3D12Device*              m_device   = nullptr;
    ID3D12GraphicsCommandList* m_cmdList  = nullptr;
    uint32_t                   m_frameIndex = 0;

    // バッファ
    DynamicBuffer m_triangleVertexBuffer;
    DynamicBuffer m_lineVertexBuffer;
    DynamicBuffer m_constantBuffer;

    // シェーダー
    Shader m_shaderCompiler;

    // パイプライン
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_trianglePSO;
    ComPtr<ID3D12PipelineState> m_linePSO;

    // バッチ状態
    PrimitiveVertex* m_mappedTriVertices = nullptr;
    PrimitiveVertex* m_mappedLineVertices = nullptr;
    uint32_t m_triVertexCount  = 0;
    uint32_t m_lineVertexCount = 0;

    // スクリーン
    uint32_t m_screenWidth  = 1280;
    uint32_t m_screenHeight = 720;
    XMMATRIX m_projectionMatrix;
    bool     m_useCustomProjection = false;
};

} // namespace GX
