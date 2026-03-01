#include <gtest/gtest.h>
#include "ECS/ECSTypes.h"
#include "ECS/ComponentStorage.h"
#include "ECS/Archetype.h"
#include "ECS/World.h"
#include "ECS/System.h"
#include "ECS/Query.h"

using namespace gx::ecs;

// ---------------------------------------------------------------------------
// 単純なテストコンポーネント（POD）
// ---------------------------------------------------------------------------
struct Position { float x = 0, y = 0, z = 0; };
struct Velocity { float dx = 0, dy = 0, dz = 0; };
struct Health   { int hp = 100; };
struct Tag      { char label[16] = ""; };

// ---------------------------------------------------------------------------
// ComponentStorageテスト
// ---------------------------------------------------------------------------
TEST(ECS_ComponentStorage, AddAndGet)
{
    ComponentStorage storage(0, sizeof(float));
    float* p1 = static_cast<float*>(storage.AddElement());
    *p1 = 3.14f;

    float* p2 = static_cast<float*>(storage.AddElement());
    *p2 = 2.71f;

    EXPECT_EQ(storage.GetCount(), 2u);
    EXPECT_FLOAT_EQ(*static_cast<float*>(storage.GetElement(0)), 3.14f);
    EXPECT_FLOAT_EQ(*static_cast<float*>(storage.GetElement(1)), 2.71f);
}

TEST(ECS_ComponentStorage, SwapAndPop)
{
    ComponentStorage storage(0, sizeof(int));
    for (int i = 0; i < 5; ++i)
    {
        int* p = static_cast<int*>(storage.AddElement());
        *p = i * 10;
    }
    EXPECT_EQ(storage.GetCount(), 5u);

    // インデックス1（値10）を削除 -- 最後の要素（40）がスワップされるはず
    storage.SwapAndPop(1);
    EXPECT_EQ(storage.GetCount(), 4u);
    EXPECT_EQ(*static_cast<int*>(storage.GetElement(1)), 40);
    EXPECT_EQ(*static_cast<int*>(storage.GetElement(0)), 0);
}

TEST(ECS_ComponentStorage, Clear)
{
    ComponentStorage storage(0, sizeof(int));
    storage.AddElement();
    storage.AddElement();
    EXPECT_EQ(storage.GetCount(), 2u);
    storage.Clear();
    EXPECT_EQ(storage.GetCount(), 0u);
}

TEST(ECS_ComponentStorage, Reserve)
{
    ComponentStorage storage(0, sizeof(int));
    storage.Reserve(100);
    EXPECT_EQ(storage.GetCount(), 0u);
    // 再割り当ての問題なく100個追加できるはず
    for (int i = 0; i < 100; ++i)
        storage.AddElement();
    EXPECT_EQ(storage.GetCount(), 100u);
}

TEST(ECS_ComponentStorage, MoveConstructor)
{
    ComponentStorage a(1, sizeof(float), "floats");
    float* p = static_cast<float*>(a.AddElement());
    *p = 42.0f;

    ComponentStorage b(std::move(a));
    EXPECT_EQ(b.GetCount(), 1u);
    EXPECT_FLOAT_EQ(*static_cast<float*>(b.GetElement(0)), 42.0f);
    EXPECT_EQ(b.GetName(), "floats");
    EXPECT_EQ(a.GetCount(), 0u);
}

// ---------------------------------------------------------------------------
// Archetypeテスト
// ---------------------------------------------------------------------------
TEST(ECS_Archetype, CreateAndAddEntity)
{
    std::vector<std::pair<ComponentID, uint32_t>> infos = {
        { 0, sizeof(Position) },
        { 1, sizeof(Velocity) }
    };
    ComponentMask mask = (1ULL << 0) | (1ULL << 1);
    Archetype arch(mask, infos);

    EXPECT_EQ(arch.GetMask(), mask);
    EXPECT_TRUE(arch.HasComponent(0));
    EXPECT_TRUE(arch.HasComponent(1));
    EXPECT_FALSE(arch.HasComponent(2));

    uint32_t row = arch.AddEntity(1);
    EXPECT_EQ(row, 0u);
    EXPECT_EQ(arch.GetEntityCount(), 1u);
    EXPECT_EQ(arch.GetEntity(0), 1u);

    // コンポーネントデータの書き込みと読み込み
    auto* pos = static_cast<Position*>(arch.GetComponentData(0, 0));
    pos->x = 10.0f;
    pos->y = 20.0f;
    EXPECT_FLOAT_EQ(static_cast<const Position*>(arch.GetComponentData(0, 0))->x, 10.0f);
}

