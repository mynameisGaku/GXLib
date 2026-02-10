// Agent 6: GUI Core + All Widgets (23 classes)
// Auto-generated API reference data
{

// ========================================================================
// Widget (GUI/Widget.h) — page-Widget
// ========================================================================

'Widget-WidgetType': [
  'enum class WidgetType { Panel, Text, Button, Image, TextInput, Slider, CheckBox, RadioButton, DropDown, ListView, ScrollView, ProgressBar, TabView, Dialog, Canvas, Spacer }',
  'ウィジェットの種類を表す列挙型。GetType() の戻り値として使用される。Panel はコンテナ、Text はテキスト表示、Button はクリック可能なボタンなど、GUIシステムで利用可能なすべてのウィジェットタイプを定義する。ウィジェットの種類に応じた処理分岐やスタイルシートのタイプセレクタに使用される。',
  '// ウィジェット種類による分岐\nauto* w = ctx.FindById("myWidget");\nif (w->GetType() == GX::GUI::WidgetType::Button) {\n    auto* btn = static_cast<GX::GUI::Button*>(w);\n    btn->SetText(L"Clicked!");\n}',
  '• 全16種類のウィジェットタイプが定義されている\n• StyleSheet のタイプセレクタ（例: Button { ... }）で使用される\n• GetType() は純粋仮想関数であり、各サブクラスで必ずオーバーライドされる'
],

'Widget-UIEventType': [
  'enum class UIEventType { MouseDown, MouseUp, MouseMove, MouseWheel, MouseEnter, MouseLeave, KeyDown, KeyUp, CharInput, FocusGained, FocusLost, Click, ValueChanged, Submit }',
  'UIイベントの種類を表す列挙型。マウス操作（Down/Up/Move/Wheel/Enter/Leave）、キーボード操作（KeyDown/KeyUp/CharInput）、フォーカス（Gained/Lost）、および高レベルイベント（Click/ValueChanged/Submit）を定義する。UIContext のイベントディスパッチシステムで使用される。',
  '// イベントハンドラ内で種類をチェック\nwidget->onEvent = [](const GX::GUI::UIEvent& e) {\n    if (e.type == GX::GUI::UIEventType::Click) {\n        // クリック処理\n    }\n    if (e.type == GX::GUI::UIEventType::KeyDown) {\n        // キー入力処理 (e.keyCode)\n    }\n};',
  '• MouseEnter/MouseLeave はホバー状態の管理に使用される\n• CharInput は WM_CHAR メッセージから生成され、TextInput で文字入力に使用\n• Click は MouseDown + MouseUp の組み合わせで自動生成される\n• Submit は TextInput で Enter キー押下時に発生する'
],

'Widget-LayoutRect': [
  'struct LayoutRect { float x, y, width, height; bool Contains(float px, float py) const; }',
  'レイアウト計算済みの矩形を表す構造体。x, y は左上座標、width, height は幅と高さを保持する。Contains() メソッドでポイントがこの矩形内に含まれるかをテストできる。layoutRect は親からの相対座標、globalRect はスクリーン座標を表す。',
  '// ウィジェットの矩形情報にアクセス\nauto* btn = ctx.FindById("myButton");\nconst auto& rect = btn->globalRect;\nfloat centerX = rect.x + rect.width / 2.0f;\nfloat centerY = rect.y + rect.height / 2.0f;\nbool hit = rect.Contains(mouseX, mouseY);',
  '• layoutRect は親ウィジェットからの相対座標、globalRect はスクリーン座標\n• Contains() はヒットテストに使用される（x <= px <= x+width）\n• レイアウト計算は UIContext::ComputeLayout() で行われる'
],

'Widget-UIEvent': [
  'struct UIEvent { UIEventType type; UIEventPhase phase; float mouseX, mouseY; int mouseButton, wheelDelta, keyCode; wchar_t charCode; Widget* target; bool handled, stopPropagation; }',
  'UIイベントデータを格納する構造体。イベント種類、フェーズ（Capture/Target/Bubble）、マウス座標、キーコードなどの情報を持つ。handled を true にするとデフォルト処理を抑制し、stopPropagation を true にするとバブリングを停止できる。target はイベント発生元のウィジェット。',
  '// OnEvent オーバーライドでイベント処理\nbool MyWidget::OnEvent(const GX::GUI::UIEvent& event) {\n    if (event.type == GX::GUI::UIEventType::MouseDown) {\n        // マウス座標を利用\n        float lx = event.mouseX - globalRect.x;\n        event.handled = true;\n        return true;\n    }\n    return false;\n}',
  '• phase は Capture→Target→Bubble の3フェーズでディスパッチされる\n• stopPropagation = true でバブリングフェーズを中断できる\n• wheelDelta はマウスホイール回転量（正:上、負:下）\n• charCode は CharInput イベント時の入力文字（wchar_t）'
],

'Widget-GetType': [
  'virtual WidgetType GetType() const = 0',
  'ウィジェットの種類を返す純粋仮想関数。すべてのサブクラスでオーバーライドが必要。Panel は WidgetType::Panel、Button は WidgetType::Button などを返す。実行時型判定やスタイルシートのタイプセレクタマッチングに使用される。',
  '// 型チェック\nauto* w = ctx.FindById("elem");\nif (w->GetType() == GX::GUI::WidgetType::Text) {\n    auto* text = static_cast<GX::GUI::TextWidget*>(w);\n    text->SetText(L"Updated!");\n}',
  '• 純粋仮想関数のため、Widget を直接インスタンス化することはできない\n• StyleSheet の WidgetTypeToString() と連携してタイプセレクタを解決する\n• dynamic_cast よりも高速な型判定に利用可能'
],

'Widget-AddChild': [
  'void AddChild(std::unique_ptr<Widget> child)',
  '子ウィジェットを追加する。所有権は親ウィジェットに移動する（unique_ptr の move）。追加された子の m_parent は自動的にこのウィジェットに設定される。ウィジェットツリーの構築に使用する基本操作。',
  '// パネルに子ウィジェットを追加\nauto panel = std::make_unique<GX::GUI::Panel>();\nauto text = std::make_unique<GX::GUI::TextWidget>();\ntext->SetText(L"Hello");\npanel->AddChild(std::move(text));\nctx.SetRoot(std::move(panel));',
  '• unique_ptr による所有権移動のため、std::move() が必要\n• 追加後は子の m_parent が自動設定される\n• 追加順序がレンダリング順序に影響する（後から追加したものが前面）\n• layoutDirty は自動的に true にはならないため、必要に応じて手動設定する'
],

'Widget-RemoveChild': [
  'void RemoveChild(Widget* child)',
  '指定した子ウィジェットをツリーから除去する。一致する子の unique_ptr が破棄され、子ウィジェットとそのサブツリーは解放される。ウィジェットの動的な追加・削除が必要な場面で使用する。',
  '// 子ウィジェットを除去\nauto* panel = ctx.FindById("container");\nauto* child = ctx.FindById("removeMe");\nif (child) {\n    panel->RemoveChild(child);\n}',
  '• child のポインタが m_children 内に見つからない場合は何も起こらない\n• 除去後は child ポインタは無効になる（unique_ptr が破棄されるため）\n• フォーカス中のウィジェットを除去する場合は SetFocus(nullptr) を先に呼ぶべき'
],

'Widget-GetParent': [
  'Widget* GetParent() const',
  '親ウィジェットへのポインタを返す。ルートウィジェットの場合は nullptr を返す。ウィジェットツリーの上方向への探索や、RadioButton の DeselectSiblings のような兄弟ウィジェット間の操作に使用される。',
  '// 親ウィジェットにアクセス\nauto* widget = ctx.FindById("myWidget");\nif (auto* parent = widget->GetParent()) {\n    // 親が存在する場合の処理\n    auto& siblings = parent->GetChildren();\n}',
  '• ルートウィジェットでは nullptr を返す\n• 非所有ポインタのため、ライフタイム管理に注意\n• RadioButton の相互排他選択で兄弟ウィジェット探索に使用される'
],

'Widget-FindById': [
  'Widget* FindById(const std::string& id)',
  'ID文字列でウィジェットを再帰的に検索する。このウィジェットとすべての子孫を深さ優先で探索し、最初にマッチしたウィジェットを返す。見つからない場合は nullptr を返す。GUILoader で構築したツリーから特定のウィジェットを取得する際に便利。',
  '// IDでウィジェットを検索\nauto* slider = ctx.FindById("volumeSlider");\nif (slider) {\n    auto* s = static_cast<GX::GUI::Slider*>(slider);\n    s->SetValue(0.8f);\n}',
  '• 深さ優先探索のため、同一IDが複数ある場合は最初に見つかったものが返る\n• UIContext::FindById() は内部でルートの FindById() を呼ぶ\n• ID は XML の id 属性や C++ で widget->id = "name" で設定する'
],

'Widget-OnEvent': [
  'virtual bool OnEvent(const UIEvent& event)',
  'イベントハンドラ。UIContext のイベントディスパッチシステムから呼ばれる。サブクラスでオーバーライドしてウィジェット固有のイベント処理を実装する。true を返すとイベントが処理済みであることを示す。基底クラスの実装は onClick コールバックの呼び出しを行う。',
  '// カスタムウィジェットでイベント処理\nbool MyWidget::OnEvent(const GX::GUI::UIEvent& event) {\n    if (event.type == GX::GUI::UIEventType::Click) {\n        // クリック処理\n        return true;\n    }\n    return Widget::OnEvent(event);\n}',
  '• 基底クラスの OnEvent は onClick / onEvent コールバックを呼び出す\n• Button, TextInput, Slider 等は独自の OnEvent 実装を持つ\n• 3フェーズディスパッチの Target フェーズで呼ばれる\n• false を返すとバブリングが継続する'
],

'Widget-Render': [
  'virtual void Render(UIRenderer& renderer)',
  'ウィジェットの描画を行う仮想関数。UIContext::Render() からツリーを再帰的に走査して呼ばれる。各サブクラスでオーバーライドして背景矩形、テキスト、画像などの描画を実装する。visible が false の場合は呼ばれない。',
  '// カスタムウィジェットの描画\nvoid MyWidget::Render(GX::GUI::UIRenderer& renderer) {\n    // 背景描画\n    renderer.DrawRect(globalRect, computedStyle, opacity);\n    // テキスト描画\n    renderer.DrawText(globalRect.x, globalRect.y,\n                      fontHandle, L"Custom", computedStyle.color);\n}',
  '• visible = false のウィジェットは描画がスキップされる\n• 描画順はツリーの走査順（深さ優先）に従う\n• UIRenderer の Begin/End は UIContext が管理するため、個別呼び出しは不要'
],

'Widget-GetIntrinsicWidth': [
  'virtual float GetIntrinsicWidth() const / virtual float GetIntrinsicHeight() const',
  'ウィジェット固有の内在サイズを返す仮想関数。テキストウィジェットではテキスト幅、ボタンではテキスト幅+パディングなどを返す。Flexbox レイアウト計算の MeasureWidget フェーズでスタイルの width/height が Auto の場合にフォールバック値として使用される。',
  '// カスタムウィジェットで固有サイズを定義\nfloat MyWidget::GetIntrinsicWidth() const {\n    return 120.0f; // 最低幅120px\n}\nfloat MyWidget::GetIntrinsicHeight() const {\n    return 40.0f;  // 最低高さ40px\n}',
  '• 基底クラスのデフォルトは 0.0f を返す\n• Style の width/height が指定されている場合はそちらが優先される\n• TextWidget は UIRenderer を使ってテキスト幅を実測する\n• Spacer は SetSize() で設定した値を返す'
],

'Widget-id': [
  'std::string id',
  'ウィジェットの一意な識別子。FindById() での検索や、StyleSheet の #id セレクタによるスタイル適用に使用される。XML の id 属性から自動設定されるか、C++ コードで直接設定する。空文字列の場合は ID なしとして扱われる。',
  '// IDを設定してスタイルを適用\nauto btn = std::make_unique<GX::GUI::Button>();\nbtn->id = "submitBtn";\nbtn->SetText(L"Submit");\n// CSS: #submitBtn { background-color: #4CAF50; }',
  '• StyleSheet の #id セレクタの詳細度は 100\n• 同一IDが複数存在してもエラーにはならないが、FindById は最初のものを返す\n• GUILoader では XML の id 属性から自動設定される'
],

'Widget-className': [
  'std::string className',
  'ウィジェットのクラス名。StyleSheet の .class セレクタによるスタイル適用に使用される。XML の class 属性から自動設定されるか、C++ コードで直接設定する。CSS のクラスセレクタと同様の機能を提供する。',
  '// クラス名でスタイルを共有\nauto btn1 = std::make_unique<GX::GUI::Button>();\nbtn1->className = "primary";\nauto btn2 = std::make_unique<GX::GUI::Button>();\nbtn2->className = "primary";\n// CSS: .primary { background-color: #2196F3; }',
  '• StyleSheet の .class セレクタの詳細度は 10\n• 現在は単一クラス名のみサポート（スペース区切りの複数クラスは非対応）\n• GUILoader では XML の class 属性から自動設定される'
],

'Widget-visible': [
  'bool visible = true',
  'ウィジェットの表示状態。false の場合、Render() が呼ばれず描画がスキップされる。子ウィジェットも含めて非表示になる。Dialog はデフォルトで visible = false に設定される。TabView では非アクティブなタブの visible を false にして表示切替を実現する。',
  '// ウィジェットの表示切替\nauto* panel = ctx.FindById("optionsPanel");\npanel->visible = !panel->visible; // トグル\n\n// Dialog の表示\nauto* dlg = static_cast<GX::GUI::Dialog*>(ctx.FindById("confirmDlg"));\ndlg->Show(); // visible = true',
  '• 非表示ウィジェットはレイアウト計算でもスキップされる\n• 子ウィジェットも再帰的に非表示になる\n• Dialog::Show() / Hide() は内部で visible を操作する'
],

'Widget-enabled': [
  'bool enabled = true',
  'ウィジェットの有効状態。false の場合、イベント処理が無効化され、Button では disabledStyle が適用される。ユーザー操作を一時的に禁止したい場合に使用する。',
  '// ボタンを無効化\nauto* btn = static_cast<GX::GUI::Button*>(ctx.FindById("saveBtn"));\nbtn->enabled = false;\n// CSS: Button:disabled { background-color: #555; opacity: 0.5; }',
  '• Button の :disabled 擬似クラスが自動適用される\n• enabled = false でもウィジェットは描画される（グレーアウト表示など）\n• 子ウィジェットへの伝播は自動では行われない'
],

'Widget-computedStyle': [
  'Style computedStyle',
  'レイアウト・描画に使用される計算済みスタイル。StyleSheet が毎フレーム ComputeLayout() で適用するため、C++ コードで設定した値は上書きされる可能性がある。CSS が設定されていない場合は直接操作可能。',
  '// スタイルを直接設定（CSS未使用時）\nauto panel = std::make_unique<GX::GUI::Panel>();\npanel->computedStyle.backgroundColor = {0.2f, 0.2f, 0.2f, 0.9f};\npanel->computedStyle.cornerRadius = 8.0f;\npanel->computedStyle.padding = GX::GUI::StyleEdges(12.0f);',
  '• StyleSheet 使用時は毎フレーム上書きされるため、CSS で指定する方が確実\n• flexDirection, justifyContent 等の Flexbox プロパティもここに含まれる\n• hover/pressed 時の Button スタイルは hoverStyle/pressedStyle メンバーに格納される'
],

'Widget-layoutRect': [
  'LayoutRect layoutRect / LayoutRect globalRect',
  'レイアウト計算結果の矩形。layoutRect は親ウィジェットからの相対座標、globalRect はスクリーン座標を表す。UIContext::ComputeLayout() によって毎フレーム更新される。描画やヒットテストで使用される。',
  '// ウィジェットの位置・サイズを取得\nauto* widget = ctx.FindById("info");\nfloat screenX = widget->globalRect.x;\nfloat screenY = widget->globalRect.y;\nfloat w = widget->globalRect.width;\nfloat h = widget->globalRect.height;',
  '• layoutRect は親基準の相対座標、globalRect はスクリーン絶対座標\n• デザイン解像度が設定されている場合、globalRect はデザイン座標系で計算される\n• LayoutWidget() の3パスで計算される（Measure→Layout→GlobalRect）'
],

'Widget-scrollOffsetXY': [
  'float scrollOffsetX = 0.0f / float scrollOffsetY = 0.0f',
  'スクロールオフセット。ScrollView や ListView などのスクロール可能なウィジェットが内部的に使用する。UIContext::LayoutWidget のパス3で子ウィジェットの contentX/Y からこの値が減算され、スクロール位置が反映される。',
  '// スクロール位置を外部から設定\nauto* scrollView = static_cast<GX::GUI::ScrollView*>(\n    ctx.FindById("myScroll"));\nscrollView->scrollOffsetY = 100.0f; // 100px下にスクロール',
  '• ScrollView の OnEvent(MouseWheel) で自動更新される\n• ListView は内部で独自のスクロール管理を行う\n• 負の値は通常使用しない（0が先頭位置）'
],

// ========================================================================
// UIContext (GUI/UIContext.h) — page-UIContext
// ========================================================================

'UIContext-Initialize': [
  'bool Initialize(UIRenderer* renderer, uint32_t screenWidth, uint32_t screenHeight)',
  'UIContext を初期化する。UIRenderer のポインタ、スクリーンサイズを設定する。アプリケーション起動時に一度だけ呼び出す。UIRenderer は事前に Initialize 済みである必要がある。失敗時は false を返す。',
  '// UIContext の初期化\nGX::GUI::UIContext uiCtx;\nGX::GUI::UIRenderer uiRenderer;\nuiRenderer.Initialize(device, cmdQueue, 1280, 720,\n                      &spriteBatch, &textRenderer, &fontManager);\nuiCtx.Initialize(&uiRenderer, 1280, 720);',
  '• UIRenderer の Initialize を先に呼ぶ必要がある\n• screenWidth/Height は実際のウィンドウサイズを指定する\n• デザイン解像度は別途 SetDesignResolution() で設定する'
],

'UIContext-SetRoot': [
  'void SetRoot(std::unique_ptr<Widget> root)',
  'ルートウィジェットを設定する。既存のルートがある場合は置き換えられる。ウィジェットツリーの所有権は UIContext に移動する。GUILoader::BuildFromFile() の戻り値をそのまま渡すのが一般的な使い方。',
  '// GUILoader でツリーを構築して設定\nGX::GUI::GUILoader loader;\nloader.SetRenderer(&uiRenderer);\nloader.RegisterFont("default", fontHandle);\nauto tree = loader.BuildFromFile("Assets/ui/menu.xml");\nuiCtx.SetRoot(std::move(tree));',
  '• unique_ptr による所有権移動のため std::move() が必要\n• 既存のルートは自動的に破棄される\n• SetRoot 後に SetStyleSheet でスタイルを適用する'
],

'UIContext-GetRoot': [
  'Widget* GetRoot() const',
  'ルートウィジェットへのポインタを返す。SetRoot() が呼ばれていない場合は nullptr を返す。ウィジェットツリーへのアクセス開始点として使用する。',
  '// ルートウィジェットの子を走査\nauto* root = uiCtx.GetRoot();\nif (root) {\n    for (auto& child : root->GetChildren()) {\n        // 子ウィジェットの処理\n    }\n}',
  '• 非所有ポインタのため、UIContext のライフタイム内でのみ有効\n• ルート未設定時は nullptr を返す'
],

'UIContext-FindById': [
  'Widget* FindById(const std::string& id)',
  'ID文字列でウィジェットツリー全体を検索する。内部ではルートウィジェットの FindById() を再帰的に呼び出す。見つからない場合は nullptr を返す。GUILoader で構築したツリーから特定のウィジェットを取得する際の主要なAPIである。',
  '// IDでウィジェットを検索して操作\nauto* progressBar = uiCtx.FindById("loadingBar");\nif (progressBar) {\n    static_cast<GX::GUI::ProgressBar*>(progressBar)->SetValue(0.75f);\n}',
  '• ルートが未設定の場合は nullptr を返す\n• 深さ優先探索のため、大規模ツリーでは頻繁な呼び出しを避ける\n• static_cast で適切な型にキャストして使用する'
],

'UIContext-Update': [
  'void Update(float deltaTime, InputManager& input)',
  '毎フレームのGUI更新を行う。入力イベントの生成・ディスパッチ、スタイルシートの適用、Flexbox レイアウト計算、ウィジェットの Update() 呼び出しを順に実行する。メインループで毎フレーム呼び出す必要がある。',
  '// メインループでの更新\nwhile (running) {\n    float dt = timer.GetDeltaTime();\n    inputManager.Update();\n    uiCtx.Update(dt, inputManager);\n    // ... 描画 ...\n}',
  '• InputManager からマウス・キーボード状態を取得してイベントを生成する\n• StyleSheet が設定されている場合、毎フレーム ApplyToTree が呼ばれる\n• レイアウトは layoutDirty フラグに基づいて再計算される\n• 3フェーズイベントディスパッチ（Capture→Target→Bubble）がここで実行される'
],

'UIContext-Render': [
  'void Render()',
  'ウィジェットツリー全体を描画する。UIRenderer の Begin/End を管理し、ルートから再帰的に各ウィジェットの Render() を呼び出す。描画後に FlushDeferredDraws() を呼んで遅延描画（DropDown のポップアップ等）も実行する。',
  '// 描画フレーム内で呼び出し\ncmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);\nuiCtx.Render();\n// 遅延描画も自動的にフラッシュされる',
  '• UIRenderer::Begin() / End() は内部で自動管理される\n• visible = false のウィジェットはスキップされる\n• DropDown のポップアップは DeferDraw で遅延描画される（Z順序を保証するため）'
],

'UIContext-ProcessCharMessage': [
  'bool ProcessCharMessage(wchar_t ch)',
  'WM_CHAR メッセージからの文字入力を処理する。Window::AddMessageCallback() で WM_CHAR を受け取り、この関数に転送する。フォーカス中の TextInput ウィジェットに CharInput イベントとしてディスパッチされる。処理した場合は true を返す。',
  '// WM_CHAR メッセージの転送設定\nwindow.AddMessageCallback([&](UINT msg, WPARAM wp, LPARAM lp) {\n    if (msg == WM_CHAR) {\n        uiCtx.ProcessCharMessage(static_cast<wchar_t>(wp));\n    }\n});',
  '• TextInput が機能するために必須の設定\n• Window::AddMessageCallback() でメッセージコールバックを登録する\n• フォーカスされた TextInput がない場合は false を返す\n• IME 入力にも対応（wchar_t で Unicode 文字を受け取る）'
],

'UIContext-SetFocus': [
  'void SetFocus(Widget* widget) / Widget* GetFocusedWidget() const',
  'フォーカスウィジェットを設定・取得する。フォーカスが変更されると、旧ウィジェットに FocusLost、新ウィジェットに FocusGained イベントがディスパッチされる。TextInput はフォーカス時にキーボード入力を受け取る。nullptr を設定するとフォーカスを解除する。',
  '// フォーカスをプログラム的に設定\nauto* input = uiCtx.FindById("nameInput");\nuiCtx.SetFocus(input);\n\n// 現在のフォーカスを確認\nauto* focused = uiCtx.GetFocusedWidget();\nif (focused && focused->GetType() == GX::GUI::WidgetType::TextInput) {\n    // テキスト入力がフォーカス中\n}',
  '• DropDown は FocusLost でポップアップを閉じる\n• TextInput はフォーカス時にカーソルが表示される\n• クリックによるフォーカス変更は UIContext が自動管理する\n• nullptr を渡すとすべてのフォーカスが解除される'
],

'UIContext-SetStyleSheet': [
  'void SetStyleSheet(StyleSheet* sheet)',
  'スタイルシートを設定する。設定されたスタイルシートは Update() 内で毎フレーム自動的にウィジェットツリーに適用される。nullptr を設定するとスタイルシート適用を無効化する。スタイルシートのライフタイムは呼び出し側が管理する。',
  '// スタイルシートの設定\nGX::GUI::StyleSheet sheet;\nsheet.LoadFromFile("Assets/ui/menu.css");\nuiCtx.SetStyleSheet(&sheet);\n// sheet は uiCtx のライフタイム中有効であること',
  '• 非所有ポインタのため、StyleSheet オブジェクトのライフタイムを管理する必要がある\n• 毎フレーム ApplyToTree が呼ばれるため、C++ で設定した computedStyle は上書きされる\n• 動的なスタイル変更は CSS 側で :hover 等の擬似クラスを使用する'
],

'UIContext-SetDesignResolution': [
  'void SetDesignResolution(uint32_t width, uint32_t height)',
  'デザイン解像度を設定する。GUI座標空間の基準解像度を指定し、実際のウィンドウサイズに対して自動スケーリングを行う。アスペクト比が異なる場合はレターボックス（均一スケーリング）で対応する。0を指定するとスケーリングを無効化する。',
  '// デザイン解像度を設定\nuiCtx.SetDesignResolution(1280, 960);\n// 1920x1080 画面でも 1280x960 座標系でレイアウトされる\n// uniform scale = min(1920/1280, 1080/960) = 1.125',
  '• uniform scale = min(screenW/designW, screenH/designH) で計算される\n• レターボックスのオフセットが自動的に計算される\n• マウス座標もスクリーン→デザイン座標に自動変換される\n• UIRenderer にも同じデザイン解像度が設定される'
],

'UIContext-OnResize': [
  'void OnResize(uint32_t width, uint32_t height)',
  'スクリーンサイズの変更を通知する。ウィンドウリサイズ時に呼び出し、内部のレイアウト計算やスケーリングを更新する。UIRenderer::OnResize() も連動して呼ばれる。',
  '// ウィンドウリサイズ時\nwindow.AddMessageCallback([&](UINT msg, WPARAM wp, LPARAM lp) {\n    if (msg == WM_SIZE) {\n        uint32_t w = LOWORD(lp);\n        uint32_t h = HIWORD(lp);\n        uiCtx.OnResize(w, h);\n    }\n});',
  '• デザイン解像度が設定されている場合、スケール係数が再計算される\n• レイアウトの再計算がトリガーされる\n• SwapChain のリサイズとは別に呼び出す必要がある'
],

// ========================================================================
// UIRenderer (GUI/UIRenderer.h) — page-UIRenderer
// ========================================================================

'UIRenderer-Initialize': [
  'bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, uint32_t screenWidth, uint32_t screenHeight, SpriteBatch* spriteBatch, TextRenderer* textRenderer, FontManager* fontManager)',
  'UIRenderer を初期化する。D3D12 デバイス、コマンドキュー、スクリーンサイズ、および描画に必要な SpriteBatch / TextRenderer / FontManager の参照を設定する。UIRectBatch 用のパイプライン（SDF丸角矩形シェーダー）もここで作成される。',
  '// UIRenderer の初期化\nGX::GUI::UIRenderer renderer;\nrenderer.Initialize(device, cmdQueue, 1280, 720,\n                    &spriteBatch, &textRenderer, &fontManager);',
  '• SpriteBatch, TextRenderer, FontManager は事前に初期化済みである必要がある\n• UIRect.hlsl シェーダーが内部で読み込まれる\n• DynamicBuffer（VB/CB）と IndexBuffer がここで作成される'
],

'UIRenderer-Begin': [
  'void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)',
  'GUI描画フレームを開始する。コマンドリストとフレームインデックスを設定し、シザースタックを初期化する。SpriteBatch の正射影行列をデザイン解像度に合わせて設定する。UIContext::Render() から内部的に呼ばれる。',
  '// 通常は UIContext::Render() が自動的に呼ぶ\n// 手動で使う場合:\nuiRenderer.Begin(cmdList, frameIndex);\n// 描画処理...\nuiRenderer.End();',
  '• UIContext::Render() が自動管理するため、通常は直接呼ぶ必要はない\n• フレームインデックスは DynamicBuffer のダブルバッファリングに使用される\n• シザー矩形はフルスクリーンに初期化される'
],

'UIRenderer-End': [
  'void End()',
  'GUI描画フレームを終了する。SpriteBatch のフラッシュ、デザイン解像度のリセットなどの後処理を行う。Begin() と対で呼び出す必要がある。',
  '// UIContext::Render() が自動管理\n// 手動使用時:\nuiRenderer.Begin(cmdList, frameIndex);\nuiRenderer.DrawRect(rect, style);\nuiRenderer.End();',
  '• SpriteBatch の正射影行列がリセットされる\n• Begin() なしに End() を呼ぶと不正な状態になる\n• UIContext::Render() が自動的に呼ぶため、通常は意識不要'
],

'UIRenderer-DrawRect': [
  'void DrawRect(const LayoutRect& rect, const Style& style, float opacity = 1.0f)',
  'スタイル付き矩形を描画する。SDF（Signed Distance Field）シェーダーによる角丸矩形レンダリングを使用し、背景色、枠線、影、角丸をすべて1ドローコールで描画する。opacity は全体の透明度の乗算係数。',
  '// スタイル付き矩形を描画\nGX::GUI::Style style;\nstyle.backgroundColor = {0.2f, 0.3f, 0.5f, 1.0f};\nstyle.cornerRadius = 8.0f;\nstyle.borderWidth = 1.0f;\nstyle.borderColor = {0.5f, 0.5f, 0.5f, 1.0f};\nuiRenderer.DrawRect(widget->globalRect, style, 1.0f);',
  '• UIRect.hlsl の SDF シェーダーで描画される\n• SpriteBatch がアクティブな場合は内部で自動フラッシュされる\n• 1矩形あたり定数バッファ144バイト + 頂点4つを消費する\n• shadowBlur, shadowOffset が Style に設定されていれば影も描画される'
],

'UIRenderer-DrawText': [
  'void DrawText(float x, float y, int fontHandle, const std::wstring& text, const StyleColor& color, float opacity = 1.0f)',
  'テキストを描画する。内部の TextRenderer を使用してフォントアトラスからグリフを描画する。座標はデザイン座標系（デザイン解像度設定時）。UIRectBatch がアクティブな場合は SpriteBatch に自動切替する。',
  '// テキストを描画\nuiRenderer.DrawText(\n    widget->globalRect.x + 8.0f,\n    widget->globalRect.y + 4.0f,\n    fontHandle,\n    L"Hello, World!",\n    GX::GUI::StyleColor(1.0f, 1.0f, 1.0f, 1.0f));',
  '• フォントハンドルは FontManager::CreateFont() で事前に作成する\n• SpriteBatch 経由で描画されるため、PSO 切替が発生する\n• 日本語（漢字含む）はオンデマンドラスタライズされる'
],

'UIRenderer-DrawImage': [
  'void DrawImage(float x, float y, float w, float h, int textureHandle, float opacity = 1.0f)',
  'テクスチャ画像を指定矩形に描画する。SpriteBatch を使用して描画する。Image ウィジェットの内部描画や、カスタム Canvas 描画で使用する。',
  '// 画像を描画\nint texHandle = textureManager.LoadTexture("Assets/icon.png");\nuiRenderer.DrawImage(\n    widget->globalRect.x, widget->globalRect.y,\n    64.0f, 64.0f, texHandle, 1.0f);',
  '• テクスチャハンドルは TextureManager で管理されるインデックス\n• SpriteBatch 経由の描画のため、バッチ効率に影響する\n• opacity は 0.0f〜1.0f でテクスチャ全体の透明度を制御する'
],

'UIRenderer-PushScissor': [
  'void PushScissor(const LayoutRect& rect)',
  'クリッピング矩形をスタックにプッシュする。ネスト対応で、現在のシザー矩形との交差領域が新しいクリッピング領域になる。ScrollView、Image のフィットモード、overflow: hidden 等で使用される。',
  '// シザー矩形でクリッピング\nuiRenderer.PushScissor(widget->globalRect);\n// この範囲内のみ描画される\nuiRenderer.DrawText(x, y, fontHandle, L"Clipped text", color);\nuiRenderer.PopScissor();',
  '• 必ず PushScissor と PopScissor を対で呼ぶ\n• ネストした場合は交差領域（Intersect）が計算される\n• 交差結果が空（top > bottom 等）の場合はゼロサイズ矩形にクランプされる\n• デザイン解像度使用時はスクリーン座標への変換が自動的に行われる'
],

'UIRenderer-PopScissor': [
  'void PopScissor()',
  'シザー矩形をスタックからポップする。PushScissor と対で呼び出す。スタックが空になるとフルスクリーンのシザー矩形に戻る。',
  '// Push/Pop のペアリング\nuiRenderer.PushScissor(parentRect);\n  uiRenderer.PushScissor(childRect); // ネスト\n  // ... 描画 ...\n  uiRenderer.PopScissor();\nuiRenderer.PopScissor();',
  '• Push と Pop の数が一致しないとシザー状態が不正になる\n• スタックが空の場合はフルスクリーンのシザー矩形が適用される'
],

// ========================================================================
// Style (GUI/Style.h) — page-Style
// ========================================================================

'Style-FlexDirection': [
  'enum class FlexDirection { Row, Column }',
  'Flexbox の主軸方向を定義する列挙型。Row は水平方向（左→右）、Column は垂直方向（上→下）に子ウィジェットを配置する。デフォルトは Column。CSS の flex-direction プロパティに対応する。',
  '// 水平レイアウト\nauto panel = std::make_unique<GX::GUI::Panel>();\npanel->computedStyle.flexDirection = GX::GUI::FlexDirection::Row;\npanel->computedStyle.gap = 10.0f;\n// CSS: .row { flex-direction: row; gap: 10px; }',
  '• デフォルトは Column（縦方向配置）\n• CSS では flex-direction: row / column で指定\n• gap プロパティで子ウィジェット間の間隔を設定できる'
],

'Style-JustifyContent': [
  'enum class JustifyContent { Start, Center, End, SpaceBetween, SpaceAround }',
  'Flexbox の主軸方向の配置方法を定義する列挙型。Start は先頭揃え、Center は中央揃え、End は末尾揃え、SpaceBetween は均等配置（端は余白なし）、SpaceAround は均等配置（端にも半分の余白）。',
  '// 中央揃え\npanel->computedStyle.justifyContent = GX::GUI::JustifyContent::Center;\n// CSS: .centered { justify-content: center; }',
  '• CSS では justify-content: flex-start / center / flex-end / space-between / space-around で指定\n• 子ウィジェットの合計サイズが親を超える場合、SpaceBetween / SpaceAround は Start と同等になる'
],

'Style-AlignItems': [
  'enum class AlignItems { Start, Center, End, Stretch }',
  'Flexbox の交差軸方向の配置方法を定義する列挙型。Start は先頭揃え、Center は中央揃え、End は末尾揃え、Stretch は親の交差軸方向サイズに引き伸ばす。デフォルトは Stretch。',
  '// 交差軸中央揃え\npanel->computedStyle.alignItems = GX::GUI::AlignItems::Center;\n// CSS: .vcenter { align-items: center; }',
  '• デフォルトは Stretch（子が親サイズに合わせて伸張する）\n• Stretch の場合、子の交差軸方向サイズが親に合わせて調整される\n• CSS では align-items: flex-start / center / flex-end / stretch で指定'
],

'Style-PositionType': [
  'enum class PositionType { Relative, Absolute }',
  '位置指定方法を定義する列挙型。Relative は通常のフローレイアウト、Absolute は親ウィジェットの左上を基準とした絶対位置配置。Absolute の場合は posLeft / posTop で座標を指定する。',
  '// 絶対位置配置\nauto overlay = std::make_unique<GX::GUI::Panel>();\noverlay->computedStyle.position = GX::GUI::PositionType::Absolute;\noverlay->computedStyle.posLeft = GX::GUI::StyleLength::Px(50.0f);\noverlay->computedStyle.posTop = GX::GUI::StyleLength::Px(100.0f);\n// CSS: .abs { position: absolute; left: 50px; top: 100px; }',
  '• Absolute ウィジェットは Flexbox レイアウトのフローから除外される\n• posLeft / posTop で親基準の座標を指定する\n• CSS では position: absolute; left: 50px; top: 100px; で指定'
],

'Style-OverflowMode': [
  'enum class OverflowMode { Visible, Hidden, Scroll }',
  'オーバーフロー処理モードを定義する列挙型。Visible は子が親の範囲外に描画される。Hidden は親の矩形でクリッピングされる。Scroll は Hidden と同様にクリッピングし、スクロールバーが表示される。',
  '// オーバーフローを非表示\npanel->computedStyle.overflow = GX::GUI::OverflowMode::Hidden;\n// CSS: .clipped { overflow: hidden; }',
  '• ScrollView は内部で overflow: Scroll を使用する\n• Hidden / Scroll の場合、ヒットテストでも子の範囲外クリックが無視される\n• PushScissor / PopScissor でクリッピングが実装される'
],

'Style-TextAlign': [
  'enum class TextAlign { Left, Center, Right }',
  'テキストの水平揃えを定義する列挙型。Left は左揃え、Center は中央揃え、Right は右揃え。TextWidget や Button のテキスト描画位置に影響する。CSS の text-align プロパティに対応する。',
  '// テキスト中央揃え\ntext->computedStyle.textAlign = GX::GUI::TextAlign::Center;\n// CSS: .centered-text { text-align: center; }',
  '• Button のテキストはデフォルトで Center が使われることが多い\n• TextWidget のテキスト描画位置を制御する\n• CSS では text-align: left / center / right で指定'
],

'Style-flexDirection': [
  'FlexDirection flexDirection = FlexDirection::Column',
  'Flexbox の主軸方向。Column（デフォルト）は子を縦方向に配置し、Row は横方向に配置する。Panel などのコンテナウィジェットで子ウィジェットの配置方向を決定する主要プロパティ。',
  '// 横並びレイアウト\npanel->computedStyle.flexDirection = GX::GUI::FlexDirection::Row;\npanel->computedStyle.gap = 8.0f;\n// CSS: .toolbar { flex-direction: row; gap: 8px; }',
  '• ボタン群を横並びにする場合は Row を使用\n• gap と組み合わせて子ウィジェット間の間隔を制御する\n• justifyContent / alignItems と連携してレイアウトを決定する'
],

'Style-justifyContent': [
  'JustifyContent justifyContent = JustifyContent::Start',
  'Flexbox の主軸方向の子ウィジェット配置方法。Start（デフォルト）は先頭揃え。CSS の justify-content に対応し、子ウィジェットの分布を制御する。',
  '// 主軸方向で均等配置\npanel->computedStyle.justifyContent = GX::GUI::JustifyContent::SpaceBetween;\n// CSS: .spread { justify-content: space-between; }',
  '• flexGrow が設定された子がある場合、余剰スペースはそちらに優先配分される\n• SpaceBetween は最初と最後の子を端に配置し、間を均等に分配する'
],

'Style-alignItems': [
  'AlignItems alignItems = AlignItems::Stretch',
  'Flexbox の交差軸方向の子ウィジェット配置方法。Stretch（デフォルト）は子を交差軸方向に引き伸ばす。Center は中央揃え。CSS の align-items に対応する。',
  '// 交差軸で中央揃え（縦方向レイアウト時は水平方向）\npanel->computedStyle.alignItems = GX::GUI::AlignItems::Center;\n// CSS: .centered { align-items: center; }',
  '• Stretch の場合、子ウィジェットの交差軸サイズが親に合わせて拡張される\n• 子に明示的な width/height が設定されている場合は Stretch でもそちらが優先される'
],

'Style-gap': [
  'float gap = 0.0f / float flexGrow = 0.0f',
  'gap は Flexbox 子ウィジェット間の間隔（ピクセル）。flexGrow は余剰スペースの分配比率。flexGrow > 0 のウィジェットは親の余りスペースを比率に応じて分配される。CSS の gap と flex-grow に対応する。',
  '// 子ウィジェット間に10pxの間隔を設定\npanel->computedStyle.gap = 10.0f;\n// 子に flex-grow を設定して余白を埋める\nchild->computedStyle.flexGrow = 1.0f;\n// CSS: .container { gap: 10px; }\n// CSS: .fill { flex-grow: 1; }',
  '• gap は子ウィジェット間にのみ適用される（先頭・末尾には適用されない）\n• flexGrow = 0 のウィジェットは固有サイズを維持する\n• 複数の子が flexGrow > 0 の場合、比率に応じてスペースが分配される'
],

'Style-widthheight': [
  'StyleLength width, height, minWidth, minHeight, maxWidth, maxHeight',
  'ウィジェットのサイズ制約。width/height は Auto（固有サイズ使用）、Px（ピクセル）、Percent（親サイズの割合）を指定できる。minWidth/maxWidth/minHeight/maxHeight でサイズ範囲を制約する。CSS の width/height/min-width/max-width に対応する。',
  '// 幅を親の50%、高さを100pxに設定\nwidget->computedStyle.width = GX::GUI::StyleLength::Pct(50.0f);\nwidget->computedStyle.height = GX::GUI::StyleLength::Px(100.0f);\n// CSS: .half { width: 50%; height: 100px; min-width: 80px; }',
  '• Auto の場合は GetIntrinsicWidth/Height() の値がフォールバックとして使用される\n• Percent は親ウィジェットのサイズに対する割合\n• min/max 制約は最終的なサイズに適用される\n• maxWidth/maxHeight のデフォルトは 100000px（事実上無制限）'
],

'Style-margin': [
  'StyleEdges margin / StyleEdges padding',
  '外側余白（margin）と内側余白（padding）。四辺の値を個別に設定可能。margin はウィジェットの外側にスペースを確保し、padding はウィジェットの内側に子ウィジェットの配置領域を制限する。CSS の margin / padding に対応する。',
  '// 余白の設定\nwidget->computedStyle.margin = GX::GUI::StyleEdges(8.0f); // 全辺8px\nwidget->computedStyle.padding = GX::GUI::StyleEdges(4.0f, 12.0f); // 上下4px, 左右12px\n// CSS: .card { margin: 8px; padding: 4px 12px; }',
  '• StyleEdges は (all), (vertical, horizontal), (top, right, bottom, left) の3種のコンストラクタを持つ\n• CSS では margin-top, padding-left 等の個別指定も可能\n• margin はウィジェット間の間隔に影響し、gap と累積する'
],

'Style-backgroundColor': [
  'StyleColor backgroundColor / StyleColor borderColor / StyleColor color',
  'ウィジェットの色プロパティ。backgroundColor は背景色、borderColor は枠線色、color はテキスト色。StyleColor は float 型の RGBA 値（0.0〜1.0）を保持する。CSS の background-color / border-color / color に対応する。',
  '// 色の設定\nwidget->computedStyle.backgroundColor = {0.1f, 0.1f, 0.2f, 0.9f};\nwidget->computedStyle.borderColor = {0.4f, 0.4f, 0.6f, 1.0f};\nwidget->computedStyle.color = {1.0f, 1.0f, 1.0f, 1.0f};\n// CSS: Panel { background-color: #1A1A33E6; color: #FFFFFF; }',
  '• StyleColor は float RGBA（0.0〜1.0）で指定する\n• CSS では #RRGGBB / #RRGGBBAA のHEX形式で指定する\n• IsTransparent() で alpha <= 0 かをチェックできる\n• 透明（alpha=0）の場合、DrawRect で描画がスキップされることがある'
],

'Style-borderWidth': [
  'float borderWidth = 0.0f / float cornerRadius = 0.0f / float fontSize = 16.0f',
  '視覚プロパティ。borderWidth は枠線の太さ、cornerRadius は角丸の半径（ピクセル）、fontSize はテキストのフォントサイズ。SDF シェーダーにより角丸と枠線は滑らかに描画される。CSS の border-width / border-radius / font-size に対応する。',
  '// 枠線と角丸の設定\nwidget->computedStyle.borderWidth = 2.0f;\nwidget->computedStyle.cornerRadius = 12.0f;\nwidget->computedStyle.fontSize = 20.0f;\n// CSS: .card { border-width: 2px; border-radius: 12px; font-size: 20px; }',
  '• cornerRadius は 0 で直角矩形になる\n• SDF シェーダーにより、角丸はアンチエイリアスされた滑らかな描画になる\n• fontSize は TextWidget / Button のテキスト描画に影響するが、fontHandle の設定も必要'
],

// ========================================================================
// StyleSheet (GUI/StyleSheet.h) — page-StyleSheet
// ========================================================================

'StyleSheet-Load': [
  'bool LoadFromFile(const std::string& path)',
  'CSS ファイルからスタイルルールを読み込む。ファイルをテキストとして読み込み、Tokenize→Parse のパイプラインで処理する。成功時は true を返す。ファイルが見つからない場合やパースエラー時は false を返す。',
  '// CSSファイルの読み込み\nGX::GUI::StyleSheet sheet;\nif (sheet.LoadFromFile("Assets/ui/menu.css")) {\n    uiCtx.SetStyleSheet(&sheet);\n}',
  '• ファイルパスは実行ファイルからの相対パスまたは絶対パス\n• UTF-8 エンコーディングのファイルを想定\n• 読み込み後に UIContext::SetStyleSheet() で設定する\n• 複数回呼び出すと前のルールは上書きされる'
],

'StyleSheet-Parse': [
  'bool LoadFromString(const std::string& cssText)',
  'CSS テキスト文字列からスタイルルールをパースする。Tokenize でトークン列に分解し、ParseTokens でセレクタとプロパティブロックを解析してルールリストに格納する。成功時は true を返す。',
  '// CSS文字列からパース\nGX::GUI::StyleSheet sheet;\nsheet.LoadFromString(R"(\n    Panel { background-color: #222233; padding: 8px; }\n    Button:hover { background-color: #4CAF50; }\n)");\nuiCtx.SetStyleSheet(&sheet);',
  '• LoadFromFile は内部で LoadFromString を呼び出す\n• セレクタタイプ: Type, .class, #id, :pseudo がサポートされる\n• 複数セレクタ（カンマ区切り）は非対応'
],

'StyleSheet-ApplyToTree': [
  'void ApplyToTree(Widget* root) const',
  'ウィジェットツリー全体にスタイルルールを再帰的に適用する。各ウィジェットに対してマッチするルールを詳細度順に適用し、computedStyle を更新する。UIContext::Update() 内で毎フレーム自動的に呼ばれる。',
  '// ツリーにスタイルを手動適用\nGX::GUI::StyleSheet sheet;\nsheet.LoadFromFile("Assets/ui/style.css");\nsheet.ApplyToTree(uiCtx.GetRoot());',
  '• 詳細度順（id=100 > class=10 > type=1）でルールが適用される\n• 同一詳細度ではソース順序が後のルールが優先される\n• Button の :hover / :pressed / :disabled は hoverStyle / pressedStyle / disabledStyle に自動適用される\n• 毎フレーム呼ばれるため、C++ で設定した computedStyle は上書きされる'
],

'StyleSheet-ApplyProperty': [
  'static void ApplyProperty(Style& style, const std::string& name, const std::string& value)',
  '個別のスタイルプロパティを Style 構造体に適用する。プロパティ名（camelCase）と値文字列を受け取り、対応するフィールドを設定する。GUILoader のインラインスタイル属性で使用される public static メソッド。',
  '// 個別プロパティの適用\nGX::GUI::Style style;\nGX::GUI::StyleSheet::ApplyProperty(style, "backgroundColor", "#FF5722");\nGX::GUI::StyleSheet::ApplyProperty(style, "cornerRadius", "8");\nGX::GUI::StyleSheet::ApplyProperty(style, "padding", "12");',
  '• プロパティ名は camelCase 形式で指定する（kebab-case は NormalizePropertyName で変換）\n• 色は "#RRGGBB" / "#RRGGBBAA" 形式、数値は文字列形式\n• GUILoader の XML インライン属性で内部的に使用される\n• border-radius は cornerRadius のエイリアスとして扱われる'
],

'StyleSheet-NormalizePropertyName': [
  'static std::string NormalizePropertyName(const std::string& name)',
  'CSS の kebab-case プロパティ名を camelCase に変換する。例: "flex-direction" → "flexDirection"、"background-color" → "backgroundColor"。CSS パーサーと GUILoader で共通使用される正規化メソッド。',
  '// プロパティ名の正規化\nstd::string normalized = GX::GUI::StyleSheet::NormalizePropertyName("background-color");\n// normalized == "backgroundColor"\n\nstd::string normalized2 = GX::GUI::StyleSheet::NormalizePropertyName("border-radius");\n// normalized2 == "cornerRadius" (エイリアス)',
  '• ハイフンの次の文字を大文字に変換する標準的な kebab→camelCase 変換\n• "border-radius" → "cornerRadius" のようなエイリアス変換も含む\n• CSS ファイル内では kebab-case を使い、内部では camelCase に正規化される'
],

// ========================================================================
// GUILoader (GUI/GUILoader.h) — page-GUILoader
// ========================================================================

'GUILoader-SetRenderer': [
  'void SetRenderer(UIRenderer* renderer)',
  'UIRenderer を設定する。TextWidget や Button の intrinsic size 計算（GetIntrinsicWidth/Height）に必要なため、BuildFromFile の前に呼び出す必要がある。設定された renderer は構築したウィジェットにも伝播される。',
  '// GUILoader の初期化\nGX::GUI::GUILoader loader;\nloader.SetRenderer(&uiRenderer);\n// フォント・イベント登録後に BuildFromFile',
  '• BuildFromFile より前に呼び出す必要がある\n• TextWidget, Button, CheckBox 等の SetRenderer に自動的に設定される\n• 非所有ポインタのため、UIRenderer のライフタイムを管理すること'
],

'GUILoader-RegisterFont': [
  'void RegisterFont(const std::string& name, int fontHandle)',
  'フォント名とフォントハンドルの対応を登録する。XML の font 属性で指定されたフォント名をハンドルに解決するために使用する。FontManager::CreateFont() で作成したハンドルを登録する。',
  '// フォントの登録\nint fontNormal = fontManager.CreateFont(L"Yu Gothic UI", 16);\nint fontTitle = fontManager.CreateFont(L"Yu Gothic UI", 24);\nloader.RegisterFont("default", fontNormal);\nloader.RegisterFont("title", fontTitle);\n// XML: <Text font="title">Title</Text>',
  '• XML の font 属性で名前を指定してフォントを参照する\n• "default" という名前はデフォルトフォントとして使われることが多い\n• 未登録の名前が指定された場合は -1（無効ハンドル）が使用される'
],

'GUILoader-RegisterEvent': [
  'void RegisterEvent(const std::string& name, std::function<void()> handler)',
  'イベント名とハンドラ関数の対応を登録する。XML の onClick 属性で指定されたイベント名をハンドラに解決する。ボタンクリック等の UI アクションを C++ 側の関数に接続する。',
  '// イベントハンドラの登録\nloader.RegisterEvent("onStart", [&]() {\n    gameState = GameState::Playing;\n});\nloader.RegisterEvent("onQuit", [&]() {\n    running = false;\n});\n// XML: <Button onClick="onStart">Start</Button>',
  '• XML の onClick 属性でイベント名を指定する\n• ハンドラは引数なしの void() 関数\n• 未登録のイベント名は無視される（エラーにはならない）'
],

'GUILoader-RegisterTexture': [
  'void RegisterTexture(const std::string& name, int textureHandle)',
  'テクスチャ名とテクスチャハンドルの対応を登録する。XML の texture 属性で指定されたテクスチャ名をハンドルに解決する。Image ウィジェットでの画像表示に使用する。',
  '// テクスチャの登録\nint iconTex = textureManager.LoadTexture("Assets/icon.png");\nloader.RegisterTexture("icon", iconTex);\n// XML: <Image texture="icon" width="64" height="64"/>',
  '• TextureManager::LoadTexture() で読み込んだハンドルを登録する\n• XML の texture 属性でテクスチャ名を指定する\n• 未登録の名前が指定された場合は -1（無効ハンドル）が使用される'
],

'GUILoader-RegisterValueChangedEvent': [
  'void RegisterValueChangedEvent(const std::string& name, std::function<void(const std::string&)> handler)',
  '値変更イベント名とハンドラの対応を登録する。Slider、CheckBox、RadioButton 等の値が変わったときにコールバックが呼ばれる。ハンドラは変更後の値を文字列で受け取る。',
  '// 値変更ハンドラの登録\nloader.RegisterValueChangedEvent("onVolumeChange",\n    [&](const std::string& val) {\n        float volume = std::stof(val);\n        audioManager.SetVolume(volume);\n    });\n// XML: <Slider onValueChanged="onVolumeChange"/>',
  '• XML の onValueChanged 属性でイベント名を指定する\n• Slider は "0.500000" のような float 文字列を送信する\n• CheckBox は "1" / "0" の文字列を送信する\n• RadioButton は SetValue で設定した値文字列を親の onValueChanged に送信する'
],

'GUILoader-RegisterDrawCallback': [
  'void RegisterDrawCallback(const std::string& name, std::function<void(UIRenderer&, const LayoutRect&)> handler)',
  'Canvas の描画コールバック名とハンドラの対応を登録する。Canvas ウィジェットの onDraw コールバックに接続され、毎フレーム呼び出される。UIRenderer と矩形情報を受け取って自由な描画が可能。',
  '// 描画コールバックの登録\nloader.RegisterDrawCallback("drawMinimap",\n    [&](GX::GUI::UIRenderer& r, const GX::GUI::LayoutRect& rect) {\n        r.DrawSolidRect(rect.x, rect.y, rect.width, rect.height,\n                        {0.0f, 0.3f, 0.0f, 1.0f});\n    });\n// XML: <Canvas onDraw="drawMinimap"/>',
  '• Canvas の Render 内で毎フレーム呼ばれる\n• LayoutRect は Canvas ウィジェットの globalRect が渡される\n• UIRenderer の DrawRect / DrawText / DrawImage が使用可能'
],

'GUILoader-BuildFromFile': [
  'std::unique_ptr<Widget> BuildFromFile(const std::string& xmlPath)',
  'XML ファイルからウィジェットツリーを構築して返す。内部で XMLDocument::LoadFromFile() を呼び、各 XML タグを対応するウィジェットに変換する。フォント・イベント・テクスチャの登録は事前に行う必要がある。',
  '// XMLからウィジェットツリーを構築\nGX::GUI::GUILoader loader;\nloader.SetRenderer(&uiRenderer);\nloader.RegisterFont("default", fontHandle);\nloader.RegisterEvent("onStart", [&]() { Start(); });\nauto tree = loader.BuildFromFile("Assets/ui/menu.xml");\nuiCtx.SetRoot(std::move(tree));',
  '• SetRenderer / RegisterFont / RegisterEvent 等を事前に呼ぶ\n• 戻り値は unique_ptr<Widget> で、UIContext::SetRoot() に渡す\n• XML タグ名がウィジェットタイプに対応する（Panel, Button, Text, etc.）\n• インライン style 属性は StyleSheet::ApplyProperty で適用される'
],

// ========================================================================
// XMLParser (GUI/XMLParser.h) — page-XMLParser
// ========================================================================

'XMLParser-XMLNode': [
  'struct XMLNode { std::string tag; std::string text; std::unordered_map<std::string, std::string> attributes; std::vector<std::unique_ptr<XMLNode>> children; }',
  'XML DOM ノードを表す構造体。タグ名、テキストコンテンツ、属性マップ、子ノードリストを保持する。GetAttribute() で属性値を取得し、HasAttribute() で属性の存在を確認できる。GUILoader がウィジェット構築に使用する。',
  '// XMLNode の属性アクセス\nauto doc = GX::GUI::XMLDocument();\ndoc.LoadFromFile("Assets/ui/menu.xml");\nauto* root = doc.GetRoot();\nstd::string id = root->GetAttribute("id", "default");\nfor (auto& child : root->children) {\n    if (child->tag == "Button") { /* ... */ }\n}',
  '• GetAttribute の第2引数はデフォルト値（属性が存在しない場合に返される）\n• children は unique_ptr で所有される\n• text は <Tag>テキスト</Tag> の中間テキスト部分'
],

'XMLParser-Parse': [
  'bool XMLDocument::LoadFromString(const std::string& source)',
  'XML 文字列をパースして DOM ツリーを構築する。再帰降下パーサーで XML を解析し、XMLNode のツリーを生成する。BOM、コメント（<!-- -->）、自己閉じタグ（<Tag/>）、エンティティ（&amp; 等）をサポートする。',
  '// XML文字列からパース\nGX::GUI::XMLDocument doc;\ndoc.LoadFromString(R"(\n  <Panel id=\\"root\\">\n    <Text font=\\"default\\">Hello</Text>\n    <Button onClick=\\"onStart\\">Start</Button>\n  </Panel>\n)");\nauto* root = doc.GetRoot();',
  '• UTF-8 BOM (EF BB BF) は自動的にスキップされる\n• XML 宣言 (<?xml ...?>) もスキップされる\n• サポートされるエンティティ: &amp; &lt; &gt; &quot; &apos;\n• 不正な XML ではパースエラーとなり空の DOM が返される'
],

'XMLParser-ParseFile': [
  'bool XMLDocument::LoadFromFile(const std::string& path)',
  'XML ファイルを読み込んでパースする。内部でファイルをテキストとして読み込み、LoadFromString() に渡す。GUILoader::BuildFromFile() で内部的に使用される。',
  '// XMLファイルからロード\nGX::GUI::XMLDocument doc;\nif (doc.LoadFromFile("Assets/ui/menu.xml")) {\n    auto* root = doc.GetRoot();\n    // DOM を走査...\n}',
  '• ファイルパスは実行ファイルからの相対パスまたは絶対パス\n• ファイルが見つからない場合は false を返す\n• 通常は GUILoader::BuildFromFile() 経由で間接的に使用する'
],

// ========================================================================
// Panel — page-Panel (独自メソッドなし、skip不要: brief only)
// ========================================================================

// Panel has no methods in HTML - skip

// ========================================================================
// TextWidget — page-TextWidget
// ========================================================================

'TextWidget-SetText': [
  'void SetText(const std::wstring& text)',
  '表示テキストを設定する。テキスト変更時に layoutDirty が true に設定され、次フレームでレイアウトが再計算される。テキスト幅に応じて GetIntrinsicWidth() の戻り値も変化する。',
  '// テキストを設定\nauto text = std::make_unique<GX::GUI::TextWidget>();\ntext->SetText(L"スコア: 1000");\ntext->SetFontHandle(fontHandle);\n\n// 動的に更新\nauto* scoreText = static_cast<GX::GUI::TextWidget*>(ctx.FindById("score"));\nscoreText->SetText(L"スコア: " + std::to_wstring(score));',
  '• wstring で Unicode テキストを指定（日本語対応）\n• テキスト変更は layoutDirty を自動設定する\n• UIRenderer が設定されていないと intrinsic size が 0 になる'
],

'TextWidget-SetFontHandle': [
  'void SetFontHandle(int handle)',
  'フォントハンドルを設定する。FontManager::CreateFont() で作成したハンドルを指定する。フォントサイズや書体はハンドル作成時に決定される。未設定（-1）の場合はテキストが描画されない。',
  '// フォントハンドルを設定\nint font = fontManager.CreateFont(L"Yu Gothic UI", 18);\nauto text = std::make_unique<GX::GUI::TextWidget>();\ntext->SetFontHandle(font);\ntext->SetText(L"Hello!");',
  '• フォントハンドルは FontManager で事前に作成する必要がある\n• GUILoader 使用時は RegisterFont で名前→ハンドルの対応を登録する\n• -1 は無効ハンドルで、テキストは描画されない'
],

'TextWidget-SetTextColor': [
  'void SetRenderer(UIRenderer* renderer)',
  'UIRenderer を設定する。GetIntrinsicWidth/Height でテキスト幅を実測するために必要。GUILoader 使用時は自動設定されるが、手動でウィジェットを作成する場合は明示的に設定する。テキスト色は computedStyle.color で設定する。',
  '// UIRenderer を手動設定\nauto text = std::make_unique<GX::GUI::TextWidget>();\ntext->SetRenderer(&uiRenderer);\ntext->SetFontHandle(fontHandle);\ntext->SetText(L"Measured text");\n// テキスト色は computedStyle.color で指定\ntext->computedStyle.color = {1.0f, 0.8f, 0.0f, 1.0f};',
  '• GUILoader 使用時は SetRenderer が自動呼び出しされる\n• UIRenderer 未設定時は GetIntrinsicWidth/Height が 0 を返す\n• テキスト色は CSS で color: #FFFFFF のように指定可能'
],

// ========================================================================
// Button — page-Button
// ========================================================================

'Button-SetText': [
  'void SetText(const std::wstring& text)',
  'ボタンのラベルテキストを設定する。テキストは computedStyle の textAlign に従って配置される。ボタンの intrinsic size はテキスト幅 + パディングで計算される。',
  '// ボタンテキストを設定\nauto btn = std::make_unique<GX::GUI::Button>();\nbtn->SetText(L"Start Game");\nbtn->SetFontHandle(fontHandle);\nbtn->onClick = [&]() { StartGame(); };',
  '• wstring で Unicode テキストを指定（日本語対応）\n• テキスト幅が intrinsic size に反映される\n• CSS の text-align でテキスト配置を制御できる'
],

'Button-SetFontHandle': [
  'void SetFontHandle(int handle)',
  'フォントハンドルを設定する。FontManager::CreateFont() で作成したハンドルを指定する。テキスト描画と intrinsic size 計算に使用される。',
  '// フォントの設定\nint font = fontManager.CreateFont(L"Yu Gothic UI", 20);\nauto btn = std::make_unique<GX::GUI::Button>();\nbtn->SetFontHandle(font);\nbtn->SetText(L"OK");',
  '• フォントハンドルは FontManager で事前に作成する\n• GUILoader 使用時は RegisterFont で名前→ハンドルの対応を登録する'
],

'Button-hoverStyle': [
  'Style hoverStyle',
  'マウスホバー時に適用されるスタイル。StyleSheet の :hover 擬似クラスセレクタから自動設定される。ホバー時にボタンの背景色を変更するなどのビジュアルフィードバックに使用する。',
  '// ホバースタイルをコードで設定\nauto btn = std::make_unique<GX::GUI::Button>();\nbtn->hoverStyle.backgroundColor = {0.3f, 0.5f, 0.8f, 1.0f};\n// CSS: Button:hover { background-color: #4D80CC; }',
  '• StyleSheet の :hover 擬似クラスで自動設定される\n• ホバー中は computedStyle の代わりに hoverStyle の値が使用される\n• hoverStyle に設定されていないプロパティは computedStyle からフォールバックされる'
],

'Button-pressedStyle': [
  'Style pressedStyle',
  'マウス押下時に適用されるスタイル。StyleSheet の :pressed 擬似クラスセレクタから自動設定される。クリック中にボタンが押し込まれたように見せるビジュアルフィードバックに使用する。',
  '// プレススタイルをコードで設定\nauto btn = std::make_unique<GX::GUI::Button>();\nbtn->pressedStyle.backgroundColor = {0.15f, 0.3f, 0.6f, 1.0f};\n// CSS: Button:pressed { background-color: #264D99; }',
  '• StyleSheet の :pressed 擬似クラスで自動設定される\n• pressed 状態は hovered より優先される\n• マウスボタンが離されると hovered 状態に戻る'
],

'Button-disabledStyle': [
  'Style disabledStyle',
  '無効状態（enabled = false）時に適用されるスタイル。StyleSheet の :disabled 擬似クラスセレクタから自動設定される。ボタンが操作不可であることを視覚的に示す。',
  '// 無効時スタイル\nauto btn = std::make_unique<GX::GUI::Button>();\nbtn->disabledStyle.backgroundColor = {0.3f, 0.3f, 0.3f, 0.5f};\nbtn->enabled = false;\n// CSS: Button:disabled { background-color: #55555580; }',
  '• enabled = false のときに自動適用される\n• disabled 状態ではイベント処理も無効化される\n• StyleSheet の :disabled 擬似クラスで設定可能'
],

// ========================================================================
// Image — page-Image
// ========================================================================

'Image-SetTextureHandle': [
  'void SetTextureHandle(int handle)',
  'テクスチャハンドルを設定する。TextureManager::LoadTexture() で読み込んだハンドルを指定する。ウィジェットの描画領域にテクスチャが描画される。フィットモードに応じてアスペクト比が調整される。',
  '// テクスチャを設定\nint tex = textureManager.LoadTexture("Assets/logo.png");\nauto img = std::make_unique<GX::GUI::Image>();\nimg->SetTextureHandle(tex);\nimg->SetNaturalSize(256.0f, 128.0f);',
  '• TextureManager で事前にテクスチャを読み込む必要がある\n• -1 は無効ハンドルで画像が描画されない\n• GUILoader では RegisterTexture で名前→ハンドルの対応を登録する'
],

'Image-SetFitMode': [
  'void SetFit(ImageFit fit)',
  '画像のフィットモードを設定する。Stretch は矩形に引き伸ばし、Contain はアスペクト比を維持して収まるように縮小、Cover はアスペクト比を維持して矩形を覆うように拡大（はみ出し部分はクリップ）。',
  '// フィットモードを設定\nauto img = std::make_unique<GX::GUI::Image>();\nimg->SetTextureHandle(texHandle);\nimg->SetFit(GX::GUI::ImageFit::Contain);\n// アスペクト比を維持して収まるように配置',
  '• デフォルトは ImageFit::Stretch\n• Contain: 画像全体が見える（余白が生じる場合あり）\n• Cover: 矩形全体が画像で覆われる（画像の一部がクリップされる場合あり）\n• Cover 時は PushScissor でクリッピングされる'
],

// ========================================================================
// TextInput — page-TextInput
// ========================================================================

'TextInput-SetText': [
  'void SetText(const std::wstring& text) / const std::wstring& GetText() const',
  'テキスト入力の内容を設定・取得する。SetText でプログラム的にテキストを変更できる。GetText で現在の入力内容を取得する。ユーザーの入力操作でも内部テキストが更新される。',
  '// テキストの設定と取得\nauto input = std::make_unique<GX::GUI::TextInput>();\ninput->SetText(L"Initial value");\n\n// 値の取得\nauto* inp = static_cast<GX::GUI::TextInput*>(ctx.FindById("nameInput"));\nstd::wstring name = inp->GetText();',
  '• SetText はカーソル位置と選択範囲をリセットする\n• GetText は現在のテキスト内容への const 参照を返す\n• パスワードモードでも GetText は実際のテキストを返す'
],

'TextInput-SetPlaceholder': [
  'void SetPlaceholder(const std::wstring& text)',
  'プレースホルダーテキストを設定する。テキストが空の場合にグレーで表示されるヒントテキスト。入力が開始されると自動的に非表示になる。',
  '// プレースホルダーを設定\nauto input = std::make_unique<GX::GUI::TextInput>();\ninput->SetPlaceholder(L"名前を入力...");\ninput->SetFontHandle(fontHandle);',
  '• テキストが空のときのみ表示される\n• 半透明のグレーで描画される\n• フォーカス状態に関係なく、テキストが空なら表示される'
],

'TextInput-SetFontHandle': [
  'void SetFontHandle(int handle)',
  'フォントハンドルを設定する。テキスト描画、カーソル位置計算、テキスト幅計測に使用される。FontManager::CreateFont() で作成したハンドルを指定する。',
  '// フォントの設定\nint font = fontManager.CreateFont(L"Yu Gothic UI", 16);\nauto input = std::make_unique<GX::GUI::TextInput>();\ninput->SetFontHandle(font);\ninput->SetPlaceholder(L"Search...");',
  '• フォントハンドルは必須（-1 の場合テキストが描画されない）\n• カーソル位置の計算にもフォント情報が使用される'
],

'TextInput-SetMaxLength': [
  'void SetMaxLength(int maxLen)',
  '入力可能な最大文字数を設定する。0 の場合は制限なし。文字入力時にこの制限を超える入力は無視される。バリデーションやフォーム入力の制約に使用する。',
  '// 最大文字数を制限\nauto input = std::make_unique<GX::GUI::TextInput>();\ninput->SetMaxLength(20); // 最大20文字\ninput->SetPlaceholder(L"Username (max 20)");',
  '• 0 は制限なし（デフォルト）\n• ペースト時も制限が適用される（超過分は切り捨て）\n• SetText() による設定では制限は適用されない'
],

'TextInput-IsEditing': [
  'void SetPasswordMode(bool pw)',
  'パスワードモードを設定する。true の場合、入力テキストが伏字（*）で表示される。GetText() は実際のテキストを返す。ログインフォームやパスワード入力フィールドで使用する。',
  '// パスワードモードを有効化\nauto input = std::make_unique<GX::GUI::TextInput>();\ninput->SetPasswordMode(true);\ninput->SetPlaceholder(L"Password");\ninput->SetMaxLength(32);',
  '• 表示のみ伏字になり、内部テキストは平文のまま\n• GetText() は実際のパスワード文字列を返す\n• Ctrl+C（コピー）でも伏字がコピーされる'
],

'TextInput-onSubmit': [
  'std::function<void()> onSubmit',
  'Enter キー押下時に呼ばれるコールバック。フォーム送信やチャット入力の確定処理に使用する。コールバック内で GetText() を呼んで入力テキストを取得するのが一般的。',
  '// Submit コールバック\nauto input = std::make_unique<GX::GUI::TextInput>();\ninput->onSubmit = [&]() {\n    auto* inp = static_cast<GX::GUI::TextInput*>(ctx.FindById("chatInput"));\n    std::wstring msg = inp->GetText();\n    SendMessage(msg);\n    inp->SetText(L""); // クリア\n};',
  '• Enter キーで発火する\n• コールバック内で SetText(L"") を呼んで入力をクリアできる\n• GUILoader では直接登録できないため、BuildFromFile 後に手動設定する'
],

'TextInput-onTextChanged': [
  'std::function<void(const std::string&)> onValueChanged',
  'テキスト変更時に親の onValueChanged が呼ばれる。Widget 基底クラスの onValueChanged を使用し、入力中のリアルタイムバリデーションやフィルタリングに活用できる。',
  '// テキスト変更の監視\nauto input = std::make_unique<GX::GUI::TextInput>();\ninput->onValueChanged = [](const std::string& val) {\n    // 入力内容が変わるたびに呼ばれる\n};\n// GUILoader: onValueChanged="onSearchChange"',
  '• キー入力、ペースト、削除の都度呼ばれる\n• GUILoader では RegisterValueChangedEvent で登録する\n• 値は std::string で渡される（wstring から変換済み）'
],

// ========================================================================
// Slider — page-Slider
// ========================================================================

'Slider-SetValue': [
  'void SetValue(float v) / float GetValue() const',
  'スライダーの値を設定・取得する。値は SetRange で設定した min〜max の範囲にクランプされる。SetStep が設定されている場合はステップ値にスナップされる。値変更時は onValueChanged が呼ばれる。',
  '// スライダーの値を設定\nauto slider = std::make_unique<GX::GUI::Slider>();\nslider->SetRange(0.0f, 100.0f);\nslider->SetValue(50.0f);\n\n// 値の取得\nauto* s = static_cast<GX::GUI::Slider*>(ctx.FindById("volume"));\nfloat vol = s->GetValue();',
  '• 値は min〜max の範囲に自動クランプされる\n• SetStep(0) はステップなし（連続値）\n• ドラッグ操作中も値が連続的に更新される\n• デフォルト範囲は 0.0〜1.0'
],

'Slider-SetTrackColor': [
  'void SetRange(float minVal, float maxVal) / void SetStep(float step)',
  'スライダーの値範囲とステップを設定する。SetRange で最小値と最大値を指定する。SetStep でステップ値を設定すると、値がステップの倍数にスナップされる。0 はステップなし。',
  '// 範囲とステップの設定\nauto slider = std::make_unique<GX::GUI::Slider>();\nslider->SetRange(0.0f, 10.0f);\nslider->SetStep(0.5f); // 0, 0.5, 1.0, ... 10.0\n\n// CSS でトラック色を指定\n// Slider { background-color: #333; }',
  '• デフォルト範囲は 0.0〜1.0、ステップは 0（連続）\n• トラック色は computedStyle.backgroundColor、フィル色は別途描画\n• ステップ値は (value - min) を step で丸めて適用される\n• intrinsic size はデフォルトで 200x24px'
],

// ========================================================================
// CheckBox — page-CheckBox
// ========================================================================

'CheckBox-SetChecked': [
  'void SetChecked(bool checked) / bool IsChecked() const',
  'チェック状態を設定・取得する。SetChecked で true/false を設定すると onValueChanged コールバックが呼ばれる。クリック操作でもトグルされる。UI上では 18px のチェックボックスとラベルテキストが表示される。',
  '// チェック状態の設定\nauto cb = std::make_unique<GX::GUI::CheckBox>();\ncb->SetText(L"Enable Sound");\ncb->SetChecked(true);\ncb->onValueChanged = [](const std::string& val) {\n    bool enabled = (val == "1");\n};',
  '• クリック操作で自動トグルされる（OnEvent で処理）\n• onValueChanged には "1"（チェック時）/ "0"（アンチェック時）が渡される\n• チェックボックスのサイズは 18x18px 固定（k_BoxSize）\n• ラベルとの間隔は 8px（k_Gap）'
],

'CheckBox-SetLabel': [
  'void SetText(const std::wstring& text)',
  'チェックボックスのラベルテキストを設定する。テキストはチェックボックスの右側に表示される。テキスト変更時に layoutDirty が true になり、intrinsic width が再計算される。',
  '// ラベルを設定\nauto cb = std::make_unique<GX::GUI::CheckBox>();\ncb->SetText(L"BGM を有効にする");\ncb->SetFontHandle(fontHandle);',
  '• ラベルは 18px ボックスの右側に 8px の間隔で表示される\n• intrinsic width = ボックスサイズ + gap + テキスト幅\n• UIRenderer が SetRenderer で設定されていないとテキスト幅が 0 になる'
],

'CheckBox-SetFontHandle': [
  'void SetFontHandle(int handle)',
  'フォントハンドルを設定する。ラベルテキストの描画と intrinsic size 計算に使用される。FontManager::CreateFont() で作成したハンドルを指定する。',
  '// フォントの設定\nint font = fontManager.CreateFont(L"Yu Gothic UI", 14);\nauto cb = std::make_unique<GX::GUI::CheckBox>();\ncb->SetFontHandle(font);\ncb->SetText(L"Option");',
  '• GUILoader 使用時は RegisterFont で名前→ハンドルの対応を登録する\n• -1 は無効ハンドルでラベルが描画されない'
],

// ========================================================================
// RadioButton — page-RadioButton
// ========================================================================

'RadioButton-SetSelected': [
  'void SetSelected(bool selected) / bool IsSelected() const',
  '選択状態を設定・取得する。SetSelected(true) を呼ぶと同一親内の他の RadioButton は自動的に DeselectSiblings で非選択になる。UI上では 18px の円形ラジオボタンとラベルが表示される。',
  '// ラジオボタンの初期選択\nauto rb1 = std::make_unique<GX::GUI::RadioButton>();\nrb1->SetText(L"Easy");\nrb1->SetValue("easy");\nrb1->SetSelected(true); // 初期選択\n\nauto rb2 = std::make_unique<GX::GUI::RadioButton>();\nrb2->SetText(L"Hard");\nrb2->SetValue("hard");',
  '• SetSelected(true) は同一親内の他 RadioButton を自動解除する\n• 選択時、親の onValueChanged に SetValue で設定した値文字列が送信される\n• クリック操作でも選択が切り替わる\n• 円形ラジオボタンは 18px（k_CircleSize）で描画される'
],

'RadioButton-SetLabel': [
  'void SetText(const std::wstring& text)',
  'ラジオボタンのラベルテキストを設定する。テキストはラジオボタン円形の右側に表示される。テキスト変更時に layoutDirty が true になる。',
  '// ラベルを設定\nauto rb = std::make_unique<GX::GUI::RadioButton>();\nrb->SetText(L"Normal Mode");\nrb->SetFontHandle(fontHandle);\nrb->SetValue("normal");',
  '• ラベルは 18px 円形の右側に 8px の間隔で表示される\n• wstring で Unicode テキストを指定\n• UIRenderer の SetRenderer が必要（intrinsic size 計算用）'
],

'RadioButton-SetValue': [
  'void SetValue(const std::string& value)',
  '値文字列を設定する。RadioButton が選択されたとき、親ウィジェットの onValueChanged コールバックにこの値が送信される。ラジオボタングループの値を識別するために使用する。',
  '// 値の設定と取得\nauto rb = std::make_unique<GX::GUI::RadioButton>();\nrb->SetValue("option_a");\nrb->SetText(L"Option A");\n\n// 親パネルで値変更を受け取る\npanel->onValueChanged = [](const std::string& val) {\n    // val == "option_a" or "option_b"\n};',
  '• 親の onValueChanged で選択された RadioButton の値を受け取る\n• GUILoader では XML の value 属性で設定する\n• GetValue() で設定済みの値文字列を取得できる'
],

// ========================================================================
// DropDown — page-DropDown
// ========================================================================

'DropDown-SetItems': [
  'void SetItems(const std::vector<std::string>& items)',
  '選択肢のリストを設定する。内部で wstring 版も同時に作成される。既存の選択インデックスはリセットされない。ドロップダウンポップアップに表示されるアイテムリストを定義する。',
  '// アイテムの設定\nauto dd = std::make_unique<GX::GUI::DropDown>();\ndd->SetItems({"Easy", "Normal", "Hard", "Expert"});\ndd->SetSelectedIndex(1); // "Normal" を初期選択\ndd->SetFontHandle(fontHandle);',
  '• 内部で std::wstring 版（m_wideItems）も自動生成される\n• GUILoader では XML の items 属性でカンマ区切りで指定する\n• 各アイテムの高さは 28px（k_ItemHeight）固定'
],

'DropDown-SetSelectedIndex': [
  'void SetSelectedIndex(int index) / int GetSelectedIndex() const',
  '選択中のアイテムインデックスを設定・取得する。0始まりのインデックスで、SetItems で設定した配列に対応する。選択変更時に onValueChanged が呼ばれる。',
  '// 選択インデックスの操作\nauto* dd = static_cast<GX::GUI::DropDown*>(ctx.FindById("difficulty"));\ndd->SetSelectedIndex(2); // "Hard" を選択\nint idx = dd->GetSelectedIndex(); // 2',
  '• 範囲外のインデックスを設定した場合の動作は未定義\n• ユーザーがポップアップでアイテムをクリックしても更新される\n• onValueChanged には選択されたアイテムのテキストが渡される'
],

'DropDown-IsOpen': [
  'bool IsOpen() const',
  'ポップアップが展開中かどうかを返す。true の場合、ドロップダウンリストが表示されている。ポップアップはクリックで開閉し、FocusLost で自動的に閉じる。',
  '// ポップアップ状態の確認\nauto* dd = static_cast<GX::GUI::DropDown*>(ctx.FindById("dropdown"));\nif (dd->IsOpen()) {\n    // ポップアップ表示中\n}',
  '• クリックでトグル（開閉切替）される\n• FocusLost イベントで自動的に閉じる\n• ポップアップは DeferDraw で遅延描画される（他のウィジェットの上に表示）\n• intrinsic size はデフォルトで 150x30px'
],

// ========================================================================
// ListView — page-ListView
// ========================================================================

'ListView-SetItems': [
  'void SetItems(const std::vector<std::string>& items)',
  'リストのアイテムを設定する。内部で wstring 版も作成される。表示範囲内のアイテムのみ描画される最適化が適用されている。大量のアイテムでも効率的に表示可能。',
  '// アイテムの設定\nauto lv = std::make_unique<GX::GUI::ListView>();\nstd::vector<std::string> items;\nfor (int i = 0; i < 100; ++i)\n    items.push_back("Item " + std::to_string(i));\nlv->SetItems(items);\nlv->SetFontHandle(fontHandle);',
  '• 内部で wstring 版が自動生成される\n• 表示範囲外のアイテムは描画されない（仮想スクロール）\n• 各アイテムの高さは 28px（k_ItemHeight）固定\n• GUILoader では XML の items 属性でカンマ区切りで指定する'
],

'ListView-SetSelectedIndex': [
  'void SetSelectedIndex(int index) / int GetSelectedIndex() const',
  '選択アイテムのインデックスを設定・取得する。-1 は未選択状態。クリック操作でもアイテムが選択される。選択変更時に onValueChanged コールバックが呼ばれる。',
  '// アイテム選択\nauto* lv = static_cast<GX::GUI::ListView*>(ctx.FindById("fileList"));\nlv->SetSelectedIndex(0); // 最初のアイテムを選択\nint sel = lv->GetSelectedIndex(); // 0',
  '• -1 は未選択状態（デフォルト）\n• クリック操作で自動的に選択が更新される\n• onValueChanged には選択アイテムのテキストが渡される\n• マウスホイールでスクロール、クリックで選択'
],

'ListView-SetFontHandle': [
  'void SetFontHandle(int handle)',
  'フォントハンドルを設定する。リストアイテムのテキスト描画に使用される。FontManager::CreateFont() で作成したハンドルを指定する。',
  '// フォントの設定\nint font = fontManager.CreateFont(L"Yu Gothic UI", 14);\nauto lv = std::make_unique<GX::GUI::ListView>();\nlv->SetFontHandle(font);\nlv->SetItems({"Item 1", "Item 2", "Item 3"});',
  '• GUILoader では RegisterFont で名前→ハンドルの対応を登録する\n• -1 は無効ハンドルでテキストが描画されない\n• intrinsic size はデフォルトで 200x150px'
],

// ========================================================================
// ScrollView — page-ScrollView
// ========================================================================

'ScrollView-SetScrollSpeed': [
  'void SetScrollSpeed(float speed)',
  'マウスホイール1ノッチあたりのスクロール量（ピクセル）を設定する。デフォルトは 30.0f。値が大きいほどスクロールが速くなる。子ウィジェットの合計高さがScrollViewの高さを超えた場合にスクロールが有効になる。',
  '// スクロール速度の設定\nauto sv = std::make_unique<GX::GUI::ScrollView>();\nsv->SetScrollSpeed(50.0f); // 速めのスクロール\nsv->computedStyle.overflow = GX::GUI::OverflowMode::Scroll;\n// 子ウィジェットを追加\nfor (int i = 0; i < 20; ++i) {\n    auto text = std::make_unique<GX::GUI::TextWidget>();\n    text->SetText(L"Line " + std::to_wstring(i));\n    sv->AddChild(std::move(text));\n}',
  '• デフォルトは 30.0f px/ノッチ\n• overflow は Scroll が自動設定される\n• スクロールバーは 4px 幅で右端に自動表示される\n• スクロール範囲は子ウィジェットの合計高さからScrollViewの高さを引いた値にクランプされる'
],

// ========================================================================
// ProgressBar — page-ProgressBar
// ========================================================================

'ProgressBar-SetValue': [
  'void SetValue(float v) / float GetValue() const',
  '進捗値を設定・取得する。値は 0.0〜1.0 の範囲に自動クランプされる。0.0 で空、1.0 で完全に満たされたバーが表示される。ロード進捗やHP表示などに使用する。',
  '// プログレスバーの更新\nauto pb = std::make_unique<GX::GUI::ProgressBar>();\npb->SetValue(0.0f);\n\n// ロード進捗の更新\nauto* bar = static_cast<GX::GUI::ProgressBar*>(ctx.FindById("loadBar"));\nbar->SetValue(loadProgress); // 0.0 ~ 1.0',
  '• 値は自動的に 0.0〜1.0 にクランプされる\n• 表示専用ウィジェット（ユーザー操作不可）\n• intrinsic size はデフォルトで 200x20px\n• バー色は SetBarColor() でカスタマイズ可能'
],

'ProgressBar-SetBarColor': [
  'void SetBarColor(const StyleColor& c)',
  'フィルバーの色を設定する。デフォルトは青系（0.3, 0.6, 1.0, 1.0）。トラック（背景）色は computedStyle.backgroundColor で設定する。',
  '// バー色のカスタマイズ\nauto pb = std::make_unique<GX::GUI::ProgressBar>();\npb->SetBarColor({0.2f, 0.8f, 0.2f, 1.0f}); // 緑色\npb->computedStyle.backgroundColor = {0.2f, 0.2f, 0.2f, 1.0f}; // 暗い背景',
  '• デフォルトのバー色は青系（0.3f, 0.6f, 1.0f, 1.0f）\n• トラック色は computedStyle.backgroundColor で別途設定する\n• CSS では直接 bar-color を設定する方法はないため、コードで設定する'
],

// ========================================================================
// TabView — page-TabView
// ========================================================================

'TabView-SetTabNames': [
  'void SetTabNames(const std::vector<std::string>& names)',
  'タブヘッダーに表示する名前のリストを設定する。名前の数は子ウィジェットの数と対応する必要がある。各タブ名はヘッダー領域（高さ 32px）に横並びで表示される。',
  '// タブ名の設定\nauto tv = std::make_unique<GX::GUI::TabView>();\ntv->SetTabNames({"General", "Graphics", "Audio"});\ntv->SetFontHandle(fontHandle);\n// 各タブに対応する子パネルを追加\ntv->AddChild(std::move(generalPanel));\ntv->AddChild(std::move(graphicsPanel));\ntv->AddChild(std::move(audioPanel));',
  '• タブ名の数と子ウィジェットの数は一致させる\n• GUILoader では XML の tabs 属性でカンマ区切りで指定する\n• 内部で wstring 版が自動生成される\n• タブヘッダーの高さは 32px（k_TabHeaderHeight）固定'
],

'TabView-SetActiveTab': [
  'void SetActiveTab(int index) / int GetActiveTab() const',
  'アクティブなタブのインデックスを設定・取得する。対応する子ウィジェットの visible が true に、他は false に設定される。タブヘッダーのクリックでも切り替わる。',
  '// アクティブタブの設定\nauto* tv = static_cast<GX::GUI::TabView*>(ctx.FindById("settings"));\ntv->SetActiveTab(1); // "Graphics" タブを表示\nint active = tv->GetActiveTab(); // 1',
  '• 0始まりのインデックス（デフォルトは0）\n• 非アクティブタブの子ウィジェットは visible = false に設定される\n• タブヘッダーのクリックでも自動切り替えされる\n• onValueChanged にはタブ名が渡される'
],

'TabView-SetFontHandle': [
  'void SetFontHandle(int handle)',
  'タブヘッダーのテキスト描画に使用するフォントハンドルを設定する。FontManager::CreateFont() で作成したハンドルを指定する。',
  '// フォントの設定\nint font = fontManager.CreateFont(L"Yu Gothic UI", 14);\nauto tv = std::make_unique<GX::GUI::TabView>();\ntv->SetFontHandle(font);\ntv->SetTabNames({"Tab 1", "Tab 2"});',
  '• GUILoader では RegisterFont で名前→ハンドルを登録する\n• タブヘッダーのテキスト描画とホバー領域計算に使用される\n• intrinsic size はデフォルトで 300x200px'
],

// ========================================================================
// Dialog — page-Dialog
// ========================================================================

'Dialog-Show': [
  'void Show() / void Hide()',
  'ダイアログの表示・非表示を切り替える。Show() で visible = true、Hide() で visible = false に設定する。Dialog はデフォルトで visible = false（非表示）。モーダルダイアログとして画面全体にオーバーレイを表示する。',
  '// ダイアログの表示と非表示\nauto* dlg = static_cast<GX::GUI::Dialog*>(ctx.FindById("confirmDialog"));\ndlg->Show(); // 表示\n\n// 閉じるボタンのコールバック\ndlg->onClose = [dlg]() {\n    dlg->Hide();\n};',
  '• デフォルトは非表示（visible = false）\n• Show() はオーバーレイ（半透明背景）も含めて表示する\n• オーバーレイクリックで onClose コールバックが呼ばれる\n• コンテンツは画面中央に配置される'
],

'Dialog-SetTitle': [
  'void SetTitle(const std::wstring& title)',
  'ダイアログのタイトルを設定する。タイトルはダイアログ上部に描画される。設定は任意で、タイトルなしのダイアログも作成可能。',
  '// タイトルの設定\nauto dlg = std::make_unique<GX::GUI::Dialog>();\ndlg->SetTitle(L"Confirm");\ndlg->SetFontHandle(fontHandle);\ndlg->onClose = [&]() { dlg->Hide(); };',
  '• タイトルはダイアログコンテンツの上部に描画される\n• フォントハンドルの設定が必要\n• 空文字列を設定するとタイトルなしになる'
],

'Dialog-SetOverlayColor': [
  'void SetOverlayColor(float r, float g, float b, float a)',
  'オーバーレイ（背景）の色を設定する。デフォルトは半透明黒（0, 0, 0, 0.5）。モーダル感を強調するためにアルファ値を調整する。',
  '// オーバーレイ色の設定\nauto dlg = std::make_unique<GX::GUI::Dialog>();\ndlg->SetOverlayColor(0.0f, 0.0f, 0.0f, 0.7f); // より暗いオーバーレイ',
  '• デフォルトは黒の半透明（alpha=0.5）\n• 画面全体を覆うオーバーレイとして描画される\n• オーバーレイクリックで onClose が呼ばれる'
],

'Dialog-onClose': [
  'std::function<void()> onClose',
  'オーバーレイ領域（ダイアログコンテンツ外）のクリック時に呼ばれるコールバック。通常は Hide() を呼んでダイアログを閉じる処理を実装する。',
  '// 閉じるコールバックの設定\nauto dlg = std::make_unique<GX::GUI::Dialog>();\ndlg->onClose = [&]() {\n    auto* d = static_cast<GX::GUI::Dialog*>(ctx.FindById("myDialog"));\n    d->Hide();\n};',
  '• オーバーレイ（コンテンツ外）のクリックで発火する\n• GUILoader では onClick 属性で設定可能\n• コンテンツ内のクリックでは発火しない'
],

// ========================================================================
// Canvas — page-Canvas
// ========================================================================

'Canvas-onDraw': [
  'std::function<void(UIRenderer&, const LayoutRect&)> onDraw',
  'カスタム描画コールバック。Canvas の Render 内で毎フレーム呼ばれ、UIRenderer と Canvas の globalRect を受け取る。ミニマップ、グラフ、カスタムグラフィックスなどの自由描画に使用する。',
  '// Canvas の描画コールバック\nauto canvas = std::make_unique<GX::GUI::Canvas>();\ncanvas->onDraw = [](GX::GUI::UIRenderer& r, const GX::GUI::LayoutRect& rect) {\n    // 背景を描画\n    r.DrawSolidRect(rect.x, rect.y, rect.width, rect.height,\n                    {0.0f, 0.2f, 0.0f, 1.0f});\n    // カスタム描画...\n};\n// GUILoader: RegisterDrawCallback("minimap", handler)',
  '• 毎フレームの Render で呼ばれるため、状態の更新にも使用可能\n• UIRenderer の全描画メソッド（DrawRect, DrawText, DrawImage 等）が使用可能\n• GUILoader では RegisterDrawCallback で名前→ハンドラを登録する\n• intrinsic size はデフォルトで 100x100px'
],

// ========================================================================
// Spacer — page-Spacer
// ========================================================================

'Spacer-SetSize': [
  'void SetSize(float w, float h)',
  '空白スペーサーのサイズを設定する。指定した幅と高さが GetIntrinsicWidth/Height で返される。レイアウト内でウィジェット間にスペースを確保するために使用する。描画は行われない（Render は空実装）。',
  '// スペーサーでレイアウト調整\nauto spacer = std::make_unique<GX::GUI::Spacer>();\nspacer->SetSize(0.0f, 20.0f); // 高さ20pxの空白\npanel->AddChild(std::move(spacer));\n\n// flexGrow で余白を埋めるスペーサー\nauto filler = std::make_unique<GX::GUI::Spacer>();\nfiller->computedStyle.flexGrow = 1.0f;\npanel->AddChild(std::move(filler));',
  '• Render は何も描画しない（空実装）\n• flexGrow = 1.0f と組み合わせて余白を埋めるスペーサーとして使用可能\n• GUILoader では XML の width / height 属性でサイズを指定する\n• 見えないウィジェットのため、デバッグ時はレイアウト境界表示が有用'
]

}
