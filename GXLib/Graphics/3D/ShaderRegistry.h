#pragma once
/// @file ShaderRegistry.h
/// @brief シェーダーモデルPSOレジストリ
/// 6種のシェーダーモデル × 2(static/skinned) + 2(Toonアウトライン) = 14 PSO を管理する。

#include "pch.h"
#include <gxformat/shader_model.h>
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief シェーダーモデル別PSO管理
class ShaderRegistry
{
public:
    ShaderRegistry() = default;
    ~ShaderRegistry() = default;

    /// @brief 全PSO を初期化（コンパイル+生成）
    /// @param device D3D12デバイス
    /// @param rootSignature メインパス共通ルートシグネチャ
    /// @return 成功時 true
    bool Initialize(ID3D12Device* device, ID3D12RootSignature* rootSignature);

    /// @brief シェーダーモデルに対応するPSOを取得
    /// @param model シェーダーモデル
    /// @param skinned スキニング有無
    /// @return PSOへのポインタ（nullptr = 未初期化）
    ID3D12PipelineState* GetPSO(gxfmt::ShaderModel model, bool skinned) const;

    /// @brief Toonアウトライン用PSOを取得
    /// @param skinned スキニング有無
    /// @return PSOへのポインタ
    ID3D12PipelineState* GetToonOutlinePSO(bool skinned) const;

    /// @brief 全PSOを再構築（ホットリロード用）
    bool Rebuild(ID3D12Device* device);

private:
    struct ShaderModelPSO
    {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12PipelineState> psoSkinned;
    };

    bool CompileAndCreatePSO(ID3D12Device* device, gxfmt::ShaderModel model);
    bool CompileToonOutlinePSO(ID3D12Device* device);

    static constexpr uint32_t k_NumShaderModels = 6;

    Shader                       m_shaderCompiler;
    ID3D12RootSignature*         m_rootSignature = nullptr;
    ShaderModelPSO               m_psos[k_NumShaderModels];     // indexed by ShaderModel enum
    ShaderModelPSO               m_toonOutline;                 // Toon outline PSO
};

} // namespace GX
