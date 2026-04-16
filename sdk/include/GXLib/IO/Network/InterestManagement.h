#pragma once
/// @file InterestManagement.h
/// @brief インタレスト管理（ネットワーク同期最適化）
///
/// 大規模マルチプレイヤーゲームにおけるネットワーク帯域最適化のため、
/// 各プレイヤーの「関心領域 (Area of Interest)」に基づいて
/// 同期対象エンティティを動的にフィルタリングする。
/// 空間グリッドで高速なクエリを実現し、距離ベースの優先度で
/// 更新頻度を Near/Mid/Far に分類する。
/// @addtogroup grp_network/// @{

#include "pch_common.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace gx
{

/// @brief インタレスト管理設定
struct InterestConfig
{
    float aoiRadius     = 100.0f;   ///< 関心領域半径
    float gridCellSize  = 50.0f;    ///< グリッドセルサイズ
    uint32_t nearInterval = 1;      ///< Near更新間隔(フレーム)
    uint32_t midInterval  = 3;      ///< Mid更新間隔
    uint32_t farInterval  = 10;     ///< Far更新間隔
    float nearThreshold   = 0.3f;   ///< Near判定閾値 (AOI比率)
    float farThreshold    = 0.7f;   ///< Far判定閾値 (AOI比率)
};

/// @brief エンティティ位置情報
struct InterestEntity
{
    uint32_t entityId = 0;  ///< エンティティID
    float x = 0.0f;         ///< X座標
    float y = 0.0f;         ///< Y座標
    float z = 0.0f;         ///< Z座標
};

/// @brief 優先度レベル
enum class InterestPriority
{
    Near,   ///< 高頻度更新
    Mid,    ///< 中頻度
    Far     ///< 低頻度
};

/// @brief インタレスト管理システム
class InterestManagement
{
public:
    InterestManagement() = default;
    ~InterestManagement() = default;

    /// @brief 初期化
    /// @param config インタレスト管理設定
    void Initialize(const InterestConfig& config = {});

    /// @brief 設定取得
    /// @return 現在の設定への参照
    const InterestConfig& GetConfig() const { return m_config; }

    /// @brief エンティティ登録
    /// @param entityId 登録するエンティティID
    /// @param x X座標
    /// @param y Y座標
    /// @param z Z座標
    void RegisterEntity(uint32_t entityId, float x, float y, float z);

    /// @brief エンティティ位置更新
    /// @param entityId 更新するエンティティID
    /// @param x 新しいX座標
    /// @param y 新しいY座標
    /// @param z 新しいZ座標
    void UpdateEntityPosition(uint32_t entityId, float x, float y, float z);

    /// @brief エンティティ削除
    /// @param entityId 削除するエンティティID
    void RemoveEntity(uint32_t entityId);

    /// @brief エンティティ数
    /// @return 登録済みエンティティ数
    uint32_t GetEntityCount() const { return static_cast<uint32_t>(m_entities.size()); }

    /// @brief 関連エンティティクエリ
    /// @param observerEntityId 観測者のエンティティID
    /// @return 関心領域内のエンティティIDリスト
    gx::Vector<uint32_t> QueryRelevantEntities(uint32_t observerEntityId) const;

    /// @brief 距離ベース優先度を計算
    /// @param observerEntity 観測者エンティティID
    /// @param targetEntity 対象エンティティID
    /// @return 優先度レベル
    InterestPriority GetPriority(uint32_t observerEntity, uint32_t targetEntity) const;

    /// @brief 指定フレームで更新すべきか判定
    /// @param priority 優先度レベル
    /// @param frameNumber 現在のフレーム番号
    /// @return 更新すべき場合true
    bool ShouldUpdate(InterestPriority priority, uint32_t frameNumber) const;

    /// @brief グリッドセルID計算
    /// @param x X座標
    /// @param z Z座標
    /// @return セルID
    int64_t GetCellId(float x, float z) const;

    /// @brief 2エンティティ間の距離
    /// @param entityA エンティティA
    /// @param entityB エンティティB
    /// @return 距離
    float GetDistance(uint32_t entityA, uint32_t entityB) const;

    /// @brief 全エンティティクリア
    void Clear();

private:
    void UpdateGrid(uint32_t entityId);

    InterestConfig m_config;  ///< インタレスト管理設定
    gx::HashMap<uint32_t, InterestEntity> m_entities;  ///< エンティティID→位置情報マップ

    /// グリッド: cellId → エンティティIDセット
    gx::HashMap<int64_t, gx::HashSet<uint32_t>> m_grid;
};

} // namespace gx
/// @}
