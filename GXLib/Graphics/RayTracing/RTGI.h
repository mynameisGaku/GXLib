#pragma once
/// @file RTGI.h
/// @brief DXR グローバルイルミネーション (間接照明)
///
/// DxLibには無い機能。DXRレイトレーシングで間接ディフューズ照明を計算する。
/// 赤い壁の反射光が近くの白い物体を赤く染める「カラーブリーディング」等を再現。
/// 1sppコサイン半球サンプリング→テンポラル蓄積→A-Trousデノイズの3段構成。
/// RTReflectionsのBLAS/TLASインフラを共有可能 (Gキーでトグル)。

#include "pch_graphics.h"
#include "Graphics/RayTracing/RTAccelerationStructure.h"
#include "Graphics/RayTracing/RTPipeline.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Light.h"

namespace gx
{
/// @addtogroup grp_gfx_rt
/// @{

/// @brief GI用定数バッファ (256B align)
struct RTGIConstants
{
    XMFLOAT4X4 invViewProjection;  ///< 逆VP行列（ワールド座標復元用）
    XMFLOAT4X4 view;               ///< ビュー行列
    XMFLOAT4X4 invProjection;      ///< 逆射影行列
    XMFLOAT3   cameraPosition;     ///< カメラ位置（ワールド空間）
    float      maxDistance;         ///< GIレイの最大飛距離
    float      screenWidth;        ///< スクリーン幅（ピクセル）
    float      screenHeight;       ///< スクリーン高さ（ピクセル）
    float      halfWidth;          ///< 半解像度幅（ピクセル）
    float      halfHeight;         ///< 半解像度高さ（ピクセル）
    XMFLOAT3   skyTopColor;        ///< 空の天頂色（Missシェーダー用）
    float      frameIndex;         ///< フレーム番号（ノイズ生成用）
    XMFLOAT3   skyBottomColor;     ///< 空の地平色（Missシェーダー用）
    float      _pad1;              ///< パディング
};  // 256B

/// @brief テンポラル蓄積パス定数
struct RTGITemporalConstants
{
    XMFLOAT4X4 prevViewProjection; ///< 前フレームVP行列（リプロジェクション用）
    XMFLOAT4X4 invViewProjection;  ///< 現フレーム逆VP行列
    float      alpha;              ///< 新フレーム混合比率（小さいほど安定）
    float      frameCount;         ///< 累積フレーム数
    float      fullWidth;          ///< フル解像度幅（ピクセル）
    float      fullHeight;         ///< フル解像度高さ（ピクセル）
};  // 144B → 256B aligned

/// @brief 空間フィルタパス定数
struct RTGISpatialConstants
{
    float fullWidth;    ///< フル解像度幅（ピクセル）
    float fullHeight;   ///< フル解像度高さ（ピクセル）
    float stepWidth;    ///< A-Trousフィルタのステップ幅
    float sigmaDepth;   ///< 深度の重み係数
    float sigmaNormal;  ///< 法線の重み係数
    float sigmaColor;   ///< 色の重み係数
    float _pad[2];      ///< パディング
};  // 32B → 256B aligned

/// @brief コンポジットパス定数
struct RTGICompositeConstants
{
    float intensity;    ///< GI合成強度
    float debugMode;    ///< デバッグ表示モード (0=オフ)
    float fullWidth;    ///< フル解像度幅（ピクセル）
    float fullHeight;   ///< フル解像度高さ（ピクセル）
};  // 16B → 256B aligned

/// @brief DXRレイトレでグローバルイルミネーション(間接照明)を計算するクラス
///
/// 半解像度でコサイン半球1sppレイをディスパッチし、テンポラル蓄積(フル解像度)と
/// A-Trous空間フィルタ(最大8反復)でデノイズ後、HDRシーンに加算合成する。
class RTGI
{
public:
    RTGI() = default;
    ~RTGI() = default;

    /// @brief 初期化。DXRパイプライン・デノイズRT・コンポジットPSOを作成する
    /// @param device DXR対応デバイス (ID3D12Device5)
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device5* device, uint32_t width, uint32_t height);

    /// @brief RTReflectionsのBLAS/TLASを共有して二重構築を回避する
    void SetSharedAccelerationStructure(RTAccelerationStructure* shared);

    /// @brief 自前のAccelerationStructureを取得 (共有しない場合)
    RTAccelerationStructure& GetAccelStruct() { return m_ownAccelStruct; }

    /// @brief フレーム開始。インスタンスリストをクリアする
    void BeginFrame();

