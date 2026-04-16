/// @file test_ECSQueryCache.cpp
/// @brief ECSクエリキャッシュのテスト

#include <gtest/gtest.h>
#include "ECS/World.h"

using namespace gx::ecs;

// テスト用コンポーネント（test_ECS.cppとは別のTUなのでIDが異なる可能性があるが問題ない）
namespace qc_test
{
    struct Pos   { float x = 0, y = 0, z = 0; };
    struct Vel   { float dx = 0, dy = 0, dz = 0; };
    struct HP    { int hp = 100; };
    struct Mana  { int mp = 50; };
    struct Armor { int def = 10; };
}

// ============================================================================
// QueryCache 基本テスト
// ============================================================================

TEST(ECS_QueryCache, DefaultVersion)
{
    World world;
    EXPECT_EQ(world.GetQueryCache().GetVersion(), 0u);
    EXPECT_EQ(world.GetQueryCache().GetCacheEntryCount(), 0u);
}

TEST(ECS_QueryCache, ForEachPopulatesCache)
{
    World world;

    EntityID e = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e);

    // 初回ForEachでキャッシュが構築される
    int count = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count; });
    EXPECT_EQ(count, 1);

    // キャッシュにエントリが追加されたはず
    EXPECT_GE(world.GetQueryCache().GetCacheEntryCount(), 1u);
}

TEST(ECS_QueryCache, CacheHitOnSecondCall)
{
    World world;

    for (int i = 0; i < 10; ++i)
    {
        EntityID e = world.CreateEntity();
        world.AddComponent<qc_test::Pos>(e).x = static_cast<float>(i);
    }

    // 1回目
    int count1 = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count1; });
    EXPECT_EQ(count1, 10);

    uint32_t version1 = world.GetQueryCache().GetVersion();

    // 2回目 — エンティティ構成変更なし、キャッシュヒット
    int count2 = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count2; });
    EXPECT_EQ(count2, 10);

    // バージョンが変わっていないことを確認
    EXPECT_EQ(world.GetQueryCache().GetVersion(), version1);
}

TEST(ECS_QueryCache, InvalidateOnNewArchetype)
{
    World world;

    EntityID e1 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e1);

    // ForEachでキャッシュ構築
    int count = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count; });
    EXPECT_EQ(count, 1);

    uint32_t versionBefore = world.GetQueryCache().GetVersion();

    // 新しいアーキタイプを生成（Pos+Vel）— キャッシュが無効化されるはず
    EntityID e2 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e2);
    world.AddComponent<qc_test::Vel>(e2);

    // バージョンが進んでいることを確認
    EXPECT_GT(world.GetQueryCache().GetVersion(), versionBefore);

    // 2回目のForEachで再構築され、正しい結果を返す
    count = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count; });
    EXPECT_EQ(count, 2); // e1とe2の両方
}

TEST(ECS_QueryCache, CountEntitiesUsesCache)
{
    World world;

    for (int i = 0; i < 5; ++i)
    {
        EntityID e = world.CreateEntity();
        world.AddComponent<qc_test::Pos>(e);
    }
    for (int i = 0; i < 3; ++i)
    {
        EntityID e = world.CreateEntity();
        world.AddComponent<qc_test::Pos>(e);
        world.AddComponent<qc_test::Vel>(e);
    }

    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 8u);
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::Vel>();
        EXPECT_EQ(c, 3u);
    }
    EXPECT_EQ(world.CountEntities<qc_test::HP>(), 0u);
}

TEST(ECS_QueryCache, MultipleQueryTypes)
{
    World world;

    EntityID e1 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e1);

    EntityID e2 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e2);
    world.AddComponent<qc_test::Vel>(e2);

    EntityID e3 = world.CreateEntity();
    world.AddComponent<qc_test::HP>(e3);

    // 異なるクエリタイプがそれぞれ正しくキャッシュされる
    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 2u);
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::Vel>();
        EXPECT_EQ(c, 1u);
    }
    EXPECT_EQ(world.CountEntities<qc_test::HP>(), 1u);

    // 3つのクエリがキャッシュされたはず
    EXPECT_GE(world.GetQueryCache().GetCacheEntryCount(), 3u);
}

