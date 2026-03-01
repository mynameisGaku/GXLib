#include <gtest/gtest.h>
#include "Core/Reflect/PropertyMeta.h"
#include "Core/Reflect/TypeInfo.h"
#include "Core/Reflect/TypeRegistry.h"
#include "Core/Reflect/ReflectMacros.h"
#include "Core/Reflect/JsonSerializer.h"

// ---------------------------------------------------------------------------
// テスト構造体
// ---------------------------------------------------------------------------
namespace
{

struct TestVec3 { float x = 0, y = 0, z = 0; };

struct SimpleData
{
    bool active = true;
    int count = 42;
    float speed = 3.14f;
    std::string label = "hello";
    TestVec3 position;
};

} // anonymous namespace

// リフレクションマクロでSimpleDataを登録
GX_REFLECT_BEGIN(SimpleData)
    GX_REFLECT_BOOL("active", active)
    GX_REFLECT_INT("count", count)
    GX_REFLECT_FLOAT("speed", speed)
    GX_REFLECT_STRING("label", label)
    GX_REFLECT_VEC3("position", position)
GX_REFLECT_END(SimpleData)

// ---------------------------------------------------------------------------
// TypeInfoテスト
// ---------------------------------------------------------------------------
TEST(Reflection, TypeInfoBasics)
{
    gx::TypeInfo info("TestType", 64);
    EXPECT_EQ(info.GetName(), "TestType");
    EXPECT_EQ(info.GetTypeSize(), 64u);
    EXPECT_NE(info.GetTypeHash(), 0u);
    EXPECT_EQ(info.GetPropertyCount(), 0u);
}

TEST(Reflection, TypeInfoAddAndFindProperty)
{
    gx::TypeInfo info("SomeType", 16);

    gx::PropertyMeta p;
    p.name = "health";
    p.type = gx::PropertyType::Float;
    p.offset = 0;
    p.size = sizeof(float);
    info.AddProperty(p);

    EXPECT_EQ(info.GetPropertyCount(), 1u);
    const gx::PropertyMeta* found = info.FindProperty("health");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->type, gx::PropertyType::Float);

    // 見つからない場合
    EXPECT_EQ(info.FindProperty("nonexistent"), nullptr);
}

TEST(Reflection, TypeInfoGetSetBool)
{
    gx::TypeInfo info("BoolTest", sizeof(bool));
    gx::PropertyMeta p;
    p.name = "flag";
    p.type = gx::PropertyType::Bool;
    p.offset = 0;
    p.size = sizeof(bool);
    info.AddProperty(p);

    bool value = true;
    EXPECT_TRUE(info.GetBool(&value, "flag"));
    info.SetBool(&value, "flag", false);
    EXPECT_FALSE(value);
}

TEST(Reflection, TypeInfoGetSetInt)
{
    gx::TypeInfo info("IntTest", sizeof(int));
    gx::PropertyMeta p;
    p.name = "count";
    p.type = gx::PropertyType::Int;
    p.offset = 0;
    p.size = sizeof(int);
    info.AddProperty(p);

    int value = 10;
    EXPECT_EQ(info.GetInt(&value, "count"), 10);
    info.SetInt(&value, "count", 99);
    EXPECT_EQ(value, 99);
}

TEST(Reflection, TypeInfoGetSetFloat)
{
    gx::TypeInfo info("FloatTest", sizeof(float));
    gx::PropertyMeta p;
    p.name = "speed";
    p.type = gx::PropertyType::Float;
    p.offset = 0;
    p.size = sizeof(float);
    info.AddProperty(p);

    float value = 1.5f;
    EXPECT_FLOAT_EQ(info.GetFloat(&value, "speed"), 1.5f);
    info.SetFloat(&value, "speed", 9.8f);
    EXPECT_FLOAT_EQ(value, 9.8f);
}

TEST(Reflection, TypeInfoRangeClamp)
{
    gx::TypeInfo info("RangeTest", sizeof(float));
    gx::PropertyMeta p;
    p.name = "volume";
    p.type = gx::PropertyType::Float;
    p.offset = 0;
    p.size = sizeof(float);
    p.hasRange = true;
    p.minValue = 0.0f;
    p.maxValue = 1.0f;
    info.AddProperty(p);

    float value = 0.5f;
    info.SetFloat(&value, "volume", 5.0f);
    EXPECT_FLOAT_EQ(value, 1.0f); // 最大値にクランプ

    info.SetFloat(&value, "volume", -1.0f);
    EXPECT_FLOAT_EQ(value, 0.0f); // 最小値にクランプ
}

