/// @file test_ShaderVariant.cpp
/// @brief シェーダーバリアント管理システムのテスト

#include <gtest/gtest.h>
#include "Graphics/Pipeline/ShaderVariant.h"

using namespace gx;

// ============================================================================
// ShaderVariantKey テスト
// ============================================================================

TEST(ShaderVariantKey, DefaultEmpty)
{
    ShaderVariantKey key;
    EXPECT_EQ(key.GetKeywordCount(), 0u);
    EXPECT_FALSE(key.HasKeyword("ANYTHING"));
}

TEST(ShaderVariantKey, SetBoolKeyword)
{
    ShaderVariantKey key;
    key.SetKeyword("ENABLE_NORMAL_MAP", true);

    EXPECT_TRUE(key.HasKeyword("ENABLE_NORMAL_MAP"));
    EXPECT_EQ(key.GetKeywordValue("ENABLE_NORMAL_MAP"), "1");
    EXPECT_EQ(key.GetKeywordCount(), 1u);
}

TEST(ShaderVariantKey, SetValueKeyword)
{
    ShaderVariantKey key;
    key.SetKeyword("QUALITY_LEVEL", gx::String("HIGH"));

    EXPECT_TRUE(key.HasKeyword("QUALITY_LEVEL"));
    EXPECT_EQ(key.GetKeywordValue("QUALITY_LEVEL"), "HIGH");
}

TEST(ShaderVariantKey, ClearKeyword)
{
    ShaderVariantKey key;
    key.SetKeyword("FOO", true);
    EXPECT_TRUE(key.HasKeyword("FOO"));

    key.ClearKeyword("FOO");
    EXPECT_FALSE(key.HasKeyword("FOO"));
    EXPECT_EQ(key.GetKeywordCount(), 0u);
}

TEST(ShaderVariantKey, ClearNonexistentKeyword)
{
    ShaderVariantKey key;
    key.ClearKeyword("NONEXISTENT"); // no crash
    EXPECT_EQ(key.GetKeywordCount(), 0u);
}

TEST(ShaderVariantKey, DisableKeyword)
{
    ShaderVariantKey key;
    key.SetKeyword("FOO", true);
    EXPECT_TRUE(key.HasKeyword("FOO"));

    key.SetKeyword("FOO", false);
    EXPECT_FALSE(key.HasKeyword("FOO"));
}

TEST(ShaderVariantKey, GetMissingKeywordValue)
{
    ShaderVariantKey key;
    gx::String val = key.GetKeywordValue("MISSING");
    EXPECT_TRUE(val.empty());
}

TEST(ShaderVariantKey, MultipleKeywords)
{
    ShaderVariantKey key;
    key.SetKeyword("A", true);
    key.SetKeyword("B", gx::String("2"));
    key.SetKeyword("C", true);

    EXPECT_EQ(key.GetKeywordCount(), 3u);
    EXPECT_TRUE(key.HasKeyword("A"));
    EXPECT_TRUE(key.HasKeyword("B"));
    EXPECT_TRUE(key.HasKeyword("C"));
}

TEST(ShaderVariantKey, HashConsistency)
{
    ShaderVariantKey key1;
    key1.SetKeyword("A", true);
    key1.SetKeyword("B", gx::String("VALUE"));

    ShaderVariantKey key2;
    key2.SetKeyword("A", true);
    key2.SetKeyword("B", gx::String("VALUE"));

    EXPECT_EQ(key1.ComputeHash(), key2.ComputeHash());
}

TEST(ShaderVariantKey, HashOrderIndependent)
{
    // キーワード追加順序が違っても同じハッシュになることを確認
    ShaderVariantKey key1;
    key1.SetKeyword("ALPHA", true);
    key1.SetKeyword("BETA", gx::String("X"));

    ShaderVariantKey key2;
    key2.SetKeyword("BETA", gx::String("X"));
    key2.SetKeyword("ALPHA", true);

    EXPECT_EQ(key1.ComputeHash(), key2.ComputeHash());
}

TEST(ShaderVariantKey, HashDifferentKeys)
{
    ShaderVariantKey key1;
    key1.SetKeyword("A", true);

    ShaderVariantKey key2;
    key2.SetKeyword("B", true);

    EXPECT_NE(key1.ComputeHash(), key2.ComputeHash());
}

TEST(ShaderVariantKey, HashDifferentValues)
{
    ShaderVariantKey key1;
    key1.SetKeyword("QUALITY", gx::String("LOW"));

    ShaderVariantKey key2;
    key2.SetKeyword("QUALITY", gx::String("HIGH"));

    EXPECT_NE(key1.ComputeHash(), key2.ComputeHash());
}

TEST(ShaderVariantKey, HashEmptyKey)
{
    ShaderVariantKey key1;
    ShaderVariantKey key2;

    // 空キー同士のハッシュは同じ
    EXPECT_EQ(key1.ComputeHash(), key2.ComputeHash());
}

