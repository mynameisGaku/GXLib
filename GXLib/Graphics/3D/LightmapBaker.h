#pragma once
/// @file LightmapBaker.h
/// @brief ライトマップベイキングシステム（CPU/GPUレイキャスト、アトラスパッキング）
///
/// メッシュインスタンスとライトを登録し、Bake()でテクセル単位のライトマップを生成する。
/// 直接照明 + 間接照明（バウンス）を計算し、結果をアトラスにパッキングする。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include "Math/Color.h"

#include <functional>
#include <atomic>

namespace gx
{

/// @brief ライトマップ品質レベル
enum class LightmapQuality
{
    Draft,   ///< 低品質（プレビュー用）
    Medium,  ///< 中品質
    High,    ///< 高品質
    Ultra    ///< 最高品質
};

/// @brief ライトタイプ
enum class LightmapLightType
{
    Point,       ///< ポイントライト
    Directional, ///< ディレクショナルライト
    Spot         ///< スポットライト
};

/// @brief ライトマップベイキング設定
struct LightmapConfig
{
    uint32_t        resolution      = 256;                  ///< テクスチャ解像度（256-4096）
    uint32_t        samplesPerTexel = 64;                   ///< テクセルあたりのサンプル数（16-1024）
    uint32_t        bounceCount     = 2;                    ///< バウンス回数（1-8）
    LightmapQuality quality         = LightmapQuality::Medium; ///< 品質レベル
    uint32_t        padding         = 2;                    ///< チャート間パディング（ピクセル）
    uint32_t        maxAtlasSize    = 4096;                 ///< 最大アトラスサイズ
    bool            useGPU          = false;                ///< GPU使用フラグ
    Color           ambient{0.05f, 0.05f, 0.05f, 1.0f};    ///< アンビエント色
};

/// @brief ライトマップテクセル
struct LightmapTexel
{
    Vector3 position;   ///< ワールド座標位置
    Vector3 normal;     ///< ワールド法線
    Color   color;      ///< ベイク済みライト色
    bool    valid = false; ///< 有効なテクセルか
};

/// @brief ライトマップチャート（メッシュごとの領域）
struct LightmapChart
{
    uint32_t                   objectIndex = 0;  ///< メッシュオブジェクトインデックス
    std::vector<LightmapTexel> texelData;        ///< テクセルデータ
    uint32_t                   width  = 0;       ///< チャート幅
    uint32_t                   height = 0;       ///< チャート高さ
    uint32_t                   atlasX = 0;       ///< アトラス内X座標
    uint32_t                   atlasY = 0;       ///< アトラス内Y座標
};

/// @brief ライトマップアトラス
struct LightmapAtlas
{
    std::vector<LightmapChart> charts;            ///< チャート配列
    uint32_t                   totalWidth  = 0;   ///< アトラス合計幅
    uint32_t                   totalHeight = 0;   ///< アトラス合計高さ
    bool                       packed = false;    ///< パッキング済みフラグ
};

/// @brief パッキング結果
struct PackResult
{
    bool     success        = false;  ///< 成功フラグ
    float    packEfficiency = 0.0f;   ///< パッキング効率（0-1）
    uint32_t totalTexels    = 0;      ///< 総テクセル数
    uint32_t usedTexels     = 0;      ///< 使用テクセル数
};

/// @brief レイキャスト結果
struct RaycastResult
{
    bool    hit      = false;     ///< ヒットしたか
    Vector3 position;             ///< ヒット位置
    Vector3 normal;               ///< ヒット法線
    float   distance = 0.0f;      ///< レイの距離
};

/// @brief ライトマップベイカー
///
/// メッシュとライトを登録し、CPU/GPUレイキャストでライトマップを生成する。
/// 直接照明と間接照明（バウンス）の両方をサポート。
class LightmapBaker
{
public:
    LightmapBaker() = default;
    ~LightmapBaker() = default;

    /// @brief ベイキング設定を設定する
    /// @param config ライトマップ設定
    void SetConfig(const LightmapConfig& config);

    /// @brief 現在のベイキング設定を取得する
    /// @return ライトマップ設定
    const LightmapConfig& GetConfig() const { return m_config; }

    /// @brief メッシュインスタンスを追加する
    /// @param objectIndex オブジェクトインデックス
    /// @param positions 頂点位置配列
    /// @param normals 法線配列
    /// @param uvs UV座標配列
    /// @param indices インデックス配列
    void AddMeshInstance(uint32_t objectIndex,
                         const std::vector<Vector3>& positions,
                         const std::vector<Vector3>& normals,
                         const std::vector<Vector3>& uvs,
                         const std::vector<uint32_t>& indices);

