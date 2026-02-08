#pragma once
/// @file PrimitiveBatch3D.h
/// @brief 3Dプリミティブ描画（デバッグ用ワイヤフレーム等）

#include "pch.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// 3Dライン頂点
struct LineVertex3D
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

/// @brief 3Dプリミティブバッチ（ライン描画）
class PrimitiveBatch3D
{
public:
    static constexpr uint32_t k_MaxVertices = 65536;

    PrimitiveBatch3D() = default;
    ~PrimitiveBatch3D() = default;

    /// 初期化
    bool Initialize(ID3D12Device* device);

    /// バッチ開始
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
               const XMFLOAT4X4& viewProjection);

    /// ライン描画
    void DrawLine(const XMFLOAT3& p0, const XMFLOAT3& p1, const XMFLOAT4& color);

    /// ワイヤフレームボックス
    void DrawWireBox(const XMFLOAT3& center, const XMFLOAT3& extents, const XMFLOAT4& color);

    /// ワイヤフレーム球
    void DrawWireSphere(const XMFLOAT3& center, float radius, const XMFLOAT4& color,
                         uint32_t segments = 16);

    /// グリッド
    void DrawGrid(float size, uint32_t divisions, const XMFLOAT4& color);

    /// バッチ終了（描画実行）
    void End();

private:
    bool CreatePipelineState(ID3D12Device* device);

    Shader                       m_shader;
    ComPtr<ID3D12RootSignature>  m_rootSignature;
    ComPtr<ID3D12PipelineState>  m_pso;
    DynamicBuffer                m_vertexBuffer;
    DynamicBuffer                m_constantBuffer;

    ID3D12GraphicsCommandList*   m_cmdList    = nullptr;
    uint32_t                     m_frameIndex = 0;
    LineVertex3D*                m_mappedVertices = nullptr;
    uint32_t                     m_vertexCount    = 0;
};

} // namespace GX
