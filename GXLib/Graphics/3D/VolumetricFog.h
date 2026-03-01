#pragma once
/// @file VolumetricFog.h
/// @brief Froxelベースボリューメトリックフォグ
///
/// DxLibには無い機能。3Dテクスチャ (Froxelグリッド) を用いたボリューメトリックフォグ。
/// Beer-Lambert法則に基づく散乱媒質をシミュレートし、フォグボリュームの追加で
/// 局所的な濃霧エリアを表現できる。処理は3段階のGPUパス:
/// 1. Inject: FogVolumeからFroxelグリッドへ密度/色を書き込み
/// 2. Scatter: フロント→バックのレイマーチでBeer-Lambert蓄積
/// 3. Compose: HDRシーンにフォグを合成 (transmittance + in-scattering)

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Vector3.h"
#include "Math/Color.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief フォグボリュームの形状
enum class FogVolumeShape : uint32_t
{
    Box    = 0, ///< ボックス形状
    Sphere = 1, ///< スフィア形状
};

/// @brief 局所的なフォグボリューム定義
///
/// シーン内の任意の位置に配置できるフォグ領域。BoxまたはSphere形状で
/// 密度・色・吸収率・散乱異方性パラメータを個別に設定可能。
struct FogVolume
{
    FogVolumeShape shape = FogVolumeShape::Box; ///< フォグ領域の形状
    XMFLOAT3 position    = { 0.0f, 0.0f, 0.0f }; ///< ワールド空間の中心位置
    XMFLOAT3 extent      = { 5.0f, 5.0f, 5.0f };  ///< 半径/半サイズ (Box: XYZ半径, Sphere: X=半径)
    float    density     = 1.0f;                   ///< 散乱密度 (高いほど濃い)
    Color    color       = { 1.0f, 1.0f, 1.0f, 1.0f }; ///< フォグの色
    float    absorption  = 0.1f;                   ///< 吸収率 (光がどの程度吸収されるか)
    float    scatteringAnisotropy = 0.3f;          ///< 散乱異方性 [-1, 1] (Henyey-Greenstein位相関数のg)
};

/// @brief ボリューメトリックフォグのグローバル設定
struct VolumetricFogSettings
{
    bool     enabled          = false;             ///< エフェクト有効フラグ
    float    maxDistance       = 200.0f;            ///< フォグの最大到達距離
    uint32_t resolutionX      = 160;               ///< Froxelグリッド X解像度
    uint32_t resolutionY      = 90;                ///< Froxelグリッド Y解像度
    uint32_t resolutionZ      = 64;                ///< Froxelグリッド Z解像度 (奥行きスライス数)
    float    globalDensity    = 0.02f;             ///< グローバル均一フォグ密度
    float    globalAbsorption = 0.01f;             ///< グローバル吸収率
};

/// @brief Froxelグリッドベースのボリューメトリックフォグ
///
/// 3段階のComputeShader/PixelShaderパスでフォグを生成する:
/// - Inject CS: FogVolumeおよびグローバル密度をFroxelに書き込み
/// - Scatter CS: フロント→バックのBeer-Lambertレイマーチ
/// - Compose PS: HDRシーンにフォグ結果を合成
class VolumetricFog
{
public:
    VolumetricFog() = default;
    ~VolumetricFog() = default;

    /// @brief 初期化。3Dテクスチャ・PSO・定数バッファを作成する
    /// @param device D3D12デバイス (nullptrの場合はCPUのみ初期化)
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief リソースを解放する
    void Shutdown();

    /// @brief フォグボリュームを追加する
    /// @param volume フォグボリューム定義
    /// @return ボリュームID (RemoveFogVolumeに使用)
    int AddFogVolume(const FogVolume& volume);

    /// @brief フォグボリュームを削除する
    /// @param id AddFogVolumeの戻り値
    void RemoveFogVolume(int id);

    /// @brief グローバル設定を更新する
    void SetSettings(const VolumetricFogSettings& settings) { m_settings = settings; }

    /// @brief 現在のグローバル設定を取得する
    const VolumetricFogSettings& GetSettings() const { return m_settings; }

    /// @brief フォグの全パスを実行する (Inject → Scatter → Compose)
    /// @param cmdList コマンドリスト
    /// @param camera カメラ (逆VP行列の計算に使用)
    /// @param depthSRV 深度バッファSRV
    /// @param lightConstants ライト定数 (散乱ライティング用)
    /// @param frameIndex ダブルバッファ用フレームインデックス
    void Execute(ID3D12GraphicsCommandList* cmdList,
                 const XMMATRIX& invViewProj, const XMFLOAT3& cameraPos,
                 ID3D12Resource* depthSRV, const void* lightConstants,
                 uint32_t frameIndex);

    /// @brief 登録されたフォグボリューム数を取得する
    uint32_t GetFogVolumeCount() const { return static_cast<uint32_t>(m_fogVolumes.size()); }

    /// @brief エフェクトが有効かどうか
    bool IsEnabled() const { return m_settings.enabled; }

private:
    VolumetricFogSettings m_settings;              ///< グローバル設定
    std::vector<FogVolume> m_fogVolumes;           ///< 登録されたフォグボリューム
    bool m_initialized = false;                    ///< 初期化完了フラグ

    ID3D12Device* m_device = nullptr;              ///< D3D12デバイス

    // 3Dテクスチャ (Froxelグリッド)
    ComPtr<ID3D12Resource> m_fogVolumeTexture;     ///< R16G16B16A16_FLOAT 3Dテクスチャ

    // パイプライン (Inject CS, Scatter CS, Compose PS)
    Shader m_injectShader;                         ///< Inject用コンピュートシェーダー
    Shader m_scatterShader;                        ///< Scatter用コンピュートシェーダー
    Shader m_composeShader;                        ///< Compose用ピクセルシェーダー
    ComPtr<ID3D12RootSignature> m_injectRS;        ///< Inject用ルートシグネチャ
    ComPtr<ID3D12RootSignature> m_scatterRS;       ///< Scatter用ルートシグネチャ
    ComPtr<ID3D12RootSignature> m_composeRS;       ///< Compose用ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_injectPSO;       ///< Inject用パイプラインステート
    ComPtr<ID3D12PipelineState> m_scatterPSO;      ///< Scatter用パイプラインステート
    ComPtr<ID3D12PipelineState> m_composePSO;      ///< Compose用パイプラインステート

    // 定数バッファ
    DynamicBuffer m_injectCB;                      ///< Inject用定数バッファ
    DynamicBuffer m_scatterCB;                     ///< Scatter用定数バッファ
    DynamicBuffer m_composeCB;                     ///< Compose用定数バッファ

    uint32_t m_width  = 0;                         ///< 画面幅
    uint32_t m_height = 0;                         ///< 画面高さ
};

/// @}
} // namespace gx
