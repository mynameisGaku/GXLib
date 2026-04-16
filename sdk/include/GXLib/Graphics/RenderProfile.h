#pragma once
/// @file RenderProfile.h
/// @brief Unity Volume Profile 風レンダリング設定プロファイル
///
/// ポストエフェクト・影・フォグ・RT等の描画設定を統一的に管理する。
/// パラメータ毎の Override ON/OFF + 優先度付きスタック + JSON 保存/読込に対応。

#include "pch_graphics.h"
#include "Container/ContainerFwd.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/3D/Fog.h"

namespace gx
{
/// @addtogroup grp_gfx_postfx
/// @{

class Renderer3D;

// ============================================================================
// Override<T> — パラメータ毎のオーバーライド制御
// ============================================================================

/// @brief 個別パラメータのオーバーライド状態を保持するテンプレート
/// @tparam T パラメータの型
template<typename T>
struct Override
{
    T    value{};
    bool overridden = false;

    void Set(const T& v) { value = v; overridden = true; }
    void Clear()          { overridden = false; }
    void MergeFrom(const Override<T>& other)
    {
        if (other.overridden) { value = other.value; overridden = true; }
    }
};

// ============================================================================
// LerpValue — 型別補間ヘルパー
// ============================================================================

inline float LerpValue(float a, float b, float t) { return a + (b - a) * t; }
inline int   LerpValue(int a, int b, float t)     { return static_cast<int>(a + (b - a) * t + 0.5f); }
inline uint32_t LerpValue(uint32_t a, uint32_t b, float t)
{
    return static_cast<uint32_t>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t + 0.5f);
}
inline bool  LerpValue(bool a, bool /*b*/, float t) { return t < 0.5f ? a : !a; }

inline TonemapMode LerpValue(TonemapMode a, TonemapMode b, float t)
{
    return t < 0.5f ? a : b;
}

inline FogMode LerpValue(FogMode a, FogMode b, float t)
{
    return t < 0.5f ? a : b;
}

inline Vector3 LerpValue(const Vector3& a, const Vector3& b, float t)
{
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
}

inline gx::Array<float, 4> LerpValue(const gx::Array<float, 4>& a, const gx::Array<float, 4>& b, float t)
{
    return { a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t,
             a[2] + (b[2] - a[2]) * t, a[3] + (b[3] - a[3]) * t };
}

/// @brief Override<T> の補間（両方 overridden → Lerp、片方のみ → そちらを返す）
template<typename T>
Override<T> LerpOverride(const Override<T>& a, const Override<T>& b, float t)
{
    Override<T> result;
    if (a.overridden && b.overridden)
    {
        result.Set(LerpValue(a.value, b.value, t));
    }
    else if (a.overridden)
    {
        result = a;
    }
    else if (b.overridden)
    {
        result = b;
    }
    return result;
}

// ============================================================================
// RenderProfile — 全描画パラメータのプロファイル
// ============================================================================

/// @brief 全描画パラメータを Override<T> で管理するプロファイル
///
/// Unity Volume Profile のように「overridden なパラメータのみ」を適用する仕組み。
/// MergeFrom / Lerp / JSON で一括管理できる。
class RenderProfile
{
public:
    // --- Tonemapping ---
    Override<TonemapMode> tonemapMode;
    Override<float>       tonemapExposure;

    // --- Bloom ---
    Override<bool>  bloomEnabled;
    Override<float> bloomThreshold;
    Override<float> bloomIntensity;

    // --- SSAO ---
    Override<bool>  ssaoEnabled;
    Override<float> ssaoRadius;
    Override<float> ssaoBias;
    Override<float> ssaoPower;

    // --- DoF ---
    Override<bool>  dofEnabled;
    Override<float> dofFocalDistance;
    Override<float> dofFocalRange;
    Override<float> dofBokehRadius;

    // --- MotionBlur ---
    Override<bool>  motionBlurEnabled;
    Override<float> motionBlurIntensity;
    Override<int>   motionBlurSampleCount;

    // --- SSR ---
    Override<bool>  ssrEnabled;
    Override<float> ssrMaxDistance;
    Override<float> ssrStepSize;
    Override<int>   ssrMaxSteps;
    Override<float> ssrThickness;
    Override<float> ssrIntensity;

    // --- Outline ---
    Override<bool>  outlineEnabled;
    Override<float> outlineDepthThreshold;
    Override<float> outlineNormalThreshold;
    Override<float> outlineIntensity;

    // --- TAA ---
    Override<bool>  taaEnabled;
    Override<float> taaBlendFactor;
    Override<float> taaSharpness;

    // --- VolumetricLight ---
    Override<bool>  volumetricLightEnabled;
    Override<float> volumetricLightIntensity;
    Override<float> volumetricLightDensity;
    Override<float> volumetricLightDecay;

    // --- AutoExposure ---
    Override<bool>  autoExposureEnabled;
    Override<float> autoExposureSpeed;
    Override<float> autoExposureMin;
    Override<float> autoExposureMax;
    Override<float> autoExposureKeyValue;

