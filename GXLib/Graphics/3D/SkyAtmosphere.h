#pragma once
/// @file SkyAtmosphere.h
/// @brief 単散乱近似による大気空レンダリング
///
/// フルスクリーンピクセルシェーダーパスで物理ベースの天球に対するRayleigh/Mie散乱を計算する。
/// Henyey-Greenstein位相関数と視線レイに沿った数値積分を使用する。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace gx
{

/// @brief GPU定数バッファレイアウト（SkyAtmosphere.hlslと一致が必須）
struct SkyAtmosphereConstants
{
    XMFLOAT4X4 invViewProjection;   ///< 逆ビュープロジェクション行列
    XMFLOAT3   cameraPosition;      ///< カメラのワールド座標
    float      planetRadius;         ///< 惑星半径（メートル）
    XMFLOAT3   sunDirection;         ///< 太陽の方向ベクトル（正規化済み）
    float      atmosphereRadius;     ///< 大気圏上端の半径（メートル）
    XMFLOAT3   sunColor;             ///< 太陽の色（リニアRGB）
    float      sunIntensity;         ///< 太陽の強度
    XMFLOAT3   rayleighCoeff;        ///< Rayleigh散乱係数（海面高度での値）
    float      rayleighScaleHeight;  ///< Rayleigh散乱のスケールハイト（メートル）
    float      mieCoeff;             ///< Mie散乱係数
    float      mieScaleHeight;       ///< Mie散乱のスケールハイト（メートル）
    float      mieG;                 ///< Henyey-Greenstein非対称パラメータ（前方散乱の強さ）
    int        numSteps;             ///< レイマーチのステップ数
    XMFLOAT2   screenDimensions;     ///< スクリーン解像度（幅, 高さ）
    float      _pad[2];              ///< パディング（160バイトアライメント用）
};

/// @brief 物理ベース大気空レンダリング
class SkyAtmosphere
{
public:
    SkyAtmosphere() = default;
    ~SkyAtmosphere() = default;

    /// @brief リソース、シェーダー、PSOを初期化する
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief 空パスを実行する（HDRレンダーターゲットに描画）
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& destHDR, DepthBuffer& depth, const Camera3D& camera);

    /// @brief 画面リサイズを処理する
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    // --- 有効化 / 無効化 ---
    /// @brief 大気レンダリングを有効/無効にする
    /// @param v trueで有効化
    void SetEnabled(bool v) { m_enabled = v; }
    /// @brief 大気レンダリングが有効か判定する
    /// @return 有効ならtrue
    bool IsEnabled() const { return m_enabled; }

    // --- 太陽 ---
    /// @brief 太陽の方向ベクトルを設定する
    /// @param d 正規化された方向ベクトル
    void SetSunDirection(const XMFLOAT3& d) { m_sunDirection = d; }
    /// @brief 太陽の方向ベクトルを取得する
    /// @return 方向ベクトル
    const XMFLOAT3& GetSunDirection() const { return m_sunDirection; }
    /// @brief 太陽の色を設定する（リニアRGB）
    /// @param c RGB色
    void SetSunColor(const XMFLOAT3& c) { m_sunColor = c; }
    /// @brief 太陽の強度を設定する
    /// @param v 強度値
    void SetSunIntensity(float v) { m_sunIntensity = v; }
    /// @brief 太陽の強度を取得する
    /// @return 強度値
    float GetSunIntensity() const { return m_sunIntensity; }

    // --- 大気 ---
    /// @brief 惑星の半径を設定する
    /// @param r 半径（メートル）
    void SetPlanetRadius(float r) { m_planetRadius = r; }
    /// @brief 惑星の半径を取得する
    /// @return 半径（メートル）
    float GetPlanetRadius() const { return m_planetRadius; }
    /// @brief 大気圏上端の半径を設定する
    /// @param r 半径（メートル）
    void SetAtmosphereRadius(float r) { m_atmosphereRadius = r; }
    /// @brief 大気圏上端の半径を取得する
    /// @return 半径（メートル）
    float GetAtmosphereRadius() const { return m_atmosphereRadius; }

    // --- Rayleigh ---
    /// @brief Rayleigh散乱係数を設定する
    /// @param c RGB各波長の散乱係数
    void SetRayleighCoefficients(const XMFLOAT3& c) { m_rayleighCoeff = c; }
    /// @brief Rayleigh散乱のスケールハイトを設定する
    /// @param h スケールハイト（メートル）
    void SetRayleighScaleHeight(float h) { m_rayleighScaleHeight = h; }

    // --- Mie ---
    /// @brief Mie散乱係数を設定する
    /// @param c 散乱係数
    void SetMieCoefficient(float c) { m_mieCoeff = c; }
    /// @brief Mie散乱のスケールハイトを設定する
    /// @param h スケールハイト（メートル）
    void SetMieScaleHeight(float h) { m_mieScaleHeight = h; }
    /// @brief Henyey-Greenstein非対称パラメータを設定する
    /// @param g 非対称パラメータ（0=等方性、1=完全前方散乱）
    void SetMieG(float g) { m_mieG = g; }

    // --- 品質 ---
    /// @brief レイマーチのステップ数を設定する
    /// @param n ステップ数（多いほど高品質だが遅い）
    void SetNumSteps(int n) { m_numSteps = n; }
    /// @brief レイマーチのステップ数を取得する
    /// @return ステップ数
    int GetNumSteps() const { return m_numSteps; }

    /// @brief 仰角に基づいて太陽ディスクの色を計算する
    /// @param sunElevation 太陽の仰角（ラジアン、0 = 地平線、PI/2 = 天頂）
    /// @return RGB色
    static XMFLOAT3 ComputeSunColor(float sunElevation);

private:
    bool CreatePipelines(ID3D12Device* device);
    void UpdateSRVHeap(DepthBuffer& depth, uint32_t frameIndex);

    bool m_enabled = false;                ///< 有効フラグ

    // 太陽
    XMFLOAT3 m_sunDirection = { 0.0f, 1.0f, 0.0f };   ///< 太陽の方向ベクトル
    XMFLOAT3 m_sunColor = { 1.0f, 0.98f, 0.95f };     ///< 太陽の色（リニアRGB）
    float m_sunIntensity = 20.0f;                       ///< 太陽の強度

    // 大気
    float m_planetRadius = 6371000.0f;     ///< 惑星半径（メートル、地球=6371km）
    float m_atmosphereRadius = 6471000.0f; ///< 大気圏上端の半径（メートル）

    // Rayleigh散乱
    XMFLOAT3 m_rayleighCoeff = { 5.5e-6f, 13.0e-6f, 22.4e-6f };  ///< Rayleigh散乱係数（RGB）
    float m_rayleighScaleHeight = 8000.0f;  ///< Rayleighスケールハイト（メートル）

    // Mie散乱
    float m_mieCoeff = 21e-6f;        ///< Mie散乱係数
    float m_mieScaleHeight = 1200.0f; ///< Mieスケールハイト（メートル）
    float m_mieG = 0.758f;            ///< Henyey-Greenstein非対称パラメータ

    int m_numSteps = 8;               ///< レイマーチステップ数

    uint32_t m_width = 0;             ///< 描画幅（ピクセル）
    uint32_t m_height = 0;            ///< 描画高さ（ピクセル）

    // パイプライン
    Shader m_shader;                              ///< 大気シェーダー
    ComPtr<ID3D12RootSignature> m_rootSignature;  ///< ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_pso;            ///< パイプラインステートオブジェクト
    DynamicBuffer m_cb;                           ///< 定数バッファ
    DescriptorHeap m_srvHeap;                     ///< SRVデスクリプタヒープ
    ID3D12Device* m_device = nullptr;             ///< D3D12デバイス
};

} // namespace gx
/// @}
