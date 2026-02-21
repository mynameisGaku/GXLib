#pragma once
/// @file RootSignature.h
/// @brief ルートシグネチャのビルダーパターン構築
///
/// ルートシグネチャは、シェーダーが使うリソース（定数バッファ、テクスチャなど）の
/// 配置を定義するDX12固有のオブジェクト。DxLibにはない概念で、
/// C++の関数宣言における「引数の型と順番」に相当する。
///
/// AddCBV/AddSRV/AddDescriptorTable でリソースを追加し、Build()で生成する。

#include "pch.h"

namespace GX
{

/// @brief シェーダーが参照するリソースの配置をビルダーパターンで定義・構築する
class RootSignatureBuilder
{
public:
    RootSignatureBuilder() = default;
    ~RootSignatureBuilder() = default;

    /// @brief ルートCBV（定数バッファビュー）を追加する
    /// @param shaderRegister HLSLのレジスタ番号（b0, b1, ...）
    /// @param space レジスタスペース（通常0）
    /// @param visibility このリソースを参照するシェーダーステージ
    /// @return ビルダー自身（メソッドチェーン用）
    RootSignatureBuilder& AddCBV(uint32_t shaderRegister, uint32_t space = 0,
                                  D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    /// @brief ルートSRV（シェーダーリソースビュー）を追加する
    /// @param shaderRegister HLSLのレジスタ番号（t0, t1, ...）
    /// @param space レジスタスペース（通常0）
    /// @param visibility このリソースを参照するシェーダーステージ
    /// @return ビルダー自身
    RootSignatureBuilder& AddSRV(uint32_t shaderRegister, uint32_t space = 0,
                                  D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    /// @brief ディスクリプタテーブルを追加する（テクスチャバインド等に使用）
    /// @param type レンジの種別（SRV/CBV/UAV/Sampler）
    /// @param shaderRegister 開始レジスタ番号
    /// @param numDescriptors テーブル内のディスクリプタ数
    /// @param space レジスタスペース
    /// @param visibility このテーブルを参照するシェーダーステージ
    /// @param rangeFlags DESCRIPTORS_VOLATILEを指定するとフレーム間で安全に上書き可能
    /// @return ビルダー自身
    RootSignatureBuilder& AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE type,
                                              uint32_t shaderRegister,
                                              uint32_t numDescriptors = 1,
                                              uint32_t space = 0,
                                              D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL,
                                              D3D12_DESCRIPTOR_RANGE_FLAGS rangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

    /// @brief スタティックサンプラーを追加する（Wrap/Aniso16/比較なし）
    /// @param shaderRegister HLSLのサンプラーレジスタ番号（s0, s1, ...）
    /// @param space レジスタスペース
    /// @param filter テクスチャフィルタリングモード
    /// @return ビルダー自身
    RootSignatureBuilder& AddStaticSampler(uint32_t shaderRegister, uint32_t space = 0,
                                            D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    /// @brief スタティックサンプラーを追加する（アドレスモード・比較関数を個別指定）
    /// @param shaderRegister HLSLのサンプラーレジスタ番号
    /// @param filter テクスチャフィルタリングモード
    /// @param addressMode U/V/W共通のアドレスモード（Clamp, Border等）
    /// @param comparisonFunc シャドウマップ比較等に使う比較関数
    /// @param space レジスタスペース
    /// @return ビルダー自身
    RootSignatureBuilder& AddStaticSampler(uint32_t shaderRegister,
                                            D3D12_FILTER filter,
                                            D3D12_TEXTURE_ADDRESS_MODE addressMode,
                                            D3D12_COMPARISON_FUNC comparisonFunc,
                                            uint32_t space = 0);

    /// @brief ルートシグネチャフラグを設定する
    /// @param flags 入力アセンブラの有無やストリーム出力の制御等
    /// @return ビルダー自身
    RootSignatureBuilder& SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);

    /// @brief 設定済みのパラメータからルートシグネチャを構築する
    /// @param device D3D12デバイス
    /// @return 生成されたルートシグネチャ。失敗時はnullptr
    ComPtr<ID3D12RootSignature> Build(ID3D12Device* device);

private:
    std::vector<D3D12_ROOT_PARAMETER1>      m_parameters;       ///< ルートパラメータ一覧
    std::vector<D3D12_STATIC_SAMPLER_DESC>  m_staticSamplers;   ///< スタティックサンプラー一覧
    /// ディスクリプタレンジの実体保持。unique_ptrで確保してポインタの安定性を保証する。
    /// vectorのリサイズで既存要素のアドレスが変わると、pDescriptorRangesが不正ポインタになるため。
    std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE1>> m_descriptorRanges;
    D3D12_ROOT_SIGNATURE_FLAGS              m_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
};

} // namespace GX
