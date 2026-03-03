/// @file World.cpp
/// @brief World の実装
#include "pch_common.h"
#include "ECS/World.h"
#include "Core/Logger.h"

namespace gx { namespace ecs {

// ---------------------------------------------------------------------------
// QueryCache
// ---------------------------------------------------------------------------
const gx::Vector<Archetype*>& QueryCache::GetMatchingArchetypes(
    ComponentMask mask, gx::Vector<Archetype>& allArchetypes)
{
    uint64_t key = static_cast<uint64_t>(mask);

    auto it = m_cache.find(key);
    if (it != m_cache.end() && it->second.version == m_version)
    {
        // キャッシュヒット — バージョンが一致するのでそのまま返す
        return it->second.archetypes;
    }

    // キャッシュミスまたは古いバージョン — 再構築
    CacheEntry entry;
    entry.version = m_version;

    for (auto& arch : allArchetypes)
    {
        if ((arch.GetMask() & mask) == mask)
        {
            entry.archetypes.push_back(&arch);
        }
    }

    m_cache[key] = std::move(entry);
    return m_cache[key].archetypes;
}

// ---------------------------------------------------------------------------
// 構築 / 破棄
// ---------------------------------------------------------------------------
World::World()
{
    // スロット0を予約（k_InvalidEntity）
    m_entities.push_back(EntityRecord{});
}

World::~World() = default;

// ---------------------------------------------------------------------------
// エンティティ管理
// ---------------------------------------------------------------------------
EntityID World::CreateEntity()
{
    EntityID id;

    if (!m_freeList.empty())
    {
        // 解放されたスロットを再利用
        id = m_freeList.back();
        m_freeList.pop_back();
        m_entities[id].id = id;
        m_entities[id].alive = true;
        m_entities[id].archetypeIndex = UINT32_MAX;
        m_entities[id].rowIndex = UINT32_MAX;
    }
    else
    {
        // 新しいスロットを割り当て
        id = m_nextEntity++;
        EntityRecord rec;
        rec.id = id;
        rec.alive = true;
        m_entities.push_back(rec);
    }

    ++m_aliveCount;
    return id;
}

void World::DestroyEntity(EntityID entity)
{
    if (entity == k_InvalidEntity || entity >= m_entities.size()) return;
    EntityRecord& rec = m_entities[entity];
    if (!rec.alive) return;

    // アーキタイプに割り当てられている場合は削除
    if (rec.archetypeIndex != UINT32_MAX)
    {
        Archetype& arch = m_archetypes[rec.archetypeIndex];
        EntityID swapped = k_InvalidEntity;
        arch.RemoveEntity(rec.rowIndex, swapped);

        // 別のエンティティがこの行にスワップされた場合、そのレコードを更新
        if (swapped != k_InvalidEntity)
        {
            m_entities[swapped].rowIndex = rec.rowIndex;
        }
    }

    rec.alive = false;
    rec.archetypeIndex = UINT32_MAX;
    rec.rowIndex = UINT32_MAX;
    m_freeList.push_back(entity);
    --m_aliveCount;
}

bool World::IsAlive(EntityID entity) const
{
    if (entity == k_InvalidEntity || entity >= m_entities.size()) return false;
    return m_entities[entity].alive;
}

// ---------------------------------------------------------------------------
// コンポーネント管理（生 / 型なし）
// ---------------------------------------------------------------------------
void* World::AddComponentRaw(EntityID entity, ComponentID cid, uint32_t size)
{
    assert(entity != k_InvalidEntity && entity < m_entities.size());
    assert(m_entities[entity].alive);
    assert(cid < k_MaxComponents);

    // コンポーネントサイズが登録されていることを確認
    if (m_componentSizes.find(cid) == m_componentSizes.end())
        m_componentSizes[cid] = size;

    EntityRecord& rec = m_entities[entity];
    ComponentMask oldMask = GetEntityMask(entity);
    ComponentMask bit = 1ULL << cid;

    // 既にこのコンポーネントを持っている？
    if (oldMask & bit)
    {
        // 既存のデータを返す
        Archetype& arch = m_archetypes[rec.archetypeIndex];
        return arch.GetComponentData(cid, rec.rowIndex);
    }

    ComponentMask newMask = oldMask | bit;
    MoveEntity(entity, newMask);

    // 新しく追加された（ゼロ初期化済み）コンポーネントへのポインタを返す
    Archetype& arch = m_archetypes[rec.archetypeIndex];
    return arch.GetComponentData(cid, rec.rowIndex);
}

void* World::GetComponentRaw(EntityID entity, ComponentID cid)
{
    if (entity == k_InvalidEntity || entity >= m_entities.size()) return nullptr;
    const EntityRecord& rec = m_entities[entity];
    if (!rec.alive || rec.archetypeIndex == UINT32_MAX) return nullptr;

    Archetype& arch = m_archetypes[rec.archetypeIndex];
    if (!arch.HasComponent(cid)) return nullptr;
    return arch.GetComponentData(cid, rec.rowIndex);
}

bool World::HasComponentRaw(EntityID entity, ComponentID cid)
{
    if (entity == k_InvalidEntity || entity >= m_entities.size()) return false;
    const EntityRecord& rec = m_entities[entity];
    if (!rec.alive || rec.archetypeIndex == UINT32_MAX) return false;
    return (m_archetypes[rec.archetypeIndex].GetMask() & (1ULL << cid)) != 0;
}

void World::RemoveComponentRaw(EntityID entity, ComponentID cid)
{
    if (entity == k_InvalidEntity || entity >= m_entities.size()) return;
    EntityRecord& rec = m_entities[entity];
    if (!rec.alive || rec.archetypeIndex == UINT32_MAX) return;

    ComponentMask oldMask = GetEntityMask(entity);
    ComponentMask bit = 1ULL << cid;
    if (!(oldMask & bit)) return; // このコンポーネントを持っていない

    ComponentMask newMask = oldMask & ~bit;

    if (newMask == 0)
    {
        // エンティティにコンポーネントが残っていない — アーキタイプから完全に削除
        Archetype& arch = m_archetypes[rec.archetypeIndex];
        EntityID swapped = k_InvalidEntity;
        arch.RemoveEntity(rec.rowIndex, swapped);
        if (swapped != k_InvalidEntity)
            m_entities[swapped].rowIndex = rec.rowIndex;
        rec.archetypeIndex = UINT32_MAX;
        rec.rowIndex = UINT32_MAX;
    }
    else
    {
        MoveEntity(entity, newMask);
    }
}

// ---------------------------------------------------------------------------
// アーキタイプ管理
// ---------------------------------------------------------------------------
ComponentMask World::GetEntityMask(EntityID entity) const
{
    const EntityRecord& rec = m_entities[entity];
    if (rec.archetypeIndex == UINT32_MAX) return 0;
    return m_archetypes[rec.archetypeIndex].GetMask();
}

Archetype& World::FindOrCreateArchetype(ComponentMask mask)
{
    // 線形探索（アーキタイプ数は通常少ない）
    for (size_t i = 0; i < m_archetypes.size(); ++i)
    {
        if (m_archetypes[i].GetMask() == mask)
            return m_archetypes[i];
    }

    // マスクからコンポーネント情報リストを構築
    gx::Vector<std::pair<ComponentID, uint32_t>> infos;
    for (ComponentID c = 0; c < k_MaxComponents; ++c)
    {
        if (mask & (1ULL << c))
        {
            uint32_t size = 0;
            auto it = m_componentSizes.find(c);
            if (it != m_componentSizes.end())
                size = it->second;
            infos.emplace_back(c, size);
        }
    }

    m_archetypes.emplace_back(mask, infos);

    // 新しいアーキタイプが追加されたのでクエリキャッシュを無効化
    m_queryCache.Invalidate();

    return m_archetypes.back();
}

void World::MoveEntity(EntityID entity, ComponentMask newMask)
{
    EntityRecord& rec = m_entities[entity];
    ComponentMask oldMask = GetEntityMask(entity);

    // 移動先アーキタイプを検索または作成
    Archetype& newArch = FindOrCreateArchetype(newMask);

    // emplace_backで参照が無効化された可能性があるため
    // 移動先アーキタイプインデックスを再解決する必要がある
    uint32_t newArchIdx = UINT32_MAX;
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_archetypes.size()); ++i)
    {
        if (m_archetypes[i].GetMask() == newMask)
        {
            newArchIdx = i;
            break;
        }
    }
    assert(newArchIdx != UINT32_MAX);

