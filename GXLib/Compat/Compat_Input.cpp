/// @file Compat_Input.cpp
/// @brief 簡易API 入力関数の実装
///
/// DXLib互換のDIKキーコード→Win32 VKコード変換テーブルを提供。
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatContext.h"

using Ctx = GX_Internal::CompatContext;

// ============================================================================
// DIK → VK 変換テーブル (256エントリ)
// DxLibはDirectInput(DIK_*)キーコードを使うが、GXLibはWin32 VKコードで動作する。
// この変換テーブルにより CheckHitKey(KEY_INPUT_*) がそのまま使える。
// ============================================================================
static int s_dikToVK[256] = {};
static bool s_dikTableInitialized = false;

static void InitDIKTable()
{
    if (s_dikTableInitialized) return;
    s_dikTableInitialized = true;

    memset(s_dikToVK, 0, sizeof(s_dikToVK));

    s_dikToVK[0x01] = VK_ESCAPE;
    s_dikToVK[0x02] = '1';
    s_dikToVK[0x03] = '2';
    s_dikToVK[0x04] = '3';
    s_dikToVK[0x05] = '4';
    s_dikToVK[0x06] = '5';
    s_dikToVK[0x07] = '6';
    s_dikToVK[0x08] = '7';
    s_dikToVK[0x09] = '8';
    s_dikToVK[0x0A] = '9';
    s_dikToVK[0x0B] = '0';
    s_dikToVK[0x0C] = VK_OEM_MINUS;
    s_dikToVK[0x0E] = VK_BACK;
    s_dikToVK[0x0F] = VK_TAB;
    s_dikToVK[0x10] = 'Q';
    s_dikToVK[0x11] = 'W';
    s_dikToVK[0x12] = 'E';
    s_dikToVK[0x13] = 'R';
    s_dikToVK[0x14] = 'T';
    s_dikToVK[0x15] = 'Y';
    s_dikToVK[0x16] = 'U';
    s_dikToVK[0x17] = 'I';
    s_dikToVK[0x18] = 'O';
    s_dikToVK[0x19] = 'P';
    s_dikToVK[0x1A] = VK_OEM_4;
    s_dikToVK[0x1B] = VK_OEM_6;
    s_dikToVK[0x1C] = VK_RETURN;
    s_dikToVK[0x1D] = VK_LCONTROL;
    s_dikToVK[0x1E] = 'A';
    s_dikToVK[0x1F] = 'S';
    s_dikToVK[0x20] = 'D';
    s_dikToVK[0x21] = 'F';
    s_dikToVK[0x22] = 'G';
    s_dikToVK[0x23] = 'H';
    s_dikToVK[0x24] = 'J';
    s_dikToVK[0x25] = 'K';
    s_dikToVK[0x26] = 'L';
    s_dikToVK[0x27] = VK_OEM_1;
    s_dikToVK[0x2A] = VK_LSHIFT;
    s_dikToVK[0x2B] = VK_OEM_5;
    s_dikToVK[0x2C] = 'Z';
    s_dikToVK[0x2D] = 'X';
    s_dikToVK[0x2E] = 'C';
    s_dikToVK[0x2F] = 'V';
    s_dikToVK[0x30] = 'B';
    s_dikToVK[0x31] = 'N';
    s_dikToVK[0x32] = 'M';
    s_dikToVK[0x33] = VK_OEM_COMMA;
    s_dikToVK[0x34] = VK_OEM_PERIOD;
    s_dikToVK[0x35] = VK_OEM_2;
    s_dikToVK[0x36] = VK_RSHIFT;
    s_dikToVK[0x37] = VK_MULTIPLY;
    s_dikToVK[0x38] = VK_LMENU;
    s_dikToVK[0x39] = VK_SPACE;
    s_dikToVK[0x3A] = VK_CAPITAL;
    s_dikToVK[0x3B] = VK_F1;
    s_dikToVK[0x3C] = VK_F2;
    s_dikToVK[0x3D] = VK_F3;
    s_dikToVK[0x3E] = VK_F4;
    s_dikToVK[0x3F] = VK_F5;
    s_dikToVK[0x40] = VK_F6;
    s_dikToVK[0x41] = VK_F7;
    s_dikToVK[0x42] = VK_F8;
    s_dikToVK[0x43] = VK_F9;
    s_dikToVK[0x44] = VK_F10;
    s_dikToVK[0x45] = VK_NUMLOCK;
    s_dikToVK[0x46] = VK_SCROLL;
    s_dikToVK[0x47] = VK_NUMPAD7;
    s_dikToVK[0x48] = VK_NUMPAD8;
    s_dikToVK[0x49] = VK_NUMPAD9;
    s_dikToVK[0x4A] = VK_SUBTRACT;
    s_dikToVK[0x4B] = VK_NUMPAD4;
    s_dikToVK[0x4C] = VK_NUMPAD5;
    s_dikToVK[0x4D] = VK_NUMPAD6;
    s_dikToVK[0x4E] = VK_ADD;
    s_dikToVK[0x4F] = VK_NUMPAD1;
    s_dikToVK[0x50] = VK_NUMPAD2;
    s_dikToVK[0x51] = VK_NUMPAD3;
    s_dikToVK[0x52] = VK_NUMPAD0;
    s_dikToVK[0x53] = VK_DECIMAL;
    s_dikToVK[0x57] = VK_F11;
    s_dikToVK[0x58] = VK_F12;
    s_dikToVK[0x9D] = VK_RCONTROL;
    s_dikToVK[0xB5] = VK_DIVIDE;
    s_dikToVK[0xB8] = VK_RMENU;
    s_dikToVK[0xC7] = VK_HOME;
    s_dikToVK[0xC8] = VK_UP;
    s_dikToVK[0xC9] = VK_PRIOR;
    s_dikToVK[0xCB] = VK_LEFT;
    s_dikToVK[0xCD] = VK_RIGHT;
    s_dikToVK[0xCF] = VK_END;
    s_dikToVK[0xD0] = VK_DOWN;
    s_dikToVK[0xD1] = VK_NEXT;
    s_dikToVK[0xD2] = VK_INSERT;
    s_dikToVK[0xD3] = VK_DELETE;
}