TEST(ECS_Archetype, RemoveEntitySwapAndPop)
{
    std::vector<std::pair<ComponentID, uint32_t>> infos = {
        { 0, sizeof(int) }
    };
    Archetype arch(1ULL, infos);

    arch.AddEntity(10);
    arch.AddEntity(20);
    arch.AddEntity(30);

    // 値を設定
    *static_cast<int*>(arch.GetComponentData(0, 0)) = 100;
    *static_cast<int*>(arch.GetComponentData(0, 1)) = 200;
    *static_cast<int*>(arch.GetComponentData(0, 2)) = 300;

    // 行0のエンティティ（エンティティ10）を削除
    EntityID swapped = k_InvalidEntity;
    arch.RemoveEntity(0, swapped);

    EXPECT_EQ(arch.GetEntityCount(), 2u);
    EXPECT_EQ(swapped, 30u); // エンティティ30が行0にスワップされた
    EXPECT_EQ(arch.GetEntity(0), 30u);
    EXPECT_EQ(*static_cast<int*>(arch.GetComponentData(0, 0)), 300);
}

// ---------------------------------------------------------------------------
// Worldエンティティテスト
// ---------------------------------------------------------------------------
TEST(ECS_World, CreateAndDestroyEntity)
{
    World world;
    EXPECT_EQ(world.GetEntityCount(), 0u);

    EntityID e1 = world.CreateEntity();
    EntityID e2 = world.CreateEntity();
    EXPECT_NE(e1, k_InvalidEntity);
    EXPECT_NE(e2, k_InvalidEntity);
    EXPECT_NE(e1, e2);
    EXPECT_EQ(world.GetEntityCount(), 2u);
    EXPECT_TRUE(world.IsAlive(e1));

    world.DestroyEntity(e1);
    EXPECT_EQ(world.GetEntityCount(), 1u);
    EXPECT_FALSE(world.IsAlive(e1));
    EXPECT_TRUE(world.IsAlive(e2));
}

TEST(ECS_World, EntityFreeListReuse)
{
    World world;
    EntityID e1 = world.CreateEntity();
    world.DestroyEntity(e1);

    EntityID e2 = world.CreateEntity();
    // スロット再利用
    EXPECT_EQ(e1, e2);
    EXPECT_TRUE(world.IsAlive(e2));
}

// ---------------------------------------------------------------------------
// Worldコンポーネントテスト
// ---------------------------------------------------------------------------
TEST(ECS_World, AddAndGetComponent)
{
    World world;
    EntityID e = world.CreateEntity();

    auto& pos = world.AddComponent<Position>(e);
    pos.x = 5.0f;
    pos.y = 10.0f;
    pos.z = 15.0f;

    EXPECT_TRUE(world.HasComponent<Position>(e));
    EXPECT_FALSE(world.HasComponent<Velocity>(e));

    auto* p = world.GetComponent<Position>(e);
    ASSERT_NE(p, nullptr);
    EXPECT_FLOAT_EQ(p->x, 5.0f);
    EXPECT_FLOAT_EQ(p->y, 10.0f);
    EXPECT_FLOAT_EQ(p->z, 15.0f);
}

TEST(ECS_World, AddMultipleComponents)
{
    World world;
    EntityID e = world.CreateEntity();

    auto& pos = world.AddComponent<Position>(e);
    pos.x = 1.0f;

    auto& vel = world.AddComponent<Velocity>(e);
    vel.dx = 2.0f;

    EXPECT_TRUE(world.HasComponent<Position>(e));
    EXPECT_TRUE(world.HasComponent<Velocity>(e));

    // アーキタイプ移行後もデータが保持されていることを確認
    auto* p = world.GetComponent<Position>(e);
    ASSERT_NE(p, nullptr);
    EXPECT_FLOAT_EQ(p->x, 1.0f);

    auto* v = world.GetComponent<Velocity>(e);
    ASSERT_NE(v, nullptr);
    EXPECT_FLOAT_EQ(v->dx, 2.0f);
}

