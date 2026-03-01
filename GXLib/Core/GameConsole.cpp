/// @file GameConsole.cpp
/// @brief ゲーム内コンソール（REPL）の実装

#include "pch_common.h"
#include "Core/GameConsole.h"

#include <algorithm>
#include <sstream>

namespace gx
{

// =============================================================================
// コマンド管理
// =============================================================================

void GameConsole::RegisterCommand(const ConsoleCommand& cmd)
{
    m_commands[cmd.name] = cmd;
}

void GameConsole::UnregisterCommand(const std::string& name)
{
    m_commands.erase(name);
}

bool GameConsole::HasCommand(const std::string& name) const
{
    return m_commands.find(name) != m_commands.end();
}

size_t GameConsole::GetCommandCount() const
{
    return m_commands.size();
}

std::vector<std::string> GameConsole::GetCommandNames() const
{
    std::vector<std::string> names;
    names.reserve(m_commands.size());
    for (const auto& [key, cmd] : m_commands)
    {
        names.push_back(key);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// =============================================================================
// 実行
// =============================================================================

std::string GameConsole::Execute(const std::string& input)
{
    if (!m_enabled)
    {
        return "";
    }

    // 空入力は無視
    if (input.empty())
    {
        return "";
    }

    // 入力を履歴に追加
    AddHistoryEntry(ConsoleEntry::Input, input);

    // トークン化
    auto tokens = ParseArgs(input);
    if (tokens.empty())
    {
        return "";
    }

    // 最初のトークンがコマンド名
    const std::string& cmdName = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    // コマンド検索
    auto it = m_commands.find(cmdName);
    if (it == m_commands.end())
    {
        std::string errorMsg = "Unknown command: " + cmdName;
        AddHistoryEntry(ConsoleEntry::Error, errorMsg);
        return errorMsg;
    }

    // コマンド実行
    std::string result;
    try
    {
        if (it->second.handler)
        {
            result = it->second.handler(args);
        }
    }
    catch (const std::exception& e)
    {
        result = std::string("Error: ") + e.what();
        AddHistoryEntry(ConsoleEntry::Error, result);
        return result;
    }

    // 結果を履歴に追加
    if (!result.empty())
    {
        AddHistoryEntry(ConsoleEntry::Output, result);
    }

    return result;
}

// =============================================================================
// 自動補完
// =============================================================================

std::vector<std::string> GameConsole::GetAutoComplete(const std::string& partial) const
{
    std::vector<std::string> results;

    if (partial.empty())
    {
        // 空文字列の場合は全コマンド名を返す
        for (const auto& [key, cmd] : m_commands)
        {
            results.push_back(key);
        }
    }
    else
    {
        // 前方一致するコマンド名を収集
        for (const auto& [key, cmd] : m_commands)
        {
            if (key.size() >= partial.size() &&
                key.substr(0, partial.size()) == partial)
            {
                results.push_back(key);
            }
        }
    }

    std::sort(results.begin(), results.end());
    return results;
}

// =============================================================================
// 履歴
// =============================================================================

const std::vector<ConsoleEntry>& GameConsole::GetHistory() const
{
    return m_history;
}

size_t GameConsole::GetHistorySize() const
{
    return m_history.size();
}

void GameConsole::SetMaxHistory(size_t max)
{
    m_maxHistory = max;
    // 現在の履歴が新しい上限を超えている場合は先頭を削除
    while (m_history.size() > m_maxHistory)
    {
        m_history.erase(m_history.begin());
    }
}

void GameConsole::ClearHistory()
{
    m_history.clear();
}

// =============================================================================
// CVar
// =============================================================================

void GameConsole::SetCVar(const std::string& name, const std::string& value)
{
    m_cvars[name] = value;
}

std::string GameConsole::GetCVar(const std::string& name, const std::string& defaultValue) const
{
    auto it = m_cvars.find(name);
    if (it != m_cvars.end())
    {
        return it->second;
    }
    return defaultValue;
}

bool GameConsole::HasCVar(const std::string& name) const
{
    return m_cvars.find(name) != m_cvars.end();
}

// =============================================================================
// ビルトインコマンド
// =============================================================================

void GameConsole::RegisterBuiltinCommands()
{
    // help — 全コマンド一覧を表示
    {
        ConsoleCommand cmd;
        cmd.name        = "help";
        cmd.description = "List all available commands";
        cmd.usage       = "help";
        cmd.handler     = [this](const std::vector<std::string>& /*args*/) -> std::string
        {
            std::string result = "Available commands:\n";
            auto names = GetCommandNames();
            for (const auto& name : names)
            {
                auto it = m_commands.find(name);
                if (it != m_commands.end())
                {
                    result += "  " + name;
                    if (!it->second.description.empty())
                    {
                        result += " - " + it->second.description;
                    }
                    result += "\n";
                }
            }
            return result;
        };
        RegisterCommand(cmd);
    }

    // echo — 引数をそのまま返す
    {
        ConsoleCommand cmd;
        cmd.name        = "echo";
        cmd.description = "Echo the given arguments";
        cmd.usage       = "echo <text...>";
        cmd.handler     = [](const std::vector<std::string>& args) -> std::string
        {
            std::string result;
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0) result += " ";
                result += args[i];
            }
            return result;
        };
        RegisterCommand(cmd);
    }

    // set — CVar を設定
    {
        ConsoleCommand cmd;
        cmd.name        = "set";
        cmd.description = "Set a console variable";
        cmd.usage       = "set <name> <value>";
        cmd.handler     = [this](const std::vector<std::string>& args) -> std::string
        {
            if (args.size() < 2)
            {
                return "Usage: set <name> <value>";
            }
            // 2番目以降を全て値として結合
            std::string value;
            for (size_t i = 1; i < args.size(); ++i)
            {
                if (i > 1) value += " ";
                value += args[i];
            }
            SetCVar(args[0], value);
            return args[0] + " = " + value;
        };
        RegisterCommand(cmd);
    }

    // get — CVar を取得
    {
        ConsoleCommand cmd;
        cmd.name        = "get";
        cmd.description = "Get a console variable value";
        cmd.usage       = "get <name>";
        cmd.handler     = [this](const std::vector<std::string>& args) -> std::string
        {
            if (args.empty())
            {
                return "Usage: get <name>";
            }
            if (!HasCVar(args[0]))
            {
                return "Variable not found: " + args[0];
            }
            return args[0] + " = " + GetCVar(args[0]);
        };
        RegisterCommand(cmd);
    }

    // clear — 履歴をクリア
    {
        ConsoleCommand cmd;
        cmd.name        = "clear";
        cmd.description = "Clear console history";
        cmd.usage       = "clear";
        cmd.handler     = [this](const std::vector<std::string>& /*args*/) -> std::string
        {
            ClearHistory();
            return "";
        };
        RegisterCommand(cmd);
    }
}

// =============================================================================
// 有効/無効
// =============================================================================

void GameConsole::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool GameConsole::IsEnabled() const
{
    return m_enabled;
}

// =============================================================================
// Private ヘルパー
// =============================================================================

std::vector<std::string> GameConsole::ParseArgs(const std::string& input) const
{
    std::vector<std::string> tokens;
    std::string current;
    bool inQuote  = false;
    char quoteChar = '\0';

    for (size_t i = 0; i < input.size(); ++i)
    {
        char c = input[i];

        if (inQuote)
        {
            if (c == quoteChar)
            {
                // 引用符を閉じる
                inQuote = false;
            }
            else
            {
                current += c;
            }
        }
        else
        {
            if (c == '"' || c == '\'')
            {
                // 引用符開始
                inQuote   = true;
                quoteChar = c;
            }
            else if (c == ' ' || c == '\t')
            {
                // 区切り
                if (!current.empty())
                {
                    tokens.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current += c;
            }
        }
    }

    // 残りのトークン
    if (!current.empty())
    {
        tokens.push_back(current);
    }

    return tokens;
}

void GameConsole::AddHistoryEntry(ConsoleEntry::Type type, const std::string& text)
{
    m_history.push_back({type, text});

    // 最大履歴数を超えた場合は先頭から削除
    while (m_history.size() > m_maxHistory)
    {
        m_history.erase(m_history.begin());
    }
}

} // namespace gx