TEST(Reflection, TypeInfoReadOnly)
{
    gx::TypeInfo info("ROTest", sizeof(int));
    gx::PropertyMeta p;
    p.name = "id";
    p.type = gx::PropertyType::Int;
    p.offset = 0;
    p.size = sizeof(int);
    p.readOnly = true;
    info.AddProperty(p);

    int value = 42;
    info.SetInt(&value, "id", 100); // 拒否されるはず
    EXPECT_EQ(value, 42);           // 変更なし
}

TEST(Reflection, TypeInfoFactory)
{
    gx::TypeInfo info("FactoryTest", sizeof(int));
    info.SetFactory([]() -> void* {
        int* p = new int(123);
        return p;
    });

    void* ptr = info.CreateInstance();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*static_cast<int*>(ptr), 123);
    delete static_cast<int*>(ptr);
}

TEST(Reflection, TypeInfoHashConsistency)
{
    gx::TypeInfo a("SameName", 8);
    gx::TypeInfo b("SameName", 16);
    EXPECT_EQ(a.GetTypeHash(), b.GetTypeHash());

    gx::TypeInfo c("DifferentName", 8);
    EXPECT_NE(a.GetTypeHash(), c.GetTypeHash());
}

// ---------------------------------------------------------------------------
// TypeRegistryテスト
// ---------------------------------------------------------------------------
TEST(Reflection, RegistrySingleton)
{
    auto& r1 = gx::TypeRegistry::Instance();
    auto& r2 = gx::TypeRegistry::Instance();
    EXPECT_EQ(&r1, &r2);
}

TEST(Reflection, MacroRegistration)
{
    // SimpleDataはGX_REFLECTマクロにより静的初期化時に登録されているはず
    const gx::TypeInfo* info = gx::TypeRegistry::Instance().FindType("SimpleData");
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->GetName(), "SimpleData");
    EXPECT_EQ(info->GetTypeSize(), sizeof(SimpleData));
    EXPECT_EQ(info->GetPropertyCount(), 5u);
}

TEST(Reflection, MacroRegisteredProperties)
{
    const gx::TypeInfo* info = gx::TypeRegistry::Instance().FindType("SimpleData");
    ASSERT_NE(info, nullptr);

    // 各プロパティを確認
    EXPECT_NE(info->FindProperty("active"), nullptr);
    EXPECT_NE(info->FindProperty("count"), nullptr);
    EXPECT_NE(info->FindProperty("speed"), nullptr);
    EXPECT_NE(info->FindProperty("label"), nullptr);
    EXPECT_NE(info->FindProperty("position"), nullptr);

    EXPECT_EQ(info->FindProperty("active")->type, gx::PropertyType::Bool);
    EXPECT_EQ(info->FindProperty("count")->type, gx::PropertyType::Int);
    EXPECT_EQ(info->FindProperty("speed")->type, gx::PropertyType::Float);
    EXPECT_EQ(info->FindProperty("label")->type, gx::PropertyType::String);
    EXPECT_EQ(info->FindProperty("position")->type, gx::PropertyType::Vec3);
}

TEST(Reflection, MacroReadWriteValues)
{
    const gx::TypeInfo* info = gx::TypeRegistry::Instance().FindType("SimpleData");
    ASSERT_NE(info, nullptr);

    SimpleData data;
    EXPECT_TRUE(info->GetBool(&data, "active"));
    EXPECT_EQ(info->GetInt(&data, "count"), 42);
    EXPECT_FLOAT_EQ(info->GetFloat(&data, "speed"), 3.14f);
    EXPECT_EQ(info->GetString(&data, "label"), "hello");

    info->SetBool(&data, "active", false);
    info->SetInt(&data, "count", 100);
    info->SetFloat(&data, "speed", 9.8f);
    info->SetString(&data, "label", "world");

    EXPECT_FALSE(data.active);
    EXPECT_EQ(data.count, 100);
    EXPECT_FLOAT_EQ(data.speed, 9.8f);
    EXPECT_EQ(data.label, "world");
}

TEST(Reflection, RegistryFindByHash)
{
    const gx::TypeInfo* byName = gx::TypeRegistry::Instance().FindType("SimpleData");
    ASSERT_NE(byName, nullptr);

    const gx::TypeInfo* byHash = gx::TypeRegistry::Instance().FindTypeByHash(byName->GetTypeHash());
    ASSERT_NE(byHash, nullptr);
    EXPECT_EQ(byName, byHash);
}

