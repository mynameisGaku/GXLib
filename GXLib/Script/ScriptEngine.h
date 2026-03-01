#pragma once
/// @file ScriptEngine.h
/// @brief Lua スクリプトエンジン
///
/// Lua 5.4 + sol2 を使用したゲームロジックスクリプティング環境。
/// C++ 側のゲームオブジェクトや関数を Lua から呼び出せるようバインドし、
/// スクリプトファイルの読み込みと実行を管理する。

#include "pch_common.h"

// Forward declarations — Lua/sol2 は実装ファイルでのみインクルード
struct lua_State;
namespace sol { class state; }
/// @addtogroup grp_script
/// @{

namespace gx
{

// Forward declarations
class TextureManager;
class SpriteBatch;
class InputManager;

/// @brief Lua スクリプトエンジン
///
/// Lua VM を管理し、GXLib の各サブシステムとのバインディングを提供する。
/// ゲームロジックを C++ から Lua に分離でき、ホットリロードにも対応可能。
class ScriptEngine
{
public:
    ScriptEngine();
    ~ScriptEngine();

    // コピー禁止
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    /// @brief Lua VM を初期化し、標準バインディングを登録する
    /// @return 成功なら true
    bool Initialize();

    /// @brief 追加のバインディングコンテキストを設定する
    /// @param spriteBatch SpriteBatch（2D描画バインド用、nullptr可）
    /// @param texManager TextureManager（テクスチャバインド用、nullptr可）
    /// @param inputMgr InputManager（入力バインド用、nullptr可）
    void SetContext(SpriteBatch* spriteBatch, TextureManager* texManager,
                    InputManager* inputMgr = nullptr);

    /// @brief Lua スクリプトファイルを読み込んで実行する
    /// @param path ファイルパス
    /// @return 成功なら true
    bool ExecuteFile(const std::string& path);

    /// @brief Lua コード文字列を実行する
    /// @param code Lua コード
    /// @return 成功なら true
    bool ExecuteString(const std::string& code);

    /// @brief Lua のグローバル関数を呼び出す（引数なし）
    /// @param funcName 関数名
    /// @return 成功なら true
    bool CallFunction(const std::string& funcName);

    /// @brief Lua のグローバル関数を呼び出す（float引数1つ）
    /// @param funcName 関数名
    /// @param arg1 引数
    /// @return 成功なら true
    bool CallFunction(const std::string& funcName, float arg1);

    /// @brief グローバル変数を設定する（float）
    /// @param name 変数名
    /// @param value 値
    void SetGlobal(const std::string& name, float value);

    /// @brief グローバル変数を設定する（int）
    /// @param name 変数名
    /// @param value 値
    void SetGlobal(const std::string& name, int value);

    /// @brief グローバル変数を設定する（string）
    /// @param name 変数名
    /// @param value 値
    void SetGlobal(const std::string& name, const std::string& value);

    /// @brief グローバル変数を取得する（float）
    /// @param name 変数名
    /// @param defaultValue 変数が存在しない場合の既定値
    /// @return 変数の値
    float GetGlobalFloat(const std::string& name, float defaultValue = 0.0f) const;

    /// @brief グローバル変数を取得する（int）
    /// @param name 変数名
    /// @param defaultValue 変数が存在しない場合の既定値
    /// @return 変数の値
    int GetGlobalInt(const std::string& name, int defaultValue = 0) const;

    /// @brief Lua VM を破棄する
    void Shutdown();

    /// @brief 最後に発生したエラーメッセージを取得する
    /// @return エラー文字列
    const std::string& GetLastError() const { return m_lastError; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl; ///< Pimpl（sol::state を隠蔽）
    std::string m_lastError;      ///< 最後のエラーメッセージ
};

/// @}
} // namespace gx
