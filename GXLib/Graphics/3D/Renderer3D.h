#pragma once
/// @file Renderer3D.h
/// @brief 3Dレンダラー本体

#include "pch.h"
#include "Graphics/3D/Vertex3D.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Transform3D.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/AnimationPlayer.h"
#include "Graphics/3D/CascadedShadowMap.h"
#include "Graphics/3D/PointShadowMap.h"
#include "Graphics/3D/Fog.h"
#include "Graphics/3D/Skybox.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Graphics/3D/Terrain.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// オブジェクト定数バッファ（b0）
struct ObjectConstants
{
    XMFLOAT4X4 world;
    XMFLOAT4X4 worldInverseTranspose;
};

/// 1フレームあたりの最大オブジェクト数
static constexpr uint32_t k_MaxObjectsPerFrame = 512;

/// フレーム定数バッファ（b1）
struct FrameConstants
{
    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
    XMFLOAT4X4 viewProjection;
    XMFLOAT3   cameraPosition;
    float      time;
    // CSMシャドウ関連
    XMFLOAT4X4 lightVP[ShadowConstants::k_NumCascades];
    float      cascadeSplits[ShadowConstants::k_NumCascades];
    float      shadowMapSize;
    uint32_t   shadowEnabled;
    float      _fogPad[2];     // HLSL cbuffer パッキングに合わせる（float3が16バイト境界をまたがないよう8バイトパディング）
    // フォグ関連
    XMFLOAT3   fogColor;
    float      fogStart;
    float      fogEnd;
    float      fogDensity;
    uint32_t   fogMode;
    uint32_t   shadowDebugMode;  // 0=OFF, 1=Factor, 2=Cascade
    // --- offset 528 ---
    // スポットシャドウ関連
    XMFLOAT4X4 spotLightVP;
    float      spotShadowMapSize;
    int32_t    spotShadowLightIndex;
    float      _spotPad[2];
    // --- offset 608 ---
    // ポイントシャドウ関連
    XMFLOAT4X4 pointLightVP[6];
    float      pointShadowMapSize;
    int32_t    pointShadowLightIndex;
    float      _pointPad[2];
    // --- offset 1008 ---
};

static_assert(offsetof(FrameConstants, shadowEnabled) == 484, "shadowEnabled offset mismatch");
static_assert(offsetof(FrameConstants, fogColor) == 496, "fogColor offset mismatch");
static_assert(offsetof(FrameConstants, spotLightVP) == 528, "spotLightVP offset mismatch");
static_assert(offsetof(FrameConstants, pointLightVP) == 608, "pointLightVP offset mismatch");
static_assert(sizeof(FrameConstants) == 1008, "FrameConstants size mismatch");

/// GPU上のメッシュ（VB + IB）
struct GPUMesh
{
    Buffer   vertexBuffer;
    Buffer   indexBuffer;
    uint32_t indexCount = 0;
};

/// @brief 3Dレンダラークラス
class Renderer3D
{
public:
    Renderer3D() = default;
    ~Renderer3D() = default;