TEST(ShaderVariantKey, ToDefinesBool)
{
    ShaderVariantKey key;
    key.SetKeyword("ENABLE_FOG", true);

    auto defines = key.ToDefines();
    ASSERT_EQ(defines.size(), 1u);
    EXPECT_EQ(defines[0], "ENABLE_FOG");
}

TEST(ShaderVariantKey, ToDefinesValue)
{
    ShaderVariantKey key;
    key.SetKeyword("MAX_LIGHTS", gx::String("8"));

    auto defines = key.ToDefines();
    ASSERT_EQ(defines.size(), 1u);
    EXPECT_EQ(defines[0], "MAX_LIGHTS=8");
}

TEST(ShaderVariantKey, ToDefinesMultiple)
{
    ShaderVariantKey key;
    key.SetKeyword("A", true);
    key.SetKeyword("B", gx::String("2"));

    auto defines = key.ToDefines();
    EXPECT_EQ(defines.size(), 2u);

    // 順序は不定だが、両方含まれているはず
    bool hasA = false, hasB = false;
    for (const auto& d : defines)
    {
        if (d == "A") hasA = true;
        if (d == "B=2") hasB = true;
    }
    EXPECT_TRUE(hasA);
    EXPECT_TRUE(hasB);
}

TEST(ShaderVariantKey, ToDefinesEmpty)
{
    ShaderVariantKey key;
    auto defines = key.ToDefines();
    EXPECT_TRUE(defines.empty());
}

TEST(ShaderVariantKey, EqualityIdentical)
{
    ShaderVariantKey key1;
    key1.SetKeyword("X", true);
    key1.SetKeyword("Y", gx::String("Z"));

    ShaderVariantKey key2;
    key2.SetKeyword("X", true);
    key2.SetKeyword("Y", gx::String("Z"));

    EXPECT_TRUE(key1 == key2);
}

TEST(ShaderVariantKey, EqualityDifferent)
{
    ShaderVariantKey key1;
    key1.SetKeyword("X", true);

    ShaderVariantKey key2;
    key2.SetKeyword("Y", true);

    EXPECT_FALSE(key1 == key2);
}

TEST(ShaderVariantKey, EqualityDifferentSize)
{
    ShaderVariantKey key1;
    key1.SetKeyword("X", true);

    ShaderVariantKey key2;
    key2.SetKeyword("X", true);
    key2.SetKeyword("Y", true);

    EXPECT_FALSE(key1 == key2);
}

TEST(ShaderVariantKey, EqualityBothEmpty)
{
    ShaderVariantKey key1;
    ShaderVariantKey key2;
    EXPECT_TRUE(key1 == key2);
}

TEST(ShaderVariantKey, OverwriteKeyword)
{
    ShaderVariantKey key;
    key.SetKeyword("QUALITY", gx::String("LOW"));
    EXPECT_EQ(key.GetKeywordValue("QUALITY"), "LOW");

    key.SetKeyword("QUALITY", gx::String("HIGH"));
    EXPECT_EQ(key.GetKeywordValue("QUALITY"), "HIGH");
    EXPECT_EQ(key.GetKeywordCount(), 1u);
}

// ============================================================================
// ShaderVariantCollection テスト (CPU-only, GPU不要)
// ============================================================================

TEST(ShaderVariantCollection, DefaultState)
{
    ShaderVariantCollection collection;
    EXPECT_EQ(collection.GetCompiledCount(), 0u);
    EXPECT_EQ(collection.GetKeywordCount(), 0u);
}

TEST(ShaderVariantCollection, Initialize)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl", "VSMain", "PSMain");

    EXPECT_EQ(collection.GetShaderPath(), "shaders/PBR.hlsl");
    EXPECT_EQ(collection.GetCompiledCount(), 0u);
}

TEST(ShaderVariantCollection, AddKeyword)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    ShaderKeyword kw;
    kw.name = "ENABLE_NORMAL_MAP";
    kw.isMultiCompile = true;
    collection.AddKeyword(kw);

    EXPECT_EQ(collection.GetKeywordCount(), 1u);
}

TEST(ShaderVariantCollection, AddMultipleKeywords)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    ShaderKeyword kw1;
    kw1.name = "NORMAL_MAP";
    kw1.isMultiCompile = true;
    collection.AddKeyword(kw1);

    ShaderKeyword kw2;
    kw2.name = "SHADOWS";
    kw2.isMultiCompile = true;
    collection.AddKeyword(kw2);

    ShaderKeyword kw3;
    kw3.name = "FOG";
    kw3.isMultiCompile = false; // shader_feature
    collection.AddKeyword(kw3);

    EXPECT_EQ(collection.GetKeywordCount(), 3u);
}

TEST(ShaderVariantCollection, TotalVariantCountOnOff)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    // 2つのon/off multi_compileキーワード = 2*2 = 4バリアント
    ShaderKeyword kw1;
    kw1.name = "NORMAL_MAP";
    kw1.isMultiCompile = true;
    collection.AddKeyword(kw1);

    ShaderKeyword kw2;
    kw2.name = "SHADOWS";
    kw2.isMultiCompile = true;
    collection.AddKeyword(kw2);

    EXPECT_EQ(collection.GetTotalMultiCompileVariantCount(), 4u);
}

