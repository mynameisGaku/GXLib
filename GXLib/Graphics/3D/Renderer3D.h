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
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/CascadedShadowMap.h"
#include "Graphics/3D/PointShadowMap.h"
#include "Graphics/3D/Fog.h"
#include "Graphics/3D/Skybox.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Graphics/3D/Terrain.h"
#include "Graphics/3D/ShaderRegistry.h"
#include "Graphics/3D/ShaderModelConstants.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief オブジェクト定数バッファ（b0スロット）
struct ObjectConstants
{
    XMFLOAT4X4 world;                  ///< ワールド変換行列
    XMFLOAT4X4 worldInverseTranspose;  ///< ワールド逆転置行列（法線変換用）
};

/// @brief 1フレームあたりの最大オブジェクト数
static constexpr uint32_t k_MaxObjectsPerFrame = 512;

/// @brief フレーム定数バッファ（b1スロット）— カメラ・シャドウ・フォグ情報をGPUに送る
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
    // --- オフセット 528 ---
    // スポットシャドウ関連
    XMFLOAT4X4 spotLightVP;
    float      spotShadowMapSize;
    int32_t    spotShadowLightIndex;
    float      _spotPad[2];
    // --- オフセット 608 ---
    // ポイントシャドウ関連
    XMFLOAT4X4 pointLightVP[6];
    float      pointShadowMapSize;
    int32_t    pointShadowLightIndex;
    float      _pointPad[2];
    // --- オフセット 1008 ---
};

static_assert(offsetof(FrameConstants, shadowEnabled) == 484, "shadowEnabled offset mismatch");
static_assert(offsetof(FrameConstants, fogColor) == 496, "fogColor offset mismatch");
static_assert(offsetof(FrameConstants, spotLightVP) == 528, "spotLightVP offset mismatch");
static_assert(offsetof(FrameConstants, pointLightVP) == 608, "pointLightVP offset mismatch");
static_assert(sizeof(FrameConstants) == 1008, "FrameConstants size mismatch");

/// @brief GPU上のメッシュデータ（頂点バッファ + インデックスバッファ）
struct GPUMesh
{
    Buffer   vertexBuffer;   ///< 頂点バッファ
    Buffer   indexBuffer;    ///< インデックスバッファ
    uint32_t indexCount = 0; ///< インデックス数
};

/// @brief カスタムマテリアル用シェーダー設定
/// 初学者向け: 既定のPBR以外のシェーダーを差し替える時に使います。
struct ShaderProgramDesc
{
    std::wstring vsPath;
    std::wstring psPath;
    std::wstring vsEntry = L"VSMain";
    std::wstring psEntry = L"PSMain";
    std::vector<std::pair<std::wstring, std::wstring>> defines;
};

/// @brief 3Dレンダラークラス
class Renderer3D
{
public:
    Renderer3D() = default;
    ~Renderer3D() = default;

    /// @brief 3Dレンダラーを初期化する
    /// @param device D3D12デバイスへのポインタ
    /// @param cmdQueue コマンドキューへのポインタ
    /// @param screenWidth スクリーン幅（ピクセル）
    /// @param screenHeight スクリーン高さ（ピクセル）
    /// @return 初期化に成功した場合true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    uint32_t screenWidth, uint32_t screenHeight);

    /// @brief MeshDataからGPU上のメッシュリソースを作成する
    /// @param meshData メッシュの頂点・インデックスデータ
    /// @return 作成されたGPUMesh
    GPUMesh CreateGPUMesh(const MeshData& meshData);

    /// @brief カスタムシェーダー (PBR互換ルートシグネチャ) を登録する
    /// @param desc シェーダー記述
    /// @return シェーダーハンドル (失敗時 -1)
    int CreateMaterialShader(const ShaderProgramDesc& desc);

