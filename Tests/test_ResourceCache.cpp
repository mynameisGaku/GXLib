/// @file test_ResourceCache.cpp
/// @brief ResourceHandleとResourceCacheのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/ResourceHandle.h"

using namespace gx;

// テスト用の単純なリソース型
struct TestResource {
    int value = 0;
    gx::String name;
    TestResource() = default;
    TestResource(int v, const gx::String& n) : value(v), name(n) {}
};

// ============================================================================
// ResourceHandle — 構築と有効性
// ============================================================================

TEST(ResourceHandleTest, DefaultIsInvalid)
{
    ResourceHandle<TestResource> handle;
    EXPECT_FALSE(handle.IsValid());
    EXPECT_FALSE(static_cast<bool>(handle));
    EXPECT_EQ(handle.id, 0u);
    EXPECT_EQ(handle.generation, 0u);
}

TEST(ResourceHandleTest, ExplicitConstructionIsValid)
{
    ResourceHandle<TestResource> handle(1, 0);
    EXPECT_TRUE(handle.IsValid());
    EXPECT_TRUE(static_cast<bool>(handle));
    EXPECT_EQ(handle.id, 1u);
    EXPECT_EQ(handle.generation, 0u);
}

TEST(ResourceHandleTest, EqualityOperator)
{
    ResourceHandle<TestResource> a(1, 0);
    ResourceHandle<TestResource> b(1, 0);
    EXPECT_EQ(a, b);
}

TEST(ResourceHandleTest, InequalityByIdOrGeneration)
{
    ResourceHandle<TestResource> a(1, 0);
    ResourceHandle<TestResource> b(2, 0);
    ResourceHandle<TestResource> c(1, 1);
    EXPECT_NE(a, b);
    EXPECT_NE(a, c);
}

// ============================================================================
// ResourceCache — 登録と検索
// ============================================================================

TEST(ResourceCacheTest, RegisterReturnsValidHandle)
{
    ResourceCache<TestResource> cache;
    auto handle = cache.Register("testRes", std::make_unique<TestResource>(42, "foo"));
    EXPECT_TRUE(handle.IsValid());
}

TEST(ResourceCacheTest, FindByName)
{
    ResourceCache<TestResource> cache;
    auto handle = cache.Register("myRes", std::make_unique<TestResource>(10, "bar"));
    auto found = cache.Find("myRes");
    EXPECT_TRUE(found.IsValid());
    EXPECT_EQ(found.id, handle.id);
    EXPECT_EQ(found.generation, handle.generation);
}

TEST(ResourceCacheTest, FindNonexistentReturnsInvalid)
{
    ResourceCache<TestResource> cache;
    auto found = cache.Find("doesNotExist");
    EXPECT_FALSE(found.IsValid());
}

TEST(ResourceCacheTest, GetReturnsResourcePointer)
{
    ResourceCache<TestResource> cache;
    auto handle = cache.Register("res1", std::make_unique<TestResource>(77, "baz"));
    TestResource* ptr = cache.Get(handle);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->value, 77);
    EXPECT_EQ(ptr->name, "baz");
}

TEST(ResourceCacheTest, GetWithInvalidHandleReturnsNull)
{
    ResourceCache<TestResource> cache;
    ResourceHandle<TestResource> invalid;
    EXPECT_EQ(cache.Get(invalid), nullptr);
}

TEST(ResourceCacheTest, GetByNameReturnsResource)
{
    ResourceCache<TestResource> cache;
    cache.Register("named", std::make_unique<TestResource>(99, "qux"));
    TestResource* ptr = cache.GetByName("named");
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->value, 99);
}

TEST(ResourceCacheTest, GetByNameNonexistentReturnsNull)
{
    ResourceCache<TestResource> cache;
    EXPECT_EQ(cache.GetByName("missing"), nullptr);
}

// ============================================================================
// ResourceCache — 参照カウント
// ============================================================================

TEST(ResourceCacheTest, AddRefIncreasesRefCount)
{
    ResourceCache<TestResource> cache;
    auto handle = cache.Register("ref", std::make_unique<TestResource>(1, "r"));
    EXPECT_EQ(cache.GetRefCount(handle), 1u);
    cache.AddRef(handle);
    EXPECT_EQ(cache.GetRefCount(handle), 2u);
}

TEST(ResourceCacheTest, ReleaseDecreasesRefCount)
{
    ResourceCache<TestResource> cache;
    auto handle = cache.Register("rel", std::make_unique<TestResource>(1, "r"));
    cache.AddRef(handle);
    EXPECT_EQ(cache.GetRefCount(handle), 2u);
    cache.Release(handle);
    EXPECT_EQ(cache.GetRefCount(handle), 1u);
    cache.Release(handle);
    EXPECT_EQ(cache.GetRefCount(handle), 0u);
}