    // 新しいアーキタイプにエンティティを追加
    uint32_t newRow = m_archetypes[newArchIdx].AddEntity(entity);

    // 旧アーキタイプから既存のコンポーネントデータをコピー
    if (rec.archetypeIndex != UINT32_MAX)
    {
        Archetype& oldArch = m_archetypes[rec.archetypeIndex];
        ComponentMask commonMask = oldMask & newMask;

        for (ComponentID c = 0; c < k_MaxComponents; ++c)
        {
            if (commonMask & (1ULL << c))
            {
                const void* src = oldArch.GetComponentData(c, rec.rowIndex);
                void* dst = m_archetypes[newArchIdx].GetComponentData(c, newRow);
                if (src && dst)
                {
                    auto it = m_componentSizes.find(c);
                    if (it != m_componentSizes.end())
                        memcpy(dst, src, it->second);
                }
            }
        }

        // 旧アーキタイプから削除
        EntityID swapped = k_InvalidEntity;
        oldArch.RemoveEntity(rec.rowIndex, swapped);
        if (swapped != k_InvalidEntity)
        {
            m_entities[swapped].rowIndex = rec.rowIndex;
        }
    }

    // エンティティレコードを更新
    rec.archetypeIndex = newArchIdx;
    rec.rowIndex = newRow;
}

// ---------------------------------------------------------------------------
// クエリ（生）
// ---------------------------------------------------------------------------
void World::ForEachRaw(ComponentMask mask,
                       std::function<void(EntityID, Archetype&, uint32_t)> fn)
{
    const auto& matching = m_queryCache.GetMatchingArchetypes(mask, m_archetypes);

    for (auto* arch : matching)
    {
        uint32_t count = arch->GetEntityCount();
        for (uint32_t row = 0; row < count; ++row)
        {
            fn(arch->GetEntity(row), *arch, row);
        }
    }
}

// ---------------------------------------------------------------------------
// システム
// ---------------------------------------------------------------------------
void World::UpdateSystems(float deltaTime)
{
    // 優先度でソート（同一優先度の場合は登録順を維持するためstable_sort）
    std::stable_sort(m_systems.begin(), m_systems.end(),
        [](const std::unique_ptr<System>& a, const std::unique_ptr<System>& b) {
            return a->GetPriority() < b->GetPriority();
        });

    for (auto& sys : m_systems)
    {
        if (sys->IsEnabled())
        {
            sys->Update(*this, deltaTime);
        }
    }
}

}} // namespace gx::ecs
