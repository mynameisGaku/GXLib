#pragma once
/// @file RTGI.h
/// @brief DXR グローバルイルミネーション (間接照明)
///
/// DxLibには無い機能。DXRレイトレーシングで間接ディフューズ照明を計算する。
/// 赤い壁の反射光が近くの白い物体を赤く染める「カラーブリーディング」等を再現。
/// 1sppコサイン半球サンプリング→テンポラル蓄積→A-Trousデノイズの3段構成。
/// RTReflectionsのBLAS/TLASインフラを共有可能 (Gキーでトグル)。

#include "pch.h"
#include "Graphics/RayTracing/RTAccelerationStructure.h"
#include "Graphics/RayTracing/RTPipeline.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Light.h"

namespace GX
{

/// GI用定数バッファ (256B align)
struct RTGIConstants
{
    XMFLOAT4X4 invViewProjection;  // 0
    XMFLOAT4X4 view;               // 64
    XMFLOAT4X4 invProjection;      // 128
    XMFLOAT3   cameraPosition;     // 192
    float      maxDistance;         // 204
    float      screenWidth;        // 208
    float      screenHeight;       // 212
    float      halfWidth;          // 216
    float      halfHeight;         // 220
    XMFLOAT3   skyTopColor;        // 224
    float      frameIndex;         // 236
    XMFLOAT3   skyBottomColor;     // 240
    float      _pad1;              // 252
};  // 256B

/// テンポラル蓄積パス定数
struct RTGITemporalConstants
{
    XMFLOAT4X4 prevViewProjection; // 0
    XMFLOAT4X4 invViewProjection;  // 64
    float      alpha;              // 128
    float      frameCount;         // 132
    float      fullWidth;          // 136
    float      fullHeight;         // 140
};  // 144B → 256B aligned

/// 空間フィルタパス定数
struct RTGISpatialConstants
{
    float fullWidth;    // 0
    float fullHeight;   // 4
    float stepWidth;    // 8
    float sigmaDepth;   // 12
    float sigmaNormal;  // 16
    float sigmaColor;   // 20
    float _pad[2];      // 24
};  // 32B → 256B aligned

/// コンポジットパス定数
struct RTGICompositeConstants
{
    float intensity;    // 0
    float debugMode;    // 4
    float fullWidth;    // 8
    float fullHeight;   // 12
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

    bool m_enabled = false;
    float m_maxDistance = 30.0f;
    float m_intensity = 1.0f;
    float m_temporalAlpha = 0.05f;
    int   m_spatialIterations = 5;
    int   m_debugMode = 0;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;
    uint32_t m_halfWidth  = 0;
    uint32_t m_halfHeight = 0;

    ID3D12Device5* m_device5 = nullptr;

    // DXRコア
    RTAccelerationStructure m_ownAccelStruct;
    RTAccelerationStructure* m_accelStruct = nullptr;
    RTPipeline m_giPipeline;

    // 半解像度GI UAV出力
    ComPtr<ID3D12Resource> m_giUAV;
    D3D12_RESOURCE_STATES  m_giUAVState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    // テンポラル蓄積用 history (フル解像度, ダブルバッファ)
    RenderTarget m_temporalHistory[2];
    uint32_t m_temporalWriteIdx = 0;

    // 空間フィルタ用 ping-pong (フル解像度)
    RenderTarget m_spatialPingPong[2];

    // ディスパッチ用ヒープ (RTReflections同等レイアウト)
    DescriptorHeap m_dispatchHeap;

    // 定数バッファ
    DynamicBuffer m_cb;
    DynamicBuffer m_lightCB;
    LightConstants m_lightConstants = {};

    // テンポラルパス
    Shader m_denoiseShader;
    ComPtr<ID3D12RootSignature> m_temporalRS;
    ComPtr<ID3D12PipelineState> m_temporalPSO;
    DynamicBuffer m_temporalCB;
    DescriptorHeap m_temporalHeap;  // [0..3]×2frames: currentGI, history, depth, prevDepth

    // 空間フィルタパス
    ComPtr<ID3D12RootSignature> m_spatialRS;
    ComPtr<ID3D12PipelineState> m_spatialPSO;
    DynamicBuffer m_spatialCB;
    static constexpr int k_MaxSpatialIter = 8;  // max spatial iterations supported
    DescriptorHeap m_spatialHeap;  // [0..2]×k_MaxSpatialIter: input, depth, normal

    // コンポジットパス
    Shader m_compositeShader;
    ComPtr<ID3D12RootSignature> m_compositeRS;
    ComPtr<ID3D12PipelineState> m_compositePSO;
    DynamicBuffer m_compositeCB;
    DescriptorHeap m_compositeHeap;  // [0..3]×2frames: scene, gi, depth, albedo

    // 空情報
    XMFLOAT3 m_skyTopColor  = { 0.5f, 0.7f, 1.0f };
    XMFLOAT3 m_skyBottomColor = { 0.8f, 0.9f, 1.0f };

    // GBuffer法線RT (外部所有)
    RenderTarget* m_normalRT = nullptr;

    // BLASキャッシュ
    std::unordered_map<ID3D12Resource*, int> m_blasLookup;

    struct BLASGeometryInfo {
        ComPtr<ID3D12Resource> vb;
        ComPtr<ID3D12Resource> ib;
        uint32_t vertexStride = 0;
    };
    std::vector<BLASGeometryInfo> m_blasGeometry;

    // ディスパッチヒープレイアウト
    static constexpr uint32_t k_GeomSlotsBase    = 8;
    static constexpr uint32_t k_GeomSlotsCount   = 32;
    static constexpr uint32_t k_TextureSlotsBase = 40;
    static constexpr uint32_t k_MaxTextures      = 32;
    static constexpr uint32_t k_PerFrameBase     = 72;
    static constexpr uint32_t k_PerFrameCount    = 4;
    static constexpr uint32_t k_DispatchHeapSize = 80;
    std::unordered_map<ID3D12Resource*, uint32_t> m_textureLookup;
    std::vector<ComPtr<ID3D12Resource>> m_textureResources;
    uint32_t m_nextTextureSlot = 0;

    // per-instance PBRデータ
    static constexpr uint32_t k_MaxInstances = 512;
    DynamicBuffer m_instanceDataCB;

    struct InstancePBR {
        XMFLOAT4 albedoMetallic;
        XMFLOAT4 roughnessGeom;
        XMFLOAT4 extraData;
    };
    std::vector<InstancePBR> m_instanceData;

    // テンポラル用
    XMFLOAT4X4 m_previousVP = {};
    uint32_t m_frameCount = 0;

    // 前フレーム深度 (リプロジェクション用)
    ComPtr<ID3D12Resource> m_prevDepthCopy;
};

} // namespace GX
