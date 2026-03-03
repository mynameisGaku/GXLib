#pragma once
/// @file World.h
/// @brief ECSワールド（エンティティ・コンポーネント・システムを一元管理）
///
/// ECSの中心となるコンテナ。エンティティの生成/削除、コンポーネントの
/// 追加/取得、システムの実行をすべて管理する。
/// CreateEntity()でエンティティを作り、AddComponent<T>()でデータを付ける。
/// @addtogroup grp_ecs/// @{

#include "pch_common.h"
#include "ECS/ECSTypes.h"
#include "ECS/Archetype.h"
#include "ECS/Query.h"
#include "ECS/System.h"

namespace gx { namespace ecs {

// ---------------------------------------------------------------------------
// コンパイル時コンポーネントIDジェネレータ（型ごとの静的カウンタ）
// ---------------------------------------------------------------------------
namespace detail
{

inline ComponentID& NextComponentID()
{
    static ComponentID s_next = 0;
    return s_next;
}

} // namespace detail

/// @brief 各C++型Tに対して一意で安定したComponentIDを生成する
template<typename T>
struct ComponentTypeID
{
    static ComponentID ID()
    {
        static ComponentID id = detail::NextComponentID()++;
        return id;
    }
};

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

/// @brief ECSワールド — エンティティ、コンポーネント、アーキタイプ、システムを管理する
class World
{
public:
    World();
    ~World();

    // -----------------------------------------------------------------------
    // エンティティ管理
    // -----------------------------------------------------------------------

    /// @brief 新しいエンティティを作成する（IDを返す）
    EntityID CreateEntity();

    /// @brief エンティティを破棄しアーキタイプから削除する
    void DestroyEntity(EntityID entity);

    /// @brief エンティティがまだ生存しているか確認する
    bool IsAlive(EntityID entity) const;

    /// @brief 現在生存しているエンティティの数
    uint32_t GetEntityCount() const { return m_aliveCount; }

    // -----------------------------------------------------------------------
    // コンポーネント管理（型付き）
    // -----------------------------------------------------------------------

    /// @brief 型Tのコンポーネントを追加（デフォルト構築）し参照を返す
    template<typename T>
    T& AddComponent(EntityID entity)
    {
        ComponentID cid = ComponentTypeID<T>::ID();
        EnsureComponentRegistered<T>(cid);
        return *static_cast<T*>(AddComponentRaw(entity, cid, sizeof(T)));
    }

    /// @brief 型Tのコンポーネントへのポインタを取得する（存在しない場合nullptr）
    template<typename T>
    T* GetComponent(EntityID entity)
    {
        ComponentID cid = ComponentTypeID<T>::ID();
        return static_cast<T*>(GetComponentRaw(entity, cid));
    }

    /// @brief エンティティがコンポーネントTを持つか確認する
    template<typename T>
    bool HasComponent(EntityID entity)
    {
        ComponentID cid = ComponentTypeID<T>::ID();
        return HasComponentRaw(entity, cid);
    }

    /// @brief エンティティからコンポーネントTを削除する
    template<typename T>
    void RemoveComponent(EntityID entity)
    {
        ComponentID cid = ComponentTypeID<T>::ID();
        RemoveComponentRaw(entity, cid);
    }

    // -----------------------------------------------------------------------
    // クエリ（型付き）
    // -----------------------------------------------------------------------

    /// @brief Ts...のすべてを持つ各エンティティに対してfnを実行する
    template<typename... Ts>
    void ForEach(std::function<void(EntityID, Ts&...)> fn)
    {
        ComponentMask mask = BuildMask<Ts...>();
        ForEachRaw(mask, [&](EntityID eid, Archetype& arch, uint32_t row) {
            fn(eid, *static_cast<Ts*>(arch.GetComponentData(ComponentTypeID<Ts>::ID(), row))...);
        });
    }

