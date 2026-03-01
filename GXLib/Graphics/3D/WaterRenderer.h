#pragma once
/// @file WaterRenderer.h
/// @brief 水面レンダリング — Gerstner波アニメーションとフレネル反射で海・池を描画
///
/// XZ平面上のグリッドメッシュに対して、頂点シェーダーでGerstner波変位を計算する。
/// ピクセルシェーダーでフレネルによる反射/屈折の切り替えを処理する。
/// 風向き・波振幅・周波数・水色などのパラメータでリアルタイムに外見を調整できる。
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

/// @brief 水面レンダリング用GPU定数バッファ（Water.hlslと一致が必須）
struct WaterConstants
{
    XMFLOAT4X4 viewProjection;   ///< ビュープロジェクション行列
    XMFLOAT4X4 world;            ///< ワールド行列
    XMFLOAT3   cameraPosition;   ///< カメラのワールド座標
    float      time;              ///< 経過時間（秒）
    XMFLOAT3   sunDirection;     ///< 太陽の方向ベクトル
    float      waterLevel;        ///< 水面の高さ（ワールドY座標）
    XMFLOAT3   sunColor;         ///< 太陽の色（リニアRGB）
    float      planeSize;         ///< 水面グリッドのサイズ
    XMFLOAT4   waterColor;       ///< 水の色（RGB + 深度フォールオフ用アルファ）
    XMFLOAT2   windDirection;    ///< 風の方向（XZ平面）
    float      waveAmplitude;    ///< 波の振幅
    float      waveFrequency;    ///< 波の周波数
    float      specularPower;    ///< スペキュラの鋭さ
    float      fresnelBias;      ///< フレネルバイアス
    float      fresnelPower;     ///< フレネルべき乗
    float      _pad;             ///< パディング
};  // 224 bytes

/// @brief FFT波計算用GPU定数バッファ
struct WaterFFTConstants
{
    float  time;          ///< 経過時間（秒）
    float  amplitude;     ///< 波の振幅
    float  windSpeed;     ///< 風速
    float  windDirX;      ///< 風のX方向成分
    float  windDirY;      ///< 風のY方向成分
    int    resolution;    ///< FFT解像度
    float  _pad[2];       ///< パディング
};

/// @brief Gerstner波アニメーションを用いた水面レンダラー
class WaterRenderer
{
public:
    WaterRenderer() = default;
    ~WaterRenderer() = default;

    /// @brief 水面レンダリングリソースを初期化する
    bool Initialize(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight);

    /// @brief 水面を描画する
    void Render(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                DepthBuffer& depth, const Camera3D& camera, float time);

    /// @brief 画面リサイズを処理する
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    // パラメータ
    /// @brief 水面レンダリングを有効/無効にする
    void SetEnabled(bool v) { m_enabled = v; }
    /// @brief 水面レンダリングが有効か判定する
    bool IsEnabled() const { return m_enabled; }

    /// @brief 水面の高さを設定する（ワールドY座標）
    void SetWaterLevel(float v) { m_waterLevel = v; }
    /// @brief 水面の高さを取得する
    float GetWaterLevel() const { return m_waterLevel; }

    /// @brief 水面グリッドのサイズを設定する
    void SetPlaneSize(float v) { m_planeSize = v; }
    /// @brief 水面グリッドのサイズを取得する
    float GetPlaneSize() const { return m_planeSize; }

    /// @brief 波の振幅を設定する
    void SetWaveAmplitude(float v) { m_waveAmplitude = v; }
    /// @brief 波の振幅を取得する
    float GetWaveAmplitude() const { return m_waveAmplitude; }

    /// @brief 波の周波数を設定する
    void SetWaveFrequency(float v) { m_waveFrequency = v; }
    /// @brief 波の周波数を取得する
    float GetWaveFrequency() const { return m_waveFrequency; }

    /// @brief 風の方向を設定する（XZ平面）
    void SetWindDirection(const XMFLOAT2& d) { m_windDirection = d; }
    /// @brief 風の方向を取得する
    const XMFLOAT2& GetWindDirection() const { return m_windDirection; }

    /// @brief 水の色を設定する（RGB + 深度フォールオフアルファ）
    void SetWaterColor(const XMFLOAT4& c) { m_waterColor = c; }
    /// @brief 水の色を取得する
    const XMFLOAT4& GetWaterColor() const { return m_waterColor; }

    /// @brief 太陽の方向ベクトルを設定する
    void SetSunDirection(const XMFLOAT3& d) { m_sunDirection = d; }
    /// @brief 太陽の色を設定する
    void SetSunColor(const XMFLOAT3& c) { m_sunColor = c; }

    /// @brief フレネルバイアスを設定する
    void SetFresnelBias(float v) { m_fresnelBias = v; }
    /// @brief フレネルべき乗を設定する
    void SetFresnelPower(float v) { m_fresnelPower = v; }
    /// @brief スペキュラの鋭さを設定する
    void SetSpecularPower(float v) { m_specularPower = v; }

    /// @brief グリッド解像度を取得する（辺あたりの頂点数）
    int GetGridResolution() const { return m_gridResolution; }

private:
    bool CreatePipelines(ID3D12Device* device);
    bool CreateGridMesh(ID3D12Device* device);

    bool m_enabled = false;                        ///< 有効フラグ

    float m_waterLevel = 0.0f;                     ///< 水面の高さ（ワールドY座標）
    float m_planeSize = 512.0f;                    ///< 水面グリッドサイズ
    float m_waveAmplitude = 0.5f;                  ///< 波の振幅
    float m_waveFrequency = 0.1f;                  ///< 波の周波数
    XMFLOAT2 m_windDirection = { 1.0f, 0.3f };    ///< 風の方向（XZ平面）
    XMFLOAT4 m_waterColor = { 0.0f, 0.3f, 0.5f, 0.8f };  ///< 水の色（RGB + フォールオフα）
    XMFLOAT3 m_sunDirection = { 0.3f, -1.0f, 0.5f };      ///< 太陽の方向
    XMFLOAT3 m_sunColor = { 1.0f, 0.98f, 0.95f };         ///< 太陽の色
    float m_specularPower = 256.0f;                ///< スペキュラの鋭さ
    float m_fresnelBias = 0.02f;                   ///< フレネルバイアス
    float m_fresnelPower = 5.0f;                   ///< フレネルべき乗

    int m_gridResolution = 128;                    ///< グリッド解像度（辺あたりの頂点数）
    uint32_t m_indexCount = 0;                     ///< インデックス数

    uint32_t m_screenWidth = 0;                    ///< 画面幅（ピクセル）
    uint32_t m_screenHeight = 0;                   ///< 画面高さ（ピクセル）

    // GPUリソース
    ComPtr<ID3D12Resource> m_vertexBuffer;         ///< グリッドメッシュ頂点バッファ
    ComPtr<ID3D12Resource> m_indexBuffer;           ///< グリッドメッシュインデックスバッファ
    D3D12_VERTEX_BUFFER_VIEW m_vbView = {};        ///< 頂点バッファビュー
    D3D12_INDEX_BUFFER_VIEW m_ibView = {};         ///< インデックスバッファビュー

    // パイプライン
    Shader m_shader;                               ///< 水面シェーダー
    ComPtr<ID3D12RootSignature> m_rootSignature;   ///< ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_pso;             ///< パイプラインステート
    DynamicBuffer m_cb;                            ///< 定数バッファ
    DescriptorHeap m_srvHeap;                      ///< SRVデスクリプタヒープ
    ID3D12Device* m_device = nullptr;              ///< D3D12デバイス
};

} // namespace gx
/// @}
