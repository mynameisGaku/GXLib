#pragma once
/// @file SSAO.h
/// @brief Screen Space Ambient Occlusion (遮蔽による陰影)
///
/// DxLibには無い機能。深度バッファだけで物体の角や隙間を暗くし、
/// 立体感と奥行き感を大幅に向上させる。GBufferを持たないForward+環境でも動作する。
///
/// 処理の流れ:
/// 1. AO生成: 深度→ビュー空間位置復元→半球サンプリング(64点)→遮蔽率計算
/// 2. バイラテラルブラー: 水平+垂直の2パスでノイズ除去（エッジ保持）
/// 3. 乗算合成: MultiplyBlend PSOでHDRシーンにAO値を直接乗算

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// SSAO生成定数バッファ (射影行列+カーネル+パラメータ)
struct SSAOConstants
{
    XMFLOAT4X4 projection;       ///< 射影行列 (64B)
    XMFLOAT4X4 invProjection;    ///< 逆射影行列 (64B)
    XMFLOAT4   samples[64];     ///< 半球サンプリングカーネル (1024B)
    float      radius;           ///< サンプリング半径 (ビュー空間)
    float      bias;             ///< 深度バイアス (自己遮蔽防止)
    float      power;            ///< AO強調指数
    float      screenWidth;
    float      screenHeight;
    float      nearZ;
    float      farZ;
    float      padding;
};  // 1184B → 256-align → 1280B

/// ブラー定数バッファ (水平/垂直方向のテクセルオフセット)
struct SSAOBlurConstants
{
    float blurDirX;   ///< 水平方向: 1/width, 垂直時は0
    float blurDirY;   ///< 垂直方向: 1/height, 水平時は0
    float padding[2];
};

/// @brief 深度バッファから遮蔽を計算して立体感を高めるSSAOエフェクト
///
/// 半球サンプリング(64点)で各ピクセル周囲の遮蔽率を計算し、
/// バイラテラルブラー後にHDRシーンへ乗算合成する。R8_UNORM RTに出力。
class SSAO
{
public:
    /// サンプリング点の数。多いほど精度が上がるがコストも増える
    static constexpr uint32_t k_KernelSize = 64;
    static_assert(k_KernelSize > 0, "SSAO kernel size must be > 0");

    SSAO() = default;
    ~SSAO() = default;

    /// @brief 初期化。AO用RT・カーネル生成・PSO作成を行う
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief SSAOの全パス(AO生成→ブラー→乗算合成)を実行する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param hdrRT HDRシーン。AO値がインプレースで乗算合成される
    /// @param depthBuffer 深度バッファ (ビュー空間位置の復元に使う)
    /// @param camera カメラ (射影行列の取得に使う)
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& hdrRT, DepthBuffer& depthBuffer, const Camera3D& camera);

    /// @brief 画面リサイズ時にAO用RTを再生成する
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief サンプリング半径 (ビュー空間)。大きいほど広範囲の遮蔽を検出する
    void SetRadius(float r) { m_radius = r; }
    float GetRadius() const { return m_radius; }

    /// @brief 深度バイアス。自己遮蔽によるアーティファクト防止用
    void SetBias(float b) { m_bias = b; }
    float GetBias() const { return m_bias; }

    /// @brief AO強調指数。大きいほどコントラストが強くなる
    void SetPower(float p) { m_power = p; }
    float GetPower() const { return m_power; }

private:
    void GenerateKernel();
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = true;
    float m_radius = 0.5f;
    float m_bias   = 0.025f;
    float m_power  = 2.0f;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // AO出力RT (R8_UNORM)
    RenderTarget m_ssaoRT;
    // ブラー中間RT (R8_UNORM)
    RenderTarget m_blurTempRT;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_generateRS;
    ComPtr<ID3D12RootSignature> m_blurRS;
    ComPtr<ID3D12PipelineState> m_generatePSO;
    ComPtr<ID3D12PipelineState> m_blurHPSO;
    ComPtr<ID3D12PipelineState> m_blurVPSO;
    ComPtr<ID3D12PipelineState> m_compositePSO;

    // 定数バッファ
    DynamicBuffer m_generateCB;
    DynamicBuffer m_blurCB;

    // 半球カーネル
    XMFLOAT4 m_kernel[k_KernelSize];
};

} // namespace GX
