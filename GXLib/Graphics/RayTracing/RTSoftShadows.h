#pragma once
/// @file RTSoftShadows.h
/// @brief DXRソフトシャドウ（面積光源ベース）
///
/// DxLibには無い機能。DXRのハードウェアレイトレーシングで面積光源からの
/// ソフトシャドウを生成する。複数のジッターレイを面積光源の範囲内にディスパッチし、
/// ペナンブラ（半影）を自然に再現する。テンポラル蓄積により少ないレイ数で
/// ノイズを低減する。
///
/// 参考: "Ray Traced Shadows: Maintaining Real-Time Frame Rates" (NVIDIA)

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/RayTracing/RTAccelerationStructure.h"

namespace gx
{
/// @addtogroup grp_gfx_rt
/// @{

/// @brief RTソフトシャドウの設定パラメータ
struct RTSoftShadowSettings
{
    bool  enabled       = false;  ///< エフェクト有効フラグ
    float lightRadius   = 0.5f;   ///< 面積光源の半径 (大きいほどソフト)
    int   numRays       = 8;      ///< 1ピクセルあたりのシャドウレイ数
    float temporalBlend = 0.9f;   ///< テンポラル蓄積ブレンド率 (0=現在のみ, 1=履歴のみ)
    float maxDistance   = 100.0f;  ///< シャドウレイの最大飛距離
};

/// @brief DXRベースの面積光源ソフトシャドウ
///
/// TLASを使用してシーンに対するシャドウレイを発射し、面積光源の範囲内で
/// ジッターを加えることでペナンブラを生成する。テンポラル蓄積で
/// フレーム間のノイズを平滑化する。
class RTSoftShadows
{
public:
    RTSoftShadows() = default;
    ~RTSoftShadows() = default;

    /// @brief 初期化。DXR対応GPU (ID3D12Device5) でのみ成功する
    /// @param device DXR対応デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device5* device, uint32_t width, uint32_t height);

    /// @brief リソースを解放する
    void Shutdown();

    /// @brief 設定パラメータを更新する
    void SetSettings(const RTSoftShadowSettings& settings) { m_settings = settings; }

    /// @brief 現在の設定パラメータを取得する
    const RTSoftShadowSettings& GetSettings() const { return m_settings; }

    /// @brief TLASを設定する (RTReflectionsと共有可能)
    /// @param tlas アクセラレーション構造へのポインタ
    void SetTLAS(RTAccelerationStructure* tlas) { m_tlas = tlas; }

    /// @brief ソフトシャドウを生成する
    /// @param cmdList4 DXR対応コマンドリスト
    /// @param invViewProj 逆ビュープロジェクション行列
    /// @param cameraPos カメラのワールド位置
    /// @param lightDir ライト方向 (ワールド空間、正規化済み)
    /// @param depthSRV 深度バッファSRV
    /// @param normalSRV 法線バッファSRV
    /// @param frameIndex ダブルバッファ用フレームインデックス
    void Execute(ID3D12GraphicsCommandList4* cmdList4,
                 const XMMATRIX& invViewProj, const XMFLOAT3& cameraPos,
                 const XMFLOAT3& lightDir,
                 ID3D12Resource* depthSRV, ID3D12Resource* normalSRV,
                 uint32_t frameIndex);

    /// @brief シャドウ結果テクスチャを取得する
    /// @return シャドウマップRT (R8_UNORM、0=影、1=光)
    RenderTarget& GetShadowRT() { return m_shadowRT; }

    /// @brief エフェクトが有効かどうか
    bool IsEnabled() const { return m_settings.enabled; }

private:
    RTSoftShadowSettings m_settings;               ///< エフェクト設定
    RTAccelerationStructure* m_tlas = nullptr;      ///< TLAS (外部所有)
    bool m_initialized = false;                    ///< 初期化完了フラグ

    ID3D12Device5* m_device = nullptr;             ///< DXR対応デバイス

    uint32_t m_width  = 0;                         ///< 画面幅
    uint32_t m_height = 0;                         ///< 画面高さ
    uint32_t m_frameCounter = 0;                   ///< フレームカウンタ (ジッター用)

    // シャドウ結果テクスチャ
    RenderTarget m_shadowRT;                       ///< 現フレームのシャドウ結果 (R8_UNORM)
    RenderTarget m_historyRT;                      ///< テンポラル蓄積履歴 (R8_UNORM)

    // DXRパイプライン
    ComPtr<ID3D12RootSignature> m_raygenRS;        ///< レイ生成用ルートシグネチャ
    ComPtr<ID3D12StateObject>   m_rtPSO;           ///< DXRステートオブジェクト

    // テンポラル蓄積パス
    Shader                      m_compositeShader; ///< テンポラル合成シェーダー
    ComPtr<ID3D12RootSignature> m_compositeRS;     ///< テンポラル合成用ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_compositePSO;    ///< テンポラル合成用PSO

    // 定数バッファ
    DynamicBuffer m_rayCB;                         ///< レイ生成用定数バッファ
    DynamicBuffer m_compositeCB;                   ///< テンポラル合成用定数バッファ
};

/// @}
} // namespace gx
