/// @file test_ScriptEngine.cpp
/// @brief Phase 5: ScriptEngineテスト（Lua統合、条件コンパイル）

#include "pch.h"
#include <gtest/gtest.h>
#include "Script/ScriptEngine.h"

#ifdef GX_ENABLE_LUA

using namespace gx;

// ============================================================================
// 初期化
// ============================================================================

TEST(ScriptEngine, InitializeAndShutdown)
{
    ScriptEngine engine;
    EXPECT_TRUE(engine.Initialize());
    engine.Shutdown();
}

TEST(ScriptEngine, DoubleInitialize)
{
    ScriptEngine engine;
    EXPECT_TRUE(engine.Initialize());
    // 2回目の初期化も成功するか、無害であるべき
    EXPECT_TRUE(engine.Initialize());
    engine.Shutdown();
}

// ============================================================================
// ExecuteString
// ============================================================================

TEST(ScriptEngine, ExecuteStringSimple)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_TRUE(engine.ExecuteString("x = 42"));
    engine.Shutdown();
}

TEST(ScriptEngine, ExecuteStringInvalid)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_FALSE(engine.ExecuteString("this is not valid lua !!!"));
    EXPECT_FALSE(engine.GetLastError().empty());

    engine.Shutdown();
}

TEST(ScriptEngine, ExecuteStringMath)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_TRUE(engine.ExecuteString("result = 10 + 20 + 30"));
    int result = engine.GetGlobalInt("result");
    EXPECT_EQ(result, 60);

    engine.Shutdown();
}

// ============================================================================
// ExecuteFile
// ============================================================================

TEST(ScriptEngine, ExecuteFileNonExistent)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_FALSE(engine.ExecuteFile("nonexistent_script_xyz.lua"));

    engine.Shutdown();
}

// ============================================================================
// グローバル変数
// ============================================================================

TEST(ScriptEngine, SetAndGetGlobalFloat)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    engine.SetGlobal("myFloat", 3.14f);
    float val = engine.GetGlobalFloat("myFloat");
    EXPECT_NEAR(val, 3.14f, 0.001f);

    engine.Shutdown();
}

TEST(ScriptEngine, SetAndGetGlobalInt)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    engine.SetGlobal("myInt", 123);
    int val = engine.GetGlobalInt("myInt");
    EXPECT_EQ(val, 123);

    engine.Shutdown();
}

TEST(ScriptEngine, SetAndGetGlobalString)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    engine.SetGlobal("myString", gx::String("hello"));
    // Luaから読み取ることで設定されたことを検証
    EXPECT_TRUE(engine.ExecuteString("strLen = #myString"));
    int len = engine.GetGlobalInt("strLen");
    EXPECT_EQ(len, 5);

    engine.Shutdown();
}

TEST(ScriptEngine, GetGlobalDefaultValue)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    float val = engine.GetGlobalFloat("undefined_var", 99.0f);
    EXPECT_FLOAT_EQ(val, 99.0f);

    int ival = engine.GetGlobalInt("undefined_var", -1);
    EXPECT_EQ(ival, -1);

    engine.Shutdown();
}

// ============================================================================
// CallFunction
// ============================================================================

TEST(ScriptEngine, CallFunctionNoArgs)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_TRUE(engine.ExecuteString("function myFunc() callResult = 42 end"));
    EXPECT_TRUE(engine.CallFunction("myFunc"));
    int result = engine.GetGlobalInt("callResult");
    EXPECT_EQ(result, 42);

    engine.Shutdown();
}

TEST(ScriptEngine, CallFunctionWithFloatArg)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_TRUE(engine.ExecuteString("function addTen(x) addResult = x + 10 end"));
    EXPECT_TRUE(engine.CallFunction("addTen", 5.0f));
    float result = engine.GetGlobalFloat("addResult");
    EXPECT_NEAR(result, 15.0f, 0.001f);

    engine.Shutdown();
}

TEST(ScriptEngine, CallNonExistentFunction)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_FALSE(engine.CallFunction("no_such_function_xyz"));

    engine.Shutdown();
}

// ============================================================================
// エラーハンドリング
// ============================================================================

TEST(ScriptEngine, GetLastErrorAfterFailure)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    engine.ExecuteString("bad syntax !!!");
    EXPECT_FALSE(engine.GetLastError().empty());

    engine.Shutdown();
}

TEST(ScriptEngine, SetContextWithNullptrs)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    // すべてnullptrでもクラッシュしないこと
    engine.SetContext(nullptr, nullptr, nullptr);

    engine.Shutdown();
}

