#pragma once
/// @file RTReflections.h
/// @brief DXR レイトレーシング反射エフェクト
///
/// DxLibには無い機能。DXRのハードウェアレイトレーシングで正確な反射を生成する。
/// SSRと排他的に使用 (Y/Rキーで切替)。DXR非対応GPUではSSRにフォールバック。
/// 内部にRTAccelerationStructure + RTPipelineを所有し、フル解像度でディスパッチする。

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

/// RT反射用定数バッファ (256B align)
struct RTReflectionConstants
{
    XMFLOAT4X4 invViewProjection;  // 0
    XMFLOAT4X4 view;               // 64
    XMFLOAT4X4 invProjection;      // 128
    XMFLOAT3   cameraPosition;     // 192
    float      maxDistance;         // 204
    float      screenWidth;        // 208
    float      screenHeight;       // 212
    float      debugMode;          // 216
    float      intensity;          // 220
    XMFLOAT3   skyTopColor;        // 224
    float      _pad0;              // 236
    XMFLOAT3   skyBottomColor;     // 240
    float      _pad1;              // 252
};  // 256B

/// コンポジットパス定数 (Fresnel計算用)
struct RTCompositeConstants
{
    float      intensity;          // 0
    float      debugMode;          // 4
    float      screenWidth;        // 8
    float      screenHeight;       // 12
    XMFLOAT3   cameraPosition;     // 16
    float      _pad0;              // 28
    XMFLOAT4X4 invViewProjection;  // 32
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
                     const XMFLOAT3& albedo = { 0.5f, 0.5f, 0.5f },
                     float metallic = 0.0f, float roughness = 0.5f,
                     ID3D12Resource* albedoTex = nullptr,
                     uint32_t instanceFlags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE);

    /// @brief PBR.hlslと同一のライト配列を設定
    void SetLights(const LightData* lights, uint32_t count, const XMFLOAT3& ambient);

    /// @brief 空の色を設定 (Missシェーダー用)
    void SetSkyColors(const XMFLOAT3& top, const XMFLOAT3& bottom);

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

private:
    bool CreateCompositePipeline(ID3D12Device* device);

    bool m_enabled = false;
    float m_maxDistance = 50.0f;
    float m_intensity = 0.3f;
    int   m_debugMode = 0;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    ID3D12Device5* m_device5 = nullptr;

    // DXRコア
    RTAccelerationStructure m_accelStruct;
    RTPipeline m_rtPipeline;

    // フル解像度 UAV テクスチャ (RT出力先)
    ComPtr<ID3D12Resource> m_reflectionUAV;
    D3D12_RESOURCE_STATES  m_reflectionState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    // ディスパッチ用ディスクリプタヒープ
    // [0..7]=geometry VB/IB, [8..39]=geometry VB/IB, [40..71]=textures, [72..79]=per-frame SRV/UAV
    DescriptorHeap m_dispatchHeap;

    // 定数バッファ
    DynamicBuffer m_cb;

    // ライト定数バッファ (PBR.hlsl b2と同一)
    LightConstants m_lightConstants = {};
    DynamicBuffer m_lightCB;

    // コンポジットパス (フルスクリーン三角形)
    Shader m_compositeShader;
    ComPtr<ID3D12RootSignature> m_compositeRS;
    ComPtr<ID3D12PipelineState> m_compositePSO;
    DynamicBuffer m_compositeCB;
    // コンポジット用SRVヒープ: [0]=Scene, [1]=Depth, [2]=Reflection, [3]=Normal (×2フレーム = 8)
    DescriptorHeap m_compositeHeap;

    // 空情報 (Missシェーダー用)
    XMFLOAT3 m_skyTopColor  = { 0.5f, 0.7f, 1.0f };
    XMFLOAT3 m_skyBottomColor = { 0.8f, 0.9f, 1.0f };

    // GBuffer法線RT (外部所有)
    RenderTarget* m_normalRT = nullptr;

    // BLASキャッシュ (VBポインタ → BLASインデックス)
    std::unordered_map<ID3D12Resource*, int> m_blasLookup;

    // BLAS毎のVB/IBリソース (ClosestHitでByteAddressBufferとしてアクセス)
    struct BLASGeometryInfo {
        ComPtr<ID3D12Resource> vb;
        ComPtr<ID3D12Resource> ib;
        uint32_t vertexStride = 0;
    };
    std::vector<BLASGeometryInfo> m_blasGeometry;

    // ディスパッチヒープレイアウト定数
    // [8..39]=geometry VB/IB, [40..71]=textures, [72..79]=per-frame SRV/UAV
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

    // per-instance PBRデータ (ClosestHitシェーダー用)
    // GPU側: float4 albedoMetallic[512] + float4 roughnessGeom[512] + float4 extraData[512]
    static constexpr uint32_t k_MaxInstances = 512;
    DynamicBuffer m_instanceDataCB;

    struct InstancePBR {
        XMFLOAT4 albedoMetallic;  // .rgb=albedo, .a=metallic
        XMFLOAT4 roughnessGeom;   // .x=roughness, .y=geometryIndex, .z=texIdx, .w=hasTexture
        XMFLOAT4 extraData;       // .x=vertexStride, .yzw=reserved
    };
    std::vector<InstancePBR> m_instanceData;
};

} // namespace GX
