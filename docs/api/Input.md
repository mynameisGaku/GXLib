# Input API リファレンス

名前空間: `GX`。キーボード、マウス、ゲームパッド、アクションマッピングの入力管理。

## Keyboard

Win32 仮想キーコード (VK_*) ベースのキーボード入力管理。

| メソッド | 説明 |
|---|---|
| `void Initialize()` | 全キー状態を初期化する |
| `void Update()` | フレーム更新。前フレーム状態を保存し、現在状態を反映する |
| `bool ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)` | Win32 メッセージを処理する |
| `bool IsKeyDown(int key)` | キーが押されているか (押下中 = true) |
| `bool IsKeyTriggered(int key)` | このフレームで押されたか (トリガー判定) |
| `bool IsKeyReleased(int key)` | このフレームで離されたか |

### 使用例

```cpp
auto& kb = inputManager.GetKeyboard();
if (kb.IsKeyTriggered(VK_SPACE)) {
    // ジャンプ処理
}
if (kb.IsKeyDown(VK_LEFT)) {
    // 左移動
}
```

**注意**: DXLib 互換の `CheckHitKey()` は押下判定のみ。トリガー判定には `IsKeyTriggered()` を使う。

## Mouse

Win32 メッセージベースのマウス入力管理。

### ボタン定数 (`MouseButton` 名前空間)

| 定数 | 値 | 説明 |
|---|---|---|
| `MouseButton::Left` | 0 | 左ボタン |
| `MouseButton::Right` | 1 | 右ボタン |
| `MouseButton::Middle` | 2 | 中ボタン |

### メソッド

| メソッド | 戻り値 | 説明 |
|---|---|---|
| `GetX()` / `GetY()` | `int` | マウスの X/Y 座標 (クライアント領域基準) |
| `GetDeltaX()` / `GetDeltaY()` | `int` | 前フレームからの移動量 |
| `GetWheel()` | `int` | ホイール回転量 (正=上方向) |
| `IsButtonDown(int button)` | `bool` | ボタンが押されているか |
| `IsButtonTriggered(int button)` | `bool` | このフレームで押されたか |
| `IsButtonReleased(int button)` | `bool` | このフレームで離されたか |

### 使用例

```cpp
auto& mouse = inputManager.GetMouse();
if (mouse.IsButtonTriggered(MouseButton::Left)) {
    int x = mouse.GetX();
    int y = mouse.GetY();
    // クリック処理
}
```

## Gamepad

XInput 対応ゲームパッド管理。最大4台同時対応。デッドゾーン処理済み。

### ボタン定数 (`PadButton` 名前空間)

| 定数 | 説明 |
|---|---|
| `PadButton::A / B / X / Y` | フェイスボタン |
| `PadButton::DPadUp / Down / Left / Right` | 十字キー |
| `PadButton::LeftShoulder / RightShoulder` | LB / RB |
| `PadButton::Start / Back` | Start / Back |
| `PadButton::LeftThumb / RightThumb` | スティック押し込み |

### メソッド

| メソッド | 説明 |
|---|---|
| `bool IsConnected(int pad)` | パッドが接続されているか (pad: 0-3) |
| `bool IsButtonDown(int pad, int button)` | ボタンが押されているか |
| `bool IsButtonTriggered(int pad, int button)` | このフレームで押されたか |
| `bool IsButtonReleased(int pad, int button)` | このフレームで離されたか |
| `float GetLeftStickX/Y(int pad)` | 左スティック (-1.0 - 1.0) |
| `float GetRightStickX/Y(int pad)` | 右スティック (-1.0 - 1.0) |
| `float GetLeftTrigger(int pad)` | 左トリガー (0.0 - 1.0) |
| `float GetRightTrigger(int pad)` | 右トリガー (0.0 - 1.0) |

### 使用例

```cpp
auto& pad = inputManager.GetGamepad();
if (pad.IsConnected(0)) {
    float moveX = pad.GetLeftStickX(0);
    if (pad.IsButtonTriggered(0, PadButton::A)) { /* ジャンプ */ }
}
```

## ActionMapping

論理アクション名と物理入力の対応を管理する入力抽象化レイヤー。

### InputBinding ファクトリ

| 静的メソッド | 説明 |
|---|---|
| `InputBinding::Key(int vk)` | キーボードキーバインド |
| `InputBinding::KeyAxis(int vk, float scale)` | キーを軸として使用 (scale で方向指定) |
| `InputBinding::MouseBtn(int btn)` | マウスボタンバインド |
| `InputBinding::PadBtn(int btn, int pad=0)` | ゲームパッドボタンバインド |
| `InputBinding::PadAxis(GamepadAxisId axis, float s, float dz, int pad)` | ゲームパッド軸バインド |

### ActionMapping メソッド

| メソッド | 説明 |
|---|---|
| `void DefineAction(name, bindings)` | アクションを定義する。複数バインディング可 |
| `void RemoveAction(name)` | アクションを削除する |
| `void Update(keyboard, mouse, gamepad)` | 全アクション状態を更新する (毎フレーム呼ぶ) |
| `bool IsActionPressed(name)` | アクションが押されているか |
| `bool IsActionTriggered(name)` | このフレームで押されたか |
| `bool IsActionReleased(name)` | このフレームで離されたか |
| `float GetActionValue(name)` | アナログ値 (-1.0 - 1.0) |
| `bool LoadFromFile(path)` | JSON からバインド設定を読み込む |
| `bool SaveToFile(path)` | JSON にバインド設定を保存する |

### 使用例

```cpp
ActionMapping actions;
actions.DefineAction("Jump", {
    InputBinding::Key(VK_SPACE),
    InputBinding::PadBtn(PadButton::A)
});
actions.DefineAction("MoveX", {
    InputBinding::KeyAxis(VK_RIGHT, 1.0f),
    InputBinding::KeyAxis(VK_LEFT, -1.0f),
    InputBinding::PadAxis(GamepadAxisId::LeftStickX)
});

// 毎フレーム
actions.Update(keyboard, mouse, gamepad);
if (actions.IsActionTriggered("Jump")) { /* ジャンプ */ }
float moveX = actions.GetActionValue("MoveX");
```
