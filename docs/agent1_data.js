// Agent 1: Core + Graphics/Device + Graphics/Pipeline
// Auto-generated API reference data
const AGENT1_DATA = {

// ============================================================
// Application (Core/Application.h)
// ============================================================
'Application-AppConfig': [
  'struct AppConfig',
  'アプリケーション初期化用の設定構造体。\nウィンドウタイトル、サイズ、リサイズ可能フラグを保持し、Application::Initialize() で使用されます。',
  '// アプリケーション設定\nGX::AppConfig config;\nconfig.title = L"My Game";\nconfig.width = 1920;\nconfig.height = 1080;\nconfig.resizable = true;',
  '• デフォルトタイトルは L"GXLib Application"、サイズは 1280x720\n• resizable のデフォルトは true\n• width/height はクライアント領域のサイズであり、ウィンドウ全体のサイズではない'
],

'Application-Initialize': [
  'bool Initialize(const AppConfig& config)',
  '指定された設定を使用してWin32ウィンドウを作成し、アプリケーションを初期化します。\n他のApplicationメソッドの前に呼び出す必要があります。ウィンドウ作成に失敗した場合はfalseを返します。',
  '// アプリケーションの初期化\nGX::Application app;\nGX::AppConfig config;\nconfig.title = L"Hello GXLib";\nconfig.width = 1280;\nconfig.height = 720;\nif (!app.Initialize(config)) return -1;',
  '• Run() や ProcessMessages() の前に1回呼び出すこと\n• 内部でWindowの作成とTimerの初期化を行う\n• Win32ウィンドウ作成に失敗した場合はfalseを返す\n• 成功後、GetWindow().GetHWND() でHWNDを取得可能'
],

'Application-ProcessMessages': [
  'bool ProcessMessages()',
  '内部Windowを通じてWindowsメッセージ（WM_QUIT、WM_SIZE等）を処理します。\nユーザーがウィンドウを閉じた場合にfalseを返し、アプリケーションの終了を通知します。',
  '// メッセージ処理によるメインループ\nwhile (app.ProcessMessages()) {\n    float dt = app.GetTimer().GetDeltaTime();\n    Update(dt);\n    Render();\n}',
  '• ゲームループの先頭でフレームごとに1回呼び出す\n• WM_QUIT を受信するとfalseを返す\n• Window::AddMessageCallback で登録されたコールバックはここでディスパッチされる'
],

'Application-Shutdown': [
  'void Shutdown()',
  'Applicationが保持するすべてのリソースを解放し、クリーンアップを行います。\nウィンドウを破棄し、内部状態をリセットします。アプリケーション終了時に呼び出してください。',
  '// クリーンシャットダウン\napp.Shutdown();',
  '• Applicationオブジェクトのスコープ終了前に必ず呼び出すこと\n• 複数回呼び出しても安全\n• GPUの処理が完了していることを確認してから呼び出すこと'
],

'Application-GetWindow': [
  'Window& GetWindow()',
  '内部Windowオブジェクトへの参照を返します。\nHWND、ウィンドウサイズの取得や、入力ルーティング用のメッセージコールバック登録に使用します。',
  '// GraphicsDevice初期化用にウィンドウハンドルを取得\nHWND hwnd = app.GetWindow().GetHWND();\nuint32_t w = app.GetWindow().GetWidth();\nuint32_t h = app.GetWindow().GetHeight();',
  '• 返される参照はApplicationの生存期間中有効\n• AddMessageCallback() で入力メッセージコールバックを登録するのに使用\n• HWNDはGraphicsDeviceとSwapChainの初期化に必要'
],

'Application-IsRunning': [
  'bool IsRunning() const',
  'アプリケーションが現在実行中かどうかを返します。\nウィンドウが閉じられたか、RequestQuit() が呼ばれた後にfalseになります。',
  '// 実行状態の確認\nif (app.IsRunning()) {\n    // ゲームループを続行\n}',
  '• Initialize() 成功後からウィンドウ終了またはRequestQuit()までtrueを返す\n• 外部ループ制御パターンに有用'
],

'Application-RequestQuit': [
  'void RequestQuit()',
  'アプリケーションの終了を要求します。次のProcessMessages() またはIsRunning() の呼び出しでfalseが返されます。\nメニューの終了ボタンなどから、プログラム的にアプリケーションを終了する場合に使用します。',
  '// コードからの終了\nif (userPressedEscape) {\n    app.RequestQuit();\n}',
  '• ウィンドウは即座には破棄されず、現在のフレームは完了する\n• 次のProcessMessages() 呼び出しでfalseを返す\n• ユーザーがウィンドウを閉じるのと同等'
],

// ============================================================
// Window (Core/Window.h)
// ============================================================
'Window-Create': [
  'bool Create(const std::wstring& title, uint32_t w, uint32_t h, bool resizable)',
  '指定されたタイトルとクライアント領域のサイズでWin32ウィンドウを作成・表示します。\nウィンドウクラスの登録、HWNDの作成、表示を行います。失敗時はfalseを返します。',
  '// ウィンドウの作成\nGX::Window window;\nif (!window.Create(L"My Game", 1280, 720, true)) {\n    return -1;\n}',
  '• 通常はApplication::Initialize() から内部的に呼ばれる\n• width/height はクライアント領域のサイズ（ボーダー含むウィンドウ全体ではない）\n• ウィンドウクラス名はプロセスごとに1回登録される'
],

'Window-Destroy': [
  'void Destroy()',
  'Win32ウィンドウを破棄し、関連リソースを解放します。\nWindowのデストラクタで自動的に呼ばれますが、明示的なクリーンアップのために手動で呼ぶことも可能です。',
  '// 明示的なウィンドウ破棄\nwindow.Destroy();',
  '• 複数回呼び出しても安全\n• ~Window() で自動的に呼ばれる\n• この呼び出し後、すべてのメッセージコールバックは無効になる'
],

'Window-ProcessMessages': [
  'bool ProcessMessages()',
  'Win32メッセージキューをポンプし、保留中のすべてのメッセージをディスパッチします。\nWM_QUIT を受信するとfalseを返し、ウィンドウが閉じられることを示します。',
  '// メッセージポンプループ\nwhile (window.ProcessMessages()) {\n    // 更新と描画\n}',
  '• ウィンドウの応答性を維持するため、毎フレーム呼び出す必要がある\n• 登録済みのメッセージコールバックを各メッセージに対してディスパッチする\n• WM_SIZE はリサイズコールバックが設定されている場合にトリガーされる'
],

'Window-GetHWND': [
  'HWND GetHWND() const',
  'ネイティブWin32ウィンドウハンドル（HWND）を返します。\nDirectX 12のスワップチェーンやその他のWin32 API呼び出しの初期化に必要です。',
  '// グラフィックス初期化にHWNDを渡す\nHWND hwnd = window.GetHWND();\ngraphicsDevice.Initialize(hwnd, width, height);',
  '• Create() 呼び出し前はnullptrを返す\n• HWNDはDestroy() が呼ばれるまで有効\n• SwapChainとGraphicsDeviceの初期化に必要'
],

'Window-GetWidth': [
  'uint32_t GetWidth() const',
  'クライアント領域の現在の幅をピクセル単位で返します。\nクライアント領域はタイトルバー、ボーダー、その他のウィンドウ装飾を含みません。',
  '// ビューポート設定用にサイズを取得\nuint32_t w = window.GetWidth();\nuint32_t h = window.GetHeight();\nfloat aspect = (float)w / (float)h;',
  '• ウィンドウリサイズ時に自動更新される\n• 初期値はCreate() で設定された値\n• ビューポートやシザー矩形の計算に使用'
],

'Window-GetHeight': [
  'uint32_t GetHeight() const',
  'クライアント領域の現在の高さをピクセル単位で返します。\nウィンドウ装飾（タイトルバー、ボーダー）を含みません。',
  '// アスペクト比の計算\nfloat aspect = (float)window.GetWidth() / (float)window.GetHeight();',
  '• リサイズ時に自動更新される\n• Create() 呼び出し前は0を返す\n• スワップチェーンのバックバッファの高さと一致する'
],

'Window-IsMinimized': [
  'bool IsMinimized() const',
  'ウィンドウが現在最小化されているかどうかを返します。\nウィンドウが表示されていない場合にレンダリングをスキップするのに有用です。',
  '// 最小化時はレンダリングをスキップ\nif (!window.IsMinimized()) {\n    device.BeginFrame();\n    // ... 描画 ...\n    device.EndFrame();\n}',
  '• 最小化されたウィンドウへのレンダリングはスワップチェーンのPresentで問題を引き起こす可能性がある\n• BeginFrame/EndFrame 呼び出し前にチェックすること\n• 最小化時はウィンドウサイズが0になる場合がある'
],

'Window-ShouldClose': [
  'bool ShouldClose() const',
  'ウィンドウが閉じるリクエストを受け取ったかどうかを返します。\nWM_CLOSE または WM_DESTROY を受信するとtrueに設定されます。',
  '// 代替ループパターン\nwhile (!window.ShouldClose()) {\n    window.ProcessMessages();\n    Update();\n    Render();\n}',
  '• ProcessMessages() の戻り値チェックの代替として使用可能\n• ウィンドウプロシージャがクローズイベントで設定する'
],

'Window-SetTitle': [
  'void SetTitle(const std::wstring& title)',
  'ウィンドウのタイトルバーテキストを実行時に変更します。\nFPS、フレーム時間、デバッグ情報をタイトルバーに表示するのによく使用されます。',
  '// タイトルバーにFPSを表示\nstd::wstring title = L"My Game - FPS: " + std::to_wstring((int)timer.GetFPS());\nwindow.SetTitle(title);',
  '• 毎フレーム呼び出しても最小限のオーバーヘッド\n• デバッグビルドでパフォーマンス指標を表示するのに便利\n• 内部的にWin32 SetWindowTextW を使用'
],

'Window-AddMessageCallback': [
  'void AddMessageCallback(std::function<bool(HWND, UINT, WPARAM, LPARAM)> callback)',
  '生のWin32メッセージ（WM_KEYDOWN、WM_MOUSEMOVEなど）を受信するコールバックを登録します。\n入力クラスがキーボード、マウス、ゲームパッドのメッセージをインターセプトするために使用します。コールバックがtrueを返すとメッセージは処理済みとみなされます。',
  '// 入力メッセージハンドラの登録\nwindow.AddMessageCallback([&input](HWND hwnd, UINT msg, WPARAM w, LPARAM l) {\n    return input.ProcessMessage(hwnd, msg, w, l);\n});',
  '• 複数のコールバックを登録可能。登録順に呼ばれる\n• コールバックがtrueを返すと、後続のコールバックへの伝搬が停止する\n• InputManager、Keyboard、Mouseがメッセージルーティングに使用\n• TextInputウィジェットへのWM_CHARルーティングにも使用'
],

'Window-SetResizeCallback': [
  'void SetResizeCallback(std::function<void(uint32_t, uint32_t)> callback)',
  'ウィンドウリサイズ時に呼び出されるコールバックを設定します。\nコールバックは新しいクライアント領域の幅と高さを受け取ります。スワップチェーンやレンダーターゲットのリサイズに使用します。',
  '// リサイズ処理\nwindow.SetResizeCallback([&device](uint32_t w, uint32_t h) {\n    device.OnResize(w, h);\n});',
  '• 同時にアクティブにできるリサイズコールバックは1つだけ（最後に設定したものが有効）\n• ProcessMessages() 中にWM_SIZEを受信すると呼ばれる\n• ウィンドウが最小化されている場合、width/height は0になる場合がある'
],

// ============================================================
// Timer (Core/Timer.h)
// ============================================================
'Timer-Initialize': [
  'void Initialize()',
  'パフォーマンスカウンタの周波数を取得して高精度タイマーを初期化します。\nすべての内部カウンタをリセットします。ゲームループに入る前に1回呼び出してください。',
  '// タイマーの初期化\nGX::Timer timer;\ntimer.Initialize();',
  '• 内部的にQueryPerformanceFrequencyを使用\n• 最初のUpdate() の前に呼び出す必要がある\n• デルタタイム、合計時間、フレームカウントをゼロにリセット'
],

'Timer-Update': [
  'void Update()',
  '現在のフレームのタイマー状態を更新します。\n前回呼び出しからのデルタタイムを計算し、FPSを更新します。毎フレーム1回呼び出す必要があります。',
  '// フレームごとのタイマー更新\ntimer.Update();\nfloat dt = timer.GetDeltaTime();\nUpdateGame(dt);',
  '• フレームの先頭で正確に1回呼び出すこと\n• デルタタイムは大きなスパイク（ブレークポイント後など）を防ぐため内部的にクランプされる\n• FPSはローリング平均として計算される'
],

'Timer-GetDeltaTime': [
  'float GetDeltaTime() const',
  '前のフレームからの経過時間を秒単位で返します。\nフレームレート非依存の移動、アニメーション、物理演算の更新に使用します。',
  '// フレームレート非依存の移動\nfloat dt = timer.GetDeltaTime();\nposition.x += velocity.x * dt;\nposition.y += velocity.y * dt;',
  '• 最初のUpdate() 呼び出し前は0.0fを返す\n• 一般的な値: 60FPSで約0.016、30FPSで約0.033\n• スムーズな動きのために速度や加速度にdeltaTimeを掛ける\n• 大きなスパイク（デバッグ中の一時停止など）はクランプされる場合がある'
],

'Timer-GetTotalTime': [
  'float GetTotalTime() const',
  'Initialize() 呼び出しからの合計経過時間を秒単位で返します。\n時間ベースのアニメーション、シェーダーパラメータ、経過時間表示に有用です。',
  '// サイン波アニメーション\nfloat t = timer.GetTotalTime();\nfloat offset = sinf(t * 2.0f) * 50.0f;',
  '• 初期化時点から単調増加する\n• シェーダーのtime uniform やプロシージャルアニメーションに有用\n• 内部的にはdouble精度で、floatとして返される'
],

'Timer-GetFPS': [
  'float GetFPS() const',
  '現在のフレームレート（FPS）を平滑化された平均値として返します。\nパフォーマンス監視やデバッグオーバーレイでの表示に有用です。',
  '// FPSの表示\nstd::wstring title = L"FPS: " + std::to_wstring((int)timer.GetFPS());\nwindow.SetTitle(title);',
  '• 安定性のため約1秒間の平均値\n• 実行開始後最初の1秒間は0.0fを返す\n• 正確なフレーム単位のタイミングには不向き。GetDeltaTime() を使用すること'
],

'Timer-GetFrameCount': [
  'uint64_t GetFrameCount() const',
  '初期化以降に処理されたフレームの総数を返します。\nフレームベースのロジック、デバッグログ、テンポラルエフェクトに有用です。',
  '// 60フレームごとに実行\nif (timer.GetFrameCount() % 60 == 0) {\n    GX_LOG_INFO("Frame: %llu", timer.GetFrameCount());\n}',
  '• Update() が呼ばれるたびに1ずつ増加\n• Initialize() 後は0から開始\n• フレームベースのインデックス（Haltonジッターシーケンス等）に使用可能'
],

// ============================================================
// Logger (Core/Logger.h)
// ============================================================
'Logger-Info': [
  'static void Info(const char* fmt, ...)',
  'printf形式のフォーマットで情報ログメッセージを出力します。\nOutputDebugStringを介してVisual Studioの出力ウィンドウに表示し、オプションでログファイルにも書き込みます。',
  '// 情報メッセージの出力\nGX::Logger::Info("Initialized: %dx%d", width, height);\nGX::Logger::Info("Loaded %d textures", count);\n// マクロを使用:\nGX_LOG_INFO("Frame: %d", frameCount);',
  '• スレッドセーフ。どのスレッドからでも呼び出し可能\n• 出力に[INFO]プレフィックスが付く\n• 簡便なGX_LOG_INFOマクロを使用可能\n• OutputDebugStringとコンソール（利用可能な場合）の両方に出力'
],

'Logger-Warn': [
  'static void Warn(const char* fmt, ...)',
  '警告ログメッセージを出力します。実行を妨げない潜在的な問題を示します。\n非推奨APIの使用、フォールバック動作、パフォーマンス警告に有用です。',
  '// 警告の出力\nGX::Logger::Warn("Texture not found: %s, using fallback", path);\nGX_LOG_WARN("FPS dropped below 30: %.1f", fps);',
  '• 出力に[WARN]プレフィックスが付く\n• 致命的ではないが注意が必要な状況に使用\n• 簡便なGX_LOG_WARNマクロを使用可能'
],

'Logger-Error': [
  'static void Error(const char* fmt, ...)',
  'エラーログメッセージを出力します。機能に影響を与える障害を示します。\nリソース作成の失敗、ファイルの欠落、無効な状態に使用します。',
  '// エラーの出力\nGX::Logger::Error("Failed to create PSO: %s", name);\nGX_LOG_ERROR("Shader compilation failed: %s", errorMsg.c_str());',
  '• 出力に[ERROR]プレフィックスが付く\n• 調査が必要な重大な障害に使用\n• 簡便なGX_LOG_ERRORマクロを使用可能\n• 呼び出し元の関数でfalseを返す/例外をスローすることも検討'
],

'Logger-SetLogFile': [
  'static void SetLogFile(const std::string& path)',
  'ログ出力のファイルパスを設定します。以降のすべてのログメッセージがこのファイルにも書き込まれます。\nリリースビルドでのログ記録や事後デバッグに有用です。',
  '// ファイルログの有効化\nGX::Logger::SetLogFile("debug.log");\nGX::Logger::Info("This goes to both debugger and file");',
  '• ファイルは追記モードで開かれる\n• すべてのメッセージをキャプチャするために、ログ出力の前に設定すること\n• 空文字列を渡すとファイル出力を無効化'
],

'Logger-SetLogLevel': [
  'static void SetLogLevel(LogLevel level)',
  '出力の最小ログレベルを設定します。このレベル未満のメッセージは抑制されます。\n例えば、Warnに設定するとInfoメッセージが抑制されます。',
  '// 警告とエラーのみ表示\nGX::Logger::SetLogLevel(GX::LogLevel::Warn);\nGX::Logger::Info("suppressed");  // 出力されない\nGX::Logger::Warn("shown");       // 出力される',
  '• デフォルトはLogLevel::Info（すべてのメッセージを表示）\n• リリースビルドでノイズを削減するのに有用\n• SetLogFileが有効な場合、ファイル出力には影響しない'
],

'Logger-LogLevel': [
  'enum class LogLevel { Info, Warn, Error }',
  'ログメッセージの重大度レベルを定義します。\nInfoが最も低い重大度、Errorが最も高い重大度です。SetLogLevelで出力をフィルタリングするのに使用します。',
  '// 最小レベルを警告に設定\nGX::Logger::SetLogLevel(GX::LogLevel::Warn);',
  '• Info: 一般的な情報メッセージ（初期化、状態変更）\n• Warn: 動作を妨げない潜在的な問題\n• Error: 機能に影響を与える障害'
],

// ============================================================
// FrameAllocator (Core/FrameAllocator.h)
// ============================================================
'FrameAllocator-FrameAllocator': [
  'explicit FrameAllocator(size_t capacity = 1048576)',
  '指定された容量（バイト単位）でフレームアロケータを構築します。\nデフォルト容量は1MBです。キャッシュフレンドリーなアクセスのため、メモリは単一の連続ブロックとして確保されます。',
  '// 2MBのフレームアロケータを作成\nGX::FrameAllocator allocator(2 * 1024 * 1024);',
  '• デフォルト容量は1MB（1024 * 1024バイト）\n• バッファ全体がコンストラクタで事前確保される\n• 最悪ケースのフレームあたり一時確保量に基づいて容量を選択すること\n• コピー禁止、ムーブ可能'
],

'FrameAllocator-Allocate': [
  'void* Allocate(size_t size, size_t alignment = 16)',
  '指定されたバイト数を指定アラインメントでリニアバッファから確保します。\n内部オフセットポインタをバンプするだけのO(1)操作です。容量を超えた場合はnullptrを返します。',
  '// 生メモリの確保\nvoid* mem = allocator.Allocate(1024);\nif (mem) {\n    // このフレームでメモリを使用\n}',
  '• デフォルトアラインメントは16バイト（SIMD向け）\n• 残り容量が不足する場合はnullptrを返す\n• 個別のFree() は不要。フレーム終了時にReset() を呼ぶ\n• 確保されたメモリは次のReset() 呼び出しまでのみ有効'
],

'FrameAllocator-Allocate2': [
  'template<typename T> T* Allocate(size_t count = 1)',
  '型安全な確保で、指定された型に対して適切にアラインされたポインタを返します。\nsizeof(T) * countバイトをalignof(T)アラインメントで確保します。コンストラクタは呼び出しません。',
  '// 型付き配列の確保\nfloat* sortKeys = allocator.Allocate<float>(256);\nDirectX::XMFLOAT4* positions = allocator.Allocate<DirectX::XMFLOAT4>(100);',
  '• 正しい型アラインメントのためalignof(T) を使用\n• コンストラクタを呼び出さない。必要に応じてplacement newを使用\n• 容量不足の場合はnullptrを返す\n• POD型や配列に適している'
],

'FrameAllocator-Reset': [
  'void Reset()',
  '内部オフセットをゼロに戻してアロケータをリセットします。\n以前に確保されたすべてのメモリが無効になります。各フレームの先頭で1回呼び出してください。',
  '// フレーム開始時\nallocator.Reset();\n// このフレームの一時データを確保\nfloat* temp = allocator.Allocate<float>(128);',
  '• O(1)操作。オフセットカウンタをリセットするだけ\n• 以前に返されたすべてのポインタがダングリングになる\n• 確保されたオブジェクトのデストラクタは呼び出さない\n• フレーム開始時、フレーム単位の確保の前に呼ぶこと'
],

'FrameAllocator-GetUsedSize': [
  'size_t GetUsedSize() const',
  '現在確保されているバイト数（アラインメントパディング含む）を返します。\nメモリ使用量の監視やアロケータ容量が十分かどうかの検出に有用です。',
  '// 使用量の監視\nGX_LOG_INFO("Frame alloc: %zu / %zu bytes",\n    allocator.GetUsedSize(), allocator.GetCapacity());',
  '• アラインメントパディングを含む\n• Reset() 呼び出し後は0にリセットされる\n• 頻繁に容量に近づく場合はアロケータサイズの増加を検討'
],

'FrameAllocator-GetCapacity': [
  'size_t GetCapacity() const',
  'アロケータの総容量をバイト単位で返します。\nコンストラクタに渡された値です。',
  '// 残り容量の確認\nsize_t remaining = allocator.GetCapacity() - allocator.GetUsedSize();',
  '• 構築時に固定され、リサイズ不可\n• デフォルトは1MB\n• 容量を超える確保はnullptrを返す'
],

// ============================================================
// PoolAllocator (Core/PoolAllocator.h)
// ============================================================
'PoolAllocator-PoolAllocator': [
  'PoolAllocator(size_t blockSize, size_t blockCount)',
  '固定サイズオブジェクト用のプールアロケータを作成します。テンプレートパラメータでオブジェクト型Tとブロックサイズを定義します。\nBlockSizeオブジェクト単位のブロックでメモリを確保し、フリーリストパターンでオンデマンドに拡張します。',
  '// Widgetオブジェクト用プール（ブロックあたり64個）\nGX::PoolAllocator<GX::Widget, 64> widgetPool;',
  '• BlockSizeテンプレートパラメータのデフォルトはブロックあたり64オブジェクト\n• フリーリストが空の場合、新しいブロックがオンデマンドで確保される\n• 誤った複製を防ぐためコピー禁止\n• デストラクタで全確保ブロックを解放'
],

'PoolAllocator-Allocate': [
  'void* Allocate()',
  'プールからメモリブロックを1つO(1)時間で確保します。\nフリーリストが空の場合、BlockSizeオブジェクト分の新しいブロックが自動的に確保されます。',
  '// プールからの生メモリ確保\nvoid* mem = pool.Allocate();\nauto* widget = new (mem) GX::Widget();',
  '• 償却O(1)。フリーリストの先頭からポップ\n• 空の場合は新しいブロックを確保して自動拡張\n• 生メモリを返す。コンストラクタは呼ばれない\n• 確保+構築の組み合わせにはNew<T>() を使用'
],

'PoolAllocator-Free': [
  'void Free(void* ptr)',
  '以前に確保されたブロックをO(1)時間でフリーリストに返却します。\nデストラクタを呼び出しません。破棄+解放の組み合わせにはDelete() を使用してください。',
  '// 手動ライフサイクル\nvoid* mem = pool.Allocate();\nauto* obj = new (mem) MyObject();\nobj->~MyObject();\npool.Free(mem);',
  '• O(1)操作。フリーリストの先頭にプッシュ\n• デストラクタを呼び出さない。手動で呼ぶかDelete() を使用\n• nullptrの渡しは安全（no-op）\n• 二重解放は未定義動作'
],

'PoolAllocator-Reset': [
  'void Reset()',
  'すべてのブロックをリセットし、確保されたすべてのメモリをフリーリストに返却します。\nデストラクタを呼び出しません。以前に返されたすべてのポインタが無効になります。',
  '// プール全体のリセット\npool.Reset();\n// 以前に確保されたすべてのオブジェクトが無効になる',
  '• アクティブなオブジェクトのデストラクタを呼び出さない\n• 参照が残っていないことを確認して慎重に使用すること\n• バッチクリーンアップのシナリオに有用'
],

'PoolAllocator-GetFreeCount': [
  'size_t GetFreeCount() const',
  'プール内の空き（利用可能な）ブロック数を返します。\nプールの使用率監視やキャパシティプランニングに有用です。',
  '// プール使用率の確認\nGX_LOG_INFO("Pool: %zu active, %zu capacity",\n    pool.GetActiveCount(), pool.GetCapacity());',
  '• フリーリスト内のブロックのみをカウント\n• AllocateBlock() で新しいブロックが確保されると増加\n• 0の場合、次のAllocate() で新しいブロック確保がトリガーされる'
],

// ============================================================
// GraphicsDevice (Graphics/Device/GraphicsDevice.h)
// ============================================================
'GraphicsDevice-Initialize': [
  'bool Initialize(HWND hwnd, uint32_t w, uint32_t h)',
  'DirectX 12グラフィックスデバイスを初期化します。D3D12デバイス、DXGIファクトリ、アダプタ、コマンドキュー、コマンドリスト、スワップチェーン、ディスクリプタヒープを作成します。\nすべてのGPUリソース作成のメインエントリーポイントです。いずれかのステップが失敗するとfalseを返します。',
  '// D3D12の初期化\nGX::GraphicsDevice device;\nif (!device.Initialize(hwnd, 1280, 720)) {\n    GX_LOG_ERROR("D3D12 init failed");\n    return -1;\n}',
  '• Debugビルドでは自動的にデバッグレイヤーを有効化\n• 最もパフォーマンスの高いGPUアダプタを選択\n• CBV_SRV_UAV、RTV、DSVディスクリプタヒープを作成\n• ウィンドウ作成後に呼び出す必要がある（有効なHWNDが必要）'
],

'GraphicsDevice-Shutdown': [
  'void Shutdown()',
  'デバイス、スワップチェーン、コマンドオブジェクト、ディスクリプタヒープを含むすべてのD3D12リソースを解放します。\n処理中のオブジェクトの使用を防ぐため、リソース解放前にGPUをフラッシュします。',
  '// クリーンシャットダウン\ndevice.WaitForGPU();\ndevice.Shutdown();',
  '• Shutdown() の前にWaitForGPU() を呼び、すべてのGPU処理の完了を確認すること\n• Shutdown() 後、すべてのD3D12オブジェクト参照は無効になる\n• 複数回呼び出しても安全'
],

'GraphicsDevice-BeginFrame': [
  'void BeginFrame()',
  '現在のフレームインデックス用のコマンドアロケータとコマンドリストをリセットして新しいフレームを開始します。\nバックバッファをレンダーターゲット状態に遷移させます。EndFrame() と対で使用する必要があります。',
  '// フレームレンダリング\ndevice.BeginFrame();\nauto* cmdList = device.GetCommandList().Get();\n// ... 描画コマンドの記録 ...\ndevice.EndFrame();',
  '• 現在のフレームインデックスのコマンドアロケータをリセット（ダブルバッファ）\n• バックバッファをPRESENTからRENDER_TARGETに遷移\n• 各フレームの描画コマンド前に呼び出す必要がある\n• EndFrame() と対になり、遷移の復帰とPresent() を行う'
],

'GraphicsDevice-EndFrame': [
  'void EndFrame()',
  'コマンドリストを閉じて実行し、スワップチェーンをプレゼントして現在のフレームを終了します。\nバックバッファをプレゼント状態に遷移させ、同期用のフェンスをシグナルします。',
  '// フレーム終了\ndevice.EndFrame();\n// バックバッファがディスプレイに表示される',
  '• バックバッファをRENDER_TARGETからPRESENTに遷移\n• コマンドリストを閉じてコマンドキューで実行\n• SwapChain::Present() を呼び出し、フェンスをシグナル\n• 前のフレームがGPUで未完了の場合はブロックする（ダブルバッファ同期）'
],

'GraphicsDevice-WaitForGPU': [
  'void WaitForGPU()',
  '以前にサブミットされたすべてのGPU処理が完了するまでCPUをブロックします。\nリソース破棄、シャットダウン、リサイズ操作の前にアクセス違反を防ぐために使用します。',
  '// リサイズ前の待機\ndevice.WaitForGPU();\ndevice.OnResize(newWidth, newHeight);',
  '• CPUストールが発生する。通常のレンダリングでは毎フレーム呼び出さないこと\n• GPUが使用中の可能性があるリソースの破棄前に必須\n• Shutdown() やOnResize() の前に呼び出すこと\n• 内部的にコマンドキューのフェンスをシグナルして待機'
],

'GraphicsDevice-GetDevice': [
  'ID3D12Device* GetDevice() const',
  'GPUリソース作成に使用するD3D12デバイスの生ポインタを返します。\nバッファ、テクスチャ、パイプラインステート、ルートシグネチャ等のD3D12オブジェクト作成に必要です。',
  '// デバイスを使用してバッファを作成\nID3D12Device* d3dDevice = device.GetDevice();\nbuffer.Initialize(d3dDevice, size, stride);',
  '• Initialize() 呼び出し前はnullptrを返す\n• ポインタはShutdown() が呼ばれるまで有効\n• ほとんどのGXLibサブシステムの初期化に必要\n• 返されたポインタに対してRelease() を呼ばないこと'
],

'GraphicsDevice-GetCommandQueue': [
  'CommandQueue& GetCommandQueue()',
  'GPU処理の投入に使用する内部CommandQueueへの参照を返します。\nフェンス同期やコマンドリストの直接実行に必要です。',
  '// 追加のコマンドリストを実行\nGX::CommandQueue& queue = device.GetCommandQueue();\nqueue.ExecuteCommandList(cmdList.Get());',
  '• 単一のDirectコマンドキュー（D3D12_COMMAND_LIST_TYPE_DIRECT）\n• 返される参照はGraphicsDeviceの生存期間中有効\n• BeginFrame/EndFrame で内部的に使用される'
],

'GraphicsDevice-GetCommandList': [
  'CommandList& GetCommandList()',
  '描画コマンドの記録用に、現在のフレームのCommandListへの参照を返します。\nコマンドリストはBeginFrame() でリセットされ、EndFrame() で閉じられます。',
  '// 描画コマンドの記録\nauto* cmdList = device.GetCommandList().Get();\ncmdList->SetPipelineState(pso.Get());\ncmdList->DrawInstanced(3, 1, 0, 0);',
  '• BeginFrame() とEndFrame() の間でのみ有効\n• ダブルバッファのコマンドアロケータを使用（k_AllocatorCount = 2）\n• ->演算子がオーバーロードされており、ID3D12GraphicsCommandListに直接アクセス可能'
],

'GraphicsDevice-GetSwapChain': [
  'SwapChain& GetSwapChain()',
  'バックバッファを管理するスワップチェーンへの参照を返します。\nバックバッファリソース、RTVハンドル、サイズ、フォーマットのアクセスに使用します。',
  '// バックバッファ情報の取得\nauto& swapChain = device.GetSwapChain();\nDXGI_FORMAT fmt = swapChain.GetFormat();\nuint32_t idx = swapChain.GetCurrentBackBufferIndex();',
  '• k_BufferCount = 2 のダブルバッファ\n• デフォルトフォーマットはDXGI_FORMAT_R8G8B8A8_UNORM\n• バックバッファインデックスは0と1の間で交互に切り替わる'
],

'GraphicsDevice-GetSRVHeap': [
  'DescriptorHeap& GetSRVHeap()',
  'シェーダー可視のCBV/SRV/UAVディスクリプタヒープを返します。\nレンダリング中にテクスチャやコンスタントバッファをシェーダーにバインドするために使用します。',
  '// レンダリング用にSRVヒープをバインド\nauto& srvHeap = device.GetSRVHeap();\nID3D12DescriptorHeap* heaps[] = { srvHeap.GetHeap() };\ncmdList->SetDescriptorHeaps(1, heaps);',
  '• シェーダー可視ヒープ。SetDescriptorHeaps() でバインド可能\n• D3D12では同時にバインドできるCBV_SRV_UAVヒープは1つのみ\n• ディスクリプタ解放時のインデックス再利用にフリーリストを使用\n• TextureManager、FontManager、その他のシステムがこのヒープから確保'
],

'GraphicsDevice-GetRTVHeap': [
  'DescriptorHeap& GetRTVHeap()',
  'レンダーターゲットビュー（RTV）ディスクリプタヒープを返します。\nオフスクリーンレンダリング、ポストエフェクト、レイヤーシステム用のレンダーターゲットビュー作成に使用します。',
  '// オフスクリーンレンダーターゲット用RTVの作成\nauto& rtvHeap = device.GetRTVHeap();\nuint32_t idx = rtvHeap.AllocateIndex();\ndevice.GetDevice()->CreateRenderTargetView(resource, nullptr, rtvHeap.GetCPUHandle(idx));',
  '• シェーダー非可視（CPUのみのヒープ）\n• SwapChain、RenderTarget、RenderLayerが使用\n• レンダーターゲット破棄時にインデックスのFree() を忘れないこと'
],

'GraphicsDevice-GetDSVHeap': [
  'DescriptorHeap& GetDSVHeap()',
  'デプスステンシルビュー（DSV）ディスクリプタヒープを返します。\n深度テストやシャドウマップ用のデプスバッファビュー作成に使用します。',
  '// DSVの作成\nauto& dsvHeap = device.GetDSVHeap();\nuint32_t idx = dsvHeap.AllocateIndex();\ndevice.GetDevice()->CreateDepthStencilView(depthResource, &dsvDesc, dsvHeap.GetCPUHandle(idx));',
  '• シェーダー非可視（CPUのみのヒープ）\n• DepthBufferとシャドウマップシステムが使用\n• PointShadowMapは6面レンダリング用に独自のDSVヒープを使用'
],

'GraphicsDevice-GetFrameIndex': [
  'uint32_t GetFrameIndex() const',
  'ダブルバッファリソース管理用の現在のバックバッファインデックス（0または1）を返します。\n正しいコマンドアロケータ、動的バッファ領域、フェンス値の選択に使用します。',
  '// ダブルバッファリソースにフレームインデックスを使用\nuint32_t fi = device.GetFrameIndex();\ndynamicBuffer.Update(fi, data, size);',
  '• 各フレームで0と1の間を交互に切り替わる\n• ダブルバッファのDynamicBuffer更新に必須\n• SwapChain::GetCurrentBackBufferIndex() と一致する'
],

'GraphicsDevice-OnResize': [
  'void OnResize(uint32_t w, uint32_t h)',
  'GPUをフラッシュし、新しいサイズでスワップチェーンバッファを再作成してウィンドウリサイズを処理します。\nバックバッファの同期を維持するため、ウィンドウサイズ変更時に呼び出す必要があります。',
  '// コールバックでリサイズを処理\nwindow.SetResizeCallback([&device](uint32_t w, uint32_t h) {\n    if (w > 0 && h > 0)\n        device.OnResize(w, h);\n});',
  '• リサイズ前に内部的にWaitForGPU() を呼び出す\n• widthまたはheightが0の場合はスキップ（ウィンドウ最小化時）\n• レンダーターゲット、デプスバッファ、ポストエフェクトパイプラインもリサイズすること\n• スワップチェーンRTVは自動的に再作成される'
],

// ============================================================
// CommandQueue (Graphics/Device/CommandQueue.h)
// ============================================================
'CommandQueue-Initialize': [
  'bool Initialize(ID3D12Device* device)',
  'Directコマンドキューと関連するフェンスをGPU同期用に作成します。\nコマンドキューはすべてのグラフィックスコマンドリストの投入ポイントです。',
  '// コマンドキューの作成\nGX::CommandQueue queue;\nif (!queue.Initialize(device.GetDevice())) {\n    GX_LOG_ERROR("CommandQueue init failed");\n}',
  '• デフォルトでD3D12_COMMAND_LIST_TYPE_DIRECTキューを作成\n• 内部Fenceオブジェクトも初期化する\n• 通常はGraphicsDeviceにより内部的に管理される'
],

'CommandQueue-ExecuteCommandList': [
  'void ExecuteCommandList(ID3D12CommandList* cmdList)',
  '単一のコマンドリストをGPUに投入して実行します。\nコマンドリストはこのメソッド呼び出し前に閉じた状態でなければなりません。',
  '// コマンドリストの実行\ncmdList.Close();\nqueue.ExecuteCommandList(cmdList.Get());',
  '• 実行前にコマンドリストが閉じられている（Close() 済み）必要がある\n• コマンドはGPUにより非同期に処理される\n• 完了待ちにはFlush() またはフェンス同期を使用\n• 複数のコマンドリストをExecuteCommandLists() でバッチ処理可能'
],

'CommandQueue-Signal': [
  'uint64_t Signal(Fence& fence)',
  'コマンドキューにフェンスシグナルを挿入します。シグナルされるフェンス値を返します。\n投入された処理の完了を知るためのCPU-GPU同期に使用します。',
  '// 実行後のシグナル\nqueue.ExecuteCommandList(cmdList.Get());\nuint64_t fenceVal = queue.Signal(fence);',
  '• シグナル前にフェンス値がインクリメントされる\n• 返された値でWaitForFence() を使用して完了を待機\n• EndFrame() で自動的に呼ばれる'
],

'CommandQueue-WaitForFence': [
  'void WaitForFence(Fence& fence, uint64_t value)',
  'GPUが指定されたフェンス値に到達するまでCPUをブロックします。\nSignal() の後に、結果アクセス前にGPU処理の完了を保証するために使用します。',
  '// 特定のGPU操作を待機\nuint64_t val = queue.Signal(fence);\nqueue.WaitForFence(fence, val);\n// GPU処理が完了',
  '• CPUストールが発生する。メインレンダーループでの使用を最小限に\n• GPUの結果読み出し前（プロファイラのタイムスタンプ等）に必須\n• Win32イベント待機で実装'
],

'CommandQueue-GetQueue': [
  'ID3D12CommandQueue* GetQueue() const',
  'D3D12コマンドキューの生ポインタを返します。\nSwapChainの作成やDirectキュー参照が必要な他のD3D12 APIに必要です。',
  '// スワップチェーン初期化に渡す\nswapChain.Initialize(factory, device, queue.GetQueue(), desc);',
  '• Initialize() 呼び出し前はnullptrを返す\n• 返されたポインタに対してRelease() を呼ばないこと\n• GPUProfilerのタイムスタンプ周波数取得に必要'
],

// ============================================================
// CommandList (Graphics/Device/CommandList.h)
// ============================================================
'CommandList-Initialize': [
  'bool Initialize(ID3D12Device* device)',
  'グラフィックスコマンドリストとダブルバッファリング用の2つのコマンドアロケータを作成します。\n各アロケータはバックバッファの1フレームに対応し、GPU使用中の競合を防ぎます。',
  '// コマンドリストの作成\nGX::CommandList cmdList;\nif (!cmdList.Initialize(device.GetDevice())) {\n    GX_LOG_ERROR("CommandList init failed");\n}',
  '• ダブルバッファリング用にk_AllocatorCount（2）個のコマンドアロケータを作成\n• コマンドリストの型はD3D12_COMMAND_LIST_TYPE_DIRECT\n• 作成後、コマンドリストは閉じた状態で開始される\n• 通常はGraphicsDeviceにより内部的に管理される'
],

'CommandList-Begin': [
  'void Begin(uint32_t frameIndex)',
  '指定されたフレームインデックスのコマンドアロケータをリセットし、コマンドリストを記録用に開きます。\n各フレームの開始時、描画コマンド発行前に呼び出す必要があります。',
  '// 記録開始\nuint32_t fi = device.GetFrameIndex();\ncmdList.Begin(fi);\n// ... コマンドの記録 ...\ncmdList.End();',
  '• frameIndexは0または1（ダブルバッファ）でなければならない\n• リセット前に前フレームのアロケータがGPU上で完了している必要がある\n• 内部的にID3D12CommandAllocator::Reset() とID3D12GraphicsCommandList::Reset() を呼ぶ\n• GraphicsDevice::BeginFrame() で自動的に呼ばれる'
],

'CommandList-End': [
  'void End()',
  'コマンドリストを閉じ、記録されたコマンドを実行用にファイナライズします。\nコマンドリストをコマンドキューに投入する前に呼び出す必要があります。',
  '// 記録終了\ncmdList.End();\nqueue.ExecuteCommandList(cmdList.Get());',
  '• Close() 後はコマンドを記録できなくなる\n• ExecuteCommandList() の前に呼び出す必要がある\n• GraphicsDevice::EndFrame() で自動的に呼ばれる\n• 既に閉じたリストに対するEnd() 呼び出しはエラー'
],

'CommandList-Get': [
  'ID3D12GraphicsCommandList* Get() const',
  '描画コマンド記録用のD3D12グラフィックスコマンドリストの生ポインタを返します。\nオーバーロードされた->演算子でもアクセス可能で、より自然な構文で使用できます。',
  '// D3D12コマンドの直接記録\nauto* cmd = cmdList.Get();\ncmd->SetPipelineState(pso.Get());\ncmd->SetGraphicsRootSignature(rootSig.Get());\ncmd->DrawInstanced(3, 1, 0, 0);',
  '• オーバーロードされた->演算子でcmdList->DrawInstanced() のように使用可能\n• ポインタはCommandListオブジェクトの生存期間中有効\n• 返されたポインタに対してRelease() を呼ばないこと'
],

'CommandList-ResourceBarrier': [
  'void ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)',
  'コマンドリストにリソース状態遷移バリアを記録します。\n異なるパイプラインステージ間でリソース使用を同期するためにD3D12が要求します。',
  '// レンダリング用にレンダーターゲットを遷移\ncmdList.ResourceBarrier(renderTarget,\n    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,\n    D3D12_RESOURCE_STATE_RENDER_TARGET);',
  '• D3D12はリソース状態変更に明示的なバリアを要求する\n• 一般的な遷移: PRESENT ↔ RENDER_TARGET、PIXEL_SHADER_RESOURCE ↔ RENDER_TARGET\n• 複数バリアにはBarrierBatchを使用するとGPU効率が向上\n• バリアの欠落はGPUバリデーションエラーと未定義動作を引き起こす'
],

// ============================================================
// SwapChain (Graphics/Device/SwapChain.h)
// ============================================================
'SwapChain-Initialize': [
  'bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue, HWND hwnd, uint32_t w, uint32_t h)',
  'ダブルバッファのバックバッファと関連RTVを持つDXGIスワップチェーンを作成します。\nスワップチェーンはティアリングのないディスプレイ出力のためのフロント/バックバッファの切り替えを管理します。',
  '// スワップチェーンの作成\nGX::SwapChain swapChain;\nswapChain.Initialize(device, queue.GetQueue(), hwnd, 1280, 720);',
  '• ダブルバッファリング用にk_BufferCount（2）個のバックバッファを作成\n• フォーマットはDXGI_FORMAT_R8G8B8A8_UNORM（LDR出力）\n• 最高パフォーマンスのためDXGI_SWAP_EFFECT_FLIP_DISCARDを使用\n• バックバッファディスクリプタ用のプライベートRTVヒープを作成'
],

'SwapChain-Present': [
  'void Present(bool vsync = true)',
  'フロントバッファとバックバッファを入れ替えて、レンダリングされたフレームをディスプレイに表示します。\nvsyncがtrueの場合、ティアリングを防ぐためにVBLANKを待機します。',
  '// vsync付きプレゼント\nswapChain.Present(true);\n// vsyncなしプレゼント（FPS上限なし）\nswapChain.Present(false);',
  '• vsync=true: SyncInterval=1（ほとんどのモニタで60Hz）\n• vsync=false: SyncInterval=0（FPS上限なし、ティアリングの可能性あり）\n• GraphicsDevice::EndFrame() で自動的に呼ばれる\n• Present() 後にバックバッファインデックスが変更される'
],

'SwapChain-Resize': [
  'void Resize(ID3D12Device* device, uint32_t w, uint32_t h)',
  '新しいウィンドウサイズに合わせてスワップチェーンのバックバッファをリサイズします。\n処理中の参照がないことを保証するため、WaitForGPU() の後に呼び出す必要があります。',
  '// リサイズハンドラ\ndevice.WaitForGPU();\nswapChain.Resize(device.GetDevice(), newWidth, newHeight);',
  '• 呼び出し前にすべてのバックバッファ参照を解放する必要がある\n• RTVは内部的に再作成される\n• widthまたはheightが0の場合はスキップ（ウィンドウ最小化時）\n• 依存するレンダーターゲットやデプスバッファもリサイズすること'
],

'SwapChain-GetBackBuffer': [
  'ID3D12Resource* GetBackBuffer(uint32_t index) const',
  '指定されたバックバッファインデックスのD3D12リソースを返します。\nリソースバリアや直接リソースアクセスに使用します。',
  '// 現在のバックバッファを遷移\nID3D12Resource* bb = swapChain.GetBackBuffer(swapChain.GetCurrentBackBufferIndex());\ncmdList.ResourceBarrier(bb, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);',
  '• インデックスは0または1（k_BufferCount = 2）でなければならない\n• スワップチェーンが未初期化の場合はnullptrを返す\n• 返されたポインタに対してRelease() を呼ばないこと'
],

'SwapChain-GetBackBufferRTV': [
  'D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferRTV(uint32_t index) const',
  '指定されたバックバッファのRTVディスクリプタハンドルを返します。\nバックバッファを最終出力のレンダーターゲットとして設定するために使用します。',
  '// バックバッファをレンダーターゲットに設定\nauto rtv = swapChain.GetBackBufferRTV(swapChain.GetCurrentBackBufferIndex());\ncmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);',
  '• 各バックバッファには事前作成されたRTVがある\n• ハンドルはスワップチェーンのプライベートRTVヒープを指す\n• 正しい出力のために現在のバックバッファインデックスと一致させること'
],

'SwapChain-GetCurrentBackBufferIndex': [
  'uint32_t GetCurrentBackBufferIndex() const',
  'レンダリング先の現在のバックバッファのインデックス（0または1）を返します。\nPresent() 呼び出し後に各フレームで交互に切り替わります。',
  '// 現在のバックバッファインデックスを取得\nuint32_t idx = swapChain.GetCurrentBackBufferIndex();\nauto rtv = swapChain.GetBackBufferRTV(idx);',
  '• IDXGISwapChain3::GetCurrentBackBufferIndex() の値を返す\n• 各Present() 後に0と1の間で交互に切り替わる\n• 正しいバックバッファリソースとRTVの選択に使用'
],

'SwapChain-GetFormat': [
  'DXGI_FORMAT GetFormat() const',
  'バックバッファのDXGIフォーマットを返します。\nバックバッファに直接レンダリングするパイプラインステートの作成時に必要です。',
  '// スワップチェーンフォーマットをPSOに使用\npsoBuilder.SetRenderTargetFormat(swapChain.GetFormat());',
  '• デフォルトフォーマットはDXGI_FORMAT_R8G8B8A8_UNORM\n• HDRパイプラインは中間レンダーターゲットにR16G16B16A16_FLOATを使用\n• 最終トーンマップ/FXAA出力はスワップチェーンのLDRフォーマットを対象とする'
],

'SwapChain-GetWidth': [
  'uint32_t GetWidth() const',
  'バックバッファの幅をピクセル単位で返します。',
  '// ビューポートの設定\nD3D12_VIEWPORT vp = { 0, 0, (float)swapChain.GetWidth(), (float)swapChain.GetHeight(), 0, 1 };',
  '• Resize() 後に更新される\n• ウィンドウクライアント領域の幅と一致する\n• ビューポートやシザー矩形のサイズに使用'
],

'SwapChain-GetHeight': [
  'uint32_t GetHeight() const',
  'バックバッファの高さをピクセル単位で返します。',
  '// アスペクト比\nfloat aspect = (float)swapChain.GetWidth() / (float)swapChain.GetHeight();',
  '• Resize() 後に更新される\n• ウィンドウクライアント領域の高さと一致する'
],

// ============================================================
// DescriptorHeap (Graphics/Device/DescriptorHeap.h)
// ============================================================
'DescriptorHeap-Initialize': [
  'bool Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count, bool shaderVisible)',
  '指定された型とサイズのD3D12ディスクリプタヒープを作成します。\nシェーダー可視ヒープはパイプラインにバインドしてGPUアクセスが可能です。',
  '// シェーダー可視SRVヒープの作成\nGX::DescriptorHeap srvHeap;\nsrvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, true);',
  '• レンダリングで使用するCBV_SRV_UAVヒープはshaderVisibleをtrueにする必要がある\n• RTVとDSVヒープはシェーダー可視にしない\n• D3D12では同時にバインドできるシェーダー可視CBV_SRV_UAVヒープは1つのみ\n• countがディスクリプタの最大数を決定する'
],

'DescriptorHeap-Allocate': [
  'int Allocate()',
  'ヒープから次の利用可能なディスクリプタインデックスを確保します。\nカウンタをインクリメントする前に、以前に解放されたインデックスをフリーリストから再利用します。',
  '// ディスクリプタスロットの確保\nint idx = srvHeap.Allocate();\ndevice->CreateShaderResourceView(texture, &srvDesc, srvHeap.GetCPUHandle(idx));',
  '• まずフリーリストからインデックスを返し、その後カウンタをインクリメント\n• 返されたインデックスはGetCPUHandle() とGetGPUHandle() で使用\n• ディスクリプタが不要になったらインデックスをFree() すること\n• ヒープが満杯の場合は-1を返す'
],

'DescriptorHeap-Free': [
  'void Free(int index)',
  'ディスクリプタインデックスをフリーリストに返却して再利用可能にします。\nリソース（テクスチャ、バッファ）が破棄されディスクリプタが不要になった時に呼び出します。',
  '// テクスチャ破棄時にディスクリプタを解放\nsrvHeap.Free(textureDescriptorIndex);',
  '• 解放されたインデックスは将来のAllocate() 呼び出しで返される可能性がある\n• 二重解放は未定義動作\n• 長時間実行アプリケーションでのディスクリプタヒープ枯渇防止に必須\n• TextureManagerとFontManagerがクリーンアップ時にこれを呼ぶ'
],

'DescriptorHeap-GetCPUHandle': [
  'D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(int index) const',
  '指定されたインデックスのCPUディスクリプタハンドルを返します。\nビューの作成（CreateShaderResourceView、CreateRenderTargetView等）に使用します。',
  '// 確保されたインデックスにSRVを作成\nauto cpuHandle = srvHeap.GetCPUHandle(idx);\ndevice->CreateShaderResourceView(resource, &desc, cpuHandle);',
  '• ハンドルは heapStart + index * descriptorSize として計算される\n• シェーダー可視・非可視の両方のヒープで有効\n• ビュー作成のCPU側で使用される'
],

'DescriptorHeap-GetGPUHandle': [
  'D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int index) const',
  '指定されたインデックスのGPUディスクリプタハンドルを返します。\nSetGraphicsRootDescriptorTable() を介してパイプラインにディスクリプタをバインドするために使用します。',
  '// テクスチャをシェーダーにバインド\nauto gpuHandle = srvHeap.GetGPUHandle(textureIdx);\ncmdList->SetGraphicsRootDescriptorTable(1, gpuHandle);',
  '• シェーダー可視ヒープでのみ有効\n• 非シェーダー可視ヒープでは無効なハンドルを返す\n• テクスチャバインディング用にSetGraphicsRootDescriptorTable() と共に使用\n• GPUハンドル使用前にSetDescriptorHeaps() でヒープを設定する必要がある'
],

'DescriptorHeap-GetHeap': [
  'ID3D12DescriptorHeap* GetHeap() const',
  'D3D12ディスクリプタヒープの生ポインタを返します。\nGPUアクセス用にヒープをバインドするSetDescriptorHeaps() 呼び出しに必要です。',
  '// レンダリング用にヒープをバインド\nID3D12DescriptorHeap* heaps[] = { srvHeap.GetHeap() };\ncmdList->SetDescriptorHeaps(1, heaps);',
  '• GPUハンドルを使用する前にSetDescriptorHeaps() でバインドする必要がある\n• D3D12はCBV_SRV_UAVヒープを正確に1つバインドすることを要求\n• 専用ヒープを使用するマルチテクスチャパスでは再バインドが必要'
],

// ============================================================
// Fence (Graphics/Device/Fence.h)
// ============================================================
'Fence-Initialize': [
  'bool Initialize(ID3D12Device* device)',
  'D3D12フェンスオブジェクトとCPU待機用のWin32イベントを作成します。\nフェンスは初期値0で開始されます。',
  '// フェンスの作成\nGX::Fence fence;\nfence.Initialize(device.GetDevice());',
  '• 初期値0のID3D12Fenceを作成\n• WaitForSingleObject用のWin32イベントハンドルも作成\n• 通常はCommandQueueにより内部的に管理される\n• イベントハンドルはデストラクタで閉じられる'
],

'Fence-Signal': [
  'uint64_t Signal(ID3D12CommandQueue* queue)',
  '内部フェンス値をインクリメントし、コマンドキューでシグナルします。\nすべての先行GPU処理が完了した時に到達する新しいフェンス値を返します。',
  '// コマンド実行後のシグナル\nuint64_t val = fence.Signal(queue.GetQueue());\n// 後でこの値を待機\nfence.WaitForValue(val);',
  '• シグナル呼び出し前にフェンス値がインクリメントされる\n• GPUは先行するすべてのコマンド処理後にフェンスをこの値に設定する\n• 返された値を保存して後で待機可能'
],

'Fence-WaitForValue': [
  'void WaitForValue(uint64_t value)',
  'GPUが指定されたフェンス値に到達するまでCPUをブロックします。\nビジースピンなしの効率的な待機のためにWin32イベントを使用します。',
  '// 特定のフェンス値を待機\nfence.WaitForValue(signalValue);\n// signalValueまでのすべてのGPU処理が完了',
  '• 内部的にWaitForSingleObjectを使用\n• フェンスが既に値に到達している場合は即座に返る\n• CPUストールが発生する。メインループでは控えめに使用\n• ダブルバッファリングのフレーム同期に必須'
],

'Fence-IsComplete': [
  'bool IsComplete(uint64_t value) const',
  'GPUが指定されたフェンス値に到達したかを非ブロッキングでチェックします。\nCPUをストールさせずにGPU完了をポーリングするのに有用です。',
  '// 非ブロッキングの完了チェック\nif (fence.IsComplete(pendingValue)) {\n    // リソースの再利用が安全\n}',
  '• GetCompletedValue() >= value の場合にtrueを返す\n• CPUをブロックしない\n• 非同期リードバックパターン（GPUProfiler等）に有用'
],

'Fence-GetCompletedValue': [
  'uint64_t GetCompletedValue() const',
  'GPUが完了した最高のフェンス値を返します。\nID3D12Fence::GetCompletedValue() を呼び出して現在のGPU進行状況を問い合わせます。',
  '// GPU進行状況の確認\nuint64_t completed = fence.GetCompletedValue();\nuint64_t pending = fence.GetCurrentValue();\nGX_LOG_INFO("GPU progress: %llu / %llu", completed, pending);',
  '• 非ブロッキング呼び出し\n• GPUがコマンドを処理するにつれて完了値は単調に増加\n• Signal() が返す値と比較して完了をチェック'
],

// ============================================================
// GPUProfiler (Graphics/Device/GPUProfiler.h)
// ============================================================
'GPUProfiler-Instance': [
  'static GPUProfiler& Instance()',
  'GPUプロファイラのシングルトンインスタンスを返します。\nレンダリングコード全体でタイミングスコープを追加するためにグローバルにアクセス可能です。',
  '// プロファイラにアクセス\nauto& profiler = GX::GPUProfiler::Instance();\nprofiler.SetEnabled(true);',
  '• シングルトンパターン。アプリケーションごとに1インスタンス\n• 使用前にInitialize() を呼び出す必要がある\n• デフォルト状態は無効（m_enabled = false）'
],

'GPUProfiler-Initialize': [
  'bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue)',
  'D3D12タイムスタンプクエリヒープとダブルバッファリードバックバッファを作成します。\nティックからミリ秒への変換用にコマンドキューからタイムスタンプ周波数を取得します。',
  '// プロファイラの初期化\nGX::GPUProfiler::Instance().Initialize(device.GetDevice(), queue.GetQueue());',
  '• k_MaxTimestamps（256）個のタイムスタンプスロットを持つクエリヒープを作成\n• ダブルバッファの結果取得用に2つのリードバックバッファを作成\n• コマンドキュー初期化後に呼び出す必要がある\n• フレームあたり最大128スコープをサポート（各2タイムスタンプ）'
],

'GPUProfiler-BeginFrame': [
  'void BeginFrame(ID3D12GraphicsCommandList* cmdList)',
  '前フレームの結果を読み出して新しいプロファイリングフレームを開始します。\n現在のフレームのタイムスタンプカウンタをリセットします。各フレームの開始時に呼び出します。',
  '// フレームごとのプロファイリング\nprofiler.BeginFrame(cmdList);\n// ... スコープ計測の記録 ...\nprofiler.EndFrame(cmdList);',
  '• 前フレームの結果を読み出す（ダブルバッファリングによりGPUストールなし）\n• 現在のフレームのスコープとタイムスタンプカウンタをリセット\n• BeginScope/EndScope ペアの前に呼び出すこと'
],

'GPUProfiler-EndFrame': [
  'void EndFrame(ID3D12GraphicsCommandList* cmdList)',
  'タイムスタンプクエリをリードバックバッファにリゾルブしてプロファイリングフレームを終了します。\n結果は次のフレームのBeginFrame() 呼び出しで利用可能になります。',
  '// プロファイリングフレームの終了\nprofiler.EndFrame(cmdList);\n// 結果は次フレームでGetResults() から利用可能',
  '• ID3D12GraphicsCommandList::ResolveQueryDataを発行して結果をリードバックバッファにコピー\n• すべてのBeginScope/EndScope ペアの後に呼び出す必要がある\n• ダブルバッファリードバックにより結果は1フレーム遅延'
],

'GPUProfiler-BeginScope': [
  'void BeginScope(ID3D12GraphicsCommandList* cmdList, const char* name)',
  'タイムスタンプクエリを発行して名前付きGPUタイミングスコープを開始します。\n囲まれたコマンドのGPU時間を計測するためにEndScope() と対で使用します。',
  '// シャドウパスの計測\nprofiler.BeginScope(cmdList, "Shadow Pass");\n// ... シャドウレンダリングコマンド ...\nprofiler.EndScope(cmdList);',
  '• nameは文字列リテラルか、フレームを超えた生存期間を持つ必要がある\n• フレームあたり最大128スコープ（k_MaxTimestamps / 2）\n• プロファイラが有効な場合のみ記録（SetEnabled(true)）\n• 例外安全なスコーピングにはGPUProfileScope RAIIラッパーを使用'
],

'GPUProfiler-EndScope': [
  'void EndScope(ID3D12GraphicsCommandList* cmdList)',
  '2番目のタイムスタンプクエリを発行して現在のGPUタイミングスコープを終了します。\nBeginScopeとEndScopeの時間差が計測されたGPU実行時間です。',
  '// スコープ計測の終了\nprofiler.EndScope(cmdList);',
  '• 対応するBeginScope() と対で使用する必要がある\n• 次の利用可能なタイムスタンプスロットにEndQueryを発行\n• スコープ結果は次のBeginFrame() 後にGetResults() で報告される'
],

'GPUProfiler-SetEnabled': [
  'void SetEnabled(bool enabled) / bool IsEnabled() const',
  'GPUプロファイラを有効/無効にします。無効時はBeginScope/EndScopeがno-opになります。\nデフォルト設定ではPキーでトグル。IsEnabled() クエリとToggleEnabled() も提供します。',
  '// Pキーでプロファイラをトグル\nif (keyboard.IsKeyTriggered(VK_P)) {\n    GX::GPUProfiler::Instance().ToggleEnabled();\n}',
  '• 通常動作時のオーバーヘッドを避けるためデフォルトは無効（false）\n• 有効時のタイムスタンプクエリのGPUオーバーヘッドは最小\n• 無効化しても以前の結果はクリアされない'
],

'GPUProfiler-GetResults': [
  'const std::vector<ProfileResult>& GetResults() const',
  '前フレームのプロファイリング結果リストを返します。\n各結果にはスコープ名と計測されたGPU時間（ミリ秒単位）が含まれます。',
  '// プロファイリング結果の表示\nfor (auto& r : profiler.GetResults()) {\n    GX_LOG_INFO("%s: %.2f ms", r.name, r.timeMs);\n}',
  '• リードバック遅延により結果は1フレーム遅延\n• 各ProfileResultにはname（スコープ名）とtimeMs（ミリ秒単位の持続時間）がある\n• プロファイラ無効時または最初の完全フレーム前は空'
],

'GPUProfiler-ProfileResult': [
  'struct ProfileResult { const char* name; float timeMs; int depth; }',
  '単一のGPUプロファイリングスコープのタイミング結果を格納します。\nnameはスコープ識別子、timeMsは計測されたGPU実行時間、depthはネストレベルを示します。',
  '// 結果の反復処理\nfor (auto& r : profiler.GetResults()) {\n    std::string indent(r.depth * 2, \' \');\n    GX_LOG_INFO("%s%s: %.3f ms", indent.c_str(), r.name, r.timeMs);\n}',
  '• nameはBeginScope に渡された文字列を指す（有効な状態を維持する必要がある）\n• timeMsはGPUタイムスタンプticks / frequency * 1000で計算\n• depthはネストされたスコープのインデント付きHUD表示に使用'
],

// ============================================================
// BarrierBatch (Graphics/Device/BarrierBatch.h)
// ============================================================
'BarrierBatch-Add': [
  'void Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)',
  'バッチに遷移バリアを追加します。バリアは即座に発行されず、バッチ投入用に蓄積されます。\nバッチが満杯（Nバリア）の場合、新しいバリアを追加する前に自動的にフラッシュします。',
  '// 複数の遷移をバッチ処理\nGX::BarrierBatch<> batch(cmdList);\nbatch.Transition(colorRT, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);\nbatch.Transition(depthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);\n// デストラクタで自動フラッシュ',
  '• before == after の場合はバリアをスキップ（no-op）\n• 内部配列が満杯の場合に自動フラッシュ（デフォルトN=16）\n• UAV() メソッドによるUAVバリアもサポート\n• テンプレートパラメータNが最大バッチサイズを制御（スタック確保配列）'
],

'BarrierBatch-Flush': [
  'void Flush(ID3D12GraphicsCommandList* cmdList)',
  '蓄積されたすべてのバリアを単一のResourceBarrier() 呼び出しで発行します。\n複数バリアの1回の呼び出しへのバッチ化はGPUパイプライン同期をより効率的にします。',
  '// 手動フラッシュ\nbatch.Transition(rt1, before1, after1);\nbatch.Transition(rt2, before2, after2);\nbatch.Flush(); // 両方のバリアを1回の呼び出しで発行',
  '• バリアが蓄積されていない場合はno-op\n• デストラクタで自動的に呼ばれる\n• バッチ化によりGPU同期オーバーヘッドを削減\n• フラッシュ後に内部カウンタを0にリセット'
],

'BarrierBatch-IsEmpty': [
  'uint32_t Count() const',
  'バッチに現在蓄積されているバリアの数を返します。\nデバッグや条件付きフラッシュに有用です。',
  '// 保留中のバリアを確認\nif (batch.Count() > 0) {\n    batch.Flush();\n}',
  '• Flush() または構築後は0を返す\n• 最大値はN（テンプレートパラメータ、デフォルト16）\n• デストラクタが自動的にFlush() を呼ぶため、手動チェックが必要になることは稀'
],

// ============================================================
// RootSignature (Graphics/Pipeline/RootSignature.h)
// ============================================================
'RootSignature-AddCBV': [
  'RootSignatureBuilder& AddCBV(uint32_t reg, uint32_t space = 0)',
  'ルートシグネチャにルートCBV（定数バッファビュー）パラメータを追加します。\n指定されたレジスタとスペースのHLSL cbufferに対応します。',
  '// CBV付きルートシグネチャの構築\nGX::RootSignatureBuilder builder;\nbuilder.AddCBV(0)    // b0: フレーム定数\n       .AddCBV(1);   // b1: オブジェクト定数\nauto rootSig = builder.Build(device);',
  '• ルートCBVは64ビットGPU仮想アドレス（ルートシグネチャ内で8バイト）\n• 頻繁に変更されるバッファにはディスクリプタテーブルより効率的\n• デフォルトspaceは0\n• メソッドチェーン用に*thisを返す（ビルダーパターン）'
],

'RootSignature-AddSRV': [
  'RootSignatureBuilder& AddSRV(uint32_t reg, uint32_t space = 0)',
  'ルートSRV（シェーダーリソースビュー）パラメータを追加します。\nルートSRVは生/構造化バッファにのみ動作し、Texture2D.Sample() 操作には使用できません。',
  '// 構造化バッファ用ルートSRVの追加\nbuilder.AddSRV(0);  // t0: 構造化バッファ',
  '• ルートSRVはバッファリソース専用（テクスチャには使用不可）\n• Texture2D/TextureCubeにはAddDescriptorTable() を使用すること\n• ルートSRVをTexture2D.Sample() で使用するとGPUクラッシュが発生\n• チェーン用に*thisを返す'
],

'RootSignature-AddDescriptorTable': [
  'RootSignatureBuilder& AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t reg, uint32_t count, uint32_t space = 0)',
  'SRV、CBV、またはUAVディスクリプタの1つのレンジを含むディスクリプタテーブルパラメータを追加します。\nテクスチャをシェーダーにバインドするために必要です。レンジは指定されたレジスタから開始します。',
  '// テクスチャ用SRVテーブルの追加\nbuilder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);  // t0: テクスチャ1枚',
  '• Texture2D.Sample() 操作に必須（ルートSRVでは動作しない）\n• ディスクリプタレンジはポインタ無効化防止のためunique_ptrに格納\n• countは連続レンジ内のディスクリプタ数を指定\n• マルチテクスチャパスではcountに必要なテクスチャ数を設定'
],

'RootSignature-AddStaticSampler': [
  'RootSignatureBuilder& AddStaticSampler(uint32_t reg, D3D12_FILTER filter)',
  '指定されたレジスタとフィルタモードで静的サンプラーを追加します。\n静的サンプラーはルートシグネチャにベイクされ、ルートパラメータスロットを消費しません。',
  '// リニアとポイントサンプラーの追加\nbuilder.AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);\nbuilder.AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT);',
  '• 静的サンプラーはランタイムコストゼロ（ルートシグネチャにコンパイル）\n• デフォルトアドレスモードはWrap\n• オーバーロード版はシャドウサンプリング用にaddressModeとcomparisonFuncを受け付ける\n• ルートシグネチャあたり最大2032静的サンプラー（D3D12の制限）'
],

'RootSignature-AddConstants': [
  'RootSignatureBuilder& AddConstants(uint32_t reg, uint32_t num32BitValues, uint32_t space = 0)',
  'ルートシグネチャにインラインルート定数を追加します。\nルート定数はコンスタントバッファリソースを必要とせず、SetGraphicsRoot32BitConstants() で直接設定されます。',
  '// 4つのfloatルート定数を追加\nbuilder.AddConstants(0, 4);  // b0: 4 x 32ビット値\n// 描画時に設定:\ncmdList->SetGraphicsRoot32BitConstants(paramIdx, 4, &data, 0);',
  '• 各32ビット値はルートシグネチャ内で1 DWORDのコスト（合計最大64 DWORD）\n• 小さく頻繁に変更されるデータ（オブジェクトID、時間等）を渡す最速の方法\n• バッファリソース不要。データはルート引数に直接埋め込み'
],

'RootSignature-Build': [
  'bool Build(ID3D12Device* device, RootSignature& out)',
  '追加されたすべてのパラメータとサンプラーからD3D12ルートシグネチャをシリアライズして作成します。\nシリアライズまたは作成に失敗した場合はfalseを返します。',
  '// ルートシグネチャの構築\nGX::RootSignatureBuilder builder;\nbuilder.AddCBV(0).AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1).AddStaticSampler(0);\nauto rootSig = builder.Build(device);',
  '• 内部的にD3D12SerializeVersionedRootSignatureを使用\n• デフォルトフラグはインプットアセンブラの入力レイアウトを許可\n• パラメータがD3D12の制限（64 DWORDルートシグネチャサイズ）を超えると失敗\n• 返されたComPtrがID3D12RootSignatureを保持'
],

'RootSignature-Get': [
  'ID3D12RootSignature* Get() const',
  'パイプラインにバインドするためのD3D12ルートシグネチャの生ポインタを返します。\nコマンドリストのSetGraphicsRootSignature() で使用します。',
  '// ルートシグネチャのバインド\ncmdList->SetGraphicsRootSignature(rootSig.Get());',
  '• Build() が呼ばれていないか失敗した場合はnullptrを返す\n• ルートパラメータ設定前にバインドする必要がある\n• 同じルートシグネチャを複数のPSOで共有可能'
],

// ============================================================
// PipelineState (Graphics/Pipeline/PipelineState.h)
// ============================================================
'PipelineState-SetRootSignature': [
  'PipelineStateBuilder& SetRootSignature(ID3D12RootSignature* rootSig)',
  'このPSOのリソースバインディングレイアウトを定義するルートシグネチャを設定します。\n描画時にバインドされるルートシグネチャと一致する必要があります。',
  '// PSOビルダーにルートシグネチャを設定\nGX::PipelineStateBuilder psoBuilder;\npsoBuilder.SetRootSignature(rootSig.Get());',
  '• Build() の前に設定する必要がある\n• ルートシグネチャはシェーダーが期待するリソースを定義する\n• ルートシグネチャの不一致はD3D12バリデーションエラーを引き起こす'
],

'PipelineState-SetVertexShader': [
  'PipelineStateBuilder& SetVertexShader(const Shader& vs)',
  'パイプラインの頂点シェーダーバイトコードを設定します。\n頂点シェーダーは頂点位置をオブジェクト空間からクリップ空間に変換します。',
  '// 頂点シェーダーの設定\nauto vs = shader.CompileFromFile(L"Shaders/VS.hlsl", L"main", L"vs_6_0");\npsoBuilder.SetVertexShader(vs.GetBytecode());',
  '• ShaderBlob::GetBytecode() からのD3D12_SHADER_BYTECODEを受け付ける\n• Build() の前に設定する必要がある\n• 頂点シェーダーの入力レイアウトはSetInputLayout() と一致すること'
],

'PipelineState-SetPixelShader': [
  'PipelineStateBuilder& SetPixelShader(const Shader& ps)',
  'パイプラインのピクセルシェーダーバイトコードを設定します。\nピクセルシェーダーはラスタライズされた各ピクセルの最終カラーを計算します。',
  '// ピクセルシェーダーの設定\nauto ps = shader.CompileFromFile(L"Shaders/PS.hlsl", L"main", L"ps_6_0");\npsoBuilder.SetPixelShader(ps.GetBytecode());',
  '• ShaderBlob::GetBytecode() からのD3D12_SHADER_BYTECODEを受け付ける\n• 深度のみパス（シャドウレンダリング）ではオプション\n• 出力フォーマットはSetRenderTargetFormat() と一致すること'
],

'PipelineState-SetInputLayout': [
  'PipelineStateBuilder& SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* elements, uint32_t count)',
  '頂点バッファデータの構造を記述する頂点入力レイアウトを定義します。\n頂点シェーダーの入力シグネチャと一致する必要があります。',
  '// 入力レイアウトの定義\nD3D12_INPUT_ELEMENT_DESC layout[] = {\n    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},\n    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}\n};\npsoBuilder.SetInputLayout(layout, 2);',
  '• セマンティック名は頂点シェーダー入力と一致すること（POSITION、TEXCOORD、NORMAL等）\n• オフセットとフォーマットは実際の頂点構造体レイアウトと一致すること\n• フルスクリーン三角形シェーダー（SV_VertexID）は空の入力レイアウト（nullptr, 0）を使用'
],

'PipelineState-SetRenderTargetFormat': [
  'PipelineStateBuilder& SetRenderTargetFormat(DXGI_FORMAT format)',
  'PSOのレンダーターゲットフォーマットを設定します。実際のレンダーターゲットと一致する必要があります。\nデフォルトはDXGI_FORMAT_R8G8B8A8_UNORMです。',
  '// HDRレンダーターゲット\npsoBuilder.SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);\n// LDRバックバッファ\npsoBuilder.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);',
  '• HDRパイプラインはR16G16B16A16_FLOATを使用\n• LDRパイプラインとスワップチェーンはR8G8B8A8_UNORMを使用\n• フォーマットの不一致はD3D12バリデーションエラーを引き起こす\n• インデックスパラメータで複数RTフォーマットを設定可能'
],

'PipelineState-SetDepthFormat': [
  'PipelineStateBuilder& SetDepthFormat(DXGI_FORMAT format)',
  'PSOのデプスバッファフォーマットを設定します。\n一般的なフォーマットは標準深度用のD32_FLOATと深度+ステンシル用のD24_UNORM_S8_UINTです。',
  '// デプスフォーマットの設定\npsoBuilder.SetDepthFormat(DXGI_FORMAT_D32_FLOAT);',
  '• DXGI_FORMAT_UNKNOWNで深度テストを無効化\n• 描画時にバインドされる実際のデプスバッファフォーマットと一致すること\n• シャドウマップPSOはカラーレンダーターゲットなしでD32_FLOATを使用'
],

'PipelineState-SetBlendState': [
  'PipelineStateBuilder& SetBlendState(D3D12_BLEND_DESC blendDesc)',
  'D3D12_BLEND_DESC構造体を使用してブレンドステートを直接設定します。\n一般的なブレンドモードには便利メソッド（SetAlphaBlend、SetAdditiveBlend等）を使用してください。',
  '// カスタムブレンドステート\nD3D12_BLEND_DESC blend = {};\nblend.RenderTarget[0].BlendEnable = TRUE;\n// ... ブレンドファクタの設定 ...\npsoBuilder.SetBlendState(blend);',
  '• 一般的なケースにはSetAlphaBlend()、SetAdditiveBlend()、SetSubtractiveBlend()、SetMultiplyBlend() を推奨\n• デフォルトは不透明（ブレンドなし）\n• ブレンドステートはすべてのレンダーターゲットに適用される'
],

'PipelineState-SetRasterizerState': [
  'PipelineStateBuilder& SetRasterizerState(D3D12_RASTERIZER_DESC desc)',
  '面カリング、塗りつぶしモード、デプスバイアスを制御するラスタライザステートを設定します。\n一般的な設定用の便利メソッドSetCullMode() とSetFillMode() が利用可能です。',
  '// ワイヤフレームモード\npsoBuilder.SetFillMode(D3D12_FILL_MODE_WIREFRAME);\n// 両面レンダリング用にカリングを無効化\npsoBuilder.SetCullMode(D3D12_CULL_MODE_NONE);',
  '• デフォルトカリングモードはBACK（標準の裏面カリング）\n• デフォルト塗りつぶしモードはSOLID\n• シャドウマップPSOはシャドウアクネ防止にデプスバイアスをよく使用\n• SetDepthBias() でbias/clamp/slopeScaleを設定'
],

'PipelineState-SetDepthStencilState': [
  'PipelineStateBuilder& SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC desc)',
  '深度テストとステンシルテストの設定を行います。\n便利メソッドSetDepthEnable() とSetDepthWriteMask() が利用可能です。',
  '// 深度書き込みを無効化（半透明オブジェクト用）\npsoBuilder.SetDepthEnable(true);\npsoBuilder.SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ZERO);',
  '• デフォルトは深度テストON、深度書き込みON、比較LESS\n• ポストエフェクトのフルスクリーンパスは通常深度を無効化\n• SetDepthComparisonFunc() でカスタム比較（LESS_EQUAL等）が可能'
],

'PipelineState-SetPrimitiveTopology': [
  'PipelineStateBuilder& SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)',
  'PSOのプリミティブトポロジー型（三角形、ライン、ポイント）を設定します。\n描画時にIASetPrimitiveTopology() でコマンドリストに設定されるトポロジーと一致する必要があります。',
  '// 三角形トポロジー（最も一般的）\npsoBuilder.SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);\n// デバッグレンダリング用ラインポロジー\npsoBuilder.SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);',
  '• TRIANGLEがデフォルトで3Dレンダリングに最も一般的\n• LINEはPrimitiveBatchのライン/ワイヤフレーム描画に使用\n• PSOトポロジー型はより広範（TRIANGLE）だが、IASetPrimitiveTopologyはより具体的（TRIANGLELIST、TRIANGLESTRIP）'
],

'PipelineState-Build': [
  'bool Build(ID3D12Device* device, PipelineState& out)',
  '設定されたビルダー設定からD3D12パイプラインステートオブジェクトを作成します。\nBuild() 呼び出し前にすべてのシェーダー、フォーマット、ステートを設定する必要があります。',
  '// PSOの構築\nGX::PipelineStateBuilder builder;\nbuilder.SetRootSignature(rootSig.Get())\n       .SetVertexShader(vs.GetBytecode())\n       .SetPixelShader(ps.GetBytecode())\n       .SetInputLayout(layout, count)\n       .SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);\nauto pso = builder.Build(device);',
  '• PSO作成は高コスト。PSOをキャッシュして再利用すること\n• ComPtr<ID3D12PipelineState> を返す\n• ShaderHotReloadは登録されたPSORebuilderコールバックでPSOを再作成\n• すべての3D PSOはHDRパイプライン互換のためR16G16B16A16_FLOATを使用する必要がある'
],

// ============================================================
// Shader (Graphics/Pipeline/Shader.h)
// ============================================================
'Shader-CompileFromFile': [
  'ShaderBlob CompileFromFile(const std::wstring& path, const std::wstring& entry, const std::wstring& target)',
  'DXC（DirectXシェーダーコンパイラ）を使用してHLSLシェーダーファイルをコンパイルします。\nコンパイル済みバイトコードを含むShaderBlobを返します。結果のIsValid() を確認してください。',
  '// 頂点・ピクセルシェーダーのコンパイル\nGX::Shader compiler;\ncompiler.Initialize();\nauto vs = compiler.CompileFromFile(L"Shaders/PBR.hlsl", L"VSMain", L"vs_6_0");\nauto ps = compiler.CompileFromFile(L"Shaders/PBR.hlsl", L"PSMain", L"ps_6_0");',
  '• ターゲット文字列: "vs_6_0"（頂点）、"ps_6_0"（ピクセル）、"cs_6_0"（コンピュート）\n• コンパイル失敗時はvalid=falseのShaderBlobを返す\n• 失敗時のエラーメッセージ取得にGetLastError() を使用\n• CompileFromFile呼び出し前にInitialize() を1回呼ぶ必要がある'
],

'Shader-CompileFromFile2': [
  'ShaderBlob CompileFromFile(const std::wstring& path, const std::wstring& entry, const std::wstring& target, const std::vector<std::pair<std::wstring, std::wstring>>& defines)',
  'バリアント生成用のプリプロセッサ定義付きでHLSLシェーダーをコンパイルします。\n各定義はDXCに-D NAME=VALUEとして渡される名前-値ペアです。',
  '// 定義付きコンパイル\nstd::vector<std::pair<std::wstring, std::wstring>> defines = {\n    {L"USE_NORMAL_MAP", L"1"},\n    {L"MAX_LIGHTS", L"8"}\n};\nauto ps = compiler.CompileFromFile(L"Shaders/PBR.hlsl", L"PSMain", L"ps_6_0", defines);',
  '• 定義はDXC引数として追加: -D NAME=VALUE\n• シェーダーパーミュテーション（機能トグル、品質レベル）に使用\n• 定義の一意なセットごとに異なるシェーダーバリアントが生成される\n• ShaderLibraryは完全なShaderKey（path+entry+target+defines）でバリアントをキャッシュ'
],

'Shader-GetBytecode': [
  'D3D12_SHADER_BYTECODE GetBytecode() const',
  'コンパイル済みシェーダーバイトコードをD3D12_SHADER_BYTECODE構造体として返します。\nPipelineStateBuilderで頂点・ピクセルシェーダーを設定する際に使用します。',
  '// PSO作成にバイトコードを使用\nauto vs = compiler.CompileFromFile(L"Shaders/VS.hlsl", L"main", L"vs_6_0");\npsoBuilder.SetVertexShader(vs.GetBytecode());',
  '• シェーダーが無効の場合は{nullptr, 0}を返す\n• バイトコードポインタはShaderBlobが存在する限り有効\n• Shaderクラスではなく、ShaderBlobで定義されている'
],

'Shader-IsValid': [
  'bool IsValid() const',
  '最後のコンパイルが成功したかどうかを返します。\nシェーダーが使用可能かどうかを判断するためにCompileFromFile後にチェックしてください。',
  '// コンパイル結果の確認\nauto ps = compiler.CompileFromFile(L"Shaders/PS.hlsl", L"main", L"ps_6_0");\nif (!ps.valid) {\n    GX_LOG_ERROR("Shader compile failed: %s", compiler.GetLastError().c_str());\n}',
  '• validフラグはShaderコンパイラではなくShaderBlobに設定される\n• falseはコンパイルエラーの発生を意味する\n• 詳細なエラーメッセージの取得にGetLastError() を使用'
],

'Shader-GetLastError': [
  'const std::string& GetLastError() const',
  '直近の失敗したコンパイルのエラーメッセージを返します。\n行番号やエラー説明を含むDXCエラー出力を格納しています。',
  '// コンパイルエラーの表示\nif (!ps.valid) {\n    GX_LOG_ERROR("Compile error:\\n%s", compiler.GetLastError().c_str());\n}',
  '• 新しいCompileFromFile呼び出しごとにクリアされる\n• DXC診断出力（行番号、エラー説明）を含む\n• ShaderHotReloadが画面にエラーオーバーレイを表示するのに使用\n• 最後のコンパイルが成功した場合は空文字列'
],

'Shader-ShaderDefine': [
  'struct ShaderDefine { std::wstring name; std::wstring value; }',
  'シェーダーコンパイル用のプリプロセッサ定義を表します。\nシェーダーバリアント生成のためdefinesオーバーロード版CompileFromFileと共に使用します。',
  '// 定義の作成\nstd::vector<std::pair<std::wstring, std::wstring>> defines;\ndefines.push_back({L"ENABLE_SHADOWS", L"1"});\ndefines.push_back({L"QUALITY", L"HIGH"});',
  '• DXCに-D NAME=VALUE引数として渡される\n• 名前-値ペアはstd::pair<std::wstring, std::wstring>として格納\n• ShaderLibraryでのキャッシュ識別用ShaderKeyの一部'
],

// ============================================================
// ShaderLibrary (Graphics/Pipeline/ShaderLibrary.h)
// ============================================================
'ShaderLibrary-Instance': [
  'static ShaderLibrary& Instance()',
  'シェーダーライブラリのシングルトンインスタンスを返します。\nアプリケーション全体のシェーダーキャッシュとPSOリビルダー登録を一元管理します。',
  '// シェーダーライブラリにアクセス\nauto& lib = GX::ShaderLibrary::Instance();\nauto vs = lib.GetShader(L"Shaders/PBR.hlsl", L"VSMain", L"vs_6_0");',
  '• シングルトンパターン。アプリケーションごとに1インスタンス\n• GetShader() の前にInitialize() を呼び出す必要がある\n• 内部mutexによりスレッドセーフ'
],

'ShaderLibrary-GetOrCompile': [
  'ShaderBlob GetShader(const std::wstring& path, const std::wstring& entry, const std::wstring& target)',
  'キャッシュ済みシェーダーを返すか、初回リクエスト時にコンパイルします。\n同じpath/entry/targetでの後続呼び出しはキャッシュされたblobを再コンパイルなしで返します。',
  '// シェーダーの取得（キャッシュ済み）\nauto vs = GX::ShaderLibrary::Instance().GetShader(\n    L"Shaders/PBR.hlsl", L"VSMain", L"vs_6_0");',
  '• キャッシュキーはfilePath + entryPoint + targetの組み合わせ\n• 初回呼び出しでシェーダーをコンパイル。後続呼び出しはキャッシュを返す\n• InvalidateFile() で特定ファイルのキャッシュをクリア\n• 内部mutexによりスレッドセーフ'
],

'ShaderLibrary-GetOrCompile2': [
  'ShaderBlob GetShaderVariant(const std::wstring& path, const std::wstring& entry, const std::wstring& target, const std::vector<std::pair<std::wstring, std::wstring>>& defines)',
  'キャッシュ済みシェーダーバリアントを返すか、指定されたプリプロセッサ定義でコンパイルします。\n定義の一意なセットごとに個別にキャッシュされます。',
  '// 定義付きシェーダーバリアントの取得\nstd::vector<std::pair<std::wstring, std::wstring>> defs = {{L"QUALITY", L"HIGH"}};\nauto ps = GX::ShaderLibrary::Instance().GetShaderVariant(\n    L"Shaders/PBR.hlsl", L"PSMain", L"ps_6_0", defs);',
  '• キャッシュキーはpath/entry/targetに加えてdefinesを含む\n• 異なるdefinesセットは個別のキャッシュエントリを作成\n• 無効化は指定ファイルのすべてのバリアントをクリア'
],

'ShaderLibrary-InvalidateFile': [
  'bool InvalidateFile(const std::wstring& path)',
  '指定されたファイルからコンパイルされたすべてのキャッシュ済みシェーダーを削除し、登録済みPSOリビルダーコールバックをトリガーします。\nすべてのPSO再構築が成功した場合にtrueを返します。ファイル変更時にShaderHotReloadが使用します。',
  '// 手動無効化\nbool ok = GX::ShaderLibrary::Instance().InvalidateFile(L"Shaders/PBR.hlsl");\nif (!ok) GX_LOG_ERROR("PSO rebuild failed");',
  '• 正規化されたファイルパスに一致するすべてのキャッシュエントリをクリア\n• そのファイルの登録済みPSORebuilderコールバックをすべて呼び出す\n• ファイルパスは正規化される（バックスラッシュ→スラッシュ、小文字化）\n• .hlsli変更時はすべてのキャッシュがクリアされる（includeの追跡なし）'
],

'ShaderLibrary-InvalidateAll': [
  'void InvalidateAll()',
  'シェーダーキャッシュ全体をクリアし、すべての登録済みPSOリビルダーをトリガーします。\n共有インクルードファイル（.hlsli）変更時に使用します（includeの依存関係追跡は未実装）。',
  '// 全キャッシュのクリア\nGX::ShaderLibrary::Instance().InvalidateAll();',
  '• すべてのキャッシュ済みShaderBlobをクリア\n• 登録済みPSORebuilderコールバックをすべてトリガー\n• .hlsliファイル変更時にShaderHotReloadが使用\n• InvalidateFile() より高コストだが、include変更の正確性を保証'
],

'ShaderLibrary-RegisterPSORebuilder': [
  'PSOCallbackID RegisterPSORebuilder(const std::wstring& shaderPath, PSORebuilder callback)',
  '指定されたシェーダーファイルが無効化された時に呼ばれるコールバックを登録します。\nコールバックはD3D12デバイスを受け取り、影響を受けるPSOを再作成します。登録解除用のコールバックIDを返します。',
  '// PSOリビルダーの登録\nauto id = GX::ShaderLibrary::Instance().RegisterPSORebuilder(\n    L"Shaders/PBR.hlsl",\n    [this](ID3D12Device* dev) { return CreatePipelines(dev); });',
  '• 各レンダラーのInitialize() でCreatePipelines() の後に呼ばれる\n• ラムダは通常thisをキャプチャしてCreatePipelines(dev) を呼ぶ\n• デフォルト設定で18以上のレンダラーがリビルダーを登録\n• UnregisterPSORebuilder() での後の登録解除用にPSOCallbackIDを返す'
],

// ============================================================
// ShaderHotReload (Graphics/Pipeline/ShaderHotReload.h)
// ============================================================
'ShaderHotReload-Initialize': [
  'bool Initialize(const std::string& shaderDir, ID3D12Device* device, ID3D12CommandQueue* queue)',
  '指定されたシェーダーディレクトリのファイル変更の監視を開始します。\n非同期変更検出のためReadDirectoryChangesWを使用するFileWatcherを使用します。',
  '// シェーダーホットリロードの開始\nGX::ShaderHotReload::Instance().Initialize(\n    device.GetDevice(), &device.GetCommandQueue());',
  '• Shaders/ ディレクトリを再帰的に監視\n• .hlslと.hlsliファイルの変更を検出\n• FileWatcherはバックグラウンドスレッドで実行\n• ShaderLibrary::Initialize() の後に呼び出す必要がある'
],

'ShaderHotReload-Update': [
  'void Update(float deltaTime)',
  'デバウンスロジック付きで保留中のシェーダーファイル変更を処理します。\nメインスレッドでフレームごとに1回呼び出します。デバウンス遅延後、GPUをフラッシュして再コンパイルをトリガーします。',
  '// フレームごとの更新\nGX::ShaderHotReload::Instance().Update(timer.GetDeltaTime());',
  '• デバウンス遅延は0.3秒で連続保存操作をバッチ処理\n• デバウンス後: GPUフラッシュ → ShaderLibrary::InvalidateFile() → PSO再構築\n• コンパイルエラーは保存されGetLastError() でアクセス可能\n• 保留中の変更がない場合はno-op'
],

'ShaderHotReload-Shutdown': [
  'void Shutdown()',
  'ファイルウォッチャーを停止し、リソースをクリーンアップします。\nすべてのレンダリング完了後のアプリケーションシャットダウン時に呼び出します。',
  '// ホットリロードのシャットダウン\nGX::ShaderHotReload::Instance().Shutdown();',
  '• FileWatcherバックグラウンドスレッドを停止\n• 保留中の変更とエラー状態をクリア\n• 複数回呼び出しても安全'
],

'ShaderHotReload-IsEnabled': [
  'bool IsEnabled() const',
  'シェーダーホットリロードが現在アクティブかどうかを返します。\nデバッグ目的で実行時にトグル可能です。',
  '// ホットリロード状態の確認\nif (GX::ShaderHotReload::Instance().IsEnabled()) {\n    GX_LOG_INFO("Hot reload active");\n}',
  '• Initialize() 後はデフォルトで有効\n• デフォルト設定ではF9キーでトグル'
],

'ShaderHotReload-SetEnabled': [
  'void SetEnabled(bool enabled)',
  '実行時にシェーダーホットリロードを有効/無効にします。\n無効時もファイル変更は検出されますが、再有効化まで処理されません。',
  '// F9キーでトグル\nif (keyboard.IsKeyTriggered(VK_F9)) {\n    auto& hr = GX::ShaderHotReload::Instance();\n    hr.SetEnabled(!hr.IsEnabled());\n}',
  '• 無効時もFileWatcherは動作するが、変更は処理されない\n• 再有効化するとキューに入った変更が即座に処理される\n• パフォーマンステスト中のホットリロード一時停止に有用'
],

'ShaderHotReload-GetLastError': [
  'const std::string& GetLastError() const',
  '直近の失敗したシェーダーリロードのエラーメッセージを返します。\n開発者がアプリケーションを再起動せずにシェーダーエラーを修正できるよう、画面にオーバーレイ表示されます。',
  '// エラーオーバーレイの表示\nif (GX::ShaderHotReload::Instance().HasError()) {\n    auto& err = GX::ShaderHotReload::Instance().GetErrorMessage();\n    textRenderer.DrawString(err, 10, 10, 14, 0xFFFF0000);\n}',
  '• DXCコンパイラのエラー出力を含む\n• 次のリロードが成功するとクリアされる\n• エラーメッセージが存在する場合にHasError() がtrueを返す\n• 通常は左上隅に赤いテキストオーバーレイとして表示'
],

};
