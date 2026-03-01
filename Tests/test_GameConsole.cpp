/// @file test_GameConsole.cpp
/// @brief GameConsole 単体テスト — コマンド登録・実行・CVar・履歴・自動補完

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/GameConsole.h"

using namespace gx;

// =============================================================================
// 基本テスト
// =============================================================================

TEST(GameConsoleTest, ConstructDestruct)
{
    GameConsole console;
    EXPECT_TRUE(console.IsEnabled());
    EXPECT_EQ(console.GetCommandCount(), 0u);
    EXPECT_EQ(console.GetHistorySize(), 0u);
}

TEST(GameConsoleTest, RegisterCommand)
{
    GameConsole console;
    ConsoleCommand cmd;
    cmd.name        = "test";
    cmd.description = "A test command";
    cmd.handler     = [](const std::vector<std::string>&) { return std::string("ok"); };

    console.RegisterCommand(cmd);
    EXPECT_TRUE(console.HasCommand("test"));
    EXPECT_EQ(console.GetCommandCount(), 1u);
}

TEST(GameConsoleTest, UnregisterCommand)
{
    GameConsole console;
    ConsoleCommand cmd;
    cmd.name    = "test";
    cmd.handler = [](const std::vector<std::string>&) { return std::string("ok"); };

    console.RegisterCommand(cmd);
    EXPECT_TRUE(console.HasCommand("test"));

    console.UnregisterCommand("test");
    EXPECT_FALSE(console.HasCommand("test"));
    EXPECT_EQ(console.GetCommandCount(), 0u);
}

TEST(GameConsoleTest, HasCommand)
{
    GameConsole console;
    EXPECT_FALSE(console.HasCommand("missing"));

    ConsoleCommand cmd;
    cmd.name    = "exists";
    cmd.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmd);

    EXPECT_TRUE(console.HasCommand("exists"));
    EXPECT_FALSE(console.HasCommand("missing"));
}

TEST(GameConsoleTest, GetCommandCount)
{
    GameConsole console;
    EXPECT_EQ(console.GetCommandCount(), 0u);

    ConsoleCommand cmd1;
    cmd1.name    = "cmd1";
    cmd1.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmd1);

    ConsoleCommand cmd2;
    cmd2.name    = "cmd2";
    cmd2.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmd2);

    EXPECT_EQ(console.GetCommandCount(), 2u);
}

TEST(GameConsoleTest, GetCommandNames)
{
    GameConsole console;

    ConsoleCommand cmdB;
    cmdB.name    = "beta";
    cmdB.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmdB);

    ConsoleCommand cmdA;
    cmdA.name    = "alpha";
    cmdA.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmdA);

    auto names = console.GetCommandNames();
    ASSERT_EQ(names.size(), 2u);
    // GetCommandNames returns sorted
    EXPECT_EQ(names[0], "alpha");
    EXPECT_EQ(names[1], "beta");
}

// =============================================================================
// 実行テスト
// =============================================================================

TEST(GameConsoleTest, ExecuteSimple)
{
    GameConsole console;
    ConsoleCommand cmd;
    cmd.name    = "greet";
    cmd.handler = [](const std::vector<std::string>&) { return std::string("hello"); };
    console.RegisterCommand(cmd);

    std::string result = console.Execute("greet");
    EXPECT_EQ(result, "hello");
}

TEST(GameConsoleTest, ExecuteWithArgs)
{
    GameConsole console;
    ConsoleCommand cmd;
    cmd.name    = "add";
    cmd.handler = [](const std::vector<std::string>& args) -> std::string
    {
        if (args.size() < 2) return "need 2 args";
        int a = std::stoi(args[0]);
        int b = std::stoi(args[1]);
        return std::to_string(a + b);
    };
    console.RegisterCommand(cmd);

    std::string result = console.Execute("add 3 5");
    EXPECT_EQ(result, "8");
}

TEST(GameConsoleTest, ExecuteUnknown)
{
    GameConsole console;

    std::string result = console.Execute("nosuchcommand");
    EXPECT_FALSE(result.empty());
    // Should contain error message about unknown command
    EXPECT_NE(result.find("Unknown command"), std::string::npos);
}

// =============================================================================
// 自動補完テスト
// =============================================================================

TEST(GameConsoleTest, AutoComplete)
{
    GameConsole console;

    ConsoleCommand cmd1;
    cmd1.name    = "help";
    cmd1.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmd1);

    ConsoleCommand cmd2;
    cmd2.name    = "hello";
    cmd2.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmd2);

    // Empty partial returns all
    auto all = console.GetAutoComplete("");
    EXPECT_EQ(all.size(), 2u);
}

