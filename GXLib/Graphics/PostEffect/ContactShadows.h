#pragma once
/// @file ContactShadows.h
/// @brief Screen Space Contact Shadows
///
/// CSMが表現できない細部のセルフシャドウをスクリーンスペースで近似する。
/// 各ピクセルからライト方向にレイマーチし、深度バッファとの交差でシャドウを判定。

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/Pipeline/Shader.h"

namespace gx
{
/// @addtogroup grp_gfx_postfx
/// @{

/// @brief コンタクトシャドウ定数バッファ（HLSL側と一致させること）
struct ContactShadowConstants
{
    XMFLOAT4X4 view;             ///< ビュー行列（転置済み）
    XMFLOAT4X4 projection;      ///< プロジェクション行列（転置済み）
    XMFLOAT4X4 invProjection;   ///< 逆プロジェクション行列（転置済み）
    XMFLOAT3   lightDirView;    ///< ビュー空間でのライト方向
    float      maxDistance;      ///< マーチ最大距離（ビュー空間）
    float      screenWidth;     ///< スクリーン幅
    float      screenHeight;    ///< スクリーン高さ
    int32_t    stepCount;       ///< マーチステップ数
    float      thickness;       ///< 厚み閾値
    float      intensity;       ///< シャドウ強度
    float      nearZ;           ///< ニアクリップ
    float      farZ;            ///< ファークリップ
    float      padding;
};
static_assert(sizeof(ContactShadowConstants) == 240, "ContactShadowConstants size mismatch");

/// @brief スクリーンスペースコンタクトシャドウ
///
/// CSMでは表現できない小さなオブジェクトや細かい凹凸のセルフシャドウを
/// スクリーンスペースのレイマーチで近似する。
class ContactShadows
{
public:
    ContactShadows() = default;
    ~ContactShadows() = default;

    /// @brief 初期化
    /// @param device D3D12デバイス
    /// @param width スクリーン幅
    /// @param height スクリーン高さ
    /// @return 成功時true
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief コンタクトシャドウを実行する
    /// @param cmdList コマンドリスト
    /// @param frameIndex フレームインデックス
    /// @param hdrRT HDRレンダーターゲット（インプレース乗算合成）
    /// @param depthBuffer 深度バッファ
    /// @param camera カメラ
    /// @param lightDirWorld ワールド空間でのメインライト方向（正規化済み）
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& hdrRT, DepthBuffer& depthBuffer,
                 const Camera3D& camera, const XMFLOAT3& lightDirWorld);

    /// @brief リサイズ処理
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief 有効/無効を切り替える
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    /// @brief 有効かどうかを返す
    bool IsEnabled() const { return m_enabled; }

    /// @brief マーチ最大距離を設定する（ビュー空間）
    void SetMaxDistance(float d) { m_maxDistance = d; }
    float GetMaxDistance() const { return m_maxDistance; }

    /// @brief マーチステップ数を設定する
    void SetStepCount(int count) { m_stepCount = count; }
    int GetStepCount() const { return m_stepCount; }

    /// @brief 厚み閾値を設定する
    void SetThickness(float t) { m_thickness = t; }
    float GetThickness() const { return m_thickness; }

    /// @brief シャドウ強度を設定する
    void SetIntensity(float i) { m_intensity = i; }
    float GetIntensity() const { return m_intensity; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;  ///< デフォルトは無効

    float   m_maxDistance = 0.3f;   ///< マーチ最大距離（ビュー空間）
    int     m_stepCount   = 16;     ///< マーチステップ数
    float   m_thickness   = 0.05f;  ///< 厚み閾値
    float   m_intensity   = 0.5f;   ///< シャドウ強度

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // レンダーターゲット
    RenderTarget m_shadowRT;  ///< R8_UNORM: シャドウマスク

    // パイプライン
    Shader                       m_shader;
    ComPtr<ID3D12RootSignature>  m_generateRS;   ///< 生成パス用 RS
    ComPtr<ID3D12RootSignature>  m_compositeRS;  ///< 合成パス用 RS
    ComPtr<ID3D12PipelineState>  m_generatePSO;  ///< コンタクトシャドウ生成
    ComPtr<ID3D12PipelineState>  m_compositePSO; ///< 乗算合成

    // 定数バッファ
    DynamicBuffer m_generateCB;
};

/// @}
} // namespace gx