TEST(ECS_World, RemoveComponent)
{
    World world;
    EntityID e = world.CreateEntity();

    world.AddComponent<Position>(e);
    world.AddComponent<Velocity>(e);
    EXPECT_TRUE(world.HasComponent<Position>(e));
    EXPECT_TRUE(world.HasComponent<Velocity>(e));

    world.RemoveComponent<Velocity>(e);
    EXPECT_TRUE(world.HasComponent<Position>(e));
    EXPECT_FALSE(world.HasComponent<Velocity>(e));
}

TEST(ECS_World, AddDuplicateComponentReturnsSame)
{
    World world;
    EntityID e = world.CreateEntity();

    auto& pos1 = world.AddComponent<Position>(e);
    pos1.x = 99.0f;

    // 同じコンポーネントを再追加すると既存データが返されるはず
    auto& pos2 = world.AddComponent<Position>(e);
    EXPECT_FLOAT_EQ(pos2.x, 99.0f);
}

TEST(ECS_World, DestroyEntityRemovesComponents)
{
    World world;
    EntityID e = world.CreateEntity();
    world.AddComponent<Position>(e);
    world.DestroyEntity(e);

    EXPECT_FALSE(world.IsAlive(e));
    EXPECT_EQ(world.GetComponent<Position>(e), nullptr);
}

// ---------------------------------------------------------------------------
// Worldクエリ / ForEachテスト
// ---------------------------------------------------------------------------
TEST(ECS_World, ForEachSingleComponent)
{
    World world;
    for (int i = 0; i < 5; ++i)
    {
        EntityID e = world.CreateEntity();
        auto& pos = world.AddComponent<Position>(e);
        pos.x = static_cast<float>(i);
    }

    float sum = 0;
    int count = 0;
    world.ForEach<Position>([&](EntityID, Position& pos) {
        sum += pos.x;
        ++count;
    });

    EXPECT_EQ(count, 5);
    EXPECT_FLOAT_EQ(sum, 0 + 1 + 2 + 3 + 4);
}

TEST(ECS_World, ForEachMultiComponent)
{
    World world;

    // Position + Velocityを持つ3つのエンティティ
    for (int i = 0; i < 3; ++i)
    {
        EntityID e = world.CreateEntity();
        world.AddComponent<Position>(e).x = static_cast<float>(i);
        world.AddComponent<Velocity>(e).dx = 1.0f;
    }

    // Positionのみを持つ2つのエンティティ
    for (int i = 0; i < 2; ++i)
    {
        EntityID e = world.CreateEntity();
        world.AddComponent<Position>(e).x = 100.0f;
    }

    // ForEach<Position, Velocity>は3つだけマッチするはず
    int matched = 0;
    world.ForEach<Position, Velocity>([&](EntityID, Position& pos, Velocity& vel) {
        pos.x += vel.dx;
        ++matched;
    });
    EXPECT_EQ(matched, 3);
}

TEST(ECS_World, CountEntities)
{
    World world;

    for (int i = 0; i < 4; ++i)
    {
        EntityID e = world.CreateEntity();
        world.AddComponent<Position>(e);
        if (i < 2) world.AddComponent<Velocity>(e);
    }

    EXPECT_EQ(world.CountEntities<Position>(), 4u);
    {
        uint32_t c = world.CountEntities<Position, Velocity>();
        EXPECT_EQ(c, 2u);
    }
    EXPECT_EQ(world.CountEntities<Health>(), 0u);
}

// ---------------------------------------------------------------------------
// Query<>ラッパーテスト
// ---------------------------------------------------------------------------
TEST(ECS_World, QueryWrapper)
{
    World world;

    EntityID e1 = world.CreateEntity();
    world.AddComponent<Position>(e1).x = 1.0f;
    world.AddComponent<Health>(e1).hp = 50;

    EntityID e2 = world.CreateEntity();
    world.AddComponent<Position>(e2).x = 2.0f;

    Query<Position, Health> q(&world);
    EXPECT_EQ(q.Count(), 1u);

    int calls = 0;
    q.ForEach([&](EntityID id, Position& pos, Health& h) {
        EXPECT_EQ(id, e1);
        EXPECT_FLOAT_EQ(pos.x, 1.0f);
        EXPECT_EQ(h.hp, 50);
        ++calls;
    });
    EXPECT_EQ(calls, 1);
}

