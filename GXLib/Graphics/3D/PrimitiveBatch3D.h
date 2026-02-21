#pragma once
/// @file PrimitiveBatch3D.h
/// @brief 3Dプリミティブ描画（デバッグ用ワイヤフレーム等）
/// DxLibの DrawLine3D / DrawSphere3D / DrawBox3D 等に相当する3D線描画。
/// Begin~End間にDraw系関数を呼ぶバッチ方式で、Endで一括描画される。

#include "pch.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief 3Dライン描画用の頂点（位置＋色）
struct LineVertex3D
{
    XMFLOAT3 position;  ///< ワールド座標
    XMFLOAT4 color;     ///< RGBA色
};

/// @brief 3Dプリミティブバッチ（ライン描画、最大65536頂点）
/// Begin/End間にDrawLine, DrawWireBox等を呼び出し、End()で一括描画する。
/// 深度テスト有り・深度書き込み無し・アルファブレンド有効のパイプラインで描画する。
class PrimitiveBatch3D
{
public:
    static constexpr uint32_t k_MaxVertices = 65536;  ///< 1バッチの最大頂点数

    PrimitiveBatch3D() = default;
    ~PrimitiveBatch3D() = default;

    /// @brief 初期化する（動的頂点バッファとPSOの生成）
    /// @param device D3D12デバイス
    /// @return 成功ならtrue
    bool Initialize(ID3D12Device* device);

    /// @brief バッチの描画開始（頂点バッファをマップし、定数を転送する）
    /// @param cmdList コマンドリスト
    /// @param frameIndex フレームインデックス（ダブルバッファ用）
    /// @param viewProjection カメラのViewProjection行列
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
               const XMFLOAT4X4& viewProjection);

    /// @brief 2点間にラインを描画する
    /// @param p0 始点
    /// @param p1 終点
    /// @param color 線の色
    void DrawLine(const XMFLOAT3& p0, const XMFLOAT3& p1, const XMFLOAT4& color);

    /// @brief ワイヤフレームのボックスを描画する（12辺）
    /// @param center 中心座標
    /// @param extents 半径（各軸方向の大きさ）
    /// @param color 線の色
    void DrawWireBox(const XMFLOAT3& center, const XMFLOAT3& extents, const XMFLOAT4& color);

    /// @brief ワイヤフレームの球を描画する（XY/XZ/YZ平面の3つの円）
    /// @param center 中心座標
    /// @param radius 半径
    /// @param color 線の色
    /// @param segments 円の分割数（デフォルト16）
    void DrawWireSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color,
                         uint32_t segments = 16);

    /// @brief XZ平面にグリッドを描画する
    /// @param size グリッドの全幅
    /// @param divisions 分割数
    /// @param color 線の色
    void DrawGrid(float size, uint32_t divisions, const XMFLOAT4& color);

    /// @brief バッチを終了してGPUに描画コマンドを発行する
    void End();

private:
    bool CreatePipelineState(ID3D12Device* device);

    Shader                       m_shader;
    ComPtr<ID3D12RootSignature>  m_rootSignature;
    ComPtr<ID3D12PipelineState>  m_pso;
    DynamicBuffer                m_vertexBuffer;    ///< 動的頂点バッファ（毎フレーム書き換え）
    DynamicBuffer                m_constantBuffer;

    ID3D12GraphicsCommandList*   m_cmdList    = nullptr;
    uint32_t                     m_frameIndex = 0;
    LineVertex3D*                m_mappedVertices = nullptr;  ///< マップ済み頂点バッファへのポインタ
    uint32_t                     m_vertexCount    = 0;
};

} // namespace GX
