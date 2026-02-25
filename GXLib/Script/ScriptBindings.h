#pragma once
/// @file ScriptBindings.h
/// @brief Lua バインディング定義
///
/// GXLib の各サブシステムを Lua から利用できるようにするバインディング。
/// Vector2/3, Color, Transform3D, Input, Drawing 等の型と関数をバインドする。

#include "pch.h"

#ifdef GX_ENABLE_LUA

#define SOL_ALL_SAFETIES_ON 1
#include <sol/forward.hpp>

namespace GX
{

class SpriteBatch;
class TextureManager;
class InputManager;

/// @brief Lua バインディングユーティリティ
namespace ScriptBindings
{
    /// @brief 数学型（Vector2, Vector3, Color）をバインドする
    /// @param lua sol::state
    void RegisterMath(sol::state& lua);

    /// @brief 入力（Keyboard, Mouse）をバインドする
    /// @param lua sol::state
    /// @param inputMgr InputManager ポインタ
    void RegisterInput(sol::state& lua, InputManager* inputMgr = nullptr);

    /// @brief 描画（SpriteBatch ラッパー）をバインドする
    /// @param lua sol::state
    /// @param batch SpriteBatch ポインタ
    /// @param texManager TextureManager ポインタ
    void RegisterDrawing(sol::state& lua, SpriteBatch* batch, TextureManager* texManager);

} // namespace ScriptBindings

} // namespace GX

#endif // GX_ENABLE_LUA
