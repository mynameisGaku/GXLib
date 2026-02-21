#pragma once
/// @file Skybox.h
/// @brief プロシージャルスカイボックスレンダラー
/// テクスチャを使わず、天頂色/地平色のグラデーション＋太陽のハイライトで空を描画する。
/// DxLibにはない機能。HDRレンダーターゲットに深度書き込みなしで描画する。

#include "pch.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief スカイボックス用定数バッファ構造体
struct SkyboxConstants
{
    XMFLOAT4X4 viewProjection;   ///< カメラのViewProjection行列
    XMFLOAT3   topColor;         ///< 天頂の色
    float      padding1;
    XMFLOAT3   bottomColor;     ///< 地平の色
    float      padding2;
    XMFLOAT3   sunDirection;    ///< 太陽の方向ベクトル
    float      sunIntensity;    ///< 太陽のHDR輝度
};

/// @brief プロシージャルスカイボックス（グラデーション＋太陽ハイライト）
/// 単位キューブを描画し、ピクセルシェーダーで空の色をプロシージャル生成する。
/// 深度はLessEqualで比較し、書き込みはOFF（z=1でシーンの最背面に描画）。
class Skybox
{
public:
    Skybox() = default;
    ~Skybox() = default;

    /// @brief スカイボックスを初期化する（キューブメッシュ・PSO・定数バッファの生成）
    /// @param device D3D12デバイス
    /// @return 成功ならtrue
    bool Initialize(ID3D12Device* device);

    /// @brief 空のグラデーション色を設定する
    /// @param topColor 天頂の色
    /// @param bottomColor 地平の色
    void SetColors(const XMFLOAT3& topColor, const XMFLOAT3& bottomColor);

    /// @brief 太陽のパラメータを設定する
    /// @param direction 太陽の方向ベクトル（正規化不要、内部で正規化しない）
    /// @param intensity HDR輝度
    void SetSun(const XMFLOAT3& direction, float intensity);

    /// @brief スカイボックスを描画する
    /// @param cmdList コマンドリスト
    /// @param frameIndex フレームインデックス（ダブルバッファ用）
    /// @param viewProjection カメラのViewProjection行列
    void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
              const XMFLOAT4X4& viewProjection);

private:
    bool CreatePipelineState(ID3D12Device* device);

    Shader                       m_shader;
    ComPtr<ID3D12RootSignature>  m_rootSignature;
    ComPtr<ID3D12PipelineState>  m_pso;
    Buffer                       m_vertexBuffer;    ///< 単位キューブの頂点
    Buffer                       m_indexBuffer;      ///< 単位キューブのインデックス
    DynamicBuffer                m_constantBuffer;

    XMFLOAT3 m_topColor     = { 0.3f, 0.5f, 0.9f };
    XMFLOAT3 m_bottomColor  = { 0.7f, 0.8f, 0.95f };
    XMFLOAT3 m_sunDirection = { 0.3f, -1.0f, 0.5f };
    float    m_sunIntensity = 5.0f;
};

} // namespace GX
