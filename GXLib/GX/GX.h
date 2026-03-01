#pragma once
/// @file GX/GX.h
/// @brief GXLib 統一ヘッダー
///
/// このファイル1つをインクルードするだけで、GXLibの全機能を利用可能。
///
/// 使用例（手続き型）:
/// @code
/// #include <GX/GX.h>
/// int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
///     gx::Init({.title = "My Game", .width = 1280, .height = 720});
///     while (gx::ProcessMsg() == 0) {
///         gx::ClearScreen();
///         gx::DrawCircle(400, 300, 50, gx::GetColor(255, 100, 50));
///         gx::Present();
///     }
///     gx::End();
/// }
/// @endcode
///
/// 使用例（OOP）:
/// @code
/// #include <GX/GX.h>
/// class MyGame : public gx::App {
///     void Draw() override {
///         gx::DrawCircle(400, 300, 50, gx::GetColor(255, 0, 0));
///     }
/// };
/// GX_APP(MyGame)
/// @endcode
/// @addtogroup grp_gx_facade
/// @{

#include "GX/Config.h"
#include "GX/App.h"
#include "GX/System.h"
#include "GX/Draw2D.h"
#include "GX/Text.h"
#include "GX/Input.h"
#include "GX/Audio.h"
#include "GX/Math.h"

// Engine facade for advanced users
#include "Core/Engine.h"
/// @}