    /// @brief BLASを構築
    int BuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                  ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                  ID3D12Resource* ib, uint32_t indexCount,
                  DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);

    /// @brief ポインタベースBLASキャッシュ
    int FindOrBuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                        ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                        ID3D12Resource* ib, uint32_t indexCount,
                        DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);

    /// @brief TLASにインスタンスを追加
    void AddInstance(int blasIndex, const XMMATRIX& worldMatrix,
                     const XMFLOAT3& albedo = { 0.5f, 0.5f, 0.5f },
                     float metallic = 0.0f, float roughness = 0.5f,
                     ID3D12Resource* albedoTex = nullptr,
                     uint32_t instanceFlags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE);

    /// @brief ジオメトリSRV作成 (全BLAS構築後)
    void CreateGeometrySRVs();

    /// @brief ライト設定
    void SetLights(const LightData* lights, uint32_t count, const XMFLOAT3& ambient);

    /// @brief 空の色設定
    void SetSkyColors(const XMFLOAT3& top, const XMFLOAT3& bottom);

    /// @brief GIの全4パス (レイディスパッチ→テンポラル→空間フィルタ→コンポジット) を実行
    /// @param cmdList4 DXR対応コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT (GI加算済み)
    /// @param depth 深度バッファ
    /// @param camera カメラ
    /// @param albedoRT アルベドRT (PBR.hlslのMRT SV_Target2で書かれたもの)
    void Execute(ID3D12GraphicsCommandList4* cmdList4, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera,
                 RenderTarget& albedoRT);

    /// @brief GBuffer法線RT (外部所有) を設定
    void SetNormalRT(RenderTarget* rt) { m_normalRT = rt; }
    void SetEnabled(bool e) { m_enabled = e; }
    bool IsEnabled() const { return m_enabled; }
    /// @brief GI合成の強度。大きいほど間接光が強くなる
    void SetIntensity(float i) { m_intensity = i; }
    float GetIntensity() const { return m_intensity; }
    /// @brief GIレイの最大飛距離
    void SetMaxDistance(float d) { m_maxDistance = d; }
    float GetMaxDistance() const { return m_maxDistance; }
    /// @brief テンポラル蓄積の新フレーム比率 (小さいほど過去フレームを重視)
    void SetTemporalAlpha(float a) { m_temporalAlpha = a; }
    float GetTemporalAlpha() const { return m_temporalAlpha; }
    /// @brief A-Trous空間フィルタの反復数 (最大8)。多いほどノイズが減る
    void SetSpatialIterations(int n) { m_spatialIterations = n; }
    int GetSpatialIterations() const { return m_spatialIterations; }
    /// @brief デバッグ表示モード (0=オフ, 1=GIのみ表示)
    void SetDebugMode(int mode) { m_debugMode = mode; }
    int GetDebugMode() const { return m_debugMode; }

    /// @brief 画面リサイズ対応
    void OnResize(ID3D12Device* device, uint32_t w, uint32_t h);

