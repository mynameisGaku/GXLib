#pragma once
/// @file ShaderRegistry.h
/// @brief シェーダーモデルPSOレジストリ
/// 6種のシェーダーモデル(PBR/Unlit/Toon/Phong/Subsurface/ClearCoat) × 2(static/skinned) +
/// 2(Toonアウトライン static/skinned) = 14 PSO を一元管理する。
/// Material.shaderModelからGetPSO()で対応PSOを自動選択できる。

#include "pch.h"
#include <gxformat/shader_model.h>
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief シェーダーモデル別PSOレジストリ
/// Renderer3Dが描画時にMaterialのシェーダーモデルとスキニング有無から
/// 適切なPSOを取得するために使用する。
class ShaderRegistry
{
public:
    ShaderRegistry() = default;
    ~ShaderRegistry() = default;

    /// @brief 全PSO(14個)をコンパイル・生成する
    /// @param device D3D12デバイス
    /// @param rootSignature メインパスで共通のルートシグネチャ
    /// @return 全PSO生成に成功したらtrue
    bool Initialize(ID3D12Device* device, ID3D12RootSignature* rootSignature);

    /// @brief シェーダーモデルとスキニング有無に対応するPSOを取得する
    /// @param model シェーダーモデル (Standard/Unlit/Toon/Phong/Subsurface/ClearCoat)
    /// @param skinned ボーンスキニング有りならtrue
    /// @return PSOへのポインタ（未初期化の場合nullptr）
    ID3D12PipelineState* GetPSO(gxfmt::ShaderModel model, bool skinned) const;

    /// @brief Toonアウトライン専用PSOを取得する
    /// @param skinned ボーンスキニング有りならtrue
    /// @return PSOへのポインタ
    /// 前面カリング＋スムース法線による頂点膨張でアウトラインを描画するPSO。
    ID3D12PipelineState* GetToonOutlinePSO(bool skinned) const;

    /// @brief 全PSOを再コンパイル・再生成する（シェーダーホットリロード時に呼ぶ）
    /// @param device D3D12デバイス
    /// @return 成功ならtrue
    bool Rebuild(ID3D12Device* device);

private:
    /// シェーダーモデル1種分のstatic/skinned PSOペア
    struct ShaderModelPSO
    {
        ComPtr<ID3D12PipelineState> pso;         ///< スタティックメッシュ用
        ComPtr<ID3D12PipelineState> psoSkinned;  ///< スキンドメッシュ用
    };

    /// 指定シェーダーモデルのPSOをコンパイル・生成する
    bool CompileAndCreatePSO(ID3D12Device* device, gxfmt::ShaderModel model);

    /// Toonアウトライン用PSOをコンパイル・生成する
    bool CompileToonOutlinePSO(ID3D12Device* device);

    static constexpr uint32_t k_NumShaderModels = 6;  ///< シェーダーモデル数

    Shader                       m_shaderCompiler;
    ID3D12RootSignature*         m_rootSignature = nullptr;
    ShaderModelPSO               m_psos[k_NumShaderModels];  ///< ShaderModel列挙値でインデックス
    ShaderModelPSO               m_toonOutline;              ///< Toonアウトライン専用（スムース法線ベース）
};

} // namespace GX
