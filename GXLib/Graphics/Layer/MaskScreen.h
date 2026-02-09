#pragma once
/// @file MaskScreen.h
/// @brief DXLibマスク互換システム
///
/// R8_UNORM RTを使い、矩形・円などのマスク形状を描画する。
/// RenderLayerのマスクとして使用される。

#include "pch.h"
#include "Graphics/Layer/RenderLayer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief マスク用定数バッファ
struct MaskConstants
{
    XMFLOAT4X4 projection;
    float maskValue;
    float padding[3];
};

/// @brief DXLib互換マスクスクリーン
class MaskScreen
{
public:
    MaskScreen() = default;
    ~MaskScreen() = default;

    /// マスクスクリーンを作成
    bool Create(ID3D12Device* device, uint32_t w, uint32_t h);

    /// 有効/無効
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// マスク全体をクリア (fill: 0=透過, 255=不透明)
    void Clear(ID3D12GraphicsCommandList* cmdList, uint8_t fill = 0);

    /// 矩形マスクを描画
    void DrawFillRect(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                      float x, float y, float w, float h, float value = 1.0f);

    /// 円マスクを描画
    void DrawCircle(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                    float cx, float cy, float radius, float value = 1.0f);

    /// マスクレイヤーを取得（SetMaskに渡す用）
    RenderLayer* GetAsLayer() { return &m_maskLayer; }

    /// リサイズ
    void OnResize(ID3D12Device* device, uint32_t w, uint32_t h);

private:
    void SetupPipeline(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, float maskValue);

    bool m_enabled = false;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    RenderLayer m_maskLayer;

    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_fillPSO;
    DynamicBuffer m_constantBuffer;
    DynamicBuffer m_vertexBuffer;

    ID3D12Device* m_device = nullptr;
};

} // namespace GX
