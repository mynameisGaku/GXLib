#pragma once
/// @file PipelineState.h
/// @brief パイプラインステートオブジェクト（PSO）ビルダー
///
/// 【初学者向け解説】
/// パイプラインステートオブジェクト（PSO）は、GPUの描画パイプライン全体の
/// 設定をまとめたオブジェクトです。
///
/// 描画パイプラインとは、3Dデータが画面上のピクセルになるまでの一連の処理です：
/// 入力アセンブラ → 頂点シェーダー → ラスタライザ → ピクセルシェーダー → 出力マージャー
///
/// PSOには以下の設定が含まれます：
/// - どのシェーダーを使うか（VS/PS）
/// - 頂点データのレイアウト（位置、色、UVなど）
/// - ラスタライザの設定（裏面カリング、ワイヤフレームなど）
/// - ブレンド状態（半透明の計算方法）
/// - デプス/ステンシル設定
/// - レンダーターゲットのフォーマット
///
/// ビルダーパターンで設定を行い、Build()でPSOを作成します。

#include "pch.h"

namespace GX
{

/// @brief グラフィックスPSOをビルダーパターンで構築するクラス
class PipelineStateBuilder
{
public:
    PipelineStateBuilder();
    ~PipelineStateBuilder() = default;

    PipelineStateBuilder& SetRootSignature(ID3D12RootSignature* rootSignature);
    PipelineStateBuilder& SetVertexShader(D3D12_SHADER_BYTECODE bytecode);
    PipelineStateBuilder& SetPixelShader(D3D12_SHADER_BYTECODE bytecode);
    PipelineStateBuilder& SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* elements, uint32_t count);
    PipelineStateBuilder& SetRenderTargetFormat(DXGI_FORMAT format, uint32_t index = 0);
    PipelineStateBuilder& SetDepthFormat(DXGI_FORMAT format);
    PipelineStateBuilder& SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
    PipelineStateBuilder& SetCullMode(D3D12_CULL_MODE mode);
    PipelineStateBuilder& SetDepthEnable(bool enable);
    PipelineStateBuilder& SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask);
    PipelineStateBuilder& SetDepthBias(int bias, float clamp, float slopeScaledBias);
    PipelineStateBuilder& SetDepthComparisonFunc(D3D12_COMPARISON_FUNC func);
    PipelineStateBuilder& SetRenderTargetCount(uint32_t count);
    PipelineStateBuilder& SetFillMode(D3D12_FILL_MODE mode);

    /// ブレンドステートを直接設定
    PipelineStateBuilder& SetBlendState(const D3D12_BLEND_DESC& blendDesc);

    /// アルファブレンド（半透明描画）を設定
    PipelineStateBuilder& SetAlphaBlend();

    /// 加算ブレンドを設定
    PipelineStateBuilder& SetAdditiveBlend();

    /// 減算ブレンドを設定
    PipelineStateBuilder& SetSubtractiveBlend();

    /// 乗算ブレンドを設定 (Result = Dest * SrcColor)
    PipelineStateBuilder& SetMultiplyBlend();

    /// PSOを構築
    ComPtr<ID3D12PipelineState> Build(ID3D12Device* device);

private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc;
};

} // namespace GX