TEST(ShaderVariantCollection, TotalVariantCountMultiValue)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    // 3値のmulti_compile + on/offのmulti_compile = 3*2 = 6バリアント
    ShaderKeyword kw1;
    kw1.name = "QUALITY";
    kw1.values = { "LOW", "MEDIUM", "HIGH" };
    kw1.isMultiCompile = true;
    collection.AddKeyword(kw1);

    ShaderKeyword kw2;
    kw2.name = "FOG";
    kw2.isMultiCompile = true;
    collection.AddKeyword(kw2);

    EXPECT_EQ(collection.GetTotalMultiCompileVariantCount(), 6u);
}

TEST(ShaderVariantCollection, TotalVariantCountShaderFeatureIgnored)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    // shader_feature はmulti_compileカウントに含まれない
    ShaderKeyword kw1;
    kw1.name = "NORMAL_MAP";
    kw1.isMultiCompile = true;
    collection.AddKeyword(kw1);

    ShaderKeyword kw2;
    kw2.name = "DEBUG_MODE";
    kw2.isMultiCompile = false;
    collection.AddKeyword(kw2);

    EXPECT_EQ(collection.GetTotalMultiCompileVariantCount(), 2u);
}

TEST(ShaderVariantCollection, TotalVariantCountNoKeywords)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    // キーワードなし = 1バリアント（デフォルト）
    EXPECT_EQ(collection.GetTotalMultiCompileVariantCount(), 1u);
}

TEST(ShaderVariantCollection, Clear)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    // コンパイルはGPU不要なので直接テストできないが、
    // Clearでバリアントが削除されることは確認できる
    collection.Clear();
    EXPECT_EQ(collection.GetCompiledCount(), 0u);
}

TEST(ShaderVariantCollection, GetVariantDebugString)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    ShaderVariantKey key;
    key.SetKeyword("FOG", true);
    key.SetKeyword("QUALITY", gx::String("HIGH"));

    gx::String debug = collection.GetVariantDebugString(key);
    EXPECT_FALSE(debug.empty());
    // デバッグ文字列にFOGとQUALITY=HIGHが含まれているはず
    EXPECT_NE(debug.find("FOG"), gx::String::npos);
    EXPECT_NE(debug.find("QUALITY"), gx::String::npos);
}

TEST(ShaderVariantCollection, GetVariantDebugStringEmpty)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    ShaderVariantKey key;
    gx::String debug = collection.GetVariantDebugString(key);
    EXPECT_EQ(debug, "<no defines>");
}

TEST(ShaderVariantCollection, PrecompileAllNullDevice)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    ShaderKeyword kw;
    kw.name = "FOG";
    kw.isMultiCompile = true;
    collection.AddKeyword(kw);

    // NULLデバイスでは0を返すはず
    int count = collection.PrecompileAll(nullptr, nullptr);
    EXPECT_EQ(count, 0);
}

TEST(ShaderVariantCollection, GetVariantNullDevice)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/PBR.hlsl");

    ShaderVariantKey key;
    key.SetKeyword("FOG", true);

    // NULLデバイスではnullptrを返すはず
    auto* pso = collection.GetVariant(key, nullptr, nullptr);
    EXPECT_EQ(pso, nullptr);
}

TEST(ShaderVariantCollection, ReinitializeClearsState)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/Old.hlsl");

    ShaderKeyword kw;
    kw.name = "FOG";
    kw.isMultiCompile = true;
    collection.AddKeyword(kw);

    EXPECT_EQ(collection.GetKeywordCount(), 1u);

    // 再初期化でキーワードがクリアされる
    collection.Initialize("shaders/New.hlsl");
    EXPECT_EQ(collection.GetKeywordCount(), 0u);
    EXPECT_EQ(collection.GetShaderPath(), "shaders/New.hlsl");
    EXPECT_EQ(collection.GetCompiledCount(), 0u);
}

TEST(ShaderVariantCollection, ComplexVariantCombination)
{
    ShaderVariantCollection collection;
    collection.Initialize("shaders/Uber.hlsl");

    // 3つのmulti_compile: 2*2*3 = 12 バリアント
    ShaderKeyword kw1;
    kw1.name = "NORMAL_MAP";
    kw1.isMultiCompile = true;
    collection.AddKeyword(kw1);

    ShaderKeyword kw2;
    kw2.name = "SHADOW";
    kw2.isMultiCompile = true;
    collection.AddKeyword(kw2);

    ShaderKeyword kw3;
    kw3.name = "QUALITY";
    kw3.values = { "LOW", "MEDIUM", "HIGH" };
    kw3.isMultiCompile = true;
    collection.AddKeyword(kw3);

    EXPECT_EQ(collection.GetTotalMultiCompileVariantCount(), 12u);
}
