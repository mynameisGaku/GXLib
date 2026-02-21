#pragma once
/// @file Terrain.h
/// @brief ハイトマップ地形レンダラー
/// DxLibにはない機能。プロシージャルノイズまたは画像データから地形メッシュを生成する。
/// PBR頂点形式(Vertex3D_PBR)でメッシュを構築するため、PBRシェーダーでそのまま描画可能。

#include "pch.h"
#include "Graphics/3D/Vertex3D.h"
#include "Graphics/Resource/Buffer.h"

namespace GX
{

/// @brief ハイトマップベースの地形メッシュ
/// グリッド状の頂点を生成し、法線・タンジェント・UVを自動計算する。
/// GetHeight()でランタイムに任意座標の高さを取得できる（キャラクターの接地等に使う）。
class Terrain
{
public:
    Terrain() = default;
    ~Terrain() = default;

    /// @brief FBMノイズを使ってプロシージャル地形を生成する
    /// @param device D3D12デバイス
    /// @param width 地形の幅（ワールド単位）
    /// @param depth 地形の奥行き（ワールド単位）
    /// @param xSegments X方向の分割数（頂点数 = xSegments+1）
    /// @param zSegments Z方向の分割数
    /// @param maxHeight 最大高さ
    /// @return 成功ならtrue
    bool CreateProcedural(ID3D12Device* device, float width, float depth,
                          uint32_t xSegments, uint32_t zSegments, float maxHeight);

    /// @brief ハイトマップ画像データから地形を生成する
    /// @param device D3D12デバイス
    /// @param heightmapData グレースケール高さデータ (0.0~1.0)
    /// @param hmWidth ハイトマップの幅（ピクセル）
    /// @param hmHeight ハイトマップの高さ（ピクセル）
    /// @param worldWidth ワールド空間での地形幅
    /// @param worldDepth ワールド空間での地形奥行き
    /// @param maxHeight 最大高さ
    /// @return 成功ならtrue
    bool CreateFromHeightmap(ID3D12Device* device,
                              const float* heightmapData, uint32_t hmWidth, uint32_t hmHeight,
                              float worldWidth, float worldDepth, float maxHeight);

    /// @brief 指定ワールド座標(x,z)の高さをバイリニア補間で取得する
    /// @param x ワールドX座標
    /// @param z ワールドZ座標
    /// @return 地形の高さ（範囲外は端の高さにクランプ）
    float GetHeight(float x, float z) const;

    /// @brief 頂点バッファを取得する
    /// @return 頂点バッファ
    const Buffer& GetVertexBuffer() const { return m_vertexBuffer; }

    /// @brief インデックスバッファを取得する
    /// @return インデックスバッファ
    const Buffer& GetIndexBuffer() const { return m_indexBuffer; }

    /// @brief インデックス数を取得する
    /// @return インデックス数
    uint32_t GetIndexCount() const { return m_indexCount; }

private:
    /// 頂点・インデックスからGPUバッファを生成する
    void BuildMesh(ID3D12Device* device, const std::vector<Vertex3D_PBR>& vertices,
                   const std::vector<uint32_t>& indices);

    /// FBMノイズによるプロシージャル高さ計算（5オクターブ）
    static float ProceduralHeight(float x, float z);

    Buffer   m_vertexBuffer;
    Buffer   m_indexBuffer;
    uint32_t m_indexCount = 0;

    // GetHeight()用の高さルックアップテーブル
    std::vector<float> m_heights;
    uint32_t m_xSegments = 0;
    uint32_t m_zSegments = 0;
    float    m_width     = 0.0f;
    float    m_depth     = 0.0f;
    float    m_originX   = 0.0f;  ///< グリッド左端のX座標
    float    m_originZ   = 0.0f;  ///< グリッド上端のZ座標
};

} // namespace GX
