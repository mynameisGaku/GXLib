#pragma once
/// @file MaskScreen.h
/// @brief DxLib互換マスクシステム
///
/// DxLibのCreateMaskScreen() / DrawFillMaskToDirectData() に相当する機能。
/// R8_UNORM RTにマスク形状 (矩形・円) を描画し、RenderLayerのSetMaskに渡すことで
/// マスク部分だけが表示される切り抜き効果を実現する。

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

/// @brief R8_UNORMマスクに矩形・円を描画するDxLib互換マスクスクリーン
///
/// RenderLayerを内部に持ち、GetAsLayer()でマスクとして渡す。
/// マスク値0=透過、1=不透明で、RenderLayerの表示範囲を制御する。
class MaskScreen
{
public:
    MaskScreen() = default;
    ~MaskScreen() = default;

    /// @brief マスクスクリーンを作成する
    /// @param device D3D12デバイス
    /// @param w 幅
    /// @param h 高さ
    /// @return 成功でtrue
    bool Create(ID3D12Device* device, uint32_t w, uint32_t h);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief マスク全体を指定値でクリアする
    /// @param cmdList コマンドリスト
    /// @param fill クリア値 (0=完全透過, 255=完全不透明)
    void Clear(ID3D12GraphicsCommandList* cmdList, uint8_t fill = 0);

    /// @brief 矩形マスクを描画する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param x 左上X座標 (ピクセル)
    /// @param y 左上Y座標 (ピクセル)
    /// @param w 幅 (ピクセル)
    /// @param h 高さ (ピクセル)
    /// @param value マスク値 (0〜1)
    void DrawFillRect(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                      float x, float y, float w, float h, float value = 1.0f);

    /// @brief 円マスクを描画する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param cx 中心X座標
    /// @param cy 中心Y座標
    /// @param radius 半径
    /// @param value マスク値 (0〜1)
    void DrawCircle(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                    float cx, float cy, float radius, float value = 1.0f);

    /// @brief 内部レイヤーをマスクとして取得する (RenderLayer::SetMaskに渡す用)
    RenderLayer* GetAsLayer() { return &m_maskLayer; }

    /// @brief 画面リサイズ対応
    void OnResize(ID3D12Device* device, uint32_t w, uint32_t h);

private:
    void SetupPipeline(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, float maskValue);
    bool CreatePipelines(ID3D12Device* device);

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