// ============================================================================
// 追加テスト: グローバル変数
// ============================================================================

TEST(ScriptEngine, SetAndGetGlobalBool)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    // Luaではboolはintegerではない。整数1として設定すれば読める
    engine.ExecuteString("myBool = 1");
    int val = engine.GetGlobalInt("myBool");
    EXPECT_EQ(val, 1);

    engine.Shutdown();
}

TEST(ScriptEngine, MultipleSetsAndGets)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    engine.SetGlobal("a", 10);
    engine.SetGlobal("b", 20);
    engine.SetGlobal("c", 30);

    EXPECT_EQ(engine.GetGlobalInt("a"), 10);
    EXPECT_EQ(engine.GetGlobalInt("b"), 20);
    EXPECT_EQ(engine.GetGlobalInt("c"), 30);

    engine.Shutdown();
}

TEST(ScriptEngine, OverwriteGlobal)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    engine.SetGlobal("val", 100);
    EXPECT_EQ(engine.GetGlobalInt("val"), 100);

    engine.SetGlobal("val", 200);
    EXPECT_EQ(engine.GetGlobalInt("val"), 200);

    engine.Shutdown();
}

TEST(ScriptEngine, GetGlobalFloatDefault)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    float val = engine.GetGlobalFloat("no_such_var", 42.5f);
    EXPECT_FLOAT_EQ(val, 42.5f);

    engine.Shutdown();
}

TEST(ScriptEngine, GetGlobalIntDefault)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    int val = engine.GetGlobalInt("no_such_var", -99);
    EXPECT_EQ(val, -99);

    engine.Shutdown();
}

// ============================================================================
// 追加テスト: ExecuteString
// ============================================================================

TEST(ScriptEngine, ExecuteEmptyString)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    // 空文字列の実行は成功するはず
    EXPECT_TRUE(engine.ExecuteString(""));

    engine.Shutdown();
}

TEST(ScriptEngine, ExecuteStringWithTableCreation)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_TRUE(engine.ExecuteString("t = {1, 2, 3}; tLen = #t"));
    int len = engine.GetGlobalInt("tLen");
    EXPECT_EQ(len, 3);

    engine.Shutdown();
}

TEST(ScriptEngine, ExecuteStringWithStringConcat)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_TRUE(engine.ExecuteString("s = 'hello' .. ' ' .. 'world'; sLen = #s"));
    int len = engine.GetGlobalInt("sLen");
    EXPECT_EQ(len, 11);

    engine.Shutdown();
}

// ============================================================================
// 追加テスト: CallFunction
// ============================================================================

TEST(ScriptEngine, CallFunctionMultipleTimes)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_TRUE(engine.ExecuteString("counter = 0; function inc() counter = counter + 1 end"));
    EXPECT_TRUE(engine.CallFunction("inc"));
    EXPECT_TRUE(engine.CallFunction("inc"));
    EXPECT_TRUE(engine.CallFunction("inc"));

    int val = engine.GetGlobalInt("counter");
    EXPECT_EQ(val, 3);

    engine.Shutdown();
}

TEST(ScriptEngine, CallFunctionWithFloatArgNegative)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    EXPECT_TRUE(engine.ExecuteString("function negate(x) negResult = -x end"));
    EXPECT_TRUE(engine.CallFunction("negate", 7.5f));
    float result = engine.GetGlobalFloat("negResult");
    EXPECT_NEAR(result, -7.5f, 0.001f);

    engine.Shutdown();
}

// ============================================================================
// 追加テスト: SetContext with partial nullptrs
// ============================================================================

TEST(ScriptEngine, SetContextPartialNullptrs)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    // SpriteBatch=nullptr, TextureManager=nullptr でもクラッシュしないこと
    engine.SetContext(nullptr, nullptr);

    engine.Shutdown();
}

// ============================================================================
// 追加テスト: エラーリカバリ
// ============================================================================

TEST(ScriptEngine, ErrorThenSuccessfulExecution)
{
    ScriptEngine engine;
    ASSERT_TRUE(engine.Initialize());

    // エラーが発生しても後続の正常なコードが実行できること
    EXPECT_FALSE(engine.ExecuteString("invalid syntax !!!"));
    EXPECT_FALSE(engine.GetLastError().empty());

    EXPECT_TRUE(engine.ExecuteString("recovery = 42"));
    int val = engine.GetGlobalInt("recovery");
    EXPECT_EQ(val, 42);

    engine.Shutdown();
}

#endif // GX_ENABLE_LUA