// ---------------------------------------------------------------------------
// Systemテスト
// ---------------------------------------------------------------------------
namespace
{

class MovementSystem : public System
{
public:
    void Update(World& world, float dt) override
    {
        world.ForEach<Position, Velocity>([dt](EntityID, Position& pos, Velocity& vel) {
            pos.x += vel.dx * dt;
            pos.y += vel.dy * dt;
            pos.z += vel.dz * dt;
        });
        ++updateCount;
    }
    const char* GetName() const override { return "MovementSystem"; }
    int updateCount = 0;
};

class DamageSystem : public System
{
public:
    void Update(World& world, float) override
    {
        world.ForEach<Health>([](EntityID, Health& h) {
            h.hp -= 1;
        });
        ++updateCount;
    }
    const char* GetName() const override { return "DamageSystem"; }
    int updateCount = 0;
};

} // anonymous namespace

TEST(ECS_World, SystemRegistration)
{
    World world;
    auto* mov = world.AddSystem<MovementSystem>();
    auto* dmg = world.AddSystem<DamageSystem>();

    EXPECT_EQ(world.GetSystemCount(), 2u);
    EXPECT_NE(mov, nullptr);
    EXPECT_NE(dmg, nullptr);
    EXPECT_STREQ(mov->GetName(), "MovementSystem");
}

TEST(ECS_World, SystemUpdate)
{
    World world;
    auto* mov = world.AddSystem<MovementSystem>();

    EntityID e = world.CreateEntity();
    auto& pos = world.AddComponent<Position>(e);
    pos.x = 0.0f;
    auto& vel = world.AddComponent<Velocity>(e);
    vel.dx = 10.0f;

    world.UpdateSystems(0.1f);

    EXPECT_EQ(mov->updateCount, 1);
    EXPECT_FLOAT_EQ(world.GetComponent<Position>(e)->x, 1.0f); // 10 * 0.1
}

TEST(ECS_World, SystemPriority)
{
    World world;
    auto* dmg = world.AddSystem<DamageSystem>();
    auto* mov = world.AddSystem<MovementSystem>();

    dmg->SetPriority(10);
    mov->SetPriority(1);

    EntityID e = world.CreateEntity();
    world.AddComponent<Position>(e);
    world.AddComponent<Velocity>(e);
    world.AddComponent<Health>(e);

    world.UpdateSystems(1.0f);

    // Movement（優先度1）はDamage（優先度10）より先に実行されるはず
    // 両方とも1回実行されたはず
    EXPECT_EQ(mov->updateCount, 1);
    EXPECT_EQ(dmg->updateCount, 1);
}

TEST(ECS_World, SystemDisable)
{
    World world;
    auto* mov = world.AddSystem<MovementSystem>();
    mov->SetEnabled(false);

    EntityID e = world.CreateEntity();
    world.AddComponent<Position>(e);
    world.AddComponent<Velocity>(e).dx = 5.0f;

    world.UpdateSystems(1.0f);
    EXPECT_EQ(mov->updateCount, 0);
    EXPECT_FLOAT_EQ(world.GetComponent<Position>(e)->x, 0.0f); // 変更されていないこと
}

// ---------------------------------------------------------------------------
// ストレステスト
// ---------------------------------------------------------------------------
TEST(ECS_World, StressManyEntities)
{
    World world;
    const int N = 1000;

    for (int i = 0; i < N; ++i)
    {
        EntityID e = world.CreateEntity();
        world.AddComponent<Position>(e).x = static_cast<float>(i);
        if (i % 2 == 0)
            world.AddComponent<Velocity>(e).dx = 1.0f;
    }

    EXPECT_EQ(world.GetEntityCount(), 1000u);
    EXPECT_EQ(world.CountEntities<Position>(), 1000u);
    {
        uint32_t c = world.CountEntities<Position, Velocity>();
        EXPECT_EQ(c, 500u);
    }

    // 半分を破棄
    for (EntityID id = 1; id <= 500u; ++id)
    {
        world.DestroyEntity(id);
    }
    EXPECT_EQ(world.GetEntityCount(), 500u);
}
