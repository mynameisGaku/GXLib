#pragma once
/// @file Decal.h
/// @brief デカールシステム（Deferred Box Projection）

#include "pch_graphics.h"
#include "Graphics/3D/Transform3D.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Color.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

class Camera3D;
class TextureManager;

/// @brief デカールブレンドモード
enum class DecalBlendMode : uint32_t
{
    Alpha    = 0,  ///< アルファブレンド（デフォルト）
    Additive = 1,  ///< 加算ブレンド
    Multiply = 2,  ///< 乗算ブレンド
    Replace  = 3,  ///< 完全置換
};

/// @brief デカールデータ
struct DecalData
{
    Transform3D transform;           ///< ワールド位置・向き・サイズ
    int textureHandle = -1;          ///< TextureManagerハンドル
    Color color = {1.0f, 1.0f, 1.0f, 1.0f};  ///< デカルカラー
    float fadeDistance = 0.5f;        ///< エッジフェード距離
    float normalThreshold = 0.7f;    ///< 法線方向しきい値（dotでフェード）
    float lifetime = -1.0f;          ///< 負=永続、正=秒後にフェード削除
    float age = 0.0f;                ///< 経過時間
    DecalBlendMode blendMode = DecalBlendMode::Alpha; ///< ブレンドモード
    int priority = 0;                ///< 描画優先度（大きいほど後に描画）
    float metallic = 0.0f;           ///< メタリック値（Deferred用）
    float roughness = 0.5f;          ///< ラフネス値（Deferred用）
    int normalTextureHandle = -1;    ///< 法線テクスチャハンドル
};

/// @brief デカール描画システム
///
/// Deferred Box Projection方式でシーンにデカールを投影する。
/// 深度バッファからワールド座標を復元し、デカールのローカル空間でUV計算を行う。
class DecalSystem
{
public:
    static constexpr uint32_t k_MaxDecals = 256;

    DecalSystem() = default;
    ~DecalSystem() = default;

    /// @brief 初期化
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief デカールを追加する
    /// @return デカールハンドル
    int AddDecal(const DecalData& decal);

    /// @brief デカールを削除する
    void RemoveDecal(int handle);

    /// @brief 寿命管理の更新
    void Update(float deltaTime);

    /// @brief デカールを描画する
    /// @param cmdList コマンドリスト
    /// @param camera カメラ
    /// @param depthBuffer 深度バッファ（SRVに遷移して読み込む）
    /// @param outputRTV 描画先RTVハンドル（HDR RT）
    /// @param texManager テクスチャマネージャ
    /// @param frameIndex フレームインデックス
    void Render(ID3D12GraphicsCommandList* cmdList,
                const Camera3D& camera,
                DepthBuffer& depthBuffer,
                D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
                TextureManager& texManager,
                uint32_t frameIndex);

    /// @brief デカール数を取得する
    int GetDecalCount() const;

    /// @brief デカールデータを取得する
    DecalData* GetDecal(int handle);

    /// @brief リソース解放
    void Shutdown();

private:
    struct DecalEntry
    {
        DecalData data;
        bool valid = false;
    };

    /// @brief デカール定数バッファ（GPU送信用、256バイトアラインメント）
    struct DecalCB
    {
        Matrix4x4  invViewProj;     ///< 逆ビュープロジェクション行列
        Matrix4x4  decalWorld;       ///< デカールワールド行列
        Matrix4x4  decalInvWorld;    ///< デカール逆ワールド行列
        Vector4    decalColor;       ///< デカールカラー
        float      fadeDistance;      ///< エッジフェード距離
        float      normalThreshold;   ///< 法線方向しきい値
        Vector2    screenSize;       ///< 画面サイズ
        float      padding[8];       ///< 256バイトアラインメント用パディング
    };

    gx::Vector<DecalEntry> m_decals;   ///< デカルエントリ配列
    gx::Vector<int> m_freeList;         ///< 空きスロットのインデックス

    // ユニットキューブメッシュ（デカルボリューム投影用）
    Buffer m_cubeVB;                     ///< キューブ頂点バッファ
    Buffer m_cubeIB;                     ///< キューブインデックスバッファ

    // PSO
    ComPtr<ID3D12PipelineState> m_pso;   ///< パイプラインステート
    ComPtr<ID3D12RootSignature> m_rs;    ///< ルートシグネチャ
    DynamicBuffer m_cb;                  ///< 定数バッファ
    DescriptorHeap m_srvHeap;            ///< SRVデスクリプタヒープ

    uint32_t m_width = 0;                ///< 画面幅（ピクセル）
    uint32_t m_height = 0;               ///< 画面高さ（ピクセル）

    bool m_initialized = false;          ///< 初期化済みフラグ

    bool CreateCubeMesh(ID3D12Device* device);
    bool CreatePSO(ID3D12Device* device);
};

/// @}
} // namespace gx
