#pragma once
/// @file MeshletStreaming.h
/// @brief メッシュレットストリーミングシステム
///
/// メッシュシェーダーパイプラインと連携して、LODレベルに応じた
/// メッシュレット単位のストリーミングロード/アンロードを管理する。
/// 画面占有率に基づいてLOD選択を行い、フレーム予算内でロードを実行する。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include <string>
#include <vector>

namespace gx
{

/// @brief メッシュレットLODレベル定義
struct MeshletLOD
{
    uint32_t meshletOffset;   ///< メッシュレットバッファ内の開始オフセット
    uint32_t meshletCount;    ///< このLODのメッシュレット数
    float maxScreenSize;      ///< このLODを使用する最大スクリーンサイズ比率
};

/// @brief ストリーミング対象メッシュ定義
struct StreamableMesh
{
    std::string assetPath;              ///< アセットファイルパス
    std::vector<MeshletLOD> lods;       ///< LODレベル配列
    uint32_t totalMeshletCount = 0;     ///< 全LOD合計のメッシュレット数
    float boundingRadius = 0.0f;        ///< バウンディング球の半径
};

/// @brief メッシュレットストリーミング設定
struct MeshletStreamingConfig
{
    float lodBias = 0.0f;                   ///< LOD選択バイアス（正=低LOD寄り）
    uint32_t maxResidentMeshlets = 65536;   ///< メモリ上に常駐可能な最大メッシュレット数
    uint32_t loadBudgetPerFrame = 64;       ///< フレーム毎のロード上限メッシュレット数
    float fadeDistance = 5.0f;              ///< LOD遷移時のフェード距離
};

/// @brief メッシュレットストリーミングマネージャー
///
/// カメラからの距離・画面占有率に基づいてメッシュ毎の最適LODを選択し、
/// 必要なメッシュレットデータをフレーム予算内でGPUメモリにストリーミングする。
/// Meshlet Shading Pipeline (Amplification + Mesh Shader) との連携を前提とする。
class MeshletStreaming
{
public:
    MeshletStreaming() = default;
    ~MeshletStreaming() = default;

    /// @brief ストリーミング設定を更新する
    void SetConfig(const MeshletStreamingConfig& config);

    /// @brief 現在のストリーミング設定を取得する
    const MeshletStreamingConfig& GetConfig() const;

    /// @brief メッシュを登録する
    /// @param mesh ストリーミング対象メッシュ情報
    /// @return メッシュID
    uint32_t RegisterMesh(const StreamableMesh& mesh);

    /// @brief メッシュの登録を解除する
    /// @param meshId RegisterMeshで取得したID
    void UnregisterMesh(uint32_t meshId);

    /// @brief 指定メッシュの現在のアクティブLODレベルを取得する
    /// @param meshId メッシュID
    /// @return LODレベル（未登録の場合は-1）
    int GetActiveLOD(uint32_t meshId) const;

    /// @brief 現在メモリに常駐中のメッシュレット数を取得する
    uint32_t GetResidentMeshletCount() const;

    /// @brief 登録済みメッシュ数を取得する
    uint32_t GetRegisteredMeshCount() const;

    /// @brief ストリーミング統計情報
    struct Stats
    {
        uint32_t residentMeshlets;   ///< 常駐中メッシュレット数
        uint32_t registeredMeshes;   ///< 登録メッシュ数
    };

    /// @brief 統計情報を取得する
    Stats GetStats() const;

private:
    /// @brief メッシュごとの内部状態
    struct MeshState
    {
        StreamableMesh mesh;
        int currentLOD = 0;
        bool resident = false;
        bool valid = false;
    };

    std::vector<MeshState> m_meshes;
    MeshletStreamingConfig m_config;
    std::vector<uint32_t> m_freeIds;
};

} // namespace gx
/// @}