// ============================================================================
// ResourceCache — ガベージコレクション
// ============================================================================

TEST(ResourceCacheTest, CollectGarbageFreesZeroRefResources)
{
    ResourceCache<TestResource> cache;
    auto handle = cache.Register("gc1", std::make_unique<TestResource>(1, "x"));
    cache.Release(handle); // 参照カウント -> 0
    EXPECT_EQ(cache.GetCount(), 1u); // まだ登録されている
    uint32_t freed = cache.CollectGarbage();
    EXPECT_EQ(freed, 1u);
    EXPECT_EQ(cache.GetCount(), 0u);
    EXPECT_EQ(cache.Get(handle), nullptr);
}

TEST(ResourceCacheTest, CollectGarbageDoesNotFreeReferencedResources)
{
    ResourceCache<TestResource> cache;
    auto handle = cache.Register("gc2", std::make_unique<TestResource>(2, "y"));
    // Register後の参照カウントは1
    uint32_t freed = cache.CollectGarbage();
    EXPECT_EQ(freed, 0u);
    EXPECT_EQ(cache.GetCount(), 1u);
    ASSERT_NE(cache.Get(handle), nullptr);
    EXPECT_EQ(cache.Get(handle)->value, 2);
}

// ============================================================================
// ResourceCache — 容量、状態、クリア
// ============================================================================

TEST(ResourceCacheTest, GetCountReflectsRegisteredResources)
{
    ResourceCache<TestResource> cache;
    EXPECT_EQ(cache.GetCount(), 0u);
    cache.Register("a", std::make_unique<TestResource>());
    cache.Register("b", std::make_unique<TestResource>());
    EXPECT_EQ(cache.GetCount(), 2u);
}

TEST(ResourceCacheTest, GetCapacityIncludesAllSlots)
{
    ResourceCache<TestResource> cache;
    cache.Register("c1", std::make_unique<TestResource>());
    cache.Register("c2", std::make_unique<TestResource>());
    EXPECT_GE(cache.GetCapacity(), 2u);
}

TEST(ResourceCacheTest, ClearRemovesAll)
{
    ResourceCache<TestResource> cache;
    auto h1 = cache.Register("d1", std::make_unique<TestResource>());
    auto h2 = cache.Register("d2", std::make_unique<TestResource>());
    cache.Clear();
    EXPECT_EQ(cache.GetCount(), 0u);
    EXPECT_EQ(cache.GetCapacity(), 0u);
    EXPECT_EQ(cache.Get(h1), nullptr);
    EXPECT_EQ(cache.Get(h2), nullptr);
}

TEST(ResourceCacheTest, GetStateReturnsLoadedAfterRegister)
{
    ResourceCache<TestResource> cache;
    auto handle = cache.Register("state", std::make_unique<TestResource>());
    EXPECT_EQ(cache.GetState(handle), ResourceState::Loaded);
}

TEST(ResourceCacheTest, GenerationIncrementsOnReuse)
{
    ResourceCache<TestResource> cache;
    auto handle1 = cache.Register("reuse", std::make_unique<TestResource>(1, "v1"));
    uint32_t gen1 = handle1.generation;
    cache.Release(handle1); // 参照カウント -> 0
    cache.CollectGarbage();  // スロットを解放し、世代をインクリメント

    // 新しいリソースを登録すると、解放されたスロットを世代を進めて再利用するべき
    auto handle2 = cache.Register("reuse2", std::make_unique<TestResource>(2, "v2"));
    EXPECT_EQ(handle2.id, handle1.id); // 同じスロット
    EXPECT_GT(handle2.generation, gen1); // 世代がインクリメントされている
}

TEST(ResourceCacheTest, RegisterSameNameUpdatesExistingSlot)
{
    ResourceCache<TestResource> cache;
    auto h1 = cache.Register("dup", std::make_unique<TestResource>(10, "first"));
    auto h2 = cache.Register("dup", std::make_unique<TestResource>(20, "second"));
    // 同じスロットを再利用
    EXPECT_EQ(h1.id, h2.id);
    // リソースは2番目のものであるべき
    TestResource* ptr = cache.Get(h2);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->value, 20);
    EXPECT_EQ(ptr->name, "second");
}

TEST(ResourceCacheTest, RegisterDifferentNamesReturnsDifferentHandles)
{
    ResourceCache<TestResource> cache;
    auto h1 = cache.Register("alpha", std::make_unique<TestResource>());
    auto h2 = cache.Register("beta", std::make_unique<TestResource>());
    EXPECT_NE(h1.id, h2.id);
}