TEST(Reflection, RegistryGetAllTypes)
{
    auto all = gx::TypeRegistry::Instance().GetAllTypes();
    // 少なくともSimpleDataは登録されているはず
    bool found = false;
    for (auto* t : all)
    {
        if (t->GetName() == "SimpleData") found = true;
    }
    EXPECT_TRUE(found);
}

TEST(Reflection, FactoryViaRegistry)
{
    const gx::TypeInfo* info = gx::TypeRegistry::Instance().FindType("SimpleData");
    ASSERT_NE(info, nullptr);

    void* ptr = info->CreateInstance();
    ASSERT_NE(ptr, nullptr);

    auto* data = static_cast<SimpleData*>(ptr);
    EXPECT_TRUE(data->active);
    EXPECT_EQ(data->count, 42);
    delete data;
}

// ---------------------------------------------------------------------------
// JsonSerializerテスト
// ---------------------------------------------------------------------------
TEST(Reflection, JsonSerialize)
{
    const gx::TypeInfo* info = gx::TypeRegistry::Instance().FindType("SimpleData");
    ASSERT_NE(info, nullptr);

    SimpleData data;
    data.active = false;
    data.count = 7;
    data.speed = 2.5f;
    data.label = "test";
    data.position = { 1.0f, 2.0f, 3.0f };

    std::string json = gx::JsonSerializer::Serialize(*info, &data);

    // 主要部分が含まれていることを確認
    EXPECT_NE(json.find("\"_type\": \"SimpleData\""), std::string::npos);
    EXPECT_NE(json.find("\"active\": false"), std::string::npos);
    EXPECT_NE(json.find("\"count\": 7"), std::string::npos);
    EXPECT_NE(json.find("\"label\": \"test\""), std::string::npos);
}

TEST(Reflection, JsonDeserialize)
{
    const gx::TypeInfo* info = gx::TypeRegistry::Instance().FindType("SimpleData");
    ASSERT_NE(info, nullptr);

    std::string json = R"({
        "_type": "SimpleData",
        "active": true,
        "count": 99,
        "speed": 7.5,
        "label": "deserialized"
    })";

    SimpleData data;
    data.active = false;
    data.count = 0;
    data.speed = 0.0f;
    data.label = "";

    bool ok = gx::JsonSerializer::Deserialize(*info, &data, json);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(data.active);
    EXPECT_EQ(data.count, 99);
    EXPECT_FLOAT_EQ(data.speed, 7.5f);
    EXPECT_EQ(data.label, "deserialized");
}

TEST(Reflection, JsonRoundTrip)
{
    const gx::TypeInfo* info = gx::TypeRegistry::Instance().FindType("SimpleData");
    ASSERT_NE(info, nullptr);

    SimpleData original;
    original.active = true;
    original.count = 123;
    original.speed = 4.56f;
    original.label = "roundtrip";

    std::string json = gx::JsonSerializer::Serialize(*info, &original);

    SimpleData restored;
    restored.active = false;
    restored.count = 0;
    restored.speed = 0.0f;
    restored.label = "";

    gx::JsonSerializer::Deserialize(*info, &restored, json);

    EXPECT_EQ(original.active, restored.active);
    EXPECT_EQ(original.count, restored.count);
    EXPECT_FLOAT_EQ(original.speed, restored.speed);
    EXPECT_EQ(original.label, restored.label);
}

TEST(Reflection, FloatRangeMacro)
{
    // テスト時に範囲付きfloat型を登録
    auto typeInfo = std::make_unique<gx::TypeInfo>("RangeStruct", sizeof(float));
    gx::PropertyMeta p;
    p.name = "clamped";
    p.type = gx::PropertyType::Float;
    p.offset = 0;
    p.size = sizeof(float);
    p.hasRange = true;
    p.minValue = -10.0f;
    p.maxValue = 10.0f;
    typeInfo->AddProperty(p);

    float val = 0.0f;
    typeInfo->SetFloat(&val, "clamped", 50.0f);
    EXPECT_FLOAT_EQ(val, 10.0f); // 最大値にクランプ

    typeInfo->SetFloat(&val, "clamped", -50.0f);
    EXPECT_FLOAT_EQ(val, -10.0f); // 最小値にクランプ
}
