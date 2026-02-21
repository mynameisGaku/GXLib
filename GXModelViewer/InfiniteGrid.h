#pragma once
/// @file InfiniteGrid.h
/// @brief シェーダーベースのY=0平面無限グリッド描画
///
/// フルスクリーン三角形でグリッドを描画する。頂点バッファ不要。
/// HDR RTとデプスバッファがバインドされた状態で呼び出す。

#include "pch.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/3D/Camera3D.h"

/// @brief Y=0平面にフルスクリーン三角形で無限グリッドを描画する
class InfiniteGrid
{
public:
    /// @brief シェーダーコンパイル、RootSignature、PSO、定数バッファの初期化
    /// @param device D3D12デバイス
    /// @param shader コンパイルに使うShaderオブジェクト
    /// @return 成功時true
    bool Initialize(ID3D12Device* device, GX::Shader& shader);

    /// @brief グリッドを描画する。Renderer3D.End()の後、PostEffect.EndScene()の前に呼ぶ。
    /// @param cmdList 描画先コマンドリスト
    /// @param frameIndex ダブルバッファのフレームインデックス
    /// @param camera カメラ（VP逆行列とカメラ位置を使用）
    void Draw(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
              const GX::Camera3D& camera);

    float gridScale     = 1.0f;    ///< 副グリッドの間隔（ワールド単位）
    float fadeDistance   = 100.0f;  ///< グリッドがフェードアウトする距離
    float majorLineEvery = 10.0f;  ///< 副グリッド何本ごとに主グリッド線を引くか

private:
    /// @brief GPU定数バッファ構造体（256バイトアラインメント）
    struct alignas(256) GridCBData
    {
        XMFLOAT4X4 viewProjectionInverse;
        XMFLOAT4X4 viewProjection;
        XMFLOAT3   cameraPos;
        float      gridScale;
        float      fadeDistance;
        float      majorLineEvery;
        float      _pad0;
        float      _pad1;
    };

    ComPtr<ID3D12RootSignature> m_rootSig;
    ComPtr<ID3D12PipelineState> m_pso;
    GX::DynamicBuffer           m_cbuffer;
};