private:
    bool CreateDenoisePipelines(ID3D12Device* device);
    bool CreateCompositePipeline(ID3D12Device* device);
    void CreateHalfResUAV();
    void CreateTemporalResources();
    void CreateSpatialResources();

    bool m_enabled = false;              ///< 有効フラグ
    float m_maxDistance = 30.0f;        ///< GIレイの最大飛距離
    float m_intensity = 1.0f;           ///< GI合成強度
    float m_temporalAlpha = 0.05f;      ///< テンポラル蓄積の新フレーム比率
    int   m_spatialIterations = 5;      ///< A-Trous空間フィルタの反復数
    int   m_debugMode = 0;              ///< デバッグ表示モード (0=オフ)

    uint32_t m_width  = 0;              ///< 画面幅（ピクセル）
    uint32_t m_height = 0;              ///< 画面高さ（ピクセル）
    uint32_t m_halfWidth  = 0;          ///< 半解像度幅（ピクセル）
    uint32_t m_halfHeight = 0;          ///< 半解像度高さ（ピクセル）

    ID3D12Device5* m_device5 = nullptr; ///< DXR対応デバイス

    // DXRコア
    RTAccelerationStructure m_ownAccelStruct;          ///< 自前のBLAS/TLAS管理
    RTAccelerationStructure* m_accelStruct = nullptr;  ///< 使用するBLAS/TLAS（共有 or 自前）
    RTPipeline m_giPipeline;                           ///< GI用DXRパイプライン

    // 半解像度GI UAV出力
    ComPtr<ID3D12Resource> m_giUAV;                    ///< 半解像度GI結果UAV
    D3D12_RESOURCE_STATES  m_giUAVState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; ///< UAVリソース状態

    // テンポラル蓄積用 history (フル解像度, ダブルバッファ)
    RenderTarget m_temporalHistory[2];                 ///< テンポラル蓄積ヒストリRT（ダブルバッファ）
    uint32_t m_temporalWriteIdx = 0;                   ///< テンポラル書き込みインデックス

    // 空間フィルタ用 ping-pong (フル解像度)
    RenderTarget m_spatialPingPong[2];                 ///< 空間フィルタ用ピンポンRT

    // ディスパッチ用ヒープ
    DescriptorHeap m_dispatchHeap;                     ///< DispatchRays用SRV/UAVヒープ

    // 定数バッファ
    DynamicBuffer m_cb;                                ///< GI定数バッファ
    DynamicBuffer m_lightCB;                           ///< ライト定数バッファ
    LightConstants m_lightConstants = {};               ///< ライト定数データ

    // テンポラルパス
    Shader m_denoiseShader;                            ///< デノイズシェーダー
    ComPtr<ID3D12RootSignature> m_temporalRS;          ///< テンポラルパス用ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_temporalPSO;         ///< テンポラルパス用PSO
    DynamicBuffer m_temporalCB;                        ///< テンポラルパス定数バッファ
    DescriptorHeap m_temporalHeap;                     ///< テンポラルパス用SRVヒープ

    // 空間フィルタパス
    ComPtr<ID3D12RootSignature> m_spatialRS;           ///< 空間フィルタ用ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_spatialPSO;          ///< 空間フィルタ用PSO
    DynamicBuffer m_spatialCB;                         ///< 空間フィルタ定数バッファ
    static constexpr int k_MaxSpatialIter = 8;         ///< サポートする最大反復数
    DescriptorHeap m_spatialHeap;                      ///< 空間フィルタ用SRVヒープ

    // コンポジットパス
    Shader m_compositeShader;                          ///< コンポジットシェーダー
    ComPtr<ID3D12RootSignature> m_compositeRS;         ///< コンポジット用ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_compositePSO;        ///< コンポジット用PSO
    DynamicBuffer m_compositeCB;                       ///< コンポジット定数バッファ
    DescriptorHeap m_compositeHeap;                    ///< コンポジット用SRVヒープ

    // 空情報
    XMFLOAT3 m_skyTopColor  = { 0.5f, 0.7f, 1.0f };   ///< 空の天頂色
    XMFLOAT3 m_skyBottomColor = { 0.8f, 0.9f, 1.0f };  ///< 空の地平色

    RenderTarget* m_normalRT = nullptr;  ///< GBuffer法線RT (外部所有)

    // BLASキャッシュ
    std::unordered_map<ID3D12Resource*, int> m_blasLookup; ///< VBポインタ→BLASインデックス検索用

    /// @brief BLAS毎のジオメトリ情報
    struct BLASGeometryInfo {
        ComPtr<ID3D12Resource> vb;     ///< 頂点バッファリソース
        ComPtr<ID3D12Resource> ib;     ///< インデックスバッファリソース
        uint32_t vertexStride = 0;     ///< 頂点ストライド（バイト）
    };
    std::vector<BLASGeometryInfo> m_blasGeometry; ///< BLAS毎のジオメトリ情報配列

    // ディスパッチヒープレイアウト
    static constexpr uint32_t k_GeomSlotsBase    = 8;   ///< ジオメトリSRV開始スロット
    static constexpr uint32_t k_GeomSlotsCount   = 32;  ///< ジオメトリSRVスロット数
    static constexpr uint32_t k_TextureSlotsBase = 40;  ///< テクスチャSRV開始スロット
    static constexpr uint32_t k_MaxTextures      = 32;  ///< テクスチャSRV最大数
    static constexpr uint32_t k_PerFrameBase     = 72;  ///< フレーム毎SRV/UAV開始スロット
    static constexpr uint32_t k_PerFrameCount    = 4;   ///< フレーム毎SRV/UAV数
    static constexpr uint32_t k_DispatchHeapSize = 80;  ///< ディスパッチヒープ総スロット数
    std::unordered_map<ID3D12Resource*, uint32_t> m_textureLookup; ///< テクスチャ→スロット検索用
    std::vector<ComPtr<ID3D12Resource>> m_textureResources;        ///< テクスチャリソース配列
    uint32_t m_nextTextureSlot = 0;  ///< 次に割り当てるテクスチャスロット

    // per-instance PBRデータ
    static constexpr uint32_t k_MaxInstances = 512; ///< インスタンス最大数
    DynamicBuffer m_instanceDataCB;                  ///< インスタンスPBRデータ定数バッファ

    /// @brief インスタンス毎のPBRマテリアル情報
    struct InstancePBR {
        XMFLOAT4 albedoMetallic;  ///< .rgb=アルベド, .a=メタリック
        XMFLOAT4 roughnessGeom;   ///< .x=ラフネス, .y=ジオメトリインデックス, .z=テクスチャインデックス, .w=テクスチャ有無
        XMFLOAT4 extraData;       ///< .x=頂点ストライド, .yzw=予約
    };
    std::vector<InstancePBR> m_instanceData; ///< インスタンスPBRデータ配列

    // テンポラル用
    XMFLOAT4X4 m_previousVP = {};            ///< 前フレームVP行列（リプロジェクション用）
    uint32_t m_frameCount = 0;               ///< 累積フレーム数

    ComPtr<ID3D12Resource> m_prevDepthCopy;   ///< 前フレーム深度コピー（リプロジェクション用）
};

/// @}
} // namespace gx
