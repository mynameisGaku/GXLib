#pragma once
/// @file GX/Input.h
/// @brief Input functions / 入力関数 — Layer 1 facade per ADR-0017
///
/// Keyboard, mouse, and gamepad input. All functions return immediately
/// (no blocking) and reflect the state at the start of the current frame.
///
/// キーボード・マウス・ゲームパッド入力。すべてノンブロッキングで、
/// 現在のフレーム開始時の状態を返す。
///
/// @code
/// // Keyboard — is SPACE pressed right now?
/// if (gx::CheckHitKey(KEY_INPUT_SPACE)) { /* jump */ }
///
/// // Mouse — where is the cursor?
/// int mx, my;
/// gx::GetMousePoint(&mx, &my);
///
/// // Action mapping (rebindable) — register once, query by name
/// gx::SetActionKey("jump", KEY_INPUT_SPACE);
/// gx::SetActionButton("jump", PAD_INPUT_A);
/// if (gx::IsActionTriggered("jump")) { /* jump */ }
/// @endcode

// Forward declarations for existing Compat functions

/// @brief Check if a key is currently pressed. キーが押されているか。
/// @param keyCode DXLib key scan code (KEY_INPUT_*).
/// @return 1 if pressed, 0 if not.
/// @code if (CheckHitKey(KEY_INPUT_ESCAPE)) break; @endcode
int CheckHitKey(int keyCode);

/// @brief Get the pressed state of all 256 keys at once. 全キー状態を一括取得。
/// @param keyStateBuf Pointer to a char[256] buffer. 各要素が0/1。
/// @return 0 on success.
/// @code char keys[256]; GetHitKeyStateAll(keys); if (keys[KEY_INPUT_W]) { /* forward */ } @endcode
int GetHitKeyStateAll(char* keyStateBuf);

/// @brief Get mouse button state as a bitmask. マウスボタンをビットフラグで取得。
/// @return bit0=left(1), bit1=right(2), bit2=middle(4).
/// @code if (GetMouseInput() & 1) { /* left click */ } @endcode
int GetMouseInput();

/// @brief Get the mouse cursor position (client coordinates). マウス座標取得。
/// @param x [out] X position (nullable). @param y [out] Y position (nullable).
/// @code int mx, my; GetMousePoint(&mx, &my); @endcode
int GetMousePoint(int* x, int* y);

/// @brief Get accumulated mouse wheel rotation since last call. ホイール回転量。
/// @return Positive = scroll up, negative = scroll down.
int GetMouseWheelRotVol();

/// @brief Get gamepad button state as a bitmask (XInput). パッドボタン状態。
/// @param inputType GX_INPUT_PAD1 .. GX_INPUT_PAD4.
/// @return Bitmask (PAD_INPUT_A, PAD_INPUT_B, ...).
/// @code int pad = GetJoypadInputState(GX_INPUT_PAD1); if (pad & PAD_INPUT_A) { /* A pressed */ } @endcode
int GetJoypadInputState(int inputType);

/// @brief Register a keyboard key binding for a named action. アクションにキーをバインド。
/// @param name Action name (e.g. "jump"). @param key KEY_INPUT_* scan code.
/// @code SetActionKey("jump", KEY_INPUT_SPACE); @endcode
void SetActionKey(const char* name, int key);

/// @brief Register a gamepad button binding for a named action. アクションにパッドボタンをバインド。
/// @param name Action name. @param padIndex Pad index (0-3). @param button PAD_INPUT_* constant.
void SetActionButton(const char* name, int padIndex, int button);

/// @brief Check if a named action is currently held. アクション押下中か。
/// @return true while any bound input is held.
bool IsActionPressed(const char* name);

/// @brief Check if a named action was triggered this frame (rising edge). 今フレームで押されたか。
/// @return true only on the frame the input transitions from released to pressed.
bool IsActionTriggered(const char* name);

/// @brief Get the analog axis value for a named action. アクションの軸値取得。
/// @return [-1.0, 1.0] for stick axes; 0.0 if no axis binding or not pressed.
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

using ::SetActionKey;
using ::SetActionButton;
using ::IsActionPressed;
using ::IsActionTriggered;
using ::GetActionAxis;

/// @brief Key scan code constants (DXLib-compatible). キーコード定数。
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