    /// 初期化
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    uint32_t screenWidth, uint32_t screenHeight);

    /// MeshDataからGPUメッシュを作成
    GPUMesh CreateGPUMesh(const MeshData& meshData);

    /// シャドウパス：指定カスケードにシーンの深度を描画
    void BeginShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                          uint32_t cascadeIndex);
    void EndShadowPass(uint32_t cascadeIndex);

    /// スポットライトシャドウパス
    void BeginSpotShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);
    void EndSpotShadowPass();

    /// ポイントライトシャドウパス（6面）
    void BeginPointShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, uint32_t face);
    void EndPointShadowPass(uint32_t face);

    /// フレーム開始（メインパス）
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
               const Camera3D& camera, float time);

    /// ライト設定
    void SetLights(const LightData* lights, uint32_t count, const XMFLOAT3& ambient);

    /// マテリアル設定
    void SetMaterial(const Material& material);

    /// メッシュ描画（シャドウパスでもメインパスでも使用可）
    void DrawMesh(const GPUMesh& mesh, const Transform3D& transform);

    /// 地形描画
    void DrawTerrain(const Terrain& terrain, const Transform3D& transform);

    /// モデル描画（マテリアル自動バインド）
    void DrawModel(const Model& model, const Transform3D& transform);

    /// スキンドモデル描画
    void DrawSkinnedModel(const Model& model, const Transform3D& transform,
                           const AnimationPlayer& animPlayer);

    /// フレーム終了
    void End();

    /// シャドウの更新（BeginShadowPass前に呼ぶ）
    void UpdateShadow(const Camera3D& camera);

    /// シャドウ有効/無効
    void SetShadowEnabled(bool enabled) { m_shadowEnabled = enabled; }
    bool IsShadowEnabled() const { return m_shadowEnabled; }

    /// シャドウデバッグモード（0=OFF, 1=Factor, 2=Cascade）
    void SetShadowDebugMode(uint32_t mode) { m_shadowDebugMode = mode; }
    uint32_t GetShadowDebugMode() const { return m_shadowDebugMode; }

    /// フォグ設定
    void SetFog(FogMode mode, const XMFLOAT3& color, float start, float end, float density = 0.01f);

    /// スカイボックス取得
    Skybox& GetSkybox() { return m_skybox; }

    /// PrimitiveBatch3D取得
    PrimitiveBatch3D& GetPrimitiveBatch3D() { return m_primitiveBatch3D; }

    /// 深度バッファ取得
    DepthBuffer& GetDepthBuffer() { return m_depthBuffer; }

    /// テクスチャマネージャー取得
    TextureManager& GetTextureManager() { return m_textureManager; }

    /// マテリアルマネージャー取得
    MaterialManager& GetMaterialManager() { return m_materialManager; }

    /// CSM取得
    CascadedShadowMap& GetCSM() { return m_csm; }

    /// 画面サイズ変更
    void OnResize(uint32_t width, uint32_t height);

private:
    bool CreatePipelineState(ID3D12Device* device);
    bool CreateShadowPipelineState(ID3D12Device* device);

    ID3D12Device*              m_device    = nullptr;
    ID3D12GraphicsCommandList* m_cmdList   = nullptr;
    uint32_t                   m_frameIndex = 0;
    uint32_t                   m_screenWidth  = 0;
    uint32_t                   m_screenHeight = 0;

    // メインパイプライン
    Shader                       m_shaderCompiler;
    ComPtr<ID3D12RootSignature>  m_rootSignature;
    ComPtr<ID3D12PipelineState>  m_pso;

    // シャドウパイプライン
    ComPtr<ID3D12RootSignature>  m_shadowRootSignature;
    ComPtr<ID3D12PipelineState>  m_shadowPso;
    DynamicBuffer                m_shadowPassCB;  // b1: lightVP for shadow pass

    // 深度バッファ
    DepthBuffer m_depthBuffer;

    // CSM
    CascadedShadowMap m_csm;
    bool              m_shadowEnabled = true;
    bool              m_inShadowPass  = false;
    uint32_t          m_shadowDebugMode = 0;

    // スポットシャドウ
    ShadowMap   m_spotShadowMap;
    XMFLOAT4X4  m_spotLightVP = {};
    int32_t     m_spotShadowLightIndex = -1;
    static constexpr uint32_t k_SpotShadowMapSize = 2048;

    // ポイントシャドウ
    PointShadowMap m_pointShadowMap;
    int32_t        m_pointShadowLightIndex = -1;

    // フォグ
    FogConstants m_fogConstants;

    // スカイボックス
    Skybox m_skybox;

    // 3Dプリミティブバッチ
    PrimitiveBatch3D m_primitiveBatch3D;

    // テクスチャマネージャー
    TextureManager m_textureManager;

    // マテリアルマネージャー
    MaterialManager m_materialManager;

    // 定数バッファ（ダブルバッファ）
    DynamicBuffer m_objectCB;     // b0: per-object (ring buffer)
    uint8_t*      m_objectCBMapped = nullptr;  // Map中のポインタ
    uint32_t      m_objectCBOffset = 0;        // リングバッファオフセット
    DynamicBuffer m_frameCB;      // b1: per-frame
    DynamicBuffer m_lightCB;      // b2: lights
    DynamicBuffer m_materialCB;   // b3: material
    DynamicBuffer m_boneCB;       // b4: bone matrices (skinned)

    // 現在のライト状態
    LightConstants m_currentLights = {};

    // デフォルトマテリアル
    Material m_defaultMaterial;
};

} // namespace GX