    // --- ContactShadows ---
    Override<bool>  contactShadowsEnabled;
    Override<float> contactShadowsMaxDistance;
    Override<int>   contactShadowsStepCount;
    Override<float> contactShadowsThickness;
    Override<float> contactShadowsIntensity;

    // --- Vignette / ChromaticAberration ---
    Override<bool>  vignetteEnabled;
    Override<float> vignetteIntensity;
    Override<float> chromaticAberrationStrength;

    // --- ColorGrading ---
    Override<bool>  colorGradingEnabled;
    Override<float> colorGradingContrast;
    Override<float> colorGradingSaturation;
    Override<float> colorGradingTemperature;

    // --- FXAA ---
    Override<bool> fxaaEnabled;

    // --- LensFlare ---
    Override<bool>  lensFlareEnabled;
    Override<float> lensFlareIntensity;
    Override<float> lensFlareThreshold;
    Override<float> lensFlareApertureBlades;
    Override<float> lensFlareStarburstStrength;

    // --- Fog ---
    Override<FogMode>  fogMode;
    Override<Vector3> fogColor;
    Override<float>    fogStart;
    Override<float>    fogEnd;
    Override<float>    fogDensity;

    // --- Shadows (CSM) ---
    Override<bool>                shadowEnabled;
    Override<gx::Array<float,4>> cascadeSplits;
    Override<bool>                cascadeBlendEnabled;
    Override<float>               cascadeBlendWidth;
    Override<bool>                pcssEnabled;
    Override<float>               pcssLightSize;

    // --- Ambient ---
    Override<Vector3> ambientColor;

    // --- RT Reflections ---
    Override<bool>     rtReflectionsEnabled;
    Override<float>    rtReflectionsMaxDistance;
    Override<float>    rtReflectionsIntensity;
    Override<uint32_t> rtReflectionsShadowSamples;

    // --- RT GI ---
    Override<bool>  rtgiEnabled;
    Override<float> rtgiIntensity;
    Override<float> rtgiMaxDistance;
    Override<float> rtgiTemporalAlpha;
    Override<int>   rtgiSpatialIterations;

    // --- SSGI (screen-space GI, RTGI fallback) ---
    Override<bool>  ssgiEnabled;
    Override<float> ssgiIntensity;
    Override<float> ssgiMaxDistance;
    Override<int>   ssgiSampleCount;
    Override<float> ssgiThickness;

    /// @brief other の overridden 値で上書きする
    void MergeFrom(const RenderProfile& other);

    /// @brief 二つのプロファイルを補間する
    static RenderProfile Lerp(const RenderProfile& a, const RenderProfile& b, float t);

    /// @brief プロファイルの overridden パラメータを PostEffectPipeline と Renderer3D に適用する
    void Apply(PostEffectPipeline& pipeline, Renderer3D* renderer = nullptr) const;

    /// @brief JSON ファイルからプロファイルを読み込む
    static bool Load(const gx::String& path, RenderProfile& out);

    /// @brief プロファイルを JSON ファイルに保存する（overridden のみ出力）
    static bool Save(const gx::String& path, const RenderProfile& profile);

    /// @brief 全パラメータ overridden、標準値
    static RenderProfile CreateDefault();

    /// @brief SSAO 強め、フォグ無し、暗め露出
    static RenderProfile CreateIndoor();

    /// @brief フォグ、ボリューメトリックライト、PCSS
    static RenderProfile CreateOutdoor();

    /// @brief DoF、モーションブラー、カラグレ
    static RenderProfile CreateCinematic();
};

// ============================================================================
// RenderProfileStack — 優先度付きプロファイルスタック
// ============================================================================

/// @brief 複数の RenderProfile を優先度付きでスタックし、最終結果をマージする
class RenderProfileStack
{
public:
    struct Entry
    {
        uint32_t      id;
        gx::String    name;
        RenderProfile profile;
        int           priority;   ///< 大きいほど優先
        float         weight;     ///< 0.0〜1.0 (フェード用)
    };

    /// @brief プロファイルを追加する
    /// @return 割り当てられた ID
    uint32_t AddProfile(const gx::String& name, const RenderProfile& profile,
                        int priority, float weight = 1.0f);

    /// @brief ID で指定したプロファイルを削除する
    void RemoveProfile(uint32_t id);

    /// @brief ID で指定したプロファイルのウェイトを変更する
    void SetWeight(uint32_t id, float weight);

    /// @brief ID で指定したプロファイルの優先度を変更する
    void SetPriority(uint32_t id, int priority);

    /// @brief 全レイヤーをマージして最終結果を返す
    RenderProfile Resolve() const;

    /// @brief Resolve() して Apply() する便利メソッド
    void Apply(PostEffectPipeline& pipeline, Renderer3D* renderer = nullptr) const;

    /// @brief 登録されたエントリ数を返す
    size_t GetEntryCount() const { return m_entries.size(); }

    /// @brief 全エントリをクリアする
    void Clear() { m_entries.clear(); }

private:
    gx::Vector<Entry> m_entries;
    uint32_t m_nextId = 1;
};

/// @}
} // namespace gx
