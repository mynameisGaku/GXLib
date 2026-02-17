// Agent 5: Input + Audio (9 classes)
// InputManager, Keyboard, Mouse, Gamepad, AudioManager, AudioDevice, Sound, SoundPlayer, MusicPlayer
{

// ============================================================
// InputManager (Input/InputManager.h)
// ============================================================
'InputManager-Initialize': [
  'void Initialize(Window& window)',
  'InputManagerを初期化し、指定ウィンドウにメッセージコールバックを登録します。Keyboard、Mouse、Gamepadの各デバイスの初期化も同時に行います。アプリケーション起動時に一度だけ呼び出してください。Windowのメッセージループに入力処理を紐付けます。\\n\\n【用語】入力マネージャはキーボード・マウス・ゲームパッドを一括管理します。毎フレーム Update() を呼ぶだけで全デバイスの状態が更新されます。',
  '// InputManager初期化\nGX::InputManager inputManager;\nGX::Window window;\nwindow.Initialize(1280, 720, L"My Game");\ninputManager.Initialize(window);',
  '* Window初期化後に呼び出すこと\n* 内部でWindow::AddMessageCallback()を使用してWin32メッセージを受信する\n* 初期化は1度だけ行う（再初期化は非対応）'
],

'InputManager-Update': [
  'void Update()',
  '全入力デバイス（Keyboard、Mouse、Gamepad）のフレーム状態を更新します。前フレームの状態を保存し、新しいフレームの入力を確定します。メインループの先頭で毎フレーム呼び出す必要があります。',
  '// 毎フレーム入力更新\nwhile (running) {\n    inputManager.Update();\n    // ゲームロジック...\n}',
  '* 毎フレームの先頭で呼び出すこと\n* Keyboard::Update()、Mouse::Update()、Gamepad::Update()を内部で順に呼び出す\n* 呼び忘れるとトリガー・リリース判定が正しく動作しない'
],

'InputManager-Shutdown': [
  'void Shutdown()',
  '入力システムの終了処理を行います。各デバイスのリソースを解放します。アプリケーション終了時に呼び出してください。',
  '// 終了時\ninputManager.Shutdown();',
  '* アプリケーション終了時に一度呼び出す\n* Shutdown後は入力取得を行わないこと\n* Initializeとペアで使用する'
],

'InputManager-GetKeyboard': [
  'Keyboard& GetKeyboard()',
  'Keyboardオブジェクトの参照を返します。IsKeyDown、IsKeyTriggered、IsKeyReleasedなどキーボード固有のAPIに直接アクセスする場合に使用します。DXLib互換のCheckHitKeyよりも詳細な判定が可能です。',
  '// Keyboardを直接使用\nGX::Keyboard& kb = inputManager.GetKeyboard();\nif (kb.IsKeyTriggered(VK_SPACE)) {\n    player.Jump();\n}',
  '* 返される参照はInputManagerが内部で保持するインスタンスのもの\n* IsKeyTriggered()で押した瞬間のみ検知可能（CheckHitKeyにはトリガー機能がない）\n* VK_*仮想キーコードを使用する'
],

'InputManager-GetMouse': [
  'Mouse& GetMouse()',
  'Mouseオブジェクトの参照を返します。ボタンのトリガー判定やデルタ移動量など、DXLib互換APIでは提供されない詳細なマウス情報にアクセスできます。',
  '// Mouse直接アクセス\nGX::Mouse& mouse = inputManager.GetMouse();\nif (mouse.IsButtonTriggered(GX::MouseButton::Left)) {\n    OnClick(mouse.GetX(), mouse.GetY());\n}',
  '* MouseButton::Left(0)、Right(1)、Middle(2)の3ボタン対応\n* GetDeltaX/Y()でFPSカメラのマウスルック実装に使える\n* ホイール値はフレームごとにリセットされる'
],

'InputManager-GetGamepad': [
  'Gamepad& GetGamepad()',
  'Gamepadオブジェクトの参照を返します。XInput対応ゲームパッドのボタン、スティック、トリガーに直接アクセスできます。最大4台のコントローラを同時に管理します。',
  '// Gamepad直接アクセス\nGX::Gamepad& pad = inputManager.GetGamepad();\nif (pad.IsConnected(0)) {\n    float lx = pad.GetLeftStickX(0);\n    float ly = pad.GetLeftStickY(0);\n    player.Move(lx, ly);\n}',
  '* pad引数は0-3（最大4台）\n* IsConnected()で接続確認してから使用すること\n* スティック値はデッドゾーン適用済み'
],

'InputManager-CheckHitKey': [
  'int CheckHitKey(int keyCode) const',
  'DXLib互換のキー押下判定関数です。指定キーが押されていれば1、離されていれば0を返します。DXLibとの互換性のためにint型を返しますが、内部ではKeyboard::IsKeyDown()を使用します。keyCodeにはDXLib形式のDIKキーコードを指定します。',
  '// DXLib互換スタイル\nif (inputManager.CheckHitKey(DIK_Z)) {\n    player.Attack();\n}\nif (inputManager.CheckHitKey(DIK_ESCAPE)) {\n    running = false;\n}',
  '* DIK_*（DirectInputキーコード）をVK_*に内部変換する256エントリのテーブルを使用\n* トリガー判定（押した瞬間のみ）が必要な場合はGetKeyboard().IsKeyTriggered()を使うこと\n* 戻り値は0か1のみ'
],

'InputManager-GetMouseInput': [
  'int GetMouseInput() const',
  'DXLib互換のマウスボタン状態取得関数です。左ボタン=1、右ボタン=2、中ボタン=4のビットフラグで現在押されているボタンを返します。複数ボタン同時押しはORされた値になります。',
  '// DXLib互換マウスボタン判定\nint btn = inputManager.GetMouseInput();\nif (btn & 1) {  // 左ボタン\n    player.Shoot();\n}\nif (btn & 2) {  // 右ボタン\n    player.Guard();\n}',
  '* MOUSE_INPUT_LEFT=1, MOUSE_INPUT_RIGHT=2, MOUSE_INPUT_MIDDLE=4\n* トリガー判定が必要な場合はGetMouse().IsButtonTriggered()を使うこと\n* 複数ボタン同時押しではビットOR値が返る'
],

'InputManager-GetMousePoint': [
  'void GetMousePoint(int* x, int* y) const',
  'DXLib互換のマウス座標取得関数です。クライアント領域座標でのマウスカーソル位置をポインタ経由で返します。GUIのヒットテストや2Dゲームのカーソル追従に使用します。',
  '// DXLib互換マウス座標\nint mx, my;\ninputManager.GetMousePoint(&mx, &my);\nspriteBatch.Draw(cursorTexture, (float)mx, (float)my);',
  '* x、yともにnullptrを渡すと該当値をスキップする\n* 座標はウィンドウのクライアント領域基準（左上が原点）\n* GUI Scalingが有効な場合はデザイン座標への変換が別途必要'
],

'InputManager-GetMouseWheel': [
  'int GetMouseWheel() const',
  'マウスホイールの回転量を取得します。正の値は上方向（奥へ）、負の値は下方向（手前へ）の回転を示します。値はフレーム単位でリセットされるため、蓄積処理は不要です。',
  '// ホイールでズーム\nint wheel = inputManager.GetMouseWheel();\nif (wheel != 0) {\n    camera.Zoom(wheel > 0 ? 0.9f : 1.1f);\n}',
  '* 値はMouse::Update()でフレームごとにリセットされる\n* WM_MOUSEWHEELメッセージから取得（WHEEL_DELTA=120単位）\n* 高速スクロール時は1フレーム内に複数回分が蓄積される'
],

// ============================================================
// Keyboard (Input/Keyboard.h)
// ============================================================
'Keyboard-Initialize': [
  'void Initialize()',
  'キーボード状態を初期化し、全256キーの状態をリセットします。InputManager::Initialize()から内部的に呼ばれます。全キーを非押下状態にクリアします。\\n\\n【用語】VK_* は Windows の仮想キーコード (Virtual Key codes) です。VK_SPACE, VK_RETURN, VK_ESCAPE 等でキーを指定します。',
  '// 通常はInputManager経由で初期化\nGX::Keyboard keyboard;\nkeyboard.Initialize();',
  '* 通常はInputManager経由で呼ばれるため直接呼ぶ必要はない\n* 256キー分のbool配列を全てfalseに初期化する\n* 再初期化すると入力状態が全てリセットされる'
],

'Keyboard-Update': [
  'void Update()',
  'フレーム更新を行い、前フレームのキー状態を保存してから現在フレームの状態を確定します。トリガーとリリースの判定はこの更新に依存します。InputManager::Update()から内部的に呼ばれます。',
  '// 通常はInputManager経由で更新\nkeyboard.Update();\n// この後 IsKeyTriggered/IsKeyReleased が正しく判定できる',
  '* 毎フレーム必ず呼ぶこと（省略するとトリガー判定が破綻する）\n* m_previousState = m_currentState, m_currentState = m_rawState の順で更新\n* rawStateはWin32メッセージで随時更新されている'
],

'Keyboard-ProcessMessage': [
  'bool ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)',
  'Win32のWM_KEYDOWN/WM_KEYUPメッセージを処理し、内部のキー状態配列を更新します。InputManagerがWindowのメッセージコールバックに登録する関数から呼ばれます。処理したメッセージであればtrueを返します。',
  '// Windowコールバック内で使用（通常は自動登録）\n// InputManager::Initialize内部:\n// window.AddMessageCallback([&](HWND, UINT msg, WPARAM w, LPARAM l) {\n//     m_keyboard.ProcessMessage(msg, w, l);\n// });',
  '* WM_KEYDOWNとWM_KEYUPのみを処理する\n* wParamから仮想キーコード（VK_*）を取得してm_rawStateを更新\n* 通常はInputManager経由で自動的にコールバック登録される'
],

'Keyboard-IsKeyDown': [
  'bool IsKeyDown(int key) const',
  '指定キーが現在押されているかどうかを返します。キーを押し続けている間ずっとtrueを返します。移動入力など継続的な入力に適しています。keyにはVK_*仮想キーコードを指定します。',
  '// 移動入力（押し続け判定）\nif (keyboard.IsKeyDown(VK_LEFT))  player.MoveLeft();\nif (keyboard.IsKeyDown(VK_RIGHT)) player.MoveRight();\nif (keyboard.IsKeyDown(VK_UP))    player.MoveUp();\nif (keyboard.IsKeyDown(VK_DOWN))  player.MoveDown();',
  '* 継続入力（移動、長押し等）に使用する\n* key範囲は0-255（k_KeyCount=256）\n* 範囲外のkey値に対する安全チェックは実装依存'
],

'Keyboard-IsKeyTriggered': [
  'bool IsKeyTriggered(int key) const',
  '指定キーが今フレームで押された瞬間かどうかを返します。前フレームで非押下かつ今フレームで押下の場合のみtrueを返します。メニュー選択やジャンプなど単発入力に最適です。',
  '// 単発入力（押した瞬間のみ）\nif (keyboard.IsKeyTriggered(VK_SPACE)) {\n    player.Jump();\n}\nif (keyboard.IsKeyTriggered(VK_RETURN)) {\n    menu.Confirm();\n}',
  '* !m_previousState[key] && m_currentState[key] で判定\n* Update()呼び出し後に正しく動作する\n* InputManager::CheckHitKey()にはトリガー機能がないため、単発入力にはこちらを使う'
],

'Keyboard-IsKeyReleased': [
  'bool IsKeyReleased(int key) const',
  '指定キーが今フレームで離された瞬間かどうかを返します。前フレームで押下かつ今フレームで非押下の場合のみtrueを返します。ボタンを離した時の処理や溜め攻撃の解放に使用します。',
  '// 離した瞬間の判定\nif (keyboard.IsKeyReleased(VK_SPACE)) {\n    player.ReleaseCharge();\n}',
  '* m_previousState[key] && !m_currentState[key] で判定\n* 溜め攻撃やチャージショットの実装に使える\n* フォーカス喪失時にはWM_KILLFOCUSで全キーがリリース扱いになる場合がある'
],

// ============================================================
// Mouse (Input/Mouse.h)
// ============================================================
'Mouse-MouseButtonLeft': [
  'constexpr int MouseButton::Left = 0',
  'マウス左ボタンのインデックス定数です。IsButtonDown、IsButtonTriggered、IsButtonReleasedの引数に使用します。一般的にメインの操作ボタンとして使われます。',
  '// 左ボタン判定\nif (mouse.IsButtonDown(GX::MouseButton::Left)) {\n    player.Shoot();\n}',
  '* 値は0\n* IsButtonDown/Triggered/Releasedの引数として使用\n* DXLib互換のGetMouseInput()では左ボタン=ビット1(0x01)に対応'
],

'Mouse-MouseButtonRight': [
  'constexpr int MouseButton::Right = 1',
  'マウス右ボタンのインデックス定数です。コンテキストメニューやサブアクション（ガード、スコープ等）に使用されることが多いボタンです。',
  '// 右ボタン判定\nif (mouse.IsButtonTriggered(GX::MouseButton::Right)) {\n    contextMenu.Show(mouse.GetX(), mouse.GetY());\n}',
  '* 値は1\n* DXLib互換のGetMouseInput()では右ボタン=ビット2(0x02)に対応\n* 右クリックメニューやサブアクション用'
],

'Mouse-MouseButtonMiddle': [
  'constexpr int MouseButton::Middle = 2',
  'マウス中ボタン（ホイールクリック）のインデックス定数です。3Dビューのパン操作やエディタ操作に使用されることが多いボタンです。',
  '// 中ボタンでパン操作\nif (mouse.IsButtonDown(GX::MouseButton::Middle)) {\n    camera.Pan(mouse.GetDeltaX(), mouse.GetDeltaY());\n}',
  '* 値は2\n* DXLib互換のGetMouseInput()では中ボタン=ビット4(0x04)に対応\n* ホイールクリックとホイール回転は別の入力として処理される'
],

'Mouse-GetX': [
  'int GetX() const',
  'マウスカーソルのX座標（クライアント領域基準）を返します。ウィンドウの左上を原点(0,0)としたピクセル座標です。UIヒットテストやカーソル描画に使用します。',
  '// カーソル座標取得\nint x = mouse.GetX();\nint y = mouse.GetY();\nspriteBatch.Draw(cursorTex, (float)x, (float)y);',
  '* ウィンドウのクライアント領域基準の座標\n* WM_MOUSEMOVEメッセージで更新される\n* ウィンドウ外にカーソルがある場合は最後にクライアント領域内にいた座標が返る'
],

'Mouse-GetY': [
  'int GetY() const',
  'マウスカーソルのY座標（クライアント領域基準）を返します。ウィンドウの左上を原点(0,0)としたピクセル座標です。Y軸は下方向が正です。',
  '// Y座標を画面下部判定に使用\nint y = mouse.GetY();\nif (y > screenHeight - 50) {\n    ShowToolbar();\n}',
  '* Y軸は画面上→下が正方向\n* ウィンドウリサイズ時もクライアント領域基準で自動追従\n* GetX()と合わせて2Dでのヒットテストに使用する'
],

'Mouse-GetDeltaX': [
  'int GetDeltaX() const',
  '前フレームからのマウスX軸移動量を返します。正の値は右方向、負の値は左方向を示します。FPSカメラのマウスルックやドラッグ操作の実装に使用します。',
  '// FPSカメラ回転\nfloat sensitivity = 0.005f;\ncamera.RotateYaw(mouse.GetDeltaX() * sensitivity);\ncamera.RotatePitch(mouse.GetDeltaY() * sensitivity);',
  '* 計算式: m_x - m_prevX\n* Update()後に前フレーム座標が保存されてから算出される\n* 初回フレームや急激なカーソル移動で大きな値になることがある'
],

'Mouse-GetDeltaY': [
  'int GetDeltaY() const',
  '前フレームからのマウスY軸移動量を返します。正の値は下方向、負の値は上方向を示します。FPSカメラのピッチ操作に使用します。',
  '// マウスドラッグ処理\nif (mouse.IsButtonDown(GX::MouseButton::Left)) {\n    dragOffset.x += mouse.GetDeltaX();\n    dragOffset.y += mouse.GetDeltaY();\n}',
  '* 計算式: m_y - m_prevY\n* Y軸は画面上→下が正（スクリーン座標系）\n* FPSカメラのピッチでは符号を反転させる場合がある'
],

'Mouse-GetWheel': [
  'int GetWheel() const',
  'マウスホイールの回転量を返します。正の値は上方向（奥へ）、負の値は下方向（手前へ）の回転です。ズーム操作やスクロール操作に使用します。フレームごとにリセットされます。',
  '// ホイールでスクロール\nint w = mouse.GetWheel();\nif (w > 0) scrollView.ScrollUp();\nif (w < 0) scrollView.ScrollDown();',
  '* WM_MOUSEWHEELから取得（WHEEL_DELTA=120単位で正規化）\n* 同一フレーム内の複数回転は蓄積される\n* Update()でフレームごとにリセットされる'
],

'Mouse-IsButtonDown': [
  'bool IsButtonDown(int button) const',
  '指定マウスボタンが現在押されているかを返します。ドラッグ操作や長押し操作など、ボタンを押し続けている間の判定に使用します。buttonにはMouseButton::Left/Right/Middleを指定します。',
  '// ドラッグ操作\nif (mouse.IsButtonDown(GX::MouseButton::Left)) {\n    selection.Resize(mouse.GetX(), mouse.GetY());\n}',
  '* MouseButton::Left(0)、Right(1)、Middle(2)を使用\n* 範囲外のbutton値はfalseを返す（Count=3）\n* ドラッグ＆ドロップやペイント操作に最適'
],

'Mouse-IsButtonTriggered': [
  'bool IsButtonTriggered(int button) const',
  '指定マウスボタンが今フレームで押された瞬間かどうかを返します。UIボタンのクリック判定やメニュー選択など、単発のクリック操作に使用します。',
  '// クリック検出\nif (mouse.IsButtonTriggered(GX::MouseButton::Left)) {\n    if (button.Contains(mouse.GetX(), mouse.GetY())) {\n        button.OnClick();\n    }\n}',
  '* !m_previousButtons[btn] && m_currentButtons[btn] で判定\n* UIのクリック処理にはIsButtonDown()ではなくこちらを使うこと\n* ダブルクリック検出は自前で時間差を計測する必要がある'
],

'Mouse-IsButtonReleased': [
  'bool IsButtonReleased(int button) const',
  '指定マウスボタンが今フレームで離された瞬間かどうかを返します。ドラッグ操作の終了判定やボタンのアップイベントに使用します。',
  '// ドラッグ終了\nif (mouse.IsButtonReleased(GX::MouseButton::Left)) {\n    dragState.End();\n    ConfirmPlacement();\n}',
  '* m_previousButtons[btn] && !m_currentButtons[btn] で判定\n* ドラッグ＆ドロップのドロップ処理に最適\n* ウィンドウ外でボタンを離した場合はキャプチャ状態に依存する'
],

// ============================================================
// Gamepad (Input/Gamepad.h)
// ============================================================
'Gamepad-PadButtonDPadUp': [
  'constexpr int PadButton::DPadUp = XINPUT_GAMEPAD_DPAD_UP',
  '十字キー上方向のボタン定数です。メニューのカーソル移動やキャラクターの方向入力に使用します。XInputのビットマスク値と同一です。',
  '// 十字キーでメニュー操作\nif (gamepad.IsButtonTriggered(0, GX::PadButton::DPadUp)) {\n    menu.MoveUp();\n}',
  '* XINPUT_GAMEPAD_DPAD_UP (0x0001) と同値\n* IsButtonDown/Triggered/Releasedの引数として使用\n* 左スティックとは独立した入力'
],

'Gamepad-PadButtonDPadDown': [
  'constexpr int PadButton::DPadDown = XINPUT_GAMEPAD_DPAD_DOWN',
  '十字キー下方向のボタン定数です。メニューの下移動やカメラのピッチ操作に使用します。',
  '// メニュー下移動\nif (gamepad.IsButtonTriggered(0, GX::PadButton::DPadDown)) {\n    menu.MoveDown();\n}',
  '* XINPUT_GAMEPAD_DPAD_DOWN (0x0002) と同値\n* 十字キーの上下左右は同時押し可能\n* デジタル入力のみ（アナログ値なし）'
],

'Gamepad-PadButtonDPadLeft': [
  'constexpr int PadButton::DPadLeft = XINPUT_GAMEPAD_DPAD_LEFT',
  '十字キー左方向のボタン定数です。メニューのページ切り替えやアイテム選択に使用します。',
  '// 十字キー左右でページ切替\nif (gamepad.IsButtonTriggered(0, GX::PadButton::DPadLeft)) {\n    inventory.PreviousPage();\n}',
  '* XINPUT_GAMEPAD_DPAD_LEFT (0x0004) と同値\n* 斜め入力はDPadUp+DPadLeftなど組み合わせで表現\n* デジタル入力のみ'
],

'Gamepad-PadButtonDPadRight': [
  'constexpr int PadButton::DPadRight = XINPUT_GAMEPAD_DPAD_RIGHT',
  '十字キー右方向のボタン定数です。メニューのページ切り替えやアイテム選択に使用します。',
  '// 十字キー右でページ切替\nif (gamepad.IsButtonTriggered(0, GX::PadButton::DPadRight)) {\n    inventory.NextPage();\n}',
  '* XINPUT_GAMEPAD_DPAD_RIGHT (0x0008) と同値\n* 十字キーは4方向＋斜め4方向の入力が可能\n* デジタル入力のみ'
],

'Gamepad-PadButtonStart': [
  'constexpr int PadButton::Start = XINPUT_GAMEPAD_START',
  'Startボタンの定数です。ゲームのポーズやメニュー表示のトグルに使用されるのが一般的です。',
  '// Startボタンでポーズ\nif (gamepad.IsButtonTriggered(0, GX::PadButton::Start)) {\n    game.TogglePause();\n}',
  '* XINPUT_GAMEPAD_START (0x0010) と同値\n* ポーズメニューの表示切替に使われることが多い\n* IsButtonTriggered()で単発検出すること'
],

'Gamepad-PadButtonBack': [
  'constexpr int PadButton::Back = XINPUT_GAMEPAD_BACK',
  'Backボタン（Selectボタン）の定数です。メニューの戻る操作やマップ表示などに使用されます。',
  '// Backボタンでマップ表示\nif (gamepad.IsButtonTriggered(0, GX::PadButton::Back)) {\n    game.ToggleMap();\n}',
  '* XINPUT_GAMEPAD_BACK (0x0020) と同値\n* Xbox OneコントローラではViewボタンに相当\n* ゲーム終了やキャンセル操作にも使用される'
],

'Gamepad-PadButtonLeftThumb': [
  'constexpr int PadButton::LeftThumb = XINPUT_GAMEPAD_LEFT_THUMB',
  '左スティック押し込み（L3）ボタンの定数です。ダッシュ切替やクラウチ操作に使われることが多いボタンです。',
  '// L3でダッシュ切替\nif (gamepad.IsButtonTriggered(0, GX::PadButton::LeftThumb)) {\n    player.ToggleSprint();\n}',
  '* XINPUT_GAMEPAD_LEFT_THUMB (0x0040) と同値\n* スティックを押し込む操作は誤入力が起きやすい\n* FPSゲームではダッシュやステルスに割り当てられることが多い'
],

'Gamepad-PadButtonRightThumb': [
  'constexpr int PadButton::RightThumb = XINPUT_GAMEPAD_RIGHT_THUMB',
  '右スティック押し込み（R3）ボタンの定数です。カメラリセットやロックオン操作に使われることが多いボタンです。',
  '// R3でカメラリセット\nif (gamepad.IsButtonTriggered(0, GX::PadButton::RightThumb)) {\n    camera.ResetToDefault();\n}',
  '* XINPUT_GAMEPAD_RIGHT_THUMB (0x0080) と同値\n* スティック操作中の押し込みは操作難度が高い\n* カメラリセットやターゲットロックに使用されることが多い'
],

'Gamepad-PadButtonLeftShoulder': [
  'constexpr int PadButton::LeftShoulder = XINPUT_GAMEPAD_LEFT_SHOULDER',
  '左ショルダーボタン（LB）の定数です。武器切り替えやガード操作に使用されることが多いデジタルボタンです。',
  '// LBで武器切替\nif (gamepad.IsButtonTriggered(0, GX::PadButton::LeftShoulder)) {\n    player.PreviousWeapon();\n}',
  '* XINPUT_GAMEPAD_LEFT_SHOULDER (0x0100) と同値\n* デジタルボタン（アナログ入力はLeftTriggerで取得）\n* アクションゲームではガードやターゲット切替に使用'
],

'Gamepad-PadButtonRightShoulder': [
  'constexpr int PadButton::RightShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER',
  '右ショルダーボタン（RB）の定数です。武器切り替えや特殊アクションに使用されることが多いデジタルボタンです。',
  '// RBで武器切替\nif (gamepad.IsButtonTriggered(0, GX::PadButton::RightShoulder)) {\n    player.NextWeapon();\n}',
  '* XINPUT_GAMEPAD_RIGHT_SHOULDER (0x0200) と同値\n* デジタルボタン（アナログ入力はRightTriggerで取得）\n* LB/RBの組み合わせでページ切替などにも使用'
],

'Gamepad-PadButtonA': [
  'constexpr int PadButton::A = XINPUT_GAMEPAD_A',
  'Aボタン（Xboxの緑ボタン）の定数です。決定、ジャンプ、アクション実行などメインの操作ボタンとして使用されます。',
  '// Aボタンでジャンプ\nif (gamepad.IsButtonTriggered(0, GX::PadButton::A)) {\n    player.Jump();\n}',
  '* XINPUT_GAMEPAD_A (0x1000) と同値\n* Xboxレイアウトで下のボタン\n* 多くのゲームで「決定」「ジャンプ」に割り当てられる'
],

'Gamepad-PadButtonB': [
  'constexpr int PadButton::B = XINPUT_GAMEPAD_B',
  'Bボタン（Xboxの赤ボタン）の定数です。キャンセルや回避操作に使用されることが一般的です。',
  '// Bボタンでキャンセル\nif (gamepad.IsButtonTriggered(0, GX::PadButton::B)) {\n    menu.Cancel();\n}',
  '* XINPUT_GAMEPAD_B (0x2000) と同値\n* Xboxレイアウトで右のボタン\n* 多くのゲームで「キャンセル」「戻る」に割り当てられる'
],

'Gamepad-PadButtonX': [
  'constexpr int PadButton::X = XINPUT_GAMEPAD_X',
  'Xボタン（Xboxの青ボタン）の定数です。サブアクションや武器使用などに割り当てられることが多いボタンです。',
  '// Xボタンでアイテム使用\nif (gamepad.IsButtonTriggered(0, GX::PadButton::X)) {\n    player.UseItem();\n}',
  '* XINPUT_GAMEPAD_X (0x4000) と同値\n* Xboxレイアウトで左のボタン\n* リロードやインタラクションに割り当てられることが多い'
],

'Gamepad-PadButtonY': [
  'constexpr int PadButton::Y = XINPUT_GAMEPAD_Y',
  'Yボタン（Xboxの黄ボタン）の定数です。武器切り替えや特殊能力の発動などに割り当てられることが多いボタンです。',
  '// Yボタンで武器切替\nif (gamepad.IsButtonTriggered(0, GX::PadButton::Y)) {\n    player.SwitchWeapon();\n}',
  '* XINPUT_GAMEPAD_Y (0x8000) と同値\n* Xboxレイアウトで上のボタン\n* 三角ボタン（PlayStation）に相当する位置'
],

'Gamepad-IsConnected': [
  'bool IsConnected(int pad) const',
  '指定パッドインデックスのコントローラが接続されているかを返します。パッド操作の前に必ず接続確認を行うことを推奨します。未接続時に他のAPI呼び出しを行うと無効な値が返ります。',
  '// 接続確認\nfor (int i = 0; i < GX::Gamepad::k_MaxPads; ++i) {\n    if (gamepad.IsConnected(i)) {\n        GX::Logger::Info(L"Pad %d connected", i);\n    }\n}',
  '* padは0-3の範囲（k_MaxPads=4）\n* XInputGetState()のERROR_DEVICE_NOT_CONNECTED結果に基づく\n* 接続状態はUpdate()ごとに更新される'
],

'Gamepad-IsButtonDown': [
  'bool IsButtonDown(int pad, int button) const',
  '指定パッドの指定ボタンが現在押されているかを返します。ボタンを押し続けている間trueを返します。buttonにはPadButton定数を使用します。',
  '// ボタン長押し判定\nif (gamepad.IsButtonDown(0, GX::PadButton::A)) {\n    player.ChargeJump();\n}',
  '* PadButton::*定数をbutton引数に使用\n* XINPUT_STATE::Gamepad.wButtonsとビットANDで判定\n* 未接続パッドでは常にfalse'
],

'Gamepad-IsButtonTriggered': [
  'bool IsButtonTriggered(int pad, int button) const',
  '指定パッドの指定ボタンが今フレームで押された瞬間かを返します。メニュー決定やジャンプなど単発入力に使用します。前フレーム非押下かつ今フレーム押下で判定されます。',
  '// メニュー決定\nif (gamepad.IsButtonTriggered(0, GX::PadButton::A)) {\n    menu.Select();\n}\nif (gamepad.IsButtonTriggered(0, GX::PadButton::B)) {\n    menu.Back();\n}',
  '* !(prevButtons & button) && (currentButtons & button) で判定\n* Update()後に正しく動作する\n* UI操作には必ずTriggeredを使うこと（Downだと連続入力になる）'
],

'Gamepad-IsButtonReleased': [
  'bool IsButtonReleased(int pad, int button) const',
  '指定パッドの指定ボタンが今フレームで離された瞬間かを返します。溜め攻撃の解放やチャージ操作の完了判定に使用します。',
  '// 溜め攻撃解放\nif (gamepad.IsButtonReleased(0, GX::PadButton::X)) {\n    player.ReleaseChargeAttack();\n}',
  '* (prevButtons & button) && !(currentButtons & button) で判定\n* 長押しからの解放タイミングを検出できる\n* 未接続パッドでは常にfalse'
],

'Gamepad-GetLeftStickX': [
  'float GetLeftStickX(int pad) const',
  '左スティックのX軸値を-1.0から1.0の範囲で返します。正の値は右方向、負の値は左方向です。デッドゾーン（k_StickDeadZone=0.24f）以下の入力は0として返されます。',
  '// 左スティックで移動\nfloat lx = gamepad.GetLeftStickX(0);\nfloat ly = gamepad.GetLeftStickY(0);\nplayer.Move(lx * speed, ly * speed);',
  '* デッドゾーン0.24f以下は0.0を返す\n* -32768〜32767のshort値を-1.0〜1.0にマッピング\n* 未接続パッドでは0.0を返す'
],

'Gamepad-GetLeftStickY': [
  'float GetLeftStickY(int pad) const',
  '左スティックのY軸値を-1.0から1.0の範囲で返します。正の値は上方向、負の値は下方向です。XInputのY軸は上が正であることに注意してください。',
  '// 左スティックY軸\nfloat ly = gamepad.GetLeftStickY(0);\nif (ly > 0.5f) {\n    player.RunForward();\n}',
  '* XInputのY軸は上が正（スクリーン座標系とは逆）\n* 2Dゲームでは符号を反転させる場合がある\n* デッドゾーン0.24f以下は0.0を返す'
],

'Gamepad-GetRightStickX': [
  'float GetRightStickX(int pad) const',
  '右スティックのX軸値を-1.0から1.0の範囲で返します。正の値は右方向、負の値は左方向です。3Dゲームのカメラ水平回転（ヨー）に主に使用します。',
  '// 右スティックでカメラ回転\nfloat rx = gamepad.GetRightStickX(0);\nfloat ry = gamepad.GetRightStickY(0);\ncamera.Rotate(rx * rotSpeed, ry * rotSpeed);',
  '* デッドゾーン0.24f以下は0.0を返す\n* FPSやTPSカメラのヨー操作に使用\n* 感度は乗算で調整する'
],

'Gamepad-GetRightStickY': [
  'float GetRightStickY(int pad) const',
  '右スティックのY軸値を-1.0から1.0の範囲で返します。正の値は上方向、負の値は下方向です。3Dゲームのカメラ垂直回転（ピッチ）に主に使用します。',
  '// 右スティックでカメラピッチ\nfloat ry = gamepad.GetRightStickY(0);\ncamera.AdjustPitch(-ry * sensitivity);  // Y反転',
  '* XInputのY軸は上が正\n* カメラピッチでは符号反転（invert Y）がオプションとして一般的\n* デッドゾーン0.24f以下は0.0を返す'
],

'Gamepad-GetLeftTrigger': [
  'float GetLeftTrigger(int pad) const',
  '左トリガー（LT）のアナログ値を0.0から1.0の範囲で返します。ブレーキ操作やスコープズーム段階など、アナログ量が必要な入力に使用します。デッドゾーン（k_TriggerDeadZone=0.12f）以下は0を返します。',
  '// LTでブレーキ\nfloat lt = gamepad.GetLeftTrigger(0);\ncar.ApplyBrake(lt);',
  '* デッドゾーン0.12f以下は0.0を返す\n* 0-255のBYTE値を0.0-1.0にマッピング\n* レースゲームのブレーキや照準スコープに使用'
],

'Gamepad-GetRightTrigger': [
  'float GetRightTrigger(int pad) const',
  '右トリガー（RT）のアナログ値を0.0から1.0の範囲で返します。アクセル操作やシュート強度など、連続的なアナログ入力が必要な場面に使用します。デッドゾーン（k_TriggerDeadZone=0.12f）以下は0を返します。',
  '// RTでアクセル\nfloat rt = gamepad.GetRightTrigger(0);\ncar.Accelerate(rt);\n// またはシュート\nif (rt > 0.8f) player.Shoot();',
  '* デッドゾーン0.12f以下は0.0を返す\n* 0-255のBYTE値を0.0-1.0にマッピング\n* レースゲームのアクセルやFPSのトリガー操作に使用'
],

// ============================================================
// AudioManager (Audio/AudioManager.h)
// ============================================================
'AudioManager-Initialize': [
  'bool Initialize()',
  'オーディオシステム全体を初期化します。内部でXAudio2エンジンの作成、マスタリングボイスの設定、SoundPlayerとMusicPlayerの初期化を行います。失敗時はfalseを返します。アプリケーション起動時に一度呼び出してください。\\n\\n【用語】XAudio2 は Windows の低遅延オーディオ API です。効果音 (SE) の同時再生や BGM のストリーミングに使用します。',
  '// オーディオ初期化\nGX::AudioManager audioManager;\nif (!audioManager.Initialize()) {\n    GX::Logger::Error(L"Audio init failed!");\n    return false;\n}',
  '* 内部でAudioDevice::Initialize()を呼び出す\n* XAudio2のCOM初期化も行う\n* 失敗時はオーディオ機能が使用不可（ゲーム自体は続行可能）'
],

'AudioManager-LoadSound': [
  'int LoadSound(const std::wstring& filePath)',
  'WAVファイルを読み込み、管理ハンドル（int）を返します。同一パスを複数回読み込んだ場合はキャッシュされた既存ハンドルを返します。戻り値のハンドルをPlaySound/PlayMusicに渡して再生します。',
  '// サウンド読み込み\nint bgm = audioManager.LoadSound(L"Assets/audio/bgm.wav");\nint se_jump = audioManager.LoadSound(L"Assets/audio/jump.wav");\nint se_shot = audioManager.LoadSound(L"Assets/audio/shot.wav");',
  '* WAV形式のみ対応（PCMデータ）\n* 同一パスの再読み込みはキャッシュヒットする（m_pathCache）\n* 最大k_MaxSounds=256個まで管理可能\n* 失敗時は-1を返す'
],

'AudioManager-PlaySound': [
  'void PlaySound(int handle, float volume = 1.0f, float pan = 0.0f)',
  'SE（効果音）を再生します。同じhandleを複数回呼び出すと重ねて再生されます（ポリフォニック）。volumeは0.0-1.0、panは-1.0（左）から1.0（右）です。短い効果音の再生に使用します。',
  '// SE再生\naudioManager.PlaySound(se_jump);           // デフォルト音量\naudioManager.PlaySound(se_shot, 0.8f);     // 音量80%\naudioManager.PlaySound(se_hit, 0.5f, -0.5f); // 左寄りパン',
  '* 同時再生数に理論的制限はないが、SourceVoiceが増えるとCPU負荷が上がる\n* 再生完了後はCleanupFinishedVoices()で自動回収される\n* 無効なhandleを渡した場合は何も起きない'
],

'AudioManager-PlayMusic': [
  'void PlayMusic(int handle, bool loop = true, float volume = 1.0f)',
  'BGM（背景音楽）を再生します。同時に再生できるBGMは1曲のみで、新しいBGMを再生すると前のBGMは自動的に停止します。loop=trueでループ再生、falseで1回再生です。',
  '// BGM再生\naudioManager.PlayMusic(bgm_title, true, 0.7f);\n// 場面転換\naudioManager.PlayMusic(bgm_battle, true, 0.8f);  // 前のBGMは自動停止',
  '* BGMは常に1曲のみ再生（前のBGMは自動停止）\n* loop=trueでXAUDIO2_LOOP_INFINITEを使用\n* フェードイン/アウトと組み合わせて自然な曲切替が可能'
],

'AudioManager-StopMusic': [
  'void StopMusic()',
  '現在再生中のBGMを即座に停止します。フェードアウトなしの即時停止です。自然な停止が必要な場合はFadeOutMusic()を先に呼んでください。',
  '// BGM即時停止\naudioManager.StopMusic();\n// フェードアウト後に停止する場合\naudioManager.FadeOutMusic(2.0f);  // 2秒かけてフェードアウト＆自動停止',
  '* 即時停止（ポップノイズの可能性あり）\n* 再生していない場合は何もしない\n* フェードアウト中に呼ぶとフェードをキャンセルして即停止'
],

'AudioManager-PauseMusic': [
  'void PauseMusic()',
  '現在再生中のBGMを一時停止します。ResumeMusic()で再開できます。ポーズ画面への遷移時などに使用します。一時停止中はBGM位置が保持されます。',
  '// ポーズ処理\nvoid OnPause() {\n    audioManager.PauseMusic();\n    ShowPauseMenu();\n}',
  '* 再生位置が保持される\n* ResumeMusic()で続きから再開できる\n* 再生していない場合は何もしない'
],

'AudioManager-ResumeMusic': [
  'void ResumeMusic()',
  '一時停止中のBGMを再開します。PauseMusic()で停止した位置から再生を続けます。ポーズ解除時に使用します。',
  '// ポーズ解除\nvoid OnResume() {\n    audioManager.ResumeMusic();\n    HidePauseMenu();\n}',
  '* PauseMusic()後に呼び出すこと\n* 一時停止していない場合は何もしない\n* フェード状態もそのまま復帰する'
],

'AudioManager-FadeInMusic': [
  'void FadeInMusic(float seconds)',
  'BGMのフェードインを開始します。指定秒数をかけて音量を0からターゲット音量まで徐々に上げます。シーン開始時の自然なBGM開始に使用します。Update()をフレームごとに呼び出す必要があります。',
  '// 3秒かけてフェードイン\naudioManager.PlayMusic(bgm, true, 0.8f);\naudioManager.FadeInMusic(3.0f);',
  '* PlayMusic()後に呼び出すこと\n* Update(deltaTime)が毎フレーム呼ばれる必要がある\n* フェード中にStopMusic()で中断可能'
],

'AudioManager-FadeOutMusic': [
  'void FadeOutMusic(float seconds)',
  'BGMのフェードアウトを開始し、完了後に自動的に停止します。指定秒数をかけて音量を0まで下げます。シーン遷移時やゲームオーバー時のBGM終了に使用します。',
  '// 2秒フェードアウト後に新BGM\naudioManager.FadeOutMusic(2.0f);\n// Update()ループ内で完了を待ってから\nif (!audioManager.IsMusicPlaying()) {\n    audioManager.PlayMusic(nextBgm);\n}',
  '* フェード完了後に自動的にStop()が呼ばれる\n* m_stopAfterFade=trueで管理される\n* Update(deltaTime)が毎フレーム呼ばれる必要がある'
],

'AudioManager-IsMusicPlaying': [
  'bool IsMusicPlaying() const',
  'BGMが現在再生中かどうかを返します。一時停止中もtrueを返します。フェードアウト完了後の曲切替タイミング判定や、BGM状態の確認に使用します。',
  '// BGM状態確認\nif (!audioManager.IsMusicPlaying()) {\n    audioManager.PlayMusic(defaultBgm);\n}',
  '* 一時停止中（Pause中）もtrueを返す\n* Stop()後やフェードアウト完了後はfalseを返す\n* 内部でMusicPlayer::IsPlaying()に委譲'
],

'AudioManager-SetSoundVolume': [
  'void SetSoundVolume(int handle, float volume)',
  '指定ハンドルのサウンドの個別音量を設定します。0.0（無音）から1.0（最大）の範囲で指定します。SE音量のカテゴリ別調整などに使用します。',
  '// 個別音量設定\naudioManager.SetSoundVolume(se_footstep, 0.3f);\naudioManager.SetSoundVolume(se_explosion, 1.0f);',
  '* 0.0-1.0の範囲で指定\n* マスターボリュームとは独立して適用される\n* 無効なhandleを渡した場合は何もしない'
],

'AudioManager-SetMasterVolume': [
  'void SetMasterVolume(float volume)',
  'マスターボリュームを設定します。全てのSEおよびBGMに影響します。0.0（無音）から1.0（最大）の範囲で指定します。ゲームのオプション画面での全体音量調整に使用します。',
  '// オプション画面でのマスターボリューム\nvoid OnVolumeSliderChanged(float value) {\n    audioManager.SetMasterVolume(value);\n}',
  '* 全てのSE・BGMに影響する\n* 内部でAudioDevice::SetMasterVolume()を呼び出す\n* 個別音量×マスターボリュームが最終出力になる'
],

'AudioManager-Update': [
  'void Update(float deltaTime)',
  'オーディオシステムのフレーム更新を行います。BGMのフェードイン・フェードアウト処理を進行させます。SoundPlayerの完了済みVoiceの回収も行います。メインループで毎フレーム呼び出してください。',
  '// メインループ\nwhile (running) {\n    float dt = timer.GetDeltaTime();\n    audioManager.Update(dt);\n    // ゲーム更新...\n}',
  '* deltaTimeはフレーム経過秒数（Timer::GetDeltaTime()等）\n* フェード処理はdeltaTimeに基づいて進行する\n* CleanupFinishedVoices()も内部で呼ばれる'
],

'AudioManager-ReleaseSound': [
  'void ReleaseSound(int handle)',
  '指定ハンドルのサウンドリソースを解放します。解放後はそのハンドルは無効になり、再生に使用できなくなります。ハンドルはfreelistに回収され再利用されます。',
  '// サウンド解放\naudioManager.ReleaseSound(se_temp);\n// se_tempは以後使用不可',
  '* 解放されたハンドルは再利用される（freeList管理）\n* 再生中のサウンドを解放すると予期しない動作の可能性がある\n* Shutdown()時に全サウンドが自動解放される'
],

'AudioManager-Shutdown': [
  'void Shutdown()',
  'オーディオシステム全体を終了します。全てのサウンドリソースを解放し、XAudio2エンジンを破棄します。アプリケーション終了時に呼び出してください。',
  '// アプリ終了時\naudioManager.Shutdown();',
  '* 全サウンドリソースが自動解放される\n* MusicPlayer・SoundPlayerも内部で終了される\n* Shutdown後はオーディオAPIを呼び出さないこと'
],

// ============================================================
// AudioDevice (Audio/AudioDevice.h)
// ============================================================
'AudioDevice-Initialize': [
  'bool Initialize()',
  'XAudio2オーディオエンジンとマスタリングボイスを作成します。COM初期化（CoInitializeEx）も行います。AudioManager::Initialize()から内部的に呼ばれます。失敗時はfalseを返します。',
  '// 通常はAudioManager経由で使用\nGX::AudioDevice device;\nif (!device.Initialize()) {\n    // エラー処理\n}',
  '* XAudio2Create() + CreateMasteringVoice()を実行\n* COM初期化も含む（CoInitializeEx COINIT_MULTITHREADED）\n* 通常はAudioManager経由で使用し、直接操作は不要'
],

'AudioDevice-Shutdown': [
  'void Shutdown()',
  'XAudio2エンジンとマスタリングボイスを解放します。COM終了処理も行います。AudioManager::Shutdown()から内部的に呼ばれます。',
  '// 通常はAudioManager経由\ndevice.Shutdown();',
  '* MasterVoice→XAudio2→COM の順に解放される\n* デストラクタからも自動的に呼ばれる\n* 通常はAudioManager経由で使用し、直接操作は不要'
],

'AudioDevice-GetXAudio2': [
  'IXAudio2* GetXAudio2() const',
  'XAudio2エンジンの生ポインタを返します。SourceVoiceの作成やXAudio2固有の低レベル操作に使用します。通常はSoundPlayerやMusicPlayerが内部的に使用します。',
  '// 低レベルアクセス（通常は不要）\nIXAudio2* xaudio = device.GetXAudio2();\nIXAudio2SourceVoice* voice = nullptr;\nxaudio->CreateSourceVoice(&voice, &format);',
  '* ComPtr内部のポインタを直接返す\n* 所有権はAudioDeviceが保持（呼び出し側でReleaseしないこと）\n* Initialize()前はnullptrを返す'
],

'AudioDevice-GetMasterVoice': [
  'IXAudio2MasteringVoice* GetMasterVoice() const',
  'マスタリングボイスのポインタを返します。マスタリングボイスはオーディオ出力の最終段で、スピーカーに接続されます。カスタムオーディオ処理で使用します。',
  '// マスターボイス取得（通常は不要）\nIXAudio2MasteringVoice* master = device.GetMasterVoice();\nXAUDIO2_VOICE_DETAILS details;\nmaster->GetVoiceDetails(&details);',
  '* XAudio2が所有するためComPtrは不要\n* DestroyVoice()を呼び出し側で行わないこと\n* 通常はSetMasterVolume()経由で操作すれば十分'
],

'AudioDevice-SetMasterVolume': [
  'void SetMasterVolume(float volume)',
  'マスタリングボイスのマスターボリュームを設定します。0.0（無音）から1.0（最大）の範囲で指定します。全てのSourceVoice出力に影響します。AudioManager::SetMasterVolume()から呼ばれます。',
  '// マスターボリューム設定\ndevice.SetMasterVolume(0.5f);  // 全体を50%に',
  '* IXAudio2MasteringVoice::SetVolume()に委譲\n* 全SourceVoiceの出力に乗算適用される\n* 通常はAudioManager::SetMasterVolume()経由で使用'
],

// ============================================================
// Sound (Audio/Sound.h)
// ============================================================
'Sound-LoadFromFile': [
  'bool LoadFromFile(const std::wstring& filePath)',
  'WAVファイルを読み込み、PCMデータをメモリに保持します。RIFFヘッダー、fmtチャンク、dataチャンクを解析してフォーマット情報とPCMデータを抽出します。成功時はtrueを返します。',
  '// WAVファイル読み込み\nGX::Sound sound;\nif (!sound.LoadFromFile(L"Assets/audio/se_jump.wav")) {\n    GX::Logger::Error(L"Sound load failed!");\n}',
  '* WAV形式（PCM）のみ対応（MP3/OGGは非対応）\n* 全データをメモリに読み込む（ストリーミング非対応）\n* 通常はAudioManager::LoadSound()経由で使用する'
],

'Sound-GetData': [
  'const uint8_t* GetData() const',
  'PCMデータの先頭ポインタを返します。XAudio2のXAUDIO2_BUFFER構造体にセットしてSourceVoiceに渡すために使用します。SoundPlayerやMusicPlayerが内部的に使用します。',
  '// PCMデータ取得（低レベル）\nconst uint8_t* pcm = sound.GetData();\nuint32_t size = sound.GetDataSize();\n// XAUDIO2_BUFFERにセット\nXAUDIO2_BUFFER buf = {};\nbuf.pAudioData = pcm;\nbuf.AudioBytes = size;',
  '* m_pcmData.data()を返す\n* 所有権はSoundオブジェクトが保持\n* LoadFromFile()前またはIsValid()==falseの場合はnullptr'
],

'Sound-GetDataSize': [
  'uint32_t GetDataSize() const',
  'PCMデータのサイズをバイト単位で返します。XAUDIO2_BUFFER::AudioBytesにセットする値です。SoundPlayerやMusicPlayerが内部的に使用します。',
  '// データサイズ確認\nuint32_t bytes = sound.GetDataSize();\nGX::Logger::Info(L"Sound data: %u bytes", bytes);',
  '* static_cast<uint32_t>(m_pcmData.size())を返す\n* 0の場合はデータが読み込まれていない\n* 通常は直接使用する必要はない'
],

'Sound-GetFormat': [
  'const WAVEFORMATEX& GetFormat() const',
  'WAVファイルのフォーマット情報（サンプルレート、チャンネル数、ビット深度等）を返します。XAudio2のCreateSourceVoice()にフォーマット情報として渡すために使用します。',
  '// フォーマット情報確認\nconst WAVEFORMATEX& fmt = sound.GetFormat();\nGX::Logger::Info(L"Channels: %d, SampleRate: %d",\n    fmt.nChannels, fmt.nSamplesPerSec);',
  '* WAVEFORMATEX構造体の参照を返す\n* nChannels(1=mono, 2=stereo), nSamplesPerSec(44100等), wBitsPerSample(16等)を含む\n* LoadFromFile()でWAVのfmtチャンクから解析される'
],

'Sound-IsValid': [
  'bool IsValid() const',
  'PCMデータが正常に読み込まれているかを返します。LoadFromFile()が成功した後はtrueを返します。再生前のバリデーションに使用します。',
  '// 読み込み確認\nif (!sound.IsValid()) {\n    GX::Logger::Error(L"Sound is not loaded");\n    return;\n}',
  '* !m_pcmData.empty()で判定\n* LoadFromFile()失敗後はfalse\n* PlaySound前のチェックに使用可能'
],

// ============================================================
// SoundPlayer (Audio/SoundPlayer.h)
// ============================================================
'SoundPlayer-Initialize': [
  'void Initialize(AudioDevice* audioDevice)',
  'SE再生プレイヤーを初期化し、AudioDeviceへの参照を保持します。AudioManager::Initialize()から内部的に呼ばれます。SourceVoice作成時にAudioDeviceのXAudio2エンジンを使用します。',
  '// 通常はAudioManager経由\nGX::SoundPlayer player;\nplayer.Initialize(&audioDevice);',
  '* AudioDeviceポインタを保持するのみ（軽量）\n* AudioDeviceは呼び出し側が生存を保証すること\n* 通常はAudioManager経由で使用する'
],

'SoundPlayer-Play': [
  'void Play(const Sound& sound, float volume = 1.0f, float pan = 0.0f)',
  'SEを再生します。呼び出しごとに新しいXAudio2 SourceVoiceを作成するため、同じサウンドを重ねて同時再生できます。volumeは0.0-1.0、panは-1.0(左)-1.0(右)です。再生完了後のVoiceはCleanupFinishedVoices()で回収されます。',
  '// SE再生\nGX::Sound jumpSE;\njumpSE.LoadFromFile(L"Assets/audio/jump.wav");\nplayer.Play(jumpSE);        // デフォルト音量・パン\nplayer.Play(jumpSE, 0.5f);  // 音量50%\nplayer.Play(jumpSE, 1.0f, -1.0f);  // 左パン',
  '* 毎回新規SourceVoiceを作成（ポリフォニック再生可能）\n* panはステレオ出力マトリクスで実現される\n* VoiceCallback::OnStreamEnd()で再生完了を検出する'
],

'SoundPlayer-GetActiveVoiceCount': [
  'int GetActiveVoiceCount() const',
  '現在再生中のアクティブなSourceVoice数を返します。パフォーマンスモニタリングやデバッグ表示に使用します。多すぎる場合はSE再生を制限する判断材料にもなります。',
  '// アクティブボイス数監視\nint count = player.GetActiveVoiceCount();\nif (count > 32) {\n    GX::Logger::Warn(L"Too many active voices: %d", count);\n}',
  '* 再生中＋再生完了待ちのVoice数の合計\n* CleanupFinishedVoices()後に減少する\n* XAudio2のSourceVoice数が多すぎるとCPU負荷が増大する'
],

'SoundPlayer-CleanupFinishedVoices': [
  'void CleanupFinishedVoices()',
  '再生が完了したSourceVoiceを検出して解放します。VoiceCallbackのisFinishedフラグに基づいて終了済みのVoiceをDestroyVoice()します。AudioManager::Update()から内部的に呼ばれます。',
  '// 通常はAudioManager::Update()内で自動呼び出し\nplayer.CleanupFinishedVoices();',
  '* OnStreamEnd()コールバックでisFinished=trueになったVoiceを回収\n* DestroyVoice()でXAudio2リソースを解放する\n* 定期的に呼ばないとメモリリークの原因になる'
],

// ============================================================
// MusicPlayer (Audio/MusicPlayer.h)
// ============================================================
'MusicPlayer-Initialize': [
  'void Initialize(AudioDevice* audioDevice)',
  'BGM再生プレイヤーを初期化し、AudioDeviceへの参照を保持します。AudioManager::Initialize()から内部的に呼ばれます。BGMは単一のSourceVoiceで再生されます。',
  '// 通常はAudioManager経由\nGX::MusicPlayer music;\nmusic.Initialize(&audioDevice);',
  '* AudioDeviceポインタを保持するのみ\n* BGMは同時に1曲のみ再生可能\n* 通常はAudioManager経由で使用する'
],

'MusicPlayer-Play': [
  'void Play(const Sound& sound, bool loop = true, float volume = 1.0f)',
  'BGMを再生します。既に再生中のBGMがあれば停止してから新しいBGMを再生します。loop=trueの場合はXAUDIO2_LOOP_INFINITEで無限ループします。',
  '// BGM再生\nGX::Sound bgm;\nbgm.LoadFromFile(L"Assets/audio/battle.wav");\nmusic.Play(bgm, true, 0.7f);  // ループ再生、音量70%',
  '* 前のBGMは自動停止される\n* SourceVoiceを破棄して新規作成する\n* loop=falseの場合は1回再生で停止'
],

'MusicPlayer-Stop': [
  'void Stop()',
  'BGMを即座に停止します。SourceVoiceを停止してバッファをフラッシュします。フェード中の場合もフェードをキャンセルして即時停止します。',
  '// BGM停止\nmusic.Stop();',
  '* 即時停止（ポップノイズの可能性あり）\n* フェード状態もリセットされる\n* m_isPlaying=falseに設定される'
],

'MusicPlayer-Pause': [
  'void Pause()',
  'BGMを一時停止します。再生位置が保持され、Resume()で続きから再開できます。ゲームのポーズ処理に使用します。',
  '// 一時停止\nmusic.Pause();\n// ... ポーズ画面 ...\nmusic.Resume();',
  '* SourceVoice::Stop()で一時停止（XAUDIO2_PLAY_TAILS）\n* m_isPaused=trueに設定される\n* 再生位置とフェード状態が保持される'
],

'MusicPlayer-Resume': [
  'void Resume()',
  '一時停止中のBGMを再開します。Pause()で停止した位置から再生を続けます。一時停止していない場合は何も行いません。',
  '// 再開\nif (music.IsPlaying()) {\n    music.Resume();  // 一時停止中なら再開\n}',
  '* SourceVoice::Start()で再開\n* m_isPaused=falseに設定される\n* Pause()されていない場合は何もしない'
],

'MusicPlayer-IsPlaying': [
  'bool IsPlaying() const',
  'BGMが再生中かどうかを返します。一時停止中もtrueを返します。Stop()後またはPlay()前はfalseです。',
  '// 再生状態確認\nif (music.IsPlaying()) {\n    // BGM再生中...\n}',
  '* m_isPlayingフラグを返す\n* Pause中もtrueを返す（停止ではないため）\n* フェードアウト完了後はfalseになる'
],

'MusicPlayer-SetVolume': [
  'void SetVolume(float volume)',
  'BGMの音量を設定します。0.0（無音）から1.0（最大）の範囲で指定します。フェード処理を行わず即座に音量を変更します。',
  '// BGM音量変更\nmusic.SetVolume(0.5f);  // 50%に設定',
  '* SourceVoice::SetVolume()に委譲\n* フェード中に呼ぶとフェード計算と競合する場合がある\n* 0.0で無音、1.0で最大音量'
],

'MusicPlayer-FadeIn': [
  'void FadeIn(float seconds)',
  'BGMのフェードインを開始します。指定秒数をかけて現在音量からターゲット音量まで徐々に上げます。Update()を毎フレーム呼び出すことでフェードが進行します。',
  '// フェードイン開始\nmusic.Play(bgm, true, 0.8f);\nmusic.FadeIn(2.0f);  // 2秒かけてフェードイン',
  '* m_fadeSpeed = 正の値（1.0/seconds）に設定される\n* Update(dt)で音量が徐々に上昇する\n* m_targetVolumeまで到達したらフェード終了'
],

'MusicPlayer-FadeOut': [
  'void FadeOut(float seconds)',
  'BGMのフェードアウトを開始します。指定秒数をかけて音量を0まで下げ、完了後に自動的にStop()を呼びます。シーン遷移やゲームオーバー時に使用します。',
  '// フェードアウト\nmusic.FadeOut(3.0f);  // 3秒かけてフェードアウト→自動停止',
  '* m_fadeSpeed = 負の値（-currentVolume/seconds）に設定\n* m_stopAfterFade=trueで完了後に自動停止\n* Update(dt)で音量が徐々に減少する'
],

'MusicPlayer-Update': [
  'void Update(float deltaTime)',
  'フェード処理のフレーム更新を行います。deltaTimeに基づいて現在音量を補間し、フェード完了後の自動停止を処理します。AudioManager::Update()から内部的に呼ばれます。',
  '// 毎フレーム更新（通常はAudioManager経由）\nmusic.Update(timer.GetDeltaTime());',
  '* フェード中でない場合は何もしない（m_fadeSpeed==0）\n* currentVolume += fadeSpeed * deltaTimeで音量を更新\n* フェードアウト完了＆stopAfterFadeの場合はStop()を呼ぶ'
],

}