TEST(ECS_QueryCache, ForEachAfterEntityDestroy)
{
    World world;

    EntityID e1 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e1).x = 1.0f;

    EntityID e2 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e2).x = 2.0f;

    // ForEach前にキャッシュ構築
    int count = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count; });
    EXPECT_EQ(count, 2);

    // エンティティ破棄
    world.DestroyEntity(e1);

    // ForEachが正しくエンティティ数を反映する
    count = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count; });
    EXPECT_EQ(count, 1);
}

TEST(ECS_QueryCache, ForEachAfterComponentRemoval)
{
    World world;

    EntityID e1 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e1);
    world.AddComponent<qc_test::Vel>(e1);

    EntityID e2 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e2);
    world.AddComponent<qc_test::Vel>(e2);

    // 2エンティティが Pos+Vel を持つ
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::Vel>();
        EXPECT_EQ(c, 2u);
    }

    // e1からVelを削除 — 新アーキタイプ(Posのみ)が作成される
    world.RemoveComponent<qc_test::Vel>(e1);

    // Pos+Velは1つだけ
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::Vel>();
        EXPECT_EQ(c, 1u);
    }
    // Posは2つ
    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 2u);
}

TEST(ECS_QueryCache, ClearCache)
{
    World world;

    EntityID e = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e);

    world.CountEntities<qc_test::Pos>();
    EXPECT_GE(world.GetQueryCache().GetCacheEntryCount(), 1u);

    world.GetQueryCache().Clear();
    EXPECT_EQ(world.GetQueryCache().GetCacheEntryCount(), 0u);

    // Clearの後もForEachが正常に動作する（再構築される）
    int count = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count; });
    EXPECT_EQ(count, 1);
}

TEST(ECS_QueryCache, DataIntegrity)
{
    World world;

    // 複数のアーキタイプにまたがるエンティティ
    EntityID e1 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e1).x = 10.0f;

    EntityID e2 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e2).x = 20.0f;
    world.AddComponent<qc_test::Vel>(e2).dx = 5.0f;

    EntityID e3 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e3).x = 30.0f;
    world.AddComponent<qc_test::Vel>(e3).dx = 10.0f;
    world.AddComponent<qc_test::HP>(e3).hp = 75;

    // Pos を持つ全エンティティの合計
    float sum = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos& p) { sum += p.x; });
    EXPECT_FLOAT_EQ(sum, 60.0f); // 10 + 20 + 30

    // Pos+Vel を持つエンティティの合計
    float velSum = 0;
    world.ForEach<qc_test::Pos, qc_test::Vel>([&](EntityID, qc_test::Pos& p, qc_test::Vel& v) {
        velSum += p.x + v.dx;
    });
    EXPECT_FLOAT_EQ(velSum, 65.0f); // (20+5) + (30+10)
}

TEST(ECS_QueryCache, StressTest)
{
    World world;
    const int N = 500;

    for (int i = 0; i < N; ++i)
    {
        EntityID e = world.CreateEntity();
        world.AddComponent<qc_test::Pos>(e).x = static_cast<float>(i);
        if (i % 2 == 0)
            world.AddComponent<qc_test::Vel>(e).dx = 1.0f;
        if (i % 3 == 0)
            world.AddComponent<qc_test::HP>(e).hp = i;
    }

    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), static_cast<uint32_t>(N));

    // Pos+Vel: i%2==0 → 250
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::Vel>();
        EXPECT_EQ(c, 250u);
    }

    // ForEach でデータ更新
    world.ForEach<qc_test::Pos, qc_test::Vel>([](EntityID, qc_test::Pos& p, qc_test::Vel& v) {
        p.x += v.dx;
    });

    // 更新が反映されているか確認
    float sum = 0;
    int count = 0;
    world.ForEach<qc_test::Pos, qc_test::Vel>([&](EntityID, qc_test::Pos& p, qc_test::Vel&) {
        sum += p.x;
        ++count;
    });
    EXPECT_EQ(count, 250);
    // 元の合計: 0+2+4+...+498 = 250*249 = 62250, 各+1.0 → 62250+250 = 62500
    EXPECT_FLOAT_EQ(sum, 62500.0f);
}

