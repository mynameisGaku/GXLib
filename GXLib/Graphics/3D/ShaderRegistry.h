#pragma once
/// @file ShaderRegistry.h
/// @brief シェーダーモデルPSOレジストリ
/// 6種のシェーダーモデル(PBR/Unlit/Toon/Phong/Subsurface/ClearCoat) × 2(static/skinned) +
/// 2(Toonアウトライン static/skinned) = 14 PSO を一元管理する。
/// Material.shaderModelからGetPSO()で対応PSOを自動選択できる。

#include "pch_graphics.h"
#include <gxformat/shader_model.h>
#include "Graphics/Pipeline/Shader.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

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

    // =========================================================================
    // Layer 2 拡張ポイント — カスタムシェーダーモデル登録 (ADR-0017 L2, T1.6/T1.7)
    // =========================================================================

    /// @brief カスタムシェーダーモデルの記述
    ///
    /// Layer 2 ユーザーが組み込み6モデル (Standard/Unlit/Toon/Phong/Subsurface/ClearCoat)
    /// に加えて独自のシェーダーを PSO レジストリに登録するための定義。
    /// Material::shaderModel を登録した custom ID に設定すると、
    /// Renderer3D が自動的にこの PSO を選択して描画する。
    ///
    /// @code
    /// ShaderRegistry::CustomShaderModelDesc desc;
    /// desc.vsPath = L"Shaders/MyAnisotropic.hlsl"; desc.vsEntry = "VSMain";
    /// desc.psPath = L"Shaders/MyAnisotropic.hlsl"; desc.psEntry = "PSMain";
    /// desc.supportsSkinning = true;
    /// uint32_t MY_MODEL = 100;   // 6-254 の範囲で任意
    /// registry.RegisterCustomShaderModel(MY_MODEL, desc);
    /// // 以後 Material.shaderModel = static_cast<ShaderModel>(MY_MODEL) で使える
    /// @endcode
    struct CustomShaderModelDesc
    {
        gx::WString vsPath;                          ///< VS HLSL パス
        gx::String  vsEntry = "VSMain";              ///< VS エントリ関数名
        gx::WString psPath;                          ///< PS HLSL パス (vsPath と同じでも可)
        gx::String  psEntry = "PSMain";              ///< PS エントリ関数名
        bool        supportsSkinning = true;         ///< スキンドメッシュPSOも作るか
        gx::Vector<gx::String> defines;              ///< 追加 #define (名前=値 形式)
    };

    /// @brief カスタムシェーダーモデルを登録する。
    /// @param customId 6-254 のユーザー定義ID (0-5 は組み込みで予約, 255 は Custom 予約)
    /// @param desc     シェーダー記述
    /// @return 登録成功で true, ID範囲外やコンパイル失敗で false (ログ出力)
    ///
    /// 既に同じID で登録済みの場合は置換 (再コンパイル)。
    /// Rebuild() でも自動的に再コンパイルされる (ホットリロード対応)。
    bool RegisterCustomShaderModel(uint32_t customId, const CustomShaderModelDesc& desc);

    /// @brief カスタムシェーダーモデルの登録を解除する。
    /// @param customId RegisterCustomShaderModel で指定したID
    /// @return 解除成功で true, 未登録なら false
    bool UnregisterCustomShaderModel(uint32_t customId);

    /// @brief 登録されたカスタムシェーダーモデル数を返す。
    size_t GetCustomShaderModelCount() const { return m_customModels.size(); }

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

    // Layer 2: カスタムシェーダーモデル登録 (ADR-0017 L2)
    struct CustomShaderModelEntry
    {
        uint32_t                id = 0;
        CustomShaderModelDesc   desc;
        ShaderModelPSO          pso;
    };
    gx::Vector<CustomShaderModelEntry> m_customModels;

    /// カスタムシェーダーモデルのPSOを (再)コンパイルする
    bool CompileCustomShaderModelPSO(ID3D12Device* device, CustomShaderModelEntry& entry);
};

/// @}
} // namespace gx