// ============================================================================
// キーボード
// ============================================================================
int CheckHitKey(int keyCode)
{
    InitDIKTable();
    auto& ctx = Ctx::Instance();
    int vk = (keyCode >= 0 && keyCode < 256) ? s_dikToVK[keyCode] : 0;
    if (vk == 0) return 0;
    return ctx.inputManager.GetKeyboard().IsKeyDown(vk) ? 1 : 0;
}

int GetHitKeyStateAll(char* keyStateBuf)
{
    InitDIKTable();
    auto& ctx = Ctx::Instance();
    memset(keyStateBuf, 0, 256);
    for (int dik = 0; dik < 256; ++dik)
    {
        int vk = s_dikToVK[dik];
        if (vk != 0 && ctx.inputManager.GetKeyboard().IsKeyDown(vk))
            keyStateBuf[dik] = 1;
    }
    return 0;
}

// ============================================================================
// マウス
// ============================================================================
int GetMouseInput()
{
    return Ctx::Instance().inputManager.GetMouseInput();
}

int GetMousePoint(int* x, int* y)
{
    Ctx::Instance().inputManager.GetMousePoint(x, y);
    return 0;
}

int GetMouseWheelRotVol()
{
    return Ctx::Instance().inputManager.GetMouseWheel();
}

// ============================================================================
// ゲームパッド
// ============================================================================
int GetJoypadInputState(int inputType)
{
    auto& ctx = Ctx::Instance();
    auto& pad = ctx.inputManager.GetGamepad();
    int padIdx = inputType;
    if (padIdx < 0 || padIdx >= GX::Gamepad::k_MaxPads) return 0;
    if (!pad.IsConnected(padIdx)) return 0;

    int result = 0;

    if (pad.IsButtonDown(padIdx, GX::PadButton::DPadDown))  result |= PAD_INPUT_DOWN;
    if (pad.IsButtonDown(padIdx, GX::PadButton::DPadLeft))  result |= PAD_INPUT_LEFT;
    if (pad.IsButtonDown(padIdx, GX::PadButton::DPadRight)) result |= PAD_INPUT_RIGHT;
    if (pad.IsButtonDown(padIdx, GX::PadButton::DPadUp))    result |= PAD_INPUT_UP;
    if (pad.IsButtonDown(padIdx, GX::PadButton::A))  result |= PAD_INPUT_A;
    if (pad.IsButtonDown(padIdx, GX::PadButton::B))  result |= PAD_INPUT_B;
    if (pad.IsButtonDown(padIdx, GX::PadButton::X))  result |= PAD_INPUT_X;
    if (pad.IsButtonDown(padIdx, GX::PadButton::Y))  result |= PAD_INPUT_Y;
    if (pad.IsButtonDown(padIdx, GX::PadButton::LeftShoulder))  result |= PAD_INPUT_L;
    if (pad.IsButtonDown(padIdx, GX::PadButton::RightShoulder)) result |= PAD_INPUT_R;
    if (pad.IsButtonDown(padIdx, GX::PadButton::Start)) result |= PAD_INPUT_START;

    return result;
}

// ============================================================================
// アクションマッピング
// ============================================================================

/// DIKキーコードをVKキーコードに変換する
static int DIKToVK(int dikCode)
{
    InitDIKTable();
    if (dikCode >= 0 && dikCode < 256) return s_dikToVK[dikCode];
    return dikCode;
}

int SetActionKey(const char* actionName, int keyCode)
{
    auto& ctx = Ctx::Instance();
    auto& am = ctx.inputManager.GetActionMapping();
    int vk = DIKToVK(keyCode);
    am.DefineAction(actionName, { GX::InputBinding::Key(vk) });
    return 0;
}

int SetActionButton(const char* actionName, int padButton)
{
    auto& ctx = Ctx::Instance();
    auto& am = ctx.inputManager.GetActionMapping();
    am.DefineAction(actionName, { GX::InputBinding::PadBtn(padButton) });
    return 0;
}

int IsActionPressed(const char* actionName)
{
    auto& ctx = Ctx::Instance();
    auto& am = ctx.inputManager.GetActionMapping();
    return am.IsActionPressed(actionName) ? 1 : 0;
}

int IsActionTriggered(const char* actionName)
{
    auto& ctx = Ctx::Instance();
    auto& am = ctx.inputManager.GetActionMapping();
    return am.IsActionTriggered(actionName) ? 1 : 0;
}

float GetActionAxis(const char* axisName)
{
    auto& ctx = Ctx::Instance();
    auto& am = ctx.inputManager.GetActionMapping();
    return am.GetActionValue(axisName);
}
