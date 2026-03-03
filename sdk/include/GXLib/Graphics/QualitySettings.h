#pragma once
/// @file QualitySettings.h
/// @brief グラフィックス品質プリセット管理
///
/// Low/Medium/High/Ultra のプリセットでポストエフェクト設定を一括変更する。
/// ゲームの設定画面で「画質」を選ぶだけで各種エフェクトが適切に切り替わる。
/// @addtogroup grp_graphics/// @{

#include "pch_graphics.h"

namespace gx
{

class PostEffectPipeline;
class GraphicsDevice;

/// @brief 品質レベル
enum class QualityLevel
{
    Low,     ///< 低品質（エフェクト最小）
    Medium,  ///< 中品質
    High,    ///< 高品質
    Ultra,   ///< 最高品質（全エフェクト有効）
    Custom,  ///< カスタム（手動設定）
};

/// @brief 品質パラメータ
struct QualityParams
{
    uint32_t shadowMapSize   = 2048;  ///< シャドウマップ解像度
    bool     ssaoEnabled     = true;  ///< SSAO有効
    float    ssaoRadius      = 0.5f;  ///< SSAOサンプル半径
    int      ssaoSamples     = 16;    ///< SSAOサンプル数
    bool     bloomEnabled    = true;  ///< Bloom有効
    float    bloomThreshold  = 1.0f;  ///< Bloomしきい値
    float    bloomIntensity  = 0.8f;  ///< Bloom強度
    bool     ssrEnabled      = false; ///< SSR有効
    bool     motionBlurEnabled = false; ///< MotionBlur有効
    bool     taaEnabled      = true;  ///< TAA有効
    bool     fxaaEnabled     = true;  ///< FXAA有効
    bool     contactShadowsEnabled = false; ///< コンタクトシャドウ有効
    bool     dofEnabled      = false; ///< DoF有効
    bool     volumetricLightEnabled = false; ///< ボリューメトリックライト有効
    uint32_t vrsMode = 0;  ///< VRSモード (0=Off, 1=PerDraw, 2=Image)
    bool     ssgiEnabled     = false; ///< SSGI有効
    float    ssgiIntensity   = 0.5f;  ///< SSGI強度
    int      ssgiSampleCount = 32;    ///< SSGIサンプル数

    /// @brief プリセットからパラメータを生成する
    static QualityParams FromPreset(QualityLevel level);
};

/// @brief グラフィックス品質設定
class QualitySettings
{
public:
    /// @brief 品質レベルを設定する
    /// @param level 品質レベル
    void SetQualityLevel(QualityLevel level);

    /// @brief 現在の品質レベルを取得する
    QualityLevel GetQualityLevel() const { return m_level; }

    /// @brief 現在のパラメータを取得する
    const QualityParams& GetParams() const { return m_params; }

    /// @brief パラメータを手動設定する（Customレベルになる）
    void SetParams(const QualityParams& params);

    /// @brief PostEffectPipelineにパラメータを適用する
    /// @param pipeline 適用先のパイプライン
    void Apply(PostEffectPipeline& pipeline) const;

    /// @brief GPU能力に応じた品質レベルを自動判定する
    /// @param device グラフィックスデバイス
    /// @return 推奨品質レベル
    static QualityLevel AutoDetect(const GraphicsDevice& device);

    /// @brief ファイルに保存する
    bool SaveToFile(const gx::String& filePath) const;

    /// @brief ファイルから読み込む
    bool LoadFromFile(const gx::String& filePath);

private:
    QualityLevel m_level = QualityLevel::High;
    QualityParams m_params = QualityParams::FromPreset(QualityLevel::High);
};

} // namespace gx
/// @}
