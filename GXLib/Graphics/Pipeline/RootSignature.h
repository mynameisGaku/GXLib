#pragma once
/// @file RootSignature.h
/// @brief ルートシグネチャビルダー
///
/// 【初学者向け解説】
/// ルートシグネチャとは、シェーダーが必要とするリソース（定数バッファ、
/// テクスチャなど）の「引数リスト」のようなものです。
///
/// 例えるなら、関数の宣言のようなものです：
/// void DrawObject(ConstantBuffer transform, Texture albedo, Sampler linearSampler);
/// この「引数の型と順番」を定義するのがルートシグネチャです。
///
/// ビルダーパターンを使って、必要なリソースを追加していき、
/// 最後にBuild()でルートシグネチャを作成します。

#include "pch.h"

namespace GX
{

/// @brief ルートシグネチャをビルダーパターンで構築するクラス
class RootSignatureBuilder
{
public:
    RootSignatureBuilder() = default;
    ~RootSignatureBuilder() = default;

    /// ルートCBV（定数バッファ）を追加
    RootSignatureBuilder& AddCBV(uint32_t shaderRegister, uint32_t space = 0,
                                  D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    /// ルートSRV（シェーダーリソース）を追加
    RootSignatureBuilder& AddSRV(uint32_t shaderRegister, uint32_t space = 0,
                                  D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    /// ディスクリプタテーブルを追加（SRV/CBV/UAVのテーブル）
    /// テクスチャバインド等で使用。shaderRegisterから始まるnumDescriptors個のディスクリプタ範囲を定義
    RootSignatureBuilder& AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE type,
                                              uint32_t shaderRegister,
                                              uint32_t numDescriptors = 1,
                                              uint32_t space = 0,
                                              D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    /// スタティックサンプラーを追加
    RootSignatureBuilder& AddStaticSampler(uint32_t shaderRegister, uint32_t space = 0,
                                            D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    /// スタティックサンプラーを追加（アドレスモード・比較関数指定）
    RootSignatureBuilder& AddStaticSampler(uint32_t shaderRegister,
                                            D3D12_FILTER filter,
                                            D3D12_TEXTURE_ADDRESS_MODE addressMode,
                                            D3D12_COMPARISON_FUNC comparisonFunc,
                                            uint32_t space = 0);

    /// ルートシグネチャフラグを設定
    RootSignatureBuilder& SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);

    /// ルートシグネチャを構築
    ComPtr<ID3D12RootSignature> Build(ID3D12Device* device);

private:
    std::vector<D3D12_ROOT_PARAMETER1>      m_parameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC>  m_staticSamplers;
    /// ディスクリプタレンジを安定したメモリに保持（ポインタ無効化防止のためdeque使用）
    std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE1>> m_descriptorRanges;
    D3D12_ROOT_SIGNATURE_FLAGS              m_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
};

} // namespace GX