    /// @brief コンポーネントセットTs...に一致するエンティティ数を返す
    template<typename... Ts>
    uint32_t CountEntities()
    {
        ComponentMask mask = BuildMask<Ts...>();
        uint32_t count = 0;
        for (auto& arch : m_archetypes)
        {
            if ((arch.GetMask() & mask) == mask)
                count += arch.GetEntityCount();
        }
        return count;
    }

    // -----------------------------------------------------------------------
    // システム
    // -----------------------------------------------------------------------

    /// @brief 型Tのシステムを追加する（argsで構築）
    template<typename T, typename... Args>
    T* AddSystem(Args&&... args)
    {
        auto sys = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = sys.get();
        m_systems.push_back(std::move(sys));
        return ptr;
    }

    /// @brief 有効なすべてのシステムを実行する（優先度順にソート）
    void UpdateSystems(float deltaTime);

    /// @brief 登録されたシステムの数
    uint32_t GetSystemCount() const { return static_cast<uint32_t>(m_systems.size()); }

    // -----------------------------------------------------------------------
    // 低レベルアクセス（Query<>テンプレートが使用）
    // -----------------------------------------------------------------------

    /// @brief 指定マスクのスーパーセットであるすべてのアーキタイプを反復する
    void ForEachRaw(ComponentMask mask,
                    std::function<void(EntityID, Archetype&, uint32_t)> fn);

private:
    // --- コンポーネント登録管理 ---
    template<typename T>
    void EnsureComponentRegistered(ComponentID cid)
    {
        if (m_componentSizes.find(cid) == m_componentSizes.end())
        {
            m_componentSizes[cid] = sizeof(T);
            m_componentNames[cid] = typeid(T).name();
        }
    }

    template<typename... Ts>
    ComponentMask BuildMask()
    {
        ComponentMask mask = 0;
        ((mask |= (1ULL << ComponentTypeID<Ts>::ID())), ...);
        return mask;
    }

    // --- 生（型なし）操作 ---
    void* AddComponentRaw(EntityID entity, ComponentID cid, uint32_t size);
    void* GetComponentRaw(EntityID entity, ComponentID cid);
    bool  HasComponentRaw(EntityID entity, ComponentID cid);
    void  RemoveComponentRaw(EntityID entity, ComponentID cid);

    // --- アーキタイプ管理 ---
    Archetype& FindOrCreateArchetype(ComponentMask mask);
    void MoveEntity(EntityID entity, ComponentMask newMask);

    // --- 内部ヘルパー ---
    ComponentMask GetEntityMask(EntityID entity) const;

    // --- データ ---
    gx::Vector<EntityRecord> m_entities;   ///< EntityIDでインデックス（スロット0は未使用）
    gx::Vector<Archetype> m_archetypes;   ///< 全アーキタイプのリスト
    gx::Vector<EntityID> m_freeList;      ///< 再利用可能なEntityIDのフリーリスト
    uint32_t m_aliveCount = 0;             ///< 現在生存中のエンティティ数
    EntityID m_nextEntity = 1;             ///< 次に割り当てるEntityID

    gx::HashMap<ComponentID, uint32_t> m_componentSizes;   ///< コンポーネントIDごとのバイトサイズ
    gx::HashMap<ComponentID, gx::String> m_componentNames; ///< コンポーネントIDごとのデバッグ名
    gx::Vector<std::unique_ptr<System>> m_systems;                ///< 登録されたシステムのリスト
};

// ---------------------------------------------------------------------------
// Query<>の実装（完全なWorld定義が必要）
// ---------------------------------------------------------------------------
template<typename... Ts>
void Query<Ts...>::ForEach(std::function<void(EntityID, Ts&...)> fn)
{
    m_world->ForEach<Ts...>(std::move(fn));
}

template<typename... Ts>
uint32_t Query<Ts...>::Count() const
{
    return const_cast<World*>(m_world)->CountEntities<Ts...>();
}

}} // namespace gx::ecs
/// @}