TEST(GameConsoleTest, AutoCompletePartial)
{
    GameConsole console;

    ConsoleCommand cmd1;
    cmd1.name    = "help";
    cmd1.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmd1);

    ConsoleCommand cmd2;
    cmd2.name    = "hello";
    cmd2.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmd2);

    ConsoleCommand cmd3;
    cmd3.name    = "quit";
    cmd3.handler = [](const std::vector<std::string>&) { return std::string(); };
    console.RegisterCommand(cmd3);

    auto matches = console.GetAutoComplete("hel");
    ASSERT_EQ(matches.size(), 2u);
    EXPECT_EQ(matches[0], "hello");
    EXPECT_EQ(matches[1], "help");

    auto qMatch = console.GetAutoComplete("q");
    ASSERT_EQ(qMatch.size(), 1u);
    EXPECT_EQ(qMatch[0], "quit");
}

// =============================================================================
// 履歴テスト
// =============================================================================

TEST(GameConsoleTest, History)
{
    GameConsole console;
    ConsoleCommand cmd;
    cmd.name    = "test";
    cmd.handler = [](const std::vector<std::string>&) { return std::string("result"); };
    console.RegisterCommand(cmd);

    console.Execute("test");

    const auto& history = console.GetHistory();
    ASSERT_GE(history.size(), 2u); // Input + Output
    EXPECT_EQ(history[0].type, ConsoleEntry::Input);
    EXPECT_EQ(history[0].text, "test");
    EXPECT_EQ(history[1].type, ConsoleEntry::Output);
    EXPECT_EQ(history[1].text, "result");
}

TEST(GameConsoleTest, MaxHistory)
{
    GameConsole console;
    console.SetMaxHistory(5);

    ConsoleCommand cmd;
    cmd.name    = "echo";
    cmd.handler = [](const std::vector<std::string>& args) -> std::string
    {
        return args.empty() ? "" : args[0];
    };
    console.RegisterCommand(cmd);

    // Each Execute adds 2 entries (Input + Output with non-empty result)
    for (int i = 0; i < 10; ++i)
    {
        console.Execute("echo " + std::to_string(i));
    }

    EXPECT_LE(console.GetHistorySize(), 5u);
}

TEST(GameConsoleTest, ClearHistory)
{
    GameConsole console;
    ConsoleCommand cmd;
    cmd.name    = "test";
    cmd.handler = [](const std::vector<std::string>&) { return std::string("ok"); };
    console.RegisterCommand(cmd);

    console.Execute("test");
    EXPECT_GT(console.GetHistorySize(), 0u);

    console.ClearHistory();
    EXPECT_EQ(console.GetHistorySize(), 0u);
}

// =============================================================================
// CVar テスト
// =============================================================================

TEST(GameConsoleTest, CVar)
{
    GameConsole console;

    EXPECT_FALSE(console.HasCVar("fps"));
    EXPECT_EQ(console.GetCVar("fps", "60"), "60"); // default

    console.SetCVar("fps", "120");
    EXPECT_TRUE(console.HasCVar("fps"));
    EXPECT_EQ(console.GetCVar("fps"), "120");

    // Overwrite
    console.SetCVar("fps", "30");
    EXPECT_EQ(console.GetCVar("fps"), "30");
}

// =============================================================================
// ビルトインコマンドテスト
// =============================================================================

TEST(GameConsoleTest, RegisterBuiltins)
{
    GameConsole console;
    console.RegisterBuiltinCommands();

    EXPECT_TRUE(console.HasCommand("help"));
    EXPECT_TRUE(console.HasCommand("echo"));
    EXPECT_TRUE(console.HasCommand("set"));
    EXPECT_TRUE(console.HasCommand("get"));
    EXPECT_TRUE(console.HasCommand("clear"));

    // Test echo
    std::string echoResult = console.Execute("echo hello world");
    EXPECT_EQ(echoResult, "hello world");

    // Test set/get
    console.Execute("set myvar 42");
    EXPECT_EQ(console.GetCVar("myvar"), "42");

    std::string getResult = console.Execute("get myvar");
    EXPECT_NE(getResult.find("42"), std::string::npos);
}

// =============================================================================
// 有効/無効テスト
// =============================================================================

TEST(GameConsoleTest, EnableDisable)
{
    GameConsole console;
    EXPECT_TRUE(console.IsEnabled());

    ConsoleCommand cmd;
    cmd.name    = "test";
    cmd.handler = [](const std::vector<std::string>&) { return std::string("ok"); };
    console.RegisterCommand(cmd);

    // Disable — Execute should return empty
    console.SetEnabled(false);
    EXPECT_FALSE(console.IsEnabled());
    std::string result = console.Execute("test");
    EXPECT_TRUE(result.empty());

    // Re-enable
    console.SetEnabled(true);
    EXPECT_TRUE(console.IsEnabled());
    result = console.Execute("test");
    EXPECT_EQ(result, "ok");
}