    /// @brief CSMシャドウパスを開始する（指定カスケードにシーンの深度を描画）
    /// @param cmdList グラフィックスコマンドリスト
    /// @param frameIndex 現在のフレームインデックス
    /// @param cascadeIndex カスケードインデックス（0〜k_NumCascades-1）
    void BeginShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                          uint32_t cascadeIndex);

    /// @brief CSMシャドウパスを終了する
    /// @param cascadeIndex カスケードインデックス
    void EndShadowPass(uint32_t cascadeIndex);

    /// @brief スポットライトシャドウパスを開始する
    /// @param cmdList グラフィックスコマンドリスト
    /// @param frameIndex 現在のフレームインデックス
    void BeginSpotShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// @brief スポットライトシャドウパスを終了する
    void EndSpotShadowPass();

    /// @brief ポイントライトシャドウパスを開始する（6面キューブマップの1面）
    /// @param cmdList グラフィックスコマンドリスト
    /// @param frameIndex 現在のフレームインデックス
    /// @param face キューブマップの面インデックス（0〜5）
    void BeginPointShadowPass(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, uint32_t face);

    /// @brief ポイントライトシャドウパスを終了する
    /// @param face キューブマップの面インデックス
    void EndPointShadowPass(uint32_t face);

    /// @brief メインレンダリングパスのフレームを開始する
    /// @param cmdList グラフィックスコマンドリスト
    /// @param frameIndex 現在のフレームインデックス
    /// @param camera 3Dカメラ（TAAジッター適用のためnon-const）
    /// @param time 経過時間（秒）
    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
               const Camera3D& camera, float time);

    /// @brief シーン内のライトを設定する
    /// @param lights ライトデータの配列
    /// @param count ライトの数
    /// @param ambient アンビエントライトの色
    void SetLights(const LightData* lights, uint32_t count, const XMFLOAT3& ambient);

    /// @brief マテリアルを設定する
    /// @param material 設定するマテリアル
    void SetMaterial(const Material& material);

    /// @brief メッシュを描画する（シャドウパスでもメインパスでも使用可）
    /// @param mesh GPUメッシュ
    /// @param transform ワールド変換
    void DrawMesh(const GPUMesh& mesh, const Transform3D& transform);

    /// @brief メッシュを描画する（ワールド行列直接指定 -- 物理シミュレーション等で使用）
    /// @param mesh GPUメッシュ
    /// @param worldMatrix ワールド変換行列
    void DrawMesh(const GPUMesh& mesh, const XMMATRIX& worldMatrix);

    /// @brief 地形を描画する
    /// @param terrain 地形オブジェクト
    /// @param transform ワールド変換
    void DrawTerrain(const Terrain& terrain, const Transform3D& transform);

    /// @brief モデルを描画する（マテリアル自動バインド）
    /// @param model 描画するモデル
    /// @param transform ワールド変換
    void DrawModel(const Model& model, const Transform3D& transform);

    /// @brief スキニングアニメーション付きモデルを描画する
    /// @param model 描画するモデル
    /// @param transform ワールド変換
    /// @param animPlayer アニメーションプレイヤー（ボーン行列計算済み）
    void DrawSkinnedModel(const Model& model, const Transform3D& transform,
                           const AnimationPlayer& animPlayer);

    /// @brief スキニングモデルを描画する (Animator)
    void DrawSkinnedModel(const Model& model, const Transform3D& transform,
                           const Animator& animator);

    /// @brief フレームの描画を終了する
    void End();

    /// @brief シャドウマップを更新する（BeginShadowPass前に呼ぶ）
    /// @param camera シャドウ計算の基準となるカメラ
    void UpdateShadow(const Camera3D& camera);

    /// @brief シャドウの有効/無効を設定する
    /// @param enabled trueでシャドウ有効
    void SetShadowEnabled(bool enabled) { m_shadowEnabled = enabled; }

    /// @brief シャドウが有効かどうかを取得する
    /// @return シャドウが有効な場合true
    bool IsShadowEnabled() const { return m_shadowEnabled; }

    /// @brief シャドウデバッグモードを設定する
    /// @param mode デバッグモード（0=OFF, 1=Factor, 2=Cascade可視化）
    void SetShadowDebugMode(uint32_t mode) { m_shadowDebugMode = mode; }

    /// @brief シャドウデバッグモードを取得する
    /// @return 現在のデバッグモード
    uint32_t GetShadowDebugMode() const { return m_shadowDebugMode; }

    /// @brief フォグ（霧）効果を設定する
    /// @param mode フォグモード（Linear/Exp/Exp2等）
    /// @param color フォグの色
    /// @param start フォグ開始距離（Linearモード用）
    /// @param end フォグ終了距離（Linearモード用）
    /// @param density フォグの密度（Exp/Exp2モード用）
    void SetFog(FogMode mode, const XMFLOAT3& color, float start, float end, float density = 0.01f);

    /// @brief スカイボックスを取得する
    /// @return Skyboxへの参照
    Skybox& GetSkybox() { return m_skybox; }

    /// @brief 3Dプリミティブバッチを取得する
    /// @return PrimitiveBatch3Dへの参照
    PrimitiveBatch3D& GetPrimitiveBatch3D() { return m_primitiveBatch3D; }

    /// @brief 深度バッファを取得する
    /// @return DepthBufferへの参照
    DepthBuffer& GetDepthBuffer() { return m_depthBuffer; }

    /// @brief テクスチャマネージャーを取得する
    /// @return TextureManagerへの参照
    TextureManager& GetTextureManager() { return m_textureManager; }

    /// @brief マテリアルマネージャーを取得する
    /// @return MaterialManagerへの参照
    MaterialManager& GetMaterialManager() { return m_materialManager; }

    /// @brief カスケードシャドウマップを取得する
    /// @return CascadedShadowMapへの参照
    CascadedShadowMap& GetCSM() { return m_csm; }

    /// @brief 画面サイズの変更を処理する
    /// @param width 新しい幅（ピクセル）
    /// @param height 新しい高さ（ピクセル）
    void OnResize(uint32_t width, uint32_t height);