    /// @brief 登録済みメッシュインスタンス数を取得する
    /// @return メッシュ数
    uint32_t GetMeshInstanceCount() const { return static_cast<uint32_t>(m_meshInstances.size()); }

    /// @brief ライトソースを追加する
    /// @param type ライトタイプ
    /// @param position 位置
    /// @param direction 方向
    /// @param color 色
    /// @param intensity 強度
    /// @param radius 半径
    void AddLight(LightmapLightType type, const Vector3& position,
                  const Vector3& direction, const Color& color,
                  float intensity, float radius);

    /// @brief 登録済みライト数を取得する
    /// @return ライト数
    uint32_t GetLightCount() const { return static_cast<uint32_t>(m_lights.size()); }

    /// @brief フルベイキング（直接照明 + 間接バウンス）を実行する
    void Bake();

    /// @brief 直接照明のみベイキングする
    void BakeDirectOnly();

    /// @brief ベイク済みアトラスを取得する
    /// @return ライトマップアトラスへの参照
    const LightmapAtlas& GetAtlas() const { return m_atlas; }

    /// @brief アトラスパッキング（Shelf-First-Fit）を実行する
    /// @return パッキング結果
    PackResult PackAtlas();

    /// @brief 全状態をリセットする
    void Clear();

    /// @brief ベイキング進捗を取得する（0.0 - 1.0）
    /// @return 進捗
    float GetProgress() const { return m_progress.load(std::memory_order_relaxed); }

    /// @brief ベイキングをキャンセルする
    void Cancel() { m_cancelled.store(true, std::memory_order_relaxed); }

private:
    /// @brief メッシュインスタンスデータ
    struct MeshInstance
    {
        uint32_t              objectIndex;
        std::vector<Vector3>  positions;
        std::vector<Vector3>  normals;
        std::vector<Vector3>  uvs;
        std::vector<uint32_t> indices;
    };

    /// @brief ライトデータ
    struct LightSource
    {
        LightmapLightType type;
        Vector3           position;
        Vector3           direction;
        Color             color;
        float             intensity;
        float             radius;
    };

    /// @brief シャドウレイのトレース（遮蔽判定）
    /// @param origin レイ始点
    /// @param direction レイ方向
    /// @param maxDistance 最大距離
    /// @return 遮蔽がなければtrue
    bool TraceShadowRay(const Vector3& origin, const Vector3& direction, float maxDistance) const;

    /// @brief レイとシーンの交差判定
    /// @param origin レイ始点
    /// @param direction レイ方向
    /// @return レイキャスト結果
    RaycastResult TraceRay(const Vector3& origin, const Vector3& direction) const;

    /// @brief 半球サンプリング（コサイン重み付き分布）
    /// @param normal 法線方向
    /// @param sampleIndex サンプル番号
    /// @param totalSamples 合計サンプル数
    /// @return サンプル方向ベクトル
    Vector3 SampleHemisphere(const Vector3& normal, uint32_t sampleIndex, uint32_t totalSamples) const;

    /// @brief テクセルへのライト寄与を計算する
    /// @param texelPos テクセル位置
    /// @param texelNormal テクセル法線
    /// @param light ライトソース
    /// @return ライト寄与色
    Color AccumulateLight(const Vector3& texelPos, const Vector3& texelNormal,
                          const LightSource& light) const;

    /// @brief メッシュからチャートを生成する
    /// @param mesh メッシュインスタンス
    /// @return 生成されたチャート
    LightmapChart GenerateChart(const MeshInstance& mesh) const;

    /// @brief レイと三角形の交差判定（Moller-Trumbore）
    /// @param origin レイ始点
    /// @param dir レイ方向
    /// @param v0 三角形頂点0
    /// @param v1 三角形頂点1
    /// @param v2 三角形頂点2
    /// @param outT 交差距離
    /// @return 交差があればtrue
    bool RayTriangleIntersect(const Vector3& origin, const Vector3& dir,
                              const Vector3& v0, const Vector3& v1, const Vector3& v2,
                              float& outT) const;

    /// @brief 直接照明パスを実行する
    void BakeDirectPass();

    /// @brief 間接照明バウンスパスを実行する
    /// @param bounceIndex バウンスインデックス
    void BakeBouncePass(uint32_t bounceIndex);

    LightmapConfig              m_config;
    std::vector<MeshInstance>   m_meshInstances;
    std::vector<LightSource>    m_lights;
    LightmapAtlas               m_atlas;
    std::atomic<float>          m_progress{0.0f};
    std::atomic<bool>           m_cancelled{false};
};

} // namespace gx
/// @}
