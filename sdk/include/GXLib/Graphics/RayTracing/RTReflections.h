#pragma once
/// @file RTReflections.h
/// @brief DXR レイトレーシング反射エフェクト
///
/// DxLibには無い機能。DXRのハードウェアレイトレーシングで正確な反射を生成する。
/// SSRと排他的に使用 (Y/Rキーで切替)。DXR非対応GPUではSSRにフォールバック。
/// 内部にRTAccelerationStructure + RTPipelineを所有し、フル解像度でディスパッチする。

#include "pch_graphics.h"
#include "Math/Vector4.h"
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

/// @brief RT反射用定数バッファ (256B align)
struct RTReflectionConstants
{
    Matrix4x4 invViewProjection;  ///< 逆VP行列（ワールド座標復元用）
    Matrix4x4 view;               ///< ビュー行列
    Matrix4x4 invProjection;      ///< 逆射影行列
    Vector3   cameraPosition;     ///< カメラ位置（ワールド空間）
    float      maxDistance;         ///< レイの最大飛距離
    float      screenWidth;        ///< スクリーン幅（ピクセル）
    float      screenHeight;       ///< スクリーン高さ（ピクセル）
    float      debugMode;          ///< デバッグ表示モード (0=オフ)
    float      intensity;          ///< 反射強度
    Vector3   skyTopColor;        ///< 空の天頂色（Missシェーダー用）
    uint32_t   shadowSamples;     ///< シャドウサンプル数 (1-8)
    Vector3   skyBottomColor;     ///< 空の地平色（Missシェーダー用）
    float      _pad1;              ///< パディング
};  // 256B

/// @brief コンポジットパス定数 (Fresnel計算用)
struct RTCompositeConstants
{
    float      intensity;          ///< 反射強度
    float      debugMode;          ///< デバッグ表示モード
    float      screenWidth;        ///< スクリーン幅（ピクセル）
    float      screenHeight;       ///< スクリーン高さ（ピクセル）
    Vector3   cameraPosition;     ///< カメラ位置（ワールド空間）
    float      _pad0;              ///< パディング
    Matrix4x4 invViewProjection;  ///< 逆VP行列（法線復元用）
};  // 96B → fits in 256B slot

/// @brief DXR反射エフェクト。ハードウェアレイトレで正確な映り込みを生成する
///
/// BLAS/TLASを毎フレーム構築し、DispatchRaysで反射テクスチャを生成、
/// フルスクリーン三角形のコンポジットパスでHDRシーンに合成する。
/// PBRマテリアル情報(albedo/metallic/roughness)をper-instanceデータとして渡す。
class RTReflections
{
public:
    RTReflections() = default;
    ~RTReflections() = default;

    /// @brief 初期化。DXR対応GPU (ID3D12Device5) でのみ成功する
    /// @param device DXR対応デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device5* device, uint32_t width, uint32_t height);

