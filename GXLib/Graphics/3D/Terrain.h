#pragma once
/// @file Terrain.h
/// @brief ハイトマップ地形レンダラー

#include "pch.h"
#include "Graphics/3D/Vertex3D.h"
#include "Graphics/Resource/Buffer.h"

namespace GX
{

/// @brief ハイトマップ地形
class Terrain
{
public:
    Terrain() = default;
    ~Terrain() = default;

    /// プロシージャルハイトマップ地形を生成
    /// @param device D3D12デバイス
    /// @param width 地形の幅
    /// @param depth 地形の奥行き
    /// @param xSegments X方向の分割数
    /// @param zSegments Z方向の分割数
    /// @param maxHeight 最大高さ
    bool CreateProcedural(ID3D12Device* device, float width, float depth,
                          uint32_t xSegments, uint32_t zSegments, float maxHeight);

    /// ハイトマップ画像から地形を生成
    /// @param device D3D12デバイス
    /// @param heightmapData グレースケール高さデータ（0.0-1.0）
    /// @param hmWidth ハイトマップ幅
    /// @param hmHeight ハイトマップ高さ
    /// @param worldWidth ワールド空間での幅
    /// @param worldDepth ワールド空間での奥行き
    /// @param maxHeight 最大高さ
    bool CreateFromHeightmap(ID3D12Device* device,
                              const float* heightmapData, uint32_t hmWidth, uint32_t hmHeight,
                              float worldWidth, float worldDepth, float maxHeight);

    /// 指定ワールド座標(x,z)での高さを取得
    float GetHeight(float x, float z) const;

    const Buffer& GetVertexBuffer() const { return m_vertexBuffer; }
    const Buffer& GetIndexBuffer() const { return m_indexBuffer; }
    uint32_t GetIndexCount() const { return m_indexCount; }

private:
    void BuildMesh(ID3D12Device* device, const std::vector<Vertex3D_PBR>& vertices,
                   const std::vector<uint32_t>& indices);

    /// パーリンノイズ風のハイト生成
    static float ProceduralHeight(float x, float z);

    Buffer   m_vertexBuffer;
    Buffer   m_indexBuffer;
    uint32_t m_indexCount = 0;

    // 高さルックアップ用
    std::vector<float> m_heights;
    uint32_t m_xSegments = 0;
    uint32_t m_zSegments = 0;
    float    m_width     = 0.0f;
    float    m_depth     = 0.0f;
    float    m_originX   = 0.0f;  // 左上のX座標
    float    m_originZ   = 0.0f;  // 左上のZ座標
};

} // namespace GX
