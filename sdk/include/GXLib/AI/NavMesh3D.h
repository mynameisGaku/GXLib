#pragma once
/// @file NavMesh3D.h
/// @brief ボクセルベースの3Dナビゲーションメッシュ
///
/// 飛行・水泳など3D空間を自由に移動するエージェント向けのパスファインディング。
/// 3Dボクセルグリッドで空間を管理し、Off-mesh Link（ジャンプ/はしご等）にも対応する。
/// @addtogroup grp_ai/// @{

#include "pch_common.h"
#include "Math/Vector3.h"

namespace gx
{

class PrimitiveBatch3D;

/// @brief 3Dボクセルの状態
enum class VoxelState : uint8_t
{
    Blocked = 0,   ///< 通行不可
    Open    = 1,   ///< 通行可能
    Water   = 2,   ///< 水中
    Air     = 3,   ///< 空中
};

/// @brief Off-mesh link（ジャンプ/はしご/テレポート等の特殊経路）
struct OffMeshLink
{
    Vector3 start;
    Vector3 end;
    float    cost = 1.0f;      ///< 通過コスト
    bool     bidirectional = true;  ///< 双方向か
    gx::String tag;           ///< タグ("jump", "ladder", "teleport"等)
};

/// @brief 3Dナビゲーションメッシュ
class NavMesh3D
{
public:
    NavMesh3D() = default;
    ~NavMesh3D() = default;

    /// @brief ボクセルグリッドを構築
    /// @param minBounds ワールド空間の最小座標
    /// @param maxBounds ワールド空間の最大座標
    /// @param voxelSize ボクセル1辺のサイズ
    /// @return 成功時true
    bool Build(const Vector3& minBounds, const Vector3& maxBounds, float voxelSize = 1.0f);

    /// @brief ジオメトリからボクセルを自動生成（三角形メッシュのラスタライズ）
    bool BuildFromGeometry(const float* vertices, int vertexCount,
                           const int* indices, int indexCount,
                           float voxelSize = 1.0f);

    /// @brief ボクセルの状態を設定
    void SetVoxel(int x, int y, int z, VoxelState state);

    /// @brief ボクセルの状態を取得
    VoxelState GetVoxel(int x, int y, int z) const;

    /// @brief ワールド座標をボクセルインデックスに変換
    bool WorldToVoxel(const Vector3& worldPos, int& outX, int& outY, int& outZ) const;

    /// @brief ボクセルインデックスをワールド座標に変換
    Vector3 VoxelToWorld(int x, int y, int z) const;

    /// @brief コスト倍率を設定
    void SetVoxelCost(int x, int y, int z, float cost);

    /// @brief 3D A*パスファインディング
    /// @param start 開始位置
    /// @param end 目標位置
    /// @param path 出力パス
    /// @param allowedStates 通行可能なボクセル状態のビットマスク
    /// @return パスが見つかったらtrue
    bool FindPath(const Vector3& start, const Vector3& end,
                  gx::Vector<Vector3>& path,
                  uint8_t allowedStates = 0xFE) const;

    /// @brief Off-mesh linkを追加
    uint32_t AddOffMeshLink(const OffMeshLink& link);

    /// @brief Off-mesh linkを削除
    void RemoveOffMeshLink(uint32_t index);

    /// @brief Off-mesh link数を取得
    uint32_t GetOffMeshLinkCount() const { return static_cast<uint32_t>(m_offMeshLinks.size()); }

    /// @brief 最近接のOpenボクセル座標を取得
    Vector3 FindNearestOpen(const Vector3& pos) const;

    /// @brief レイキャスト（3Dボクセル空間での線分判定）
    bool Raycast(const Vector3& origin, const Vector3& direction, float maxDist,
                 Vector3& hitPos) const;

    /// @brief デバッグ描画
    void DebugDraw(PrimitiveBatch3D& batch, bool showBlocked = false) const;

    /// @brief X方向のグリッドサイズを取得する
    /// @return X方向のボクセル数
    int GetSizeX() const { return m_sizeX; }

    /// @brief Y方向のグリッドサイズを取得する
    /// @return Y方向のボクセル数
    int GetSizeY() const { return m_sizeY; }

    /// @brief Z方向のグリッドサイズを取得する
    /// @return Z方向のボクセル数
    int GetSizeZ() const { return m_sizeZ; }

    /// @brief ボクセルサイズを取得する
    /// @return ボクセル1辺のサイズ（ワールド単位）
    float GetVoxelSize() const { return m_voxelSize; }

    /// @brief ワールド空間の最小座標を取得する
    /// @return 最小座標
    Vector3 GetMinBounds() const { return m_minBounds; }

    /// @brief Openなボクセル数を取得する
    /// @return 通行可能なボクセルの総数
    uint32_t GetOpenVoxelCount() const;

    /// @brief ボクセルグリッドをクリア
    void Clear();

private:
    int VoxelIndex(int x, int y, int z) const;
    float GetCost(int x, int y, int z) const;
    bool IsInBounds(int x, int y, int z) const;

    gx::Vector<VoxelState> m_voxels;       ///< ボクセル状態配列
    gx::Vector<float> m_costs;             ///< ボクセルごとのコスト倍率
    gx::Vector<OffMeshLink> m_offMeshLinks; ///< Off-meshリンクリスト
    Vector3 m_minBounds = {0, 0, 0};      ///< ワールド空間の最小座標
    float m_voxelSize = 1.0f;               ///< ボクセル1辺のサイズ
    int m_sizeX = 0, m_sizeY = 0, m_sizeZ = 0; ///< 各軸のボクセル数
};

} // namespace gx
/// @}