    /// @brief BLASを構築（初期化時に全メッシュ分呼ぶ）
    /// @return BLASインデックス
    int BuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                  ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                  ID3D12Resource* ib, uint32_t indexCount,
                  DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);

    /// @brief ポインタベースBLASキャッシュ（同じVBなら再ビルドしない）
    int FindOrBuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                        ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                        ID3D12Resource* ib, uint32_t indexCount,
                        DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);

    /// @brief フレーム開始時に呼ぶ
    void BeginFrame();

    /// @brief TLASにインスタンスを追加 (PBRマテリアル情報付き)
    /// @param albedoTex テクスチャリソース (nullならalbedo定数のみ)
    void AddInstance(int blasIndex, const XMMATRIX& worldMatrix,
                     const Vector3& albedo = { 0.5f, 0.5f, 0.5f },
                     float metallic = 0.0f, float roughness = 0.5f,
                     ID3D12Resource* albedoTex = nullptr,
                     uint32_t instanceFlags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE);

    /// @brief PBR.hlslと同一のライト配列を設定
    void SetLights(const LightData* lights, uint32_t count, const Vector3& ambient);

    /// @brief 空の色を設定 (Missシェーダー用)
    void SetSkyColors(const Vector3& top, const Vector3& bottom);

    /// @brief RT反射を実行してシーンに合成
    /// @param cmdList4 DXR対応コマンドリスト
    /// @param frameIndex フレームインデックス
    /// @param srcHDR 入力HDR RT
    /// @param destHDR 出力HDR RT
    /// @param depth 深度バッファ
    /// @param camera カメラ
    void Execute(ID3D12GraphicsCommandList4* cmdList4, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    /// @brief ジオメトリSRV (VB/IB ByteAddressBuffer) を作成。全BLAS構築後に呼ぶ。
    void CreateGeometrySRVs();

    /// @brief リサイズ
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief レイの最大飛距離
    void SetMaxDistance(float d) { m_maxDistance = d; }
    float GetMaxDistance() const { return m_maxDistance; }

    /// @brief 反射の強度
    void SetIntensity(float i) { m_intensity = i; }
    float GetIntensity() const { return m_intensity; }

    /// @brief GBuffer法線RT (外部所有) を設定。コンポジットパスで使用
    void SetNormalRT(RenderTarget* rt) { m_normalRT = rt; }

    /// @brief デバッグ表示モード (0=オフ, 1=反射のみ表示)
    void SetDebugMode(int mode) { m_debugMode = mode; }
    int GetDebugMode() const { return m_debugMode; }

    /// @brief シャドウサンプル数 (1-8)
    void SetShadowSamples(uint32_t count) { m_shadowSamples = (std::min)(count, 8u); }
    uint32_t GetShadowSamples() const { return m_shadowSamples; }

private:
    bool CreateCompositePipeline(ID3D12Device* device);

    bool m_enabled = false;            ///< 有効フラグ
    float m_maxDistance = 50.0f;      ///< レイの最大飛距離
    float m_intensity = 1.0f;         ///< 反射強度
    int   m_debugMode = 0;            ///< デバッグ表示モード (0=オフ)
    uint32_t m_shadowSamples = 4;    ///< シャドウサンプル数 (default 4)
    bool m_firstExecution = true;    ///< 初回実行フラグ (warm-up用)

    uint32_t m_width  = 0;            ///< 画面幅（ピクセル）
    uint32_t m_height = 0;            ///< 画面高さ（ピクセル）

    ID3D12Device5* m_device5 = nullptr; ///< DXR対応デバイス

    // DXRコア
    RTAccelerationStructure m_accelStruct; ///< BLAS/TLAS管理
    RTPipeline m_rtPipeline;               ///< DXRパイプライン

    // フル解像度 UAV テクスチャ (RT出力先)
    ComPtr<ID3D12Resource> m_reflectionUAV;  ///< 反射結果UAVテクスチャ
    D3D12_RESOURCE_STATES  m_reflectionState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; ///< UAVリソース状態

    // ディスパッチ用ディスクリプタヒープ
    DescriptorHeap m_dispatchHeap;   ///< DispatchRays用SRV/UAVヒープ

    // 定数バッファ
    DynamicBuffer m_cb;              ///< RT反射定数バッファ

    // ライト定数バッファ (RTReflections.hlsl b2 と同一レイアウト, 16ライト)
    ShaderLightConstants m_rtLightConstants = {};  ///< RT用ライト定数データ
    DynamicBuffer m_lightCB;               ///< ライト定数バッファ

    // コンポジットパス (フルスクリーン三角形)
    Shader m_compositeShader;                        ///< コンポジットシェーダー
    ComPtr<ID3D12RootSignature> m_compositeRS;       ///< コンポジット用ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_compositePSO;      ///< コンポジット用PSO
    DynamicBuffer m_compositeCB;                     ///< コンポジット定数バッファ
    DescriptorHeap m_compositeHeap;                  ///< コンポジット用SRVヒープ (Scene/Depth/Reflection/Normal ×2フレーム)

    // 空情報 (Missシェーダー用)
    Vector3 m_skyTopColor  = { 0.5f, 0.7f, 1.0f };   ///< 空の天頂色
    Vector3 m_skyBottomColor = { 0.8f, 0.9f, 1.0f };  ///< 空の地平色

    RenderTarget* m_normalRT = nullptr;  ///< GBuffer法線RT (外部所有)

    // BLASキャッシュ (VBポインタ → BLASインデックス)
    gx::HashMap<ID3D12Resource*, int> m_blasLookup; ///< VBポインタ→BLASインデックス検索用

    /// @brief BLAS毎のジオメトリ情報 (ClosestHitでByteAddressBufferとしてアクセス)
    struct BLASGeometryInfo {
        ComPtr<ID3D12Resource> vb;     ///< 頂点バッファリソース
        ComPtr<ID3D12Resource> ib;     ///< インデックスバッファリソース
        uint32_t vertexStride = 0;     ///< 頂点ストライド（バイト）
    };
    gx::Vector<BLASGeometryInfo> m_blasGeometry; ///< BLAS毎のジオメトリ情報配列

    // ディスパッチヒープレイアウト定数
    static constexpr uint32_t k_GeomSlotsBase    = 8;   ///< ジオメトリSRV開始スロット
    static constexpr uint32_t k_GeomSlotsCount   = 32;  ///< ジオメトリSRVスロット数
    static constexpr uint32_t k_TextureSlotsBase = 40;  ///< テクスチャSRV開始スロット
    static constexpr uint32_t k_MaxTextures      = 32;  ///< テクスチャSRV最大数
    static constexpr uint32_t k_PerFrameBase     = 72;  ///< フレーム毎SRV/UAV開始スロット
    static constexpr uint32_t k_PerFrameCount    = 4;   ///< フレーム毎SRV/UAV数
    static constexpr uint32_t k_DispatchHeapSize = 80;  ///< ディスパッチヒープ総スロット数
    gx::HashMap<ID3D12Resource*, uint32_t> m_textureLookup; ///< テクスチャ→スロット検索用
    gx::Vector<ComPtr<ID3D12Resource>> m_textureResources;        ///< テクスチャリソース配列
    uint32_t m_nextTextureSlot = 0;  ///< 次に割り当てるテクスチャスロット

    // per-instance PBRデータ (ClosestHitシェーダー用)
    static constexpr uint32_t k_MaxInstances = 512; ///< インスタンス最大数
    DynamicBuffer m_instanceDataCB;                  ///< インスタンスPBRデータ定数バッファ

    /// @brief インスタンス毎のPBRマテリアル情報
    struct InstancePBR {
        Vector4 albedoMetallic;  ///< .rgb=アルベド, .a=メタリック
        Vector4 roughnessGeom;   ///< .x=ラフネス, .y=ジオメトリインデックス, .z=テクスチャインデックス, .w=テクスチャ有無
        Vector4 extraData;       ///< .x=頂点ストライド, .yzw=予約
    };
    gx::Vector<InstancePBR> m_instanceData; ///< インスタンスPBRデータ配列
};

/// @}
} // namespace gx