TEST(ECS_QueryCache, SystemUsesCache)
{
    World world;

    EntityID e = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e).x = 0.0f;
    world.AddComponent<qc_test::Vel>(e).dx = 10.0f;

    // システムからForEachが呼ばれてもキャッシュが正常に動作する
    class MoveSystem : public System
    {
    public:
        void Update(World& w, float dt) override
        {
            w.ForEach<qc_test::Pos, qc_test::Vel>([dt](EntityID, qc_test::Pos& p, qc_test::Vel& v) {
                p.x += v.dx * dt;
            });
        }
        const char* GetName() const override { return "MoveSystem"; }
    };

    world.AddSystem<MoveSystem>();
    world.UpdateSystems(0.5f);

    EXPECT_FLOAT_EQ(world.GetComponent<qc_test::Pos>(e)->x, 5.0f);
}

TEST(ECS_QueryCache, InvalidateOnAddComponent)
{
    World world;

    EntityID e = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e);

    // キャッシュ構築
    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 1u);
    uint32_t v1 = world.GetQueryCache().GetVersion();

    // 新しいコンポーネント追加で新アーキタイプ作成 → キャッシュ無効化
    world.AddComponent<qc_test::Vel>(e);
    uint32_t v2 = world.GetQueryCache().GetVersion();
    EXPECT_GT(v2, v1);

    // 再構築後の正確性
    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 1u);
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::Vel>();
        EXPECT_EQ(c, 1u);
    }
}

TEST(ECS_QueryCache, EmptyQuery)
{
    World world;

    // コンポーネントが一切ない状態でのクエリ
    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 0u);
    EXPECT_EQ(world.CountEntities<qc_test::Vel>(), 0u);

    int count = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos&) { ++count; });
    EXPECT_EQ(count, 0);
}

TEST(ECS_QueryCache, ManyArchetypes)
{
    World world;

    // 異なるコンポーネント構成の多数のアーキタイプ
    EntityID e1 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e1);

    EntityID e2 = world.CreateEntity();
    world.AddComponent<qc_test::Vel>(e2);

    EntityID e3 = world.CreateEntity();
    world.AddComponent<qc_test::HP>(e3);

    EntityID e4 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e4);
    world.AddComponent<qc_test::Vel>(e4);

    EntityID e5 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e5);
    world.AddComponent<qc_test::HP>(e5);

    EntityID e6 = world.CreateEntity();
    world.AddComponent<qc_test::Vel>(e6);
    world.AddComponent<qc_test::HP>(e6);

    EntityID e7 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e7);
    world.AddComponent<qc_test::Vel>(e7);
    world.AddComponent<qc_test::HP>(e7);

    // 各クエリの正確性を確認
    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 4u); // e1, e4, e5, e7
    EXPECT_EQ(world.CountEntities<qc_test::Vel>(), 4u); // e2, e4, e6, e7
    EXPECT_EQ(world.CountEntities<qc_test::HP>(), 4u);  // e3, e5, e6, e7
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::Vel>();
        EXPECT_EQ(c, 2u); // e4, e7
    }
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::HP>();
        EXPECT_EQ(c, 2u); // e5, e7
    }
    {
        uint32_t c = world.CountEntities<qc_test::Pos, qc_test::Vel, qc_test::HP>();
        EXPECT_EQ(c, 1u); // e7 only
    }
}

TEST(ECS_QueryCache, QueryAfterClearAndRepopulate)
{
    World world;

    // エンティティを作成
    EntityID e1 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e1);
    EntityID e2 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e2);

    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 2u);

    // 全削除
    world.DestroyEntity(e1);
    world.DestroyEntity(e2);
    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 0u);

    // 再作成
    EntityID e3 = world.CreateEntity();
    world.AddComponent<qc_test::Pos>(e3).x = 99.0f;

    EXPECT_EQ(world.CountEntities<qc_test::Pos>(), 1u);

    float val = 0;
    world.ForEach<qc_test::Pos>([&](EntityID, qc_test::Pos& p) { val = p.x; });
    EXPECT_FLOAT_EQ(val, 99.0f);
}
