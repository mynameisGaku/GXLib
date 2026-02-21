#pragma once
/// @file PipelineState.h
/// @brief パイプラインステートオブジェクト（PSO）のビルダーパターン構築
///
/// PSOは描画パイプラインの設定をまとめたDX12オブジェクト。
/// DxLibのSetDrawBlendMode/SetWriteZBufferなどに相当する設定が全てここに入る。
///
/// シェーダー、頂点レイアウト、ラスタライザ、ブレンド、深度テスト、
/// レンダーターゲットフォーマットなどを設定し、Build()でPSOを生成する。
///
/// DX12ではPSOの変更がステート切り替えに当たるため、描画方法ごとに
/// 事前にPSOを作成しておく必要がある。

#include "pch.h"

namespace GX
{

/// @brief 描画パイプライン設定をビルダーパターンで組み立て、PSOを生成する
class PipelineStateBuilder
{
public:
    /// @brief コンストラクタ。妥当なデフォルト値で初期化される（ソリッド描画、背面カリング、深度テスト有効）
    PipelineStateBuilder();
    ~PipelineStateBuilder() = default;

    /// @brief このPSOで使うルートシグネチャを設定する
    /// @param rootSignature ルートシグネチャのポインタ
    /// @return ビルダー自身
    PipelineStateBuilder& SetRootSignature(ID3D12RootSignature* rootSignature);

    /// @brief 頂点シェーダーのバイトコードを設定する
    /// @param bytecode ShaderBlob::GetBytecode()で取得した値を渡す
    /// @return ビルダー自身
    PipelineStateBuilder& SetVertexShader(D3D12_SHADER_BYTECODE bytecode);

    /// @brief ピクセルシェーダーのバイトコードを設定する
    /// @param bytecode ShaderBlob::GetBytecode()で取得した値を渡す
    /// @return ビルダー自身
    PipelineStateBuilder& SetPixelShader(D3D12_SHADER_BYTECODE bytecode);

    /// @brief 頂点入力レイアウトを設定する（位置、法線、UV等の構成）
    /// @param elements 入力要素の配列
    /// @param count 配列の要素数
    /// @return ビルダー自身
    PipelineStateBuilder& SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* elements, uint32_t count);

    /// @brief レンダーターゲットのフォーマットを設定する
    /// @param format ピクセルフォーマット（HDRならR16G16B16A16_FLOAT）
    /// @param index レンダーターゲットのインデックス（MRT使用時に1以上を指定）
    /// @return ビルダー自身
    PipelineStateBuilder& SetRenderTargetFormat(DXGI_FORMAT format, uint32_t index = 0);

    /// @brief 深度バッファのフォーマットを設定する
    /// @param format 深度フォーマット（通常 D32_FLOAT）
    /// @return ビルダー自身
    PipelineStateBuilder& SetDepthFormat(DXGI_FORMAT format);

    /// @brief プリミティブトポロジタイプを設定する
    /// @param type トポロジ種別（三角形、線、点など）
    /// @return ビルダー自身
    PipelineStateBuilder& SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);

    /// @brief カリングモードを設定する
    /// @param mode NONE=カリングなし、BACK=背面カリング、FRONT=前面カリング
    /// @return ビルダー自身
    PipelineStateBuilder& SetCullMode(D3D12_CULL_MODE mode);

    /// @brief 深度テストの有効/無効を設定する
    /// @param enable trueで深度テスト有効
    /// @return ビルダー自身
    PipelineStateBuilder& SetDepthEnable(bool enable);

    /// @brief 深度書き込みマスクを設定する
    /// @param mask ALL=書き込みあり、ZERO=書き込みなし（半透明描画時等）
    /// @return ビルダー自身
    PipelineStateBuilder& SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK mask);

    /// @brief 深度バイアスを設定する（シャドウマップのアクネ対策等）
    /// @param bias 整数バイアス値
    /// @param clamp バイアスのクランプ上限
    /// @param slopeScaledBias ポリゴン傾斜に応じたバイアス
    /// @return ビルダー自身
    PipelineStateBuilder& SetDepthBias(int bias, float clamp, float slopeScaledBias);

    /// @brief 深度比較関数を設定する
    /// @param func 比較関数（デフォルトはLESS）
    /// @return ビルダー自身
    PipelineStateBuilder& SetDepthComparisonFunc(D3D12_COMPARISON_FUNC func);

    /// @brief レンダーターゲット数を設定する
    /// @param count RT数。0を指定すると全フォーマットがUNKNOWNにリセットされる（深度のみ描画用）
    /// @return ビルダー自身
    PipelineStateBuilder& SetRenderTargetCount(uint32_t count);

    /// @brief フィルモードを設定する
    /// @param mode SOLID=塗りつぶし、WIREFRAME=ワイヤフレーム表示
    /// @return ビルダー自身
    PipelineStateBuilder& SetFillMode(D3D12_FILL_MODE mode);

    /// @brief ブレンドステート全体を直接設定する
    /// @param blendDesc D3D12ブレンド設定構造体
    /// @return ビルダー自身
    PipelineStateBuilder& SetBlendState(const D3D12_BLEND_DESC& blendDesc);

    /// @brief アルファブレンドを設定する（DxLibのDX_BLENDMODE_ALPHAに相当）
    /// @return ビルダー自身
    PipelineStateBuilder& SetAlphaBlend();

    /// @brief 加算ブレンドを設定する（DxLibのDX_BLENDMODE_ADDに相当）
    /// @return ビルダー自身
    PipelineStateBuilder& SetAdditiveBlend();

    /// @brief 減算ブレンドを設定する（DxLibのDX_BLENDMODE_SUBに相当）
    /// @return ビルダー自身
    PipelineStateBuilder& SetSubtractiveBlend();

    /// @brief 乗算ブレンドを設定する（Result = Dest * SrcColor）（DxLibのDX_BLENDMODE_MULAに相当）
    /// @return ビルダー自身
    PipelineStateBuilder& SetMultiplyBlend();

    /// @brief 設定済みの内容からPSOを構築する
    /// @param device D3D12デバイス
    /// @return 生成されたPSO。失敗時はnullptr
    ComPtr<ID3D12PipelineState> Build(ID3D12Device* device);

private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc;  ///< PSO設定の実体
};

} // namespace GX
