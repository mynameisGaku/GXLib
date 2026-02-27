/// @file ScriptBindings.cpp
/// @brief Lua バインディングの実装
#include "pch_common.h"
#include "Script/ScriptBindings.h"

#ifdef GX_ENABLE_LUA

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Color.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Resource/TextureManager.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "Input/InputManager.h"

namespace gx
{
namespace ScriptBindings
{

void RegisterMath(sol::state& lua)
{
    // Vector2
    lua.new_usertype<Vector2>("Vector2",
        sol::constructors<Vector2(), Vector2(float, float)>(),
        "x", &Vector2::x,
        "y", &Vector2::y,
        "Length", &Vector2::Length,
        "LengthSquared", &Vector2::LengthSquared,
        "Normalized", &Vector2::Normalized,
        sol::meta_function::addition, [](const Vector2& a, const Vector2& b) { return a + b; },
        sol::meta_function::subtraction, [](const Vector2& a, const Vector2& b) { return a - b; },
        sol::meta_function::multiplication, [](const Vector2& a, float s) { return a * s; }
    );

    // Vector3
    lua.new_usertype<Vector3>("Vector3",
        sol::constructors<Vector3(), Vector3(float, float, float)>(),
        "x", &Vector3::x,
        "y", &Vector3::y,
        "z", &Vector3::z,
        "Length", &Vector3::Length,
        "LengthSquared", &Vector3::LengthSquared,
        "Normalized", &Vector3::Normalized,
        sol::meta_function::addition, [](const Vector3& a, const Vector3& b) { return a + b; },
        sol::meta_function::subtraction, [](const Vector3& a, const Vector3& b) { return a - b; },
        sol::meta_function::multiplication, [](const Vector3& a, float s) { return a * s; }
    );

    // Color
    lua.new_usertype<Color>("Color",
        sol::constructors<Color(), Color(float, float, float, float)>(),
        "r", &Color::r,
        "g", &Color::g,
        "b", &Color::b,
        "a", &Color::a,
        "ToRGBA", &Color::ToRGBA
    );

    // Math utility functions
    lua["Lerp"] = [](float a, float b, float t) { return a + (b - a) * t; };
    lua["Clamp"] = [](float v, float lo, float hi) { return (std::max)(lo, (std::min)(v, hi)); };
}

void RegisterInput(sol::state& lua, InputManager* inputMgr)
{
    if (!inputMgr) return;

    // Keyboard — グローバル関数として公開
    lua["IsKeyDown"] = [inputMgr](int key) -> bool {
        return inputMgr->GetKeyboard().IsKeyDown(key);
    };
    lua["IsKeyTriggered"] = [inputMgr](int key) -> bool {
        return inputMgr->GetKeyboard().IsKeyTriggered(key);
    };
    lua["IsKeyReleased"] = [inputMgr](int key) -> bool {
        return inputMgr->GetKeyboard().IsKeyReleased(key);
    };

    // Mouse
    lua["GetMouseX"] = [inputMgr]() -> int {
        return inputMgr->GetMouse().GetX();
    };
    lua["GetMouseY"] = [inputMgr]() -> int {
        return inputMgr->GetMouse().GetY();
    };
    lua["IsMouseButtonDown"] = [inputMgr](int btn) -> bool {
        return inputMgr->GetMouse().IsButtonDown(btn);
    };
    lua["IsMouseButtonTriggered"] = [inputMgr](int btn) -> bool {
        return inputMgr->GetMouse().IsButtonTriggered(btn);
    };

    // よく使うキーコード定数（Win32 VKコード — Keyboard::IsKeyDown()に直接渡せる）
    lua["KEY_UP"]    = VK_UP;
    lua["KEY_DOWN"]  = VK_DOWN;
    lua["KEY_LEFT"]  = VK_LEFT;
    lua["KEY_RIGHT"] = VK_RIGHT;
    lua["KEY_SPACE"] = VK_SPACE;
    lua["KEY_RETURN"]= VK_RETURN;
    lua["KEY_ESCAPE"]= VK_ESCAPE;
    lua["KEY_Z"]     = static_cast<int>('Z');
    lua["KEY_X"]     = static_cast<int>('X');
}

void RegisterDrawing(sol::state& lua, SpriteBatch* batch, TextureManager* texManager)
{
    if (!batch || !texManager) return;

    // テクスチャ読み込み
    lua["LoadTexture"] = [texManager](const std::string& path) -> int {
        std::wstring wpath(path.begin(), path.end());
        return texManager->LoadTexture(wpath);
    };

    // 基本描画
    lua["DrawGraph"] = [batch](float x, float y, int handle) {
        batch->DrawGraph(x, y, handle);
    };
    lua["DrawRectGraph"] = [batch](float x, float y, int srcX, int srcY, int w, int h, int handle) {
        batch->DrawRectGraph(x, y, srcX, srcY, w, h, handle, true);
    };
    lua["DrawBox"] = [batch](float x1, float y1, float x2, float y2,
                             float r, float g, float b, float a, bool fill) {
        batch->SetDrawColor(r, g, b, a);
        // SpriteBatch は矩形塗りつぶし描画をサポート
    };
    lua["SetDrawColor"] = [batch](float r, float g, float b, float a) {
        batch->SetDrawColor(r, g, b, a);
    };
}

} // namespace ScriptBindings
} // namespace gx

#endif // GX_ENABLE_LUA
