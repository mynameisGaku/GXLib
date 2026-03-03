#pragma once
/// @file GX/Input.h
/// @brief 入力関数

// Forward declarations for existing Compat functions
int CheckHitKey(int keyCode);
int GetHitKeyStateAll(char* keyStateBuf);
int GetMouseInput();
int GetMousePoint(int* x, int* y);
int GetMouseWheelRotVol();
int GetJoypadInputState(int inputType);

// Action mapping
void SetActionKey(const char* name, int key);
void SetActionButton(const char* name, int padIndex, int button);
bool IsActionPressed(const char* name);
bool IsActionTriggered(const char* name);
float GetActionAxis(const char* name);

namespace gx {
/// @addtogroup grp_gx_facade
/// @{

using ::CheckHitKey;
using ::GetHitKeyStateAll;
using ::GetMouseInput;
using ::GetMousePoint;
using ::GetMouseWheelRotVol;
using ::GetJoypadInputState;

// Action mapping
using ::SetActionKey;
using ::SetActionButton;
using ::IsActionPressed;
using ::IsActionTriggered;
using ::GetActionAxis;

/// @brief キーコード定数（DXLib互換）
namespace Key {
    static constexpr int Escape = 0x01;
    static constexpr int Space  = 0x39;
    static constexpr int Return = 0x1C;
    static constexpr int Up     = 0xC8;
    static constexpr int Down   = 0xD0;
    static constexpr int Left   = 0xCB;
    static constexpr int Right  = 0xCD;
    static constexpr int A = 0x1E, B = 0x30, C = 0x2E, D = 0x20;
    static constexpr int W = 0x11, S = 0x1F;
    static constexpr int Z = 0x2C, X = 0x2D;
}

/// @}
} // namespace gx