private:
    bool CreatePipelineState(ID3D12Device* device);
    bool CreateShadowPipelineState(ID3D12Device* device);
    void BindPipeline(bool skinned, int shaderHandle);
    void BindPipelineForModel(bool skinned, int shaderHandle, gxfmt::ShaderModel model);

    ID3D12Device*              m_device    = nullptr;
    ID3D12GraphicsCommandList* m_cmdList   = nullptr;
    uint32_t                   m_frameIndex = 0;
    uint32_t                   m_screenWidth  = 0;
    uint32_t                   m_screenHeight = 0;

    // メインパイプライン
    Shader                       m_shaderCompiler;
    ComPtr<ID3D12RootSignature>  m_rootSignature;
    ComPtr<ID3D12PipelineState>  m_pso;
    ComPtr<ID3D12PipelineState>  m_psoSkinned;

    // シェーダーモデルPSOレジストリ
    ShaderRegistry               m_shaderRegistry;

    // シャドウパイプライン
    ComPtr<ID3D12RootSignature>  m_shadowRootSignature;
    ComPtr<ID3D12PipelineState>  m_shadowPso;
    ComPtr<ID3D12PipelineState>  m_shadowPsoSkinned;
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
    DynamicBuffer m_materialCB;   // b3: material (ring buffer)
    uint8_t*      m_materialCBMapped = nullptr;
    uint32_t      m_materialCBOffset = 0;
    DynamicBuffer m_boneCB;       // b4: bone matrices (skinned)

    // 現在のライト状態
    LightConstants m_currentLights = {};

    // デフォルトマテリアル
    Material m_defaultMaterial;

    // カスタムシェーダー（PBR互換のルートシグネチャ）
    struct ShaderPipeline
    {
        ShaderProgramDesc desc;
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12PipelineState> psoSkinned;
    };
    std::unordered_map<int, ShaderPipeline> m_customShaders;
    int m_nextShaderHandle = 1;

    // デフォルトテクスチャ
    int m_defaultWhiteTex = -1;
    int m_defaultNormalTex = -1;
    int m_defaultBlackTex = -1;

    ID3D12PipelineState* m_currentPso = nullptr;

    // 冗長バインド防止用 — 前回バインドしたVB/IBリソース
    ID3D12Resource* m_lastBoundVB = nullptr;
    ID3D12Resource* m_lastBoundIB = nullptr;
};

} // namespace GX
