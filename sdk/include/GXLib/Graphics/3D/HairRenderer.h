#pragma once
/// @file HairRenderer.h
/// @brief ヘアレンダリング (Marschner反射モデル)
///
/// Marschner et al. "Light Scattering from Human Hair Fibers" に基づく
/// 物理ベースヘアシェーディング。R/TT/TRTの3つの散乱成分で毛髪の光沢を再現。
/// ストランドデータをLine List形式で描画し、シェーダー側でMarschnerモデルを計算する。

#include "pch_graphics.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief ヘアストランドの制御点
struct HairControlPoint
{
    Vector3 position;       ///< ワールド空間位置
    float   thickness;      ///< 太さ (0.0~1.0)
};

/// @brief ヘアストランド (1本の毛)
struct HairStrand
{
    gx::Vector<HairControlPoint> controlPoints;
};

/// @brief ヘアマテリアル設定
struct HairMaterial
{
    Vector3 baseColor    = {0.15f, 0.08f, 0.04f};  ///< ベースカラー（メラニン由来）
    float   roughnessR   = 0.1f;     ///< R成分ラフネス（正反射）
    float   roughnessTT  = 0.2f;     ///< TT成分ラフネス（透過）
    float   roughnessTRT = 0.3f;     ///< TRT成分ラフネス（内部反射）
    float   shift        = -0.1f;    ///< キューティクル傾斜角 (radians)
    float   ior          = 1.55f;    ///< 屈折率
    float   absorption   = 0.2f;     ///< 内部吸収係数
    float   specularMultiplier = 1.0f; ///< スペキュラ強度倍率
    float   scatterStrength = 0.5f;    ///< 散乱強度
};

/// @brief ヘアレンダラー
///
/// HairStrand のリストから Line List 頂点を構築し、
/// Marschner反射モデルを実装したシェーダーで描画する。
class HairRenderer
{
public:
    HairRenderer() = default;
    ~HairRenderer() = default;

    /// @brief レンダラーを初期化する（シェーダーコンパイル・PSO生成・バッファ確保）
    /// @param device D3D12デバイス
    /// @return 成功時true
    bool Initialize(ID3D12Device* device);

    /// @brief GPU資源を解放する
    void Shutdown();

    /// @brief ストランドデータをアップロードする
    /// @param strands ストランドの配列
    void SetStrands(const gx::Vector<HairStrand>& strands);

    /// @brief マテリアルを設定する
    /// @param mat ヘアマテリアル設定
    void SetMaterial(const HairMaterial& mat) { m_material = mat; }

    /// @brief 現在のマテリアルを取得する
    /// @return マテリアルのconst参照
    const HairMaterial& GetMaterial() const { return m_material; }

    /// @brief ヘアを描画する
    /// @param cmdList コマンドリスト
    /// @param frameIndex フレームインデックス（ダブルバッファ用 0 or 1）
    /// @param viewProj ビュー×プロジェクション行列
    /// @param cameraPos カメラのワールド位置
    /// @param lightDir ディレクショナルライトの方向（光源へ向かうベクトル）
    /// @param lightColor ライトカラー
    void Render(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                const Matrix4x4& viewProj, const Vector3& cameraPos,
                const Vector3& lightDir, const Vector3& lightColor);

    /// @brief ストランド数を取得する
    uint32_t GetStrandCount() const { return m_strandCount; }

    /// @brief 頂点数を取得する
    uint32_t GetVertexCount() const { return m_totalVertices; }

    /// @brief PSO構築済みか判定する
    bool IsReady() const { return m_psoReady; }

private:
    /// @brief パイプラインステートオブジェクトを生成する
    /// @param device D3D12デバイス
    /// @return 成功時true
    bool CreatePipeline(ID3D12Device* device);

    /// @brief ストランドデータからLine List頂点バッファを構築する
    /// @param strands ストランドの配列
    void BuildVertexBuffer(const gx::Vector<HairStrand>& strands);

    /// @brief ヘア描画用の頂点（位置・接線・太さ・ストランドパラメータ）
    struct HairVertex
    {
        Vector3 position;     ///< ワールド空間位置
        Vector3 tangent;      ///< ストランド方向の接線ベクトル
        float   thickness;    ///< 太さ (0.0~1.0)
        float   strandParam;  ///< 根元からの正規化距離 (0=root, 1=tip)
    };

    /// @brief シェーダー定数バッファのレイアウト（256Bアライメント）
    struct alignas(256) HairConstants
    {
        float viewProj[16];     ///< ビュー×プロジェクション行列 (64B)
        float cameraPos[4];     ///< カメラ位置 (xyz, w=pad) (16B)
        float lightDir[4];      ///< ライト方向 (xyz, w=pad) (16B)
        float lightColor[4];    ///< ライトカラー (xyz, w=pad) (16B)
        float baseColor[4];     ///< ベースカラー (xyz, w=roughnessR) (16B)
        float roughness[4];     ///< roughnessTT, roughnessTRT, shift, ior (16B)
        float params[4];        ///< absorption, specularMultiplier, scatterStrength, pad (16B)
        float padding[24];      ///< 256Bアライメント用パディング
    };

    // GPU resources
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    Buffer m_vertexBuffer;
    DynamicBuffer m_cb;

    HairMaterial m_material;
    uint32_t m_strandCount = 0;
    uint32_t m_totalVertices = 0;
    ID3D12Device* m_device = nullptr;
    bool m_psoReady = false;
};

/// @}
} // namespace gx
