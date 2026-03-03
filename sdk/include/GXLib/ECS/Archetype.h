#pragma once
/// @file Archetype.h
/// @brief アーキタイプ（同じコンポーネント構成を持つエンティティのグループ）
///
/// 「Position + Velocity を持つエンティティ全員」のように、
/// 同じコンポーネント型の組み合わせを持つエンティティを1つの
/// アーキタイプにまとめてデータを連続配置する。ECS高速化の核心。
/// @addtogroup grp_ecs/// @{

#include "pch_common.h"
#include "ECS/ECSTypes.h"
#include "ECS/ComponentStorage.h"

namespace gx { namespace ecs {

/// @brief アーキタイプは、まったく同じコンポーネント型のセットを持つすべての
///        エンティティをグループ化する。各コンポーネント型は独自の連続ストレージ
///        列を持ち、エンティティは行として扱われる。
class Archetype
{
public:
    /// @param mask           このアーキタイプのコンポーネントIDビットマスク
    /// @param componentInfos (ComponentID, バイト単位のコンポーネントサイズ)のペア
    Archetype(ComponentMask mask,
              const gx::Vector<std::pair<ComponentID, uint32_t>>& componentInfos);

    // ムーブのみ（ComponentStorageはコピー不可）
    Archetype(Archetype&&) noexcept = default;
    Archetype& operator=(Archetype&&) noexcept = default;
    Archetype(const Archetype&) = delete;
    Archetype& operator=(const Archetype&) = delete;

    /// @brief このアーキタイプのコンポーネントマスクを取得する
    ComponentMask GetMask() const { return m_mask; }

    /// @brief このアーキタイプが特定のコンポーネントを持つか確認する
    /// @param id 確認するコンポーネント型ID
    /// @return 指定コンポーネントを含む場合true
    bool HasComponent(ComponentID id) const;

    /// @brief このアーキタイプにエンティティを追加する（全ストレージに行を追加）
    /// @return エンティティに割り当てられた行インデックス
    uint32_t AddEntity(EntityID entity);

    /// @brief 指定行のエンティティを削除する（スワップアンドポップ）
    /// @param row           削除する行
    /// @param swappedEntity [out] この行にスワップされたエンティティ
    ///                      （行が最後の要素だった場合はk_InvalidEntity）
    void RemoveEntity(uint32_t row, EntityID& swappedEntity);

    /// @brief 指定行のコンポーネントへのポインタを取得する
    /// @param id  取得するコンポーネント型ID
    /// @param row アーキタイプ内の行インデックス
    /// @return コンポーネントデータへのポインタ
    void* GetComponentData(ComponentID id, uint32_t row);
    /// @copydoc GetComponentData(ComponentID, uint32_t)
    const void* GetComponentData(ComponentID id, uint32_t row) const;

    /// @brief 特定コンポーネントのストレージを取得する（存在しない場合nullptr）
    /// @param id 取得するコンポーネント型ID
    /// @return ストレージへのポインタ（存在しない場合nullptr）
    ComponentStorage* GetStorage(ComponentID id);

    /// @brief このアーキタイプ内のエンティティ数
    uint32_t GetEntityCount() const { return static_cast<uint32_t>(m_entities.size()); }

    /// @brief 指定行のエンティティIDを取得する
    EntityID GetEntity(uint32_t row) const { return m_entities[row]; }

    /// @brief すべてのエンティティIDを取得する
    const gx::Vector<EntityID>& GetEntities() const { return m_entities; }

private:
    ComponentMask m_mask;                                       ///< このアーキタイプのコンポーネントビットマスク
    gx::Vector<EntityID> m_entities;                            ///< このアーキタイプに属するエンティティIDリスト
    gx::HashMap<ComponentID, uint32_t> m_componentIndex;  ///< ComponentID -> ストレージインデックス
    gx::Vector<ComponentStorage> m_storages;                    ///< コンポーネント型ごとの連続ストレージ
};

}} // namespace gx::ecs
/// @}
