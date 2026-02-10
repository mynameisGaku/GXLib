// Agent 8c: Compat Layer (DXLib-compatible API)
// GXLib_System, GXLib_Graphics, GXLib_Text, GXLib_Input, GXLib_Audio,
// GXLib_3D, GXLib_Math, CompatContext, CompatTypes

{

// ============================================================
// GXLib_System  (Compat/GXLib.h)  —  page-GXLib_System
// ============================================================

'GXLib_System-GX_Init': [
  'int GX_Init()',
  'GXLibエンジンを初期化し、ウィンドウ作成・DirectX 12デバイス・全サブシステムを起動する。ChangeWindowModeやSetGraphModeで事前に設定した内容が反映される。アプリケーション開始時に最初に呼び出す関数であり、成功時は0、失敗時は-1を返す。初期化前にウィンドウモードや解像度を設定しておくことを推奨する。',
  '// GXLib初期化\nChangeWindowMode(TRUE);\nSetGraphMode(1280, 720, 32);\nif (GX_Init() == -1) return -1;\n\nSetDrawScreen(GX_SCREEN_BACK);\n\n// メインループ\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();\n    // 描画処理\n    ScreenFlip();\n}\n\nGX_End();',
  '• ChangeWindowMode, SetGraphMode はGX_Init の前に呼ぶこと\n• 内部でCompatContextシングルトンの全サブシステムを初期化する\n• 失敗した場合はエラーログを出力してから -1 を返す\n• DXLibの DxLib_Init() に相当する関数'
],

'GXLib_System-GX_End': [
  'int GX_End()',
  'GXLibエンジンを終了し、全リソースとサブシステムを解放する。GPUの全コマンド完了を待機してからデバイスやウィンドウを破棄する。アプリケーション終了時に必ず呼び出すこと。成功時は0を返す。',
  '// アプリケーション終了\nGX_End();\nreturn 0;',
  '• メインループを抜けた後に必ず呼ぶこと\n• GPU処理完了を内部で待機するため安全にリソース解放される\n• DXLibの DxLib_End() に相当する関数\n• 複数回呼んでも安全'
],

'GXLib_System-ProcessMessage': [
  'int ProcessMessage()',
  'ウィンドウメッセージの処理と入力状態の更新を行う。メインループの条件式で毎フレーム呼び出し、ウィンドウが閉じられた場合は-1を返す。正常時は0を返す。入力デバイスの状態更新もこの関数内で行われる。',
  '// メインループ\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();\n    DrawString(10, 10, "Hello GXLib!", GetColor(255, 255, 255));\n    ScreenFlip();\n}',
  '• 毎フレーム必ず呼び出すこと\n• ウィンドウメッセージ処理・キーボード/マウス/ゲームパッド状態更新を行う\n• 戻り値が -1 の場合はウィンドウが閉じられたことを示す\n• DXLibの ProcessMessage() と同一の使い方'
],

'GXLib_System-SetMainWindowText': [
  'int SetMainWindowText(const TCHAR* title)',
  'メインウィンドウのタイトルバーに表示される文字列を設定する。GX_Initの前後どちらでも呼び出し可能だが、初期化前に呼んだ場合は初期化時に反映される。ゲーム名やFPS表示などに使用する。成功時は0を返す。',
  '// ウィンドウタイトルの設定\nSetMainWindowText("My Awesome Game");\nGX_Init();\n\n// FPS表示の更新\nchar buf[64];\nsprintf(buf, "Game - FPS: %d", fps);\nSetMainWindowText(buf);',
  '• GX_Init 前に呼ぶとCompatContextの windowTitle に保存される\n• GX_Init 後に呼ぶと即座にウィンドウタイトルが変更される\n• TCHAR* はchar* として扱われる（Unicodeビルドでない場合）'
],

'GXLib_System-ChangeWindowMode': [
  'int ChangeWindowMode(int flag)',
  'ウィンドウモードとフルスクリーンモードを切り替える。TRUEでウィンドウモード、FALSEでフルスクリーンモードになる。GX_Init の前に呼び出すことを推奨する。成功時は0を返す。',
  '// ウィンドウモードで起動\nChangeWindowMode(TRUE);\nSetGraphMode(1280, 720, 32);\nGX_Init();',
  '• GX_Init の前に呼ぶのが推奨される使い方\n• デフォルトはウィンドウモード（TRUE）\n• DXLibの ChangeWindowMode() に相当する関数'
],

'GXLib_System-SetGraphMode': [
  'int SetGraphMode(int width, int height, int colorBitNum)',
  '画面解像度と色深度を設定する。GX_Initの前に呼び出すことで、指定した解像度でウィンドウが作成される。colorBitNum は通常32を指定する。成功時は0を返す。',
  '// 1920x1080で起動\nChangeWindowMode(TRUE);\nSetGraphMode(1920, 1080, 32);\nGX_Init();',
  '• GX_Init の前に呼ぶこと\n• デフォルト解像度は 1280x720\n• colorBitNum は現在32のみサポート\n• DXLibの SetGraphMode() に相当する関数'
],

'GXLib_System-GetColor': [
  'unsigned int GetColor(int r, int g, int b)',
  'RGB値（各0〜255）から描画用カラー値を生成する。戻り値は0xFFRRGGBB形式の32ビット符号なし整数で、描画関数のcolor引数に使用する。全ての2D描画関数で使える共通のカラー値生成関数。',
  '// 色の生成と使用\nunsigned int white  = GetColor(255, 255, 255);\nunsigned int red    = GetColor(255, 0, 0);\nunsigned int blue   = GetColor(0, 0, 255);\n\nDrawString(10, 10, "Hello", white);\nDrawBox(100, 100, 200, 200, red, TRUE);\nDrawCircle(320, 240, 50, blue, TRUE);',
  '• 戻り値のフォーマットは 0xFFRRGGBB（アルファは常にFF）\n• 各成分は0〜255の範囲でクランプされる\n• DXLibの GetColor() と同一の使い方'
],

'GXLib_System-GetNowCount': [
  'int GetNowCount()',
  'アプリケーション起動からの経過時間をミリ秒単位で返す。フレーム間の経過時間計測やアニメーションのタイミング制御に使用する。内部的にはGX::Timerの値をミリ秒に変換して返す。',
  '// 経過時間の計測\nint startTime = GetNowCount();\n\n// 処理実行\nDoSomething();\n\nint elapsed = GetNowCount() - startTime;\nDrawFormatString(10, 10, GetColor(255, 255, 255),\n    "処理時間: %dms", elapsed);',
  '• 戻り値はint型なので約24日でオーバーフローする可能性がある\n• フレーム間デルタタイム計算に利用可能\n• DXLibの GetNowCount() に相当する関数'
],

'GXLib_System-SetDrawScreen': [
  'int SetDrawScreen(int screen)',
  '描画先スクリーンを設定する。GX_SCREEN_BACKでバックバッファに描画（ダブルバッファリング）、GX_SCREEN_FRONTで表画面に直接描画する。通常はGX_SCREEN_BACKを指定する。成功時は0を返す。',
  '// バックバッファへの描画を設定\nGX_Init();\nSetDrawScreen(GX_SCREEN_BACK);\n\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();\n    DrawString(10, 10, "Back buffer", GetColor(255, 255, 255));\n    ScreenFlip();\n}',
  '• 通常は GX_SCREEN_BACK を指定してダブルバッファリングを行う\n• GX_SCREEN_FRONT は即座に画面に描画されるためちらつきが発生する\n• GX_Init の直後に1度呼ぶのが一般的な使い方\n• DXLibの SetDrawScreen() に相当する関数'
],

'GXLib_System-ClearDrawScreen': [
  'int ClearDrawScreen()',
  '描画先スクリーンを背景色でクリアし、フレーム描画を開始する。メインループの先頭で毎フレーム呼び出す。内部的にはBeginFrame処理（コマンドリストのリセット、レンダーターゲットのクリア等）を行う。成功時は0を返す。',
  '// 毎フレームのクリアと描画\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();\n    \n    // ここに描画処理を記述\n    DrawBox(100, 100, 300, 200, GetColor(0, 255, 0), TRUE);\n    \n    ScreenFlip();\n}',
  '• 毎フレームの描画開始時に必ず呼ぶこと\n• SetBackgroundColor で指定した色でクリアされる（デフォルトは黒）\n• ScreenFlip とペアで使用する\n• DXLibの ClearDrawScreen() に相当する関数'
],

'GXLib_System-ScreenFlip': [
  'int ScreenFlip()',
  'バックバッファの内容を画面に表示し、フレーム描画を終了する。メインループの最後で毎フレーム呼び出す。内部的にはEndFrame処理（コマンドリスト実行、Present、フレーム同期等）を行う。成功時は0を返す。',
  '// フレーム描画の完了と表示\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();\n    \n    DrawString(10, 10, "Frame!", GetColor(255, 255, 255));\n    \n    ScreenFlip();\n}',
  '• ClearDrawScreen とペアで毎フレーム呼ぶこと\n• 内部でVSyncによる同期が行われる\n• 全バッチ（SpriteBatch, PrimitiveBatch）は自動フラッシュされる\n• DXLibの ScreenFlip() に相当する関数'
],

'GXLib_System-SetBackgroundColor': [
  'int SetBackgroundColor(int r, int g, int b)',
  'ClearDrawScreen で使用される画面クリア時の背景色を設定する。各成分は0〜255で指定する。デフォルトは黒（0, 0, 0）。成功時は0を返す。',
  '// 背景色を青に設定\nSetBackgroundColor(0, 0, 128);\n\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();  // 青でクリアされる\n    ScreenFlip();\n}',
  '• デフォルトの背景色は黒（0, 0, 0）\n• 一度設定すると以降のClearDrawScreen全てに適用される\n• DXLibの SetBackgroundColor() に相当する関数'
],

// ============================================================
// GXLib_Graphics  (Compat/GXLib.h)  —  page-GXLib_Graphics
// ============================================================

'GXLib_Graphics-LoadGraph': [
  'int LoadGraph(const TCHAR* filePath)',
  '画像ファイルを読み込み、グラフィックハンドルを返す。PNG, JPG, BMP, TGA等の主要画像形式に対応する。内部ではTextureManagerを使用してGPUテクスチャを作成する。成功時はグラフィックハンドル（0以上の整数）を返し、失敗時は-1を返す。',
  '// 画像の読み込みと描画\nint hImage = LoadGraph("Assets/player.png");\nif (hImage == -1) {\n    // エラー処理\n}\n\n// メインループで描画\nDrawGraph(100, 100, hImage, TRUE);\n\n// 不要になったら解放\nDeleteGraph(hImage);',
  '• 対応形式: PNG, JPG, BMP, TGA（STB_IMAGE経由）\n• 戻り値のハンドルはDeleteGraphで解放するまで有効\n• 同一ファイルを複数回読み込むと別ハンドルが割り当てられる\n• DXLibの LoadGraph() に相当する関数'
],

'GXLib_Graphics-DeleteGraph': [
  'int DeleteGraph(int handle)',
  'グラフィックハンドルを削除し、GPUテクスチャリソースを解放する。LoadGraphやLoadDivGraphで取得したハンドルが不要になった際に呼び出す。成功時は0を返す。',
  '// リソースの解放\nint hImage = LoadGraph("Assets/enemy.png");\n// 使用後に解放\nDeleteGraph(hImage);',
  '• 無効なハンドルを渡した場合も安全（エラーにはならない）\n• 解放後のハンドルを描画に使用しないこと\n• GX_End で全リソースは自動解放されるが、明示的な解放を推奨'
],

'GXLib_Graphics-LoadDivGraph': [
  'int LoadDivGraph(const TCHAR* filePath, int allNum, int xNum, int yNum, int xSize, int ySize, int* handleBuf)',
  '画像を等間隔に分割して複数のグラフィックハンドルに読み込む。スプライトシートやアニメーションフレームの読み込みに使用する。handleBufにはallNum個以上の要素を持つ配列を渡す。成功時は0、失敗時は-1を返す。',
  '// 4x4=16分割のスプライトシート読み込み\nint handles[16];\nLoadDivGraph("Assets/anim.png", 16, 4, 4, 64, 64, handles);\n\n// アニメーション描画\nint frame = (GetNowCount() / 100) % 16;\nDrawGraph(200, 200, handles[frame], TRUE);\n\n// 全ハンドルの解放\nfor (int i = 0; i < 16; i++) {\n    DeleteGraph(handles[i]);\n}',
  '• handleBuf配列は allNum 個以上の要素が必要\n• xNum * yNum == allNum でなければならない\n• 各分割画像のサイズは xSize x ySize ピクセル\n• DXLibの LoadDivGraph() に相当する関数'
],

'GXLib_Graphics-GetGraphSize': [
  'int GetGraphSize(int handle, int* width, int* height)',
  'グラフィックハンドルに対応する画像のサイズを取得する。幅と高さがそれぞれwidth, heightに格納される。レイアウト計算や当たり判定のサイズ取得に使用する。成功時は0を返す。',
  '// 画像サイズを取得して中央に描画\nint hImg = LoadGraph("Assets/logo.png");\nint w, h;\nGetGraphSize(hImg, &w, &h);\n\nint screenW = 1280, screenH = 720;\nDrawGraph((screenW - w) / 2, (screenH - h) / 2, hImg, TRUE);',
  '• 無効なハンドルの場合は width, height に 0 が格納される\n• LoadDivGraphで分割した場合は分割後のサイズが返される'
],

'GXLib_Graphics-DrawGraph': [
  'int DrawGraph(int x, int y, int handle, int transFlag)',
  '画像を指定座標に等倍で描画する。(x, y) は画像の左上隅の位置。transFlagをTRUEにするとアルファチャンネルによる透過描画が有効になる。最も基本的な画像描画関数。成功時は0を返す。',
  '// 基本的な画像描画\nint hPlayer = LoadGraph("Assets/player.png");\n\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();\n    DrawGraph(100, 200, hPlayer, TRUE);\n    ScreenFlip();\n}',
  '• (x, y) は左上隅の座標\n• transFlag=TRUE でアルファチャンネルを使用した透過描画\n• transFlag=FALSE で不透明描画（高速）\n• SetDrawBlendMode の設定が反映される'
],

'GXLib_Graphics-DrawRotaGraph': [
  'int DrawRotaGraph(int cx, int cy, double extRate, double angle, int handle, int transFlag)',
  '画像を回転・拡大縮小して描画する。(cx, cy) は描画の中心座標、extRateは拡大率（1.0で等倍）、angleは回転角度（ラジアン単位、時計回りが正）。キャラクターの向き変更やエフェクト表現に使用する。成功時は0を返す。',
  '// 回転・拡大描画\nint hImage = LoadGraph("Assets/arrow.png");\ndouble angle = 0.0;\n\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();\n    angle += 0.02;  // 毎フレーム回転\n    DrawRotaGraph(640, 360, 2.0, angle, hImage, TRUE);\n    ScreenFlip();\n}',
  '• (cx, cy) は画像の中心が配置される座標\n• extRate: 1.0=等倍, 2.0=2倍, 0.5=半分\n• angle: ラジアン単位（360度 = 2π ≈ 6.283）\n• DXLibの DrawRotaGraph() に相当する関数'
],

'GXLib_Graphics-DrawExtendGraph': [
  'int DrawExtendGraph(int x1, int y1, int x2, int y2, int handle, int transFlag)',
  '画像を指定矩形に合わせて拡大縮小して描画する。(x1, y1)が左上、(x2, y2)が右下の座標となる。画像は指定矩形にフィットするように自動的にスケーリングされる。UI要素のリサイズ表示などに使用する。成功時は0を返す。',
  '// 矩形に合わせて拡大描画\nint hBG = LoadGraph("Assets/background.png");\n\n// 画面全体に引き伸ばして描画\nDrawExtendGraph(0, 0, 1280, 720, hBG, FALSE);\n\n// 小さく縮小して描画\nDrawExtendGraph(10, 10, 110, 110, hBG, TRUE);',
  '• 元の画像アスペクト比は維持されない（矩形に合わせて変形される）\n• x1 < x2, y1 < y2 となるように指定すること\n• DXLibの DrawExtendGraph() に相当する関数'
],

'GXLib_Graphics-DrawLine': [
  'int DrawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness = 1)',
  '始点(x1, y1)から終点(x2, y2)まで直線を描画する。colorはGetColorで生成したカラー値、thicknessは線の太さ（ピクセル単位、デフォルト1）。デバッグ表示やガイド線の描画に使用する。成功時は0を返す。',
  '// 線の描画\nunsigned int white = GetColor(255, 255, 255);\nunsigned int red   = GetColor(255, 0, 0);\n\n// 細い白線\nDrawLine(100, 100, 500, 300, white);\n\n// 太い赤線\nDrawLine(100, 200, 500, 400, red, 3);',
  '• thickness のデフォルト値は 1\n• PrimitiveBatch を内部で使用する\n• スプライト描画との混在時は自動でバッチ切替が行われる'
],

'GXLib_Graphics-DrawBox': [
  'int DrawBox(int x1, int y1, int x2, int y2, unsigned int color, int fillFlag)',
  '矩形を描画する。(x1, y1)が左上、(x2, y2)が右下の座標。fillFlagをTRUEにすると塗りつぶし、FALSEにすると枠線のみを描画する。UI背景やデバッグ表示に使用する。成功時は0を返す。',
  '// 矩形の描画\nunsigned int green = GetColor(0, 255, 0);\nunsigned int gray  = GetColor(128, 128, 128);\n\n// 塗りつぶし矩形（HPバー背景）\nDrawBox(10, 10, 210, 30, gray, TRUE);\n\n// 塗りつぶし矩形（HPバー）\nDrawBox(10, 10, 10 + hp * 2, 30, green, TRUE);\n\n// 枠線のみ\nDrawBox(10, 10, 210, 30, GetColor(255, 255, 255), FALSE);',
  '• fillFlag=TRUE: 塗りつぶし描画\n• fillFlag=FALSE: 枠線のみ描画\n• SetDrawBlendMode のブレンド設定が反映される\n• DXLibの DrawBox() に相当する関数'
],

'GXLib_Graphics-DrawCircle': [
  'int DrawCircle(int cx, int cy, int r, unsigned int color, int fillFlag = TRUE)',
  '円を描画する。(cx, cy)が中心座標、rが半径。fillFlagをTRUEにすると塗りつぶし、FALSEにすると枠線のみを描画する。fillFlagのデフォルトはTRUE。成功時は0を返す。',
  '// 円の描画\nunsigned int yellow = GetColor(255, 255, 0);\n\n// 塗りつぶし円\nDrawCircle(320, 240, 50, yellow, TRUE);\n\n// 枠線のみの円\nDrawCircle(320, 240, 80, GetColor(255, 0, 0), FALSE);',
  '• fillFlag のデフォルト値は TRUE（塗りつぶし）\n• 内部では多角形近似で描画される\n• DXLibの DrawCircle() に相当する関数'
],

'GXLib_Graphics-DrawTriangle': [
  'int DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int color, int fillFlag)',
  '三角形を描画する。3頂点の座標(x1,y1), (x2,y2), (x3,y3)を指定する。fillFlagをTRUEにすると塗りつぶし、FALSEにすると枠線のみを描画する。矢印やポインターなどの図形表現に使用する。成功時は0を返す。',
  '// 三角形の描画\nunsigned int cyan = GetColor(0, 255, 255);\n\n// 塗りつぶし三角形\nDrawTriangle(320, 100, 220, 300, 420, 300, cyan, TRUE);\n\n// 枠線のみ\nDrawTriangle(320, 100, 220, 300, 420, 300,\n    GetColor(255, 255, 255), FALSE);',
  '• 頂点の指定順序は時計回り・反時計回りどちらでも可\n• fillFlag=TRUE: 塗りつぶし, FALSE: 枠線のみ\n• DXLibの DrawTriangle() に相当する関数'
],

'GXLib_Graphics-SetDrawBlendMode': [
  'int SetDrawBlendMode(int blendMode, int blendParam)',
  '描画ブレンドモードとパラメータを設定する。以降の全ての描画関数に適用される。blendModeはGX_BLENDMODE_*定数、blendParamはブレンドの強さ（0〜255）を指定する。アルファブレンド時のblendParamは不透明度として機能する。成功時は0を返す。',
  '// 半透明描画\nSetDrawBlendMode(GX_BLENDMODE_ALPHA, 128);\nDrawBox(100, 100, 300, 300, GetColor(255, 0, 0), TRUE);\n\n// 加算ブレンド（光のエフェクト）\nSetDrawBlendMode(GX_BLENDMODE_ADD, 255);\nDrawGraph(200, 200, hEffect, TRUE);\n\n// ブレンドモードを元に戻す\nSetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);',
  '• GX_BLENDMODE_NOBLEND: ブレンド無し（不透明）\n• GX_BLENDMODE_ALPHA: アルファブレンド（blendParam=不透明度）\n• GX_BLENDMODE_ADD: 加算ブレンド\n• GX_BLENDMODE_SUB: 減算ブレンド\n• GX_BLENDMODE_MUL: 乗算ブレンド\n• 描画後はNOBLENDに戻すことを推奨'
],

// ============================================================
// GXLib_Text  (Compat/GXLib.h)  —  page-GXLib_Text
// ============================================================

'GXLib_Text-DrawString': [
  'int DrawString(int x, int y, const TCHAR* str, unsigned int color)',
  'デフォルトフォントで文字列を描画する。(x, y)は文字列の左上座標、colorはGetColorで生成した文字色。簡易的なテキスト表示に使用する。日本語文字列にも対応している。成功時は0を返す。',
  '// 文字列の描画\nunsigned int white = GetColor(255, 255, 255);\nDrawString(10, 10, "Hello, World!", white);\nDrawString(10, 40, "こんにちは、世界！", white);',
  '• デフォルトフォント（初期化時に自動生成）を使用する\n• 日本語（漢字・ひらがな・カタカナ）対応\n• フォントサイズの変更にはCreateFontToHandle + DrawStringToHandleを使用\n• DXLibの DrawString() に相当する関数'
],

'GXLib_Text-DrawFormatString': [
  'int DrawFormatString(int x, int y, unsigned int color, const TCHAR* format, ...)',
  'デフォルトフォントでprintf形式の書式付き文字列を描画する。数値の表示やデバッグ情報の出力に便利。formatにはprintf互換のフォーマット指定子（%d, %f, %s等）を使用できる。成功時は0を返す。',
  '// 書式付き文字列の描画\nint score = 12345;\nfloat fps = 59.8f;\nunsigned int white = GetColor(255, 255, 255);\n\nDrawFormatString(10, 10, white, "Score: %d", score);\nDrawFormatString(10, 40, white, "FPS: %.1f", fps);\nDrawFormatString(10, 70, white, "Pos: (%d, %d)", px, py);',
  '• printf と同じフォーマット指定子が使用可能\n• 内部でバッファにフォーマット後、DrawString相当の処理を行う\n• 引数の順序: x, y, color, format, ... （DXLibと同一）\n• DXLibの DrawFormatString() に相当する関数'
],

'GXLib_Text-GetDrawStringWidth': [
  'int GetDrawStringWidth(const TCHAR* str, int strLen)',
  'デフォルトフォントで文字列を描画した場合の幅をピクセル単位で返す。テキストの中央揃えや右揃えレイアウト計算に使用する。strLenには計測する文字数を指定する。',
  '// テキストの中央揃え描画\nconst char* text = "GAME OVER";\nint textWidth = GetDrawStringWidth(text, strlen(text));\nint screenW = 1280;\nint x = (screenW - textWidth) / 2;\nDrawString(x, 300, text, GetColor(255, 0, 0));',
  '• 戻り値はピクセル単位の描画幅\n• strLen はバイト数ではなく文字数を指定\n• デフォルトフォントの設定に依存する\n• DXLibの GetDrawStringWidth() に相当する関数'
],

'GXLib_Text-CreateFontToHandle': [
  'int CreateFontToHandle(const TCHAR* fontName, int size, int thick, int fontType = -1)',
  'カスタムフォントハンドルを作成する。fontNameにはフォント名（例: "メイリオ", "MS ゴシック"）、sizeにフォントサイズ、thickに太さ（0〜9）を指定する。作成したハンドルはDrawStringToHandleで使用する。成功時はフォントハンドル、失敗時は-1を返す。',
  '// フォントの作成と使用\nint hLargeFont = CreateFontToHandle("メイリオ", 32, 5);\nint hSmallFont = CreateFontToHandle("MS ゴシック", 14, 2);\n\nDrawStringToHandle(10, 10, "タイトル",\n    GetColor(255, 255, 0), hLargeFont);\nDrawStringToHandle(10, 50, "説明テキスト",\n    GetColor(200, 200, 200), hSmallFont);\n\n// 不要になったら解放\nDeleteFontToHandle(hLargeFont);\nDeleteFontToHandle(hSmallFont);',
  '• fontType: GX_FONTTYPE_NORMAL(0), GX_FONTTYPE_EDGE(1), GX_FONTTYPE_ANTIALIASING(2), -1でデフォルト\n• thick: 0（最細）〜9（最太）\n• 内部でDirectWriteによるフォントアトラスが生成される\n• DXLibの CreateFontToHandle() に相当する関数'
],

'GXLib_Text-DeleteFontToHandle': [
  'int DeleteFontToHandle(int handle)',
  'CreateFontToHandleで作成したフォントハンドルを削除し、リソースを解放する。不要になったフォントは明示的に削除することを推奨する。成功時は0を返す。',
  '// フォントの解放\nint hFont = CreateFontToHandle("メイリオ", 24, 3);\n// ... 使用 ...\nDeleteFontToHandle(hFont);',
  '• 無効なハンドルを渡しても安全\n• GX_End で全フォントは自動解放されるが、明示的な解放を推奨\n• 解放後のハンドルを使用しないこと'
],

'GXLib_Text-DrawStringToHandle': [
  'int DrawStringToHandle(int x, int y, const TCHAR* str, unsigned int color, int fontHandle)',
  '指定フォントハンドルを使用して文字列を描画する。CreateFontToHandleで作成したカスタムフォントで表示する場合に使用する。デフォルトフォントとは異なるサイズや書体で描画できる。成功時は0を返す。',
  '// カスタムフォントでの描画\nint hTitle = CreateFontToHandle("メイリオ", 48, 6);\nint hBody  = CreateFontToHandle("MS 明朝", 16, 2);\n\nDrawStringToHandle(100, 50, "ゲームタイトル",\n    GetColor(255, 215, 0), hTitle);\nDrawStringToHandle(100, 120, "ストーリーテキストがここに入ります",\n    GetColor(255, 255, 255), hBody);',
  '• fontHandle はCreateFontToHandleの戻り値を使用\n• 無効なハンドルの場合は描画されない\n• SetDrawBlendMode の設定がテキスト描画にも反映される\n• DXLibの DrawStringToHandle() に相当する関数'
],

// ============================================================
// GXLib_Input  (Compat/GXLib.h)  —  page-GXLib_Input
// ============================================================

'GXLib_Input-CheckHitKey': [
  'int CheckHitKey(int keyCode)',
  '指定キーが現在押されているかどうかを確認する。keyCodeにはKEY_INPUT_*定数（DirectInput DIK_*互換コード）を指定する。押されている場合は1、押されていない場合は0を返す。毎フレーム押下状態をチェックする場合に使用する。',
  '// キー入力の確認\nif (CheckHitKey(KEY_INPUT_ESCAPE)) {\n    break;  // ESCキーで終了\n}\n\nif (CheckHitKey(KEY_INPUT_LEFT)) {\n    playerX -= 5;  // 左移動\n}\nif (CheckHitKey(KEY_INPUT_RIGHT)) {\n    playerX += 5;  // 右移動\n}\nif (CheckHitKey(KEY_INPUT_SPACE)) {\n    Jump();  // ジャンプ\n}',
  '• KEY_INPUT_* 定数はDirectInputのDIK_*と同じコード体系\n• 押下中は毎フレーム1を返す（トリガー検出ではない）\n• トリガー検出が必要な場合は前フレームとの差分を自前で管理する\n• DXLibの CheckHitKey() に相当する関数'
],

'GXLib_Input-GetHitKeyStateAll': [
  'int GetHitKeyStateAll(char* keyStateBuf)',
  '全キーの押下状態を256要素のchar配列に一括取得する。各要素は押下時1、非押下時0となる。複数キーの同時判定を効率的に行う場合に使用する。成功時は0を返す。',
  '// 全キー状態の一括取得\nchar keys[256];\nGetHitKeyStateAll(keys);\n\nif (keys[KEY_INPUT_W]) playerY -= 5;\nif (keys[KEY_INPUT_S]) playerY += 5;\nif (keys[KEY_INPUT_A]) playerX -= 5;\nif (keys[KEY_INPUT_D]) playerX += 5;\n\nif (keys[KEY_INPUT_SPACE]) {\n    Fire();\n}',
  '• 配列は256要素必要（KEY_INPUT_*定数がインデックス）\n• 1フレームに複数キーを判定する場合はCheckHitKeyよりも効率的\n• 毎フレーム呼び出すこと\n• DXLibの GetHitKeyStateAll() に相当する関数'
],

'GXLib_Input-GetMouseInput': [
  'int GetMouseInput()',
  'マウスボタンの押下状態をビットフラグとして返す。MOUSE_INPUT_LEFT, MOUSE_INPUT_RIGHT, MOUSE_INPUT_MIDDLEの論理和で表現される。複数ボタンの同時押しも検出可能。',
  '// マウスボタンの確認\nint mouse = GetMouseInput();\n\nif (mouse & MOUSE_INPUT_LEFT) {\n    // 左クリック中\n    Shoot();\n}\nif (mouse & MOUSE_INPUT_RIGHT) {\n    // 右クリック中\n    Aim();\n}\nif (mouse & MOUSE_INPUT_MIDDLE) {\n    // 中ボタン（ホイールクリック）\n}',
  '• MOUSE_INPUT_LEFT (1): 左ボタン\n• MOUSE_INPUT_RIGHT (2): 右ボタン\n• MOUSE_INPUT_MIDDLE (4): 中ボタン\n• ビット演算（&）で個別ボタンの判定を行う\n• DXLibの GetMouseInput() に相当する関数'
],

'GXLib_Input-GetMousePoint': [
  'int GetMousePoint(int* x, int* y)',
  'マウスカーソルのクライアント領域上の座標を取得する。ウィンドウの左上を(0,0)とするピクセル座標がx, yに格納される。UIのクリック判定やカーソル追従に使用する。成功時は0を返す。',
  '// マウス座標の取得\nint mx, my;\nGetMousePoint(&mx, &my);\n\n// カーソル位置に円を描画\nDrawCircle(mx, my, 10, GetColor(255, 0, 0), TRUE);\n\n// クリック判定\nif (GetMouseInput() & MOUSE_INPUT_LEFT) {\n    if (mx >= btnX && mx <= btnX + btnW &&\n        my >= btnY && my <= btnY + btnH) {\n        OnButtonClick();\n    }\n}',
  '• 座標はクライアント領域の左上が原点(0,0)\n• ウィンドウ外にカーソルがある場合も値は取得可能\n• DXLibの GetMousePoint() に相当する関数'
],

'GXLib_Input-GetMouseWheelRotVol': [
  'int GetMouseWheelRotVol()',
  'マウスホイールの回転量を取得する。上方向への回転が正、下方向が負の値となる。スクロール処理やズーム制御に使用する。前回呼び出しからの差分ではなく、フレーム中の累積回転量を返す。',
  '// マウスホイールでズーム制御\nint wheel = GetMouseWheelRotVol();\nif (wheel > 0) {\n    zoomLevel += 0.1;\n} else if (wheel < 0) {\n    zoomLevel -= 0.1;\n}\n\n// リストのスクロール\nscrollPos -= wheel * 20;',
  '• 正の値: 上方向（奥に回す）\n• 負の値: 下方向（手前に回す）\n• 回転がない場合は0を返す\n• DXLibの GetMouseWheelRotVol() に相当する関数'
],

'GXLib_Input-GetJoypadInputState': [
  'int GetJoypadInputState(int inputType)',
  'ゲームパッドの入力状態をビットフラグとして返す。inputTypeにはGX_INPUT_PAD1〜GX_INPUT_PAD4を指定する。PAD_INPUT_*定数の論理和で方向キーやボタンの状態を表現する。',
  '// ゲームパッド入力\nint pad = GetJoypadInputState(GX_INPUT_PAD1);\n\nif (pad & PAD_INPUT_UP)    playerY -= 5;\nif (pad & PAD_INPUT_DOWN)  playerY += 5;\nif (pad & PAD_INPUT_LEFT)  playerX -= 5;\nif (pad & PAD_INPUT_RIGHT) playerX += 5;\n\nif (pad & PAD_INPUT_A) {\n    Jump();\n}\nif (pad & PAD_INPUT_B) {\n    Attack();\n}',
  '• GX_INPUT_PAD1〜PAD4: 1P〜4Pのパッド指定\n• PAD_INPUT_UP/DOWN/LEFT/RIGHT: 十字キー\n• PAD_INPUT_A〜Z: 各ボタン\n• PAD_INPUT_L/R: トリガーボタン\n• PAD_INPUT_START: スタートボタン\n• ビット演算（&）で個別ボタンの判定を行う'
],

// ============================================================
// GXLib_Audio  (Compat/GXLib.h)  —  page-GXLib_Audio
// ============================================================

'GXLib_Audio-LoadSoundMem': [
  'int LoadSoundMem(const TCHAR* filePath)',
  'サウンドファイルをメモリに読み込み、サウンドハンドルを返す。WAV形式等に対応する。SE（効果音）の読み込みに主に使用する。成功時はサウンドハンドル（0以上の整数）、失敗時は-1を返す。',
  '// 効果音の読み込み\nint hShot = LoadSoundMem("Assets/shot.wav");\nint hExplosion = LoadSoundMem("Assets/explosion.wav");\nif (hShot == -1 || hExplosion == -1) {\n    // エラー処理\n}',
  '• WAV形式に対応\n• ハンドルはDeleteSoundMemで解放するまで有効\n• メモリ上に全データを展開するためファイルサイズに注意\n• DXLibの LoadSoundMem() に相当する関数'
],

'GXLib_Audio-PlaySoundMem': [
  'int PlaySoundMem(int handle, int playType, int resumeFlag = TRUE)',
  'サウンドを再生する。playTypeで再生方法を指定する。GX_PLAYTYPE_NORMALは再生完了まで待機、GX_PLAYTYPE_BACKはバックグラウンド再生（SE向け）、GX_PLAYTYPE_LOOPはループ再生（BGM向け）。成功時は0を返す。',
  '// SEのバックグラウンド再生\nif (CheckHitKey(KEY_INPUT_SPACE)) {\n    PlaySoundMem(hShot, GX_PLAYTYPE_BACK);\n}\n\n// BGMのループ再生\nPlaySoundMem(hBGM, GX_PLAYTYPE_LOOP);',
  '• GX_PLAYTYPE_NORMAL (0): 再生完了まで処理をブロック\n• GX_PLAYTYPE_BACK (1): バックグラウンド再生（SE用）\n• GX_PLAYTYPE_LOOP (2): ループ再生（BGM用）\n• resumeFlag=TRUE で最初から再生（デフォルト）\n• DXLibの PlaySoundMem() に相当する関数'
],

'GXLib_Audio-StopSoundMem': [
  'int StopSoundMem(int handle)',
  '再生中のサウンドを停止する。ループ再生中のBGMの停止や、SE再生の中断に使用する。成功時は0を返す。',
  '// サウンドの停止\nStopSoundMem(hBGM);\n\n// 全効果音を停止\nStopSoundMem(hShot);\nStopSoundMem(hExplosion);',
  '• 再生中でないハンドルに対しても安全に呼べる\n• 停止後にPlaySoundMemで再度再生可能\n• DXLibの StopSoundMem() に相当する関数'
],

'GXLib_Audio-DeleteSoundMem': [
  'int DeleteSoundMem(int handle)',
  'サウンドハンドルを削除し、メモリを解放する。不要になったサウンドは明示的に削除することを推奨する。成功時は0を返す。',
  '// サウンドリソースの解放\nDeleteSoundMem(hShot);\nDeleteSoundMem(hExplosion);\nDeleteSoundMem(hBGM);',
  '• 再生中のサウンドを削除した場合は自動停止される\n• 無効なハンドルを渡しても安全\n• GX_End で全サウンドは自動解放されるが、明示的な解放を推奨'
],

'GXLib_Audio-ChangeVolumeSoundMem': [
  'int ChangeVolumeSoundMem(int volume, int handle)',
  'サウンドの音量を変更する。volumeは0（無音）〜255（最大音量）で指定する。BGMの音量調整やフェードイン/フェードアウトに使用する。注意: 引数の順序はvolume, handleの順。成功時は0を返す。',
  '// 音量の調整\n// BGMを半分の音量に\nChangeVolumeSoundMem(128, hBGM);\n\n// SEを最大音量で\nChangeVolumeSoundMem(255, hShot);\n\n// フェードアウト\nfor (int v = 255; v >= 0; v -= 5) {\n    ChangeVolumeSoundMem(v, hBGM);\n    // 少し待つ\n}',
  '• volume: 0（無音）〜 255（最大音量）\n• 引数の順序に注意: volume が先、handle が後\n• 内部で 0-255 を 0.0-1.0 のリニアスケールに変換\n• DXLibの ChangeVolumeSoundMem() に相当する関数'
],

'GXLib_Audio-CheckSoundMem': [
  'int CheckSoundMem(int handle)',
  'サウンドが現在再生中かどうかを確認する。再生中なら1、停止中なら0を返す。ループ再生の制御や再生完了の検出に使用する。',
  '// 再生状態の確認\nif (CheckSoundMem(hBGM) == 0) {\n    // BGMが停止していたら再生開始\n    PlaySoundMem(hBGM, GX_PLAYTYPE_LOOP);\n}\n\n// SE再生完了を待つ\nwhile (CheckSoundMem(hJingle) == 1) {\n    ProcessMessage();\n}',
  '• 戻り値: 1=再生中, 0=停止中\n• ループ再生中は常に1を返す\n• 無効なハンドルの場合は0を返す\n• DXLibの CheckSoundMem() に相当する関数'
],

// ============================================================
// GXLib_3D  (Compat/GXLib.h)  —  page-GXLib_3D
// ============================================================

'GXLib_3D-SetCameraPositionAndTarget': [
  'int SetCameraPositionAndTarget(VECTOR position, VECTOR target)',
  '3Dカメラの位置と注視点を設定する。positionはカメラのワールド座標での位置、targetはカメラが向く先の座標。3Dシーンのビュー行列を決定する基本的な関数。成功時は0を返す。',
  '// カメラの設定\nVECTOR camPos = VGet(0.0f, 10.0f, -20.0f);\nVECTOR camTarget = VGet(0.0f, 0.0f, 0.0f);\nSetCameraPositionAndTarget(camPos, camTarget);\n\n// プレイヤー追従カメラ\nVECTOR playerPos = VGet(px, py, pz);\nVECTOR offset = VGet(0.0f, 5.0f, -10.0f);\nSetCameraPositionAndTarget(\n    VAdd(playerPos, offset), playerPos);',
  '• 内部でCamera3DのSetPositionとSetTargetを呼び出す\n• Up方向はデフォルトで(0, 1, 0)\n• 毎フレーム呼び出すことでカメラを動的に制御できる\n• DXLibの SetCameraPositionAndTarget_UpVecY() に近い機能'
],

'GXLib_3D-SetCameraNearFar': [
  'int SetCameraNearFar(float nearZ, float farZ)',
  '3Dカメラのニアクリップ面とファークリップ面の距離を設定する。この範囲外のオブジェクトは描画されない。デプスバッファの精度にも影響するため、適切な値を設定することが重要。成功時は0を返す。',
  '// クリップ面の設定\nSetCameraNearFar(0.1f, 1000.0f);\n\n// 広大なシーンの場合\nSetCameraNearFar(1.0f, 10000.0f);',
  '• nearZ: ニアクリップ距離（0より大きい値）\n• farZ: ファークリップ距離（nearZより大きい値）\n• nearZ を小さくしすぎるとZファイティングが発生する\n• デフォルト値は near=0.1, far=1000.0\n• DXLibの SetCameraNearFar() に相当する関数'
],

'GXLib_3D-LoadModel': [
  'int LoadModel(const TCHAR* filePath)',
  '3DモデルファイルをGPUメモリに読み込み、モデルハンドルを返す。glTF(.gltf)およびGLB(.glb)形式に対応する。PBRマテリアルやスケルタルアニメーションを含むモデルも読み込み可能。成功時はモデルハンドル（0以上の整数）、失敗時は-1を返す。',
  '// モデルの読み込み\nint hModel = LoadModel("Assets/character.glb");\nif (hModel == -1) {\n    // エラー処理\n}\n\n// 位置を設定して描画\nSetModelPosition(hModel, VGet(0.0f, 0.0f, 0.0f));\nDrawModel(hModel);\n\n// 不要になったら解放\nDeleteModel(hModel);',
  '• 対応形式: glTF 2.0 (.gltf + .bin), GLB (.glb)\n• PBRマテリアル（BaseColor, MetallicRoughness, Normal等）対応\n• cgltf ライブラリを使用してパースされる\n• DXLibの MV1LoadModel() に相当する関数'
],

'GXLib_3D-DeleteModel': [
  'int DeleteModel(int handle)',
  '3Dモデルハンドルを削除し、GPUリソースを解放する。不要になったモデルは明示的に削除することを推奨する。成功時は0を返す。',
  '// モデルの解放\nDeleteModel(hCharacter);\nDeleteModel(hStage);',
  '• 無効なハンドルを渡しても安全\n• GX_End で全モデルは自動解放される\n• 解放後のハンドルを描画に使用しないこと\n• DXLibの MV1DeleteModel() に相当する関数'
],

'GXLib_3D-DrawModel': [
  'int DrawModel(int handle)',
  '3Dモデルを現在の変換設定（位置・スケール・回転）で描画する。SetModelPosition, SetModelScale, SetModelRotationで事前に変換を設定してから呼び出す。成功時は0を返す。',
  '// モデルの描画\nSetModelPosition(hModel, VGet(0.0f, 0.0f, 0.0f));\nSetModelScale(hModel, VGet(1.0f, 1.0f, 1.0f));\nSetModelRotation(hModel, VGet(0.0f, 3.14f, 0.0f));\nDrawModel(hModel);\n\n// 複数モデルの描画\nfor (int i = 0; i < enemyCount; i++) {\n    SetModelPosition(hEnemy, enemies[i].pos);\n    DrawModel(hEnemy);\n}',
  '• 描画前にSetModelPosition等で変換を設定すること\n• 内部でRenderer3DのDrawMeshを呼び出す\n• PBRシェーダーで描画される（ライティング対応）\n• DXLibの MV1DrawModel() に相当する関数'
],

'GXLib_3D-SetModelPosition': [
  'int SetModelPosition(int handle, VECTOR position)',
  '3Dモデルのワールド座標での位置を設定する。DrawModelで描画する際にこの位置が反映される。成功時は0を返す。',
  '// モデルの位置設定\nSetModelPosition(hModel, VGet(10.0f, 0.0f, 5.0f));\n\n// フレームごとに移動\nfloat x = 0.0f;\nwhile (ProcessMessage() == 0) {\n    x += 0.1f;\n    SetModelPosition(hModel, VGet(x, 0.0f, 0.0f));\n    DrawModel(hModel);\n}',
  '• VECTOR のx, y, zがワールド座標に対応\n• 毎フレーム呼び出すことで移動を実現\n• 内部でTransform3D::SetPositionを呼び出す\n• DXLibの MV1SetPosition() に相当する関数'
],

'GXLib_3D-SetModelScale': [
  'int SetModelScale(int handle, VECTOR scale)',
  '3Dモデルの各軸方向のスケール（拡大率）を設定する。VGet(1,1,1)が等倍。均一スケーリングや軸方向への引き伸ばしが可能。成功時は0を返す。',
  '// スケールの設定\n// 等倍\nSetModelScale(hModel, VGet(1.0f, 1.0f, 1.0f));\n\n// 2倍に拡大\nSetModelScale(hModel, VGet(2.0f, 2.0f, 2.0f));\n\n// 横方向のみ引き伸ばし\nSetModelScale(hModel, VGet(2.0f, 1.0f, 1.0f));',
  '• VGet(1,1,1) が等倍\n• 各軸を個別にスケーリング可能\n• 負のスケールで反転も可能\n• DXLibの MV1SetScale() に相当する関数'
],

'GXLib_3D-SetModelRotation': [
  'int SetModelRotation(int handle, VECTOR rotation)',
  '3Dモデルの回転をオイラー角（ラジアン）で設定する。rotation.x, .y, .zがそれぞれX軸、Y軸、Z軸回りの回転角度を表す。成功時は0を返す。',
  '// 回転の設定\nfloat angle = 0.0f;\nwhile (ProcessMessage() == 0) {\n    angle += 0.02f;\n    // Y軸回りに回転\n    SetModelRotation(hModel, VGet(0.0f, angle, 0.0f));\n    DrawModel(hModel);\n}',
  '• rotation: 各軸の回転角度（ラジアン単位）\n• 回転適用順序: X → Y → Z\n• 360度 = 2π ≈ 6.283 ラジアン\n• DXLibの MV1SetRotationXYZ() に相当する関数'
],

// ============================================================
// GXLib_Math  (Compat/GXLib.h)  —  page-GXLib_Math
// ============================================================

'GXLib_Math-VGet': [
  'VECTOR VGet(float x, float y, float z)',
  'VECTOR構造体を生成するヘルパー関数。x, y, z の3成分を持つ3Dベクトルを作成する。3D空間での位置・方向・速度等の表現に使用する。最も頻繁に使われるベクトル生成関数。',
  '// ベクトルの生成\nVECTOR pos    = VGet(10.0f, 5.0f, 3.0f);\nVECTOR dir    = VGet(0.0f, 0.0f, 1.0f);  // Z方向\nVECTOR origin = VGet(0.0f, 0.0f, 0.0f);  // 原点\n\n// カメラ設定\nSetCameraPositionAndTarget(\n    VGet(0.0f, 10.0f, -20.0f),\n    VGet(0.0f, 0.0f, 0.0f)\n);',
  '• VECTOR構造体は {float x, y, z} のメンバーを持つ\n• DXLibのVGetと完全互換\n• 2D用途ではz=0.0fを指定して使用可能'
],

'GXLib_Math-VAdd': [
  'VECTOR VAdd(VECTOR a, VECTOR b)',
  '2つのベクトルの加算を行い、結果を返す。各成分が個別に加算される（a.x+b.x, a.y+b.y, a.z+b.z）。位置の移動やベクトルの合成に使用する。',
  '// ベクトルの加算\nVECTOR pos = VGet(10.0f, 0.0f, 0.0f);\nVECTOR vel = VGet(1.0f, 0.5f, 0.0f);\n\n// 位置を速度分だけ移動\npos = VAdd(pos, vel);\n\n// カメラのオフセット追加\nVECTOR camPos = VAdd(playerPos, VGet(0.0f, 5.0f, -10.0f));',
  '• a + b の各成分ごとの加算結果を返す\n• 元のベクトル a, b は変更されない\n• DXLibの VAdd() と同一の動作'
],

'GXLib_Math-VSub': [
  'VECTOR VSub(VECTOR a, VECTOR b)',
  '2つのベクトルの減算を行い、結果を返す。各成分が個別に減算される（a.x-b.x, a.y-b.y, a.z-b.z）。2点間の方向ベクトルの算出や距離計算の前処理に使用する。',
  '// ベクトルの減算\nVECTOR a = VGet(10.0f, 5.0f, 3.0f);\nVECTOR b = VGet(3.0f, 2.0f, 1.0f);\nVECTOR diff = VSub(a, b);  // (7, 3, 2)\n\n// プレイヤーから敵への方向ベクトル\nVECTOR toEnemy = VSub(enemyPos, playerPos);\nVECTOR dir = VNorm(toEnemy);  // 正規化',
  '• a - b の各成分ごとの減算結果を返す\n• 2点間の方向ベクトル算出によく使われる\n• DXLibの VSub() と同一の動作'
],

'GXLib_Math-VScale': [
  'VECTOR VScale(VECTOR v, float scale)',
  'ベクトルをスカラー倍する。全成分がscale倍される（v.x*s, v.y*s, v.z*s）。ベクトルの長さ変更や反転に使用する。',
  '// スカラー倍\nVECTOR dir = VGet(1.0f, 0.0f, 0.0f);\nVECTOR fast = VScale(dir, 5.0f);     // 5倍の速度\nVECTOR slow = VScale(dir, 0.5f);     // 半分の速度\nVECTOR rev  = VScale(dir, -1.0f);    // 反転\n\n// 正規化して速度を設定\nVECTOR moveDir = VNorm(VSub(target, pos));\nVECTOR velocity = VScale(moveDir, speed);',
  '• 全成分に均一にscaleが乗算される\n• scale=0 でゼロベクトルになる\n• scale=-1 でベクトルが反転する\n• DXLibの VScale() と同一の動作'
],

'GXLib_Math-VDot': [
  'float VDot(VECTOR a, VECTOR b)',
  '2つのベクトルの内積（ドット積）を計算する。a.x*b.x + a.y*b.y + a.z*b.z の値を返す。ベクトル間の角度判定や射影計算に使用する。同じ方向のベクトルでは正、逆方向では負の値を返す。',
  '// 内積の計算\nVECTOR forward = VGet(0.0f, 0.0f, 1.0f);\nVECTOR toEnemy = VNorm(VSub(enemyPos, playerPos));\nfloat dot = VDot(forward, toEnemy);\n\nif (dot > 0.0f) {\n    // 敵は前方にいる\n} else {\n    // 敵は後方にいる\n}',
  '• 戻り値: a・b = |a||b|cos(θ)\n• 同方向: 正の値, 直交: 0, 逆方向: 負の値\n• 正規化ベクトル同士の内積は cos(θ)\n• DXLibの VDot() と同一の動作'
],

'GXLib_Math-VCross': [
  'VECTOR VCross(VECTOR a, VECTOR b)',
  '2つのベクトルの外積（クロス積）を計算する。aとbの両方に直交するベクトルを返す。法線ベクトルの算出や回転軸の決定に使用する。結果の向きは右手系の法則に従う。',
  '// 外積の計算（法線算出）\nVECTOR edge1 = VSub(v1, v0);\nVECTOR edge2 = VSub(v2, v0);\nVECTOR normal = VNorm(VCross(edge1, edge2));\n\n// 回転軸の算出\nVECTOR up = VGet(0.0f, 1.0f, 0.0f);\nVECTOR forward = VGet(0.0f, 0.0f, 1.0f);\nVECTOR right = VCross(up, forward);  // 右方向',
  '• a × b の結果は a, b 両方に直交するベクトル\n• 順序を入れ替えると結果が反転する（VCross(a,b) = -VCross(b,a)）\n• 結果の長さは |a||b|sin(θ) （平行なベクトルではゼロベクトル）\n• DXLibの VCross() と同一の動作'
],

'GXLib_Math-VNorm': [
  'VECTOR VNorm(VECTOR v)',
  'ベクトルを正規化（単位ベクトル化）する。方向はそのままに長さを1にしたベクトルを返す。方向ベクトルの算出や速度の方向成分の取得に使用する。',
  '// ベクトルの正規化\nVECTOR toTarget = VSub(targetPos, myPos);\nVECTOR dir = VNorm(toTarget);  // 方向のみ（長さ1）\n\n// 一定速度で移動\nVECTOR velocity = VScale(dir, speed);\npos = VAdd(pos, velocity);',
  '• 戻り値のベクトルの長さは常に1.0\n• ゼロベクトルを正規化した場合の動作は未定義\n• 方向の取得に使用し、速度はVScaleで後から設定する\n• DXLibの VNorm() と同一の動作'
],

'GXLib_Math-VSize': [
  'float VSize(VECTOR v)',
  'ベクトルの大きさ（長さ、ユークリッドノルム）を返す。sqrt(x*x + y*y + z*z) を計算する。2点間の距離計算や衝突判定の距離比較に使用する。',
  '// ベクトルの長さ\nVECTOR vel = VGet(3.0f, 4.0f, 0.0f);\nfloat speed = VSize(vel);  // 5.0\n\n// 2点間の距離\nfloat dist = VSize(VSub(posA, posB));\nif (dist < 10.0f) {\n    // 衝突判定\n    OnCollision();\n}',
  '• 戻り値は0以上の浮動小数点値\n• 内部でsqrt計算があるため、距離の比較だけならVDot(v,v)の方が高速\n• ゼロベクトルの場合は0.0fを返す\n• DXLibの VSize() と同一の動作'
],

'GXLib_Math-MGetIdent': [
  'MATRIX MGetIdent()',
  '4x4単位行列を返す。対角成分が1、その他が0の行列。行列演算の初期値やリセットに使用する。',
  '// 単位行列の取得\nMATRIX mat = MGetIdent();\n\n// 変換行列の合成\nMATRIX transform = MGetIdent();\ntransform = MMult(transform, MGetRotY(angle));\ntransform = MMult(transform, MGetTranslate(pos));',
  '• 対角成分が1.0、その他が0.0の4x4行列\n• 任意の行列に単位行列を乗算すると元の行列が得られる\n• DXLibの MGetIdent() と同一の動作'
],

'GXLib_Math-MMult': [
  'MATRIX MMult(MATRIX a, MATRIX b)',
  '2つの4x4行列の乗算を行い、結果を返す。行列の合成（複数の変換を1つにまとめる）に使用する。変換の適用順序は右から左（b が先に適用される）。',
  '// 行列の合成\nMATRIX rot   = MGetRotY(3.14f);  // Y軸回転\nMATRIX trans = MGetTranslate(VGet(10.0f, 0.0f, 0.0f)); // 移動\n\n// 回転してから移動\nMATRIX world = MMult(rot, trans);\n\n// 3つの変換の合成\nMATRIX scale = MGetIdent();  // スケール行列\nMATRIX final = MMult(MMult(scale, rot), trans);',
  '• 行列の乗算は順序に依存する（MMult(a,b) != MMult(b,a)）\n• 変換は右の行列から先に適用される\n• DXLibの MMult() と同一の動作'
],

'GXLib_Math-MGetRotX': [
  'MATRIX MGetRotX(float angle)',
  'X軸回りの回転行列を生成する。angleはラジアン単位の回転角度。3Dオブジェクトの回転変換に使用する。',
  '// X軸回転\nfloat angleX = 3.14159f / 4.0f;  // 45度\nMATRIX rotX = MGetRotX(angleX);\n\n// 変換の合成\nMATRIX world = MMult(MGetRotX(pitch), MGetRotY(yaw));',
  '• angle: ラジアン単位（π/2 = 90度）\n• 右手系の回転方向に準拠\n• DXLibの MGetRotX() と同一の動作'
],

'GXLib_Math-MGetRotY': [
  'MATRIX MGetRotY(float angle)',
  'Y軸回りの回転行列を生成する。angleはラジアン単位の回転角度。キャラクターの向き変更など、水平方向の回転に最も頻繁に使用される。',
  '// Y軸回転（水平方向の向き）\nfloat yaw = 0.0f;\nyaw += 0.02f;  // 毎フレーム回転\nMATRIX rotY = MGetRotY(yaw);\n\n// 移動方向の計算\nMATRIX world = MMult(MGetRotY(facing), MGetTranslate(pos));',
  '• angle: ラジアン単位\n• Y軸は上方向なので水平面での回転になる\n• DXLibの MGetRotY() と同一の動作'
],

'GXLib_Math-MGetRotZ': [
  'MATRIX MGetRotZ(float angle)',
  'Z軸回りの回転行列を生成する。angleはラジアン単位の回転角度。2D的な回転や傾き表現に使用する。',
  '// Z軸回転（画面上の回転）\nfloat roll = 3.14159f / 6.0f;  // 30度\nMATRIX rotZ = MGetRotZ(roll);\n\n// 飛行機の姿勢制御\nMATRIX attitude = MMult(\n    MMult(MGetRotZ(roll), MGetRotX(pitch)),\n    MGetRotY(yaw)\n);',
  '• angle: ラジアン単位\n• Z軸が画面奥方向の場合、画面上の回転になる\n• DXLibの MGetRotZ() と同一の動作'
],

'GXLib_Math-MGetTranslate': [
  'MATRIX MGetTranslate(VECTOR v)',
  '平行移動行列を生成する。vの各成分が移動量となる。3Dオブジェクトの位置設定や移動に使用する。',
  '// 平行移動行列\nVECTOR offset = VGet(10.0f, 0.0f, 5.0f);\nMATRIX trans = MGetTranslate(offset);\n\n// 回転→移動の合成\nMATRIX world = MMult(MGetRotY(angle), MGetTranslate(pos));',
  '• v.x, v.y, v.z が各軸方向の移動量\n• 他の変換行列と MMult で合成して使用する\n• DXLibの MGetTranslate() と同一の動作'
],

// ============================================================
// CompatContext  (Compat/CompatContext.h)  —  page-CompatContext
// ============================================================

'CompatContext-Instance': [
  'static CompatContext& Instance()',
  'CompatContextのシングルトンインスタンスへの参照を返す。簡易API関数は全てこのインスタンスを通じて内部サブシステムにアクセスする。CompatContextはGXLibの全サブシステム（GraphicsDevice, SpriteBatch, InputManager等）を保持する。',
  '// 内部的な使用例（通常ユーザーは直接使わない）\nauto& ctx = GX_Internal::CompatContext::Instance();\nctx.Initialize();\n\n// サブシステムへのアクセス\nauto& dev = ctx.graphicsDevice;\nauto& input = ctx.inputManager;',
  '• GX_Internal 名前空間内のクラス\n• 通常はGXLib.hの簡易API関数経由で間接的に使用する\n• スレッドセーフではない（メインスレッドからのみ使用すること）\n• 内部実装クラスのため、直接使用は推奨しない'
],

'CompatContext-Initialize': [
  'bool Initialize()',
  '全GXLibサブシステムを初期化する。ウィンドウ作成、DirectX 12デバイス初期化、SpriteBatch/PrimitiveBatch/FontManager/InputManager/AudioManager/Renderer3D等の初期化を順番に行う。GX_Init()から内部的に呼び出される。成功時はtrue、失敗時はfalseを返す。',
  '// GX_Init内部で呼ばれる\nauto& ctx = GX_Internal::CompatContext::Instance();\nif (!ctx.Initialize()) {\n    // 初期化失敗\n    return -1;\n}',
  '• GX_Init() の内部実装で使用される\n• 初期化順序: Application → GraphicsDevice → CommandQueue → SwapChain → SpriteBatch → PrimitiveBatch → FontManager → TextRenderer → InputManager → AudioManager → Renderer3D → PostEffect\n• ChangeWindowMode/SetGraphMode の設定値が反映される\n• 失敗した場合はエラーログを出力する'
],

'CompatContext-Shutdown': [
  'void Shutdown()',
  '全サブシステムを終了し、リソースを解放する。GPUの全コマンド完了を待機してからデバイスやウィンドウを破棄する。GX_End()から内部的に呼び出される。',
  '// GX_End内部で呼ばれる\nauto& ctx = GX_Internal::CompatContext::Instance();\nctx.Shutdown();',
  '• GX_End() の内部実装で使用される\n• GPU処理完了待機後に各サブシステムを逆順で解放\n• 複数回呼んでも安全'
],

'CompatContext-ProcessMessage': [
  'int ProcessMessage()',
  'ウィンドウメッセージ処理と入力状態の更新を行う。Window::ProcessMessages()とInputManager::Update()を呼び出す。ウィンドウが閉じられた場合は-1、正常時は0を返す。',
  '// グローバル関数ProcessMessageの内部実装\nauto& ctx = GX_Internal::CompatContext::Instance();\nint result = ctx.ProcessMessage();\nif (result == -1) {\n    // ウィンドウが閉じられた\n}',
  '• グローバル関数 ProcessMessage() の実体\n• Window メッセージ処理 + Input 状態更新を行う\n• Audio の更新もここで行われる'
],

'CompatContext-EnsureSpriteBatch': [
  'void EnsureSpriteBatch()',
  'SpriteBatchをアクティブな描画バッチにする。PrimitiveBatchがアクティブな場合は先に自動フラッシュ（End+DrawCall）してからSpriteBatchのBeginを呼ぶ。画像描画・テキスト描画の前に内部的に呼ばれる。',
  '// 内部的に画像描画時に呼ばれる\nauto& ctx = GX_Internal::CompatContext::Instance();\nctx.EnsureSpriteBatch();\n// SpriteBatch を使用した描画が可能になる',
  '• PrimitiveBatch → SpriteBatch の自動切替を行う\n• 既にSpriteBatchがアクティブな場合は何もしない\n• ユーザーが直接呼ぶ必要はない（簡易API関数が内部で呼ぶ）\n• ActiveBatch::Sprite に設定される'
],

'CompatContext-EnsurePrimitiveBatch': [
  'void EnsurePrimitiveBatch()',
  'PrimitiveBatchをアクティブな描画バッチにする。SpriteBatchがアクティブな場合は先に自動フラッシュ（End+DrawCall）してからPrimitiveBatchのBeginを呼ぶ。プリミティブ描画（線・矩形・円等）の前に内部的に呼ばれる。',
  '// 内部的にプリミティブ描画時に呼ばれる\nauto& ctx = GX_Internal::CompatContext::Instance();\nctx.EnsurePrimitiveBatch();\n// PrimitiveBatch を使用した描画が可能になる',
  '• SpriteBatch → PrimitiveBatch の自動切替を行う\n• 既にPrimitiveBatchがアクティブな場合は何もしない\n• ユーザーが直接呼ぶ必要はない（簡易API関数が内部で呼ぶ）\n• ActiveBatch::Primitive に設定される'
],

'CompatContext-FlushAll': [
  'void FlushAll()',
  'アクティブな全バッチ（SpriteBatchまたはPrimitiveBatch）をフラッシュして終了する。描画コマンドをGPUに送信し、バッチの状態をActiveBatch::Noneにリセットする。フレーム終了時やバッチ強制終了時に使用される。',
  '// フレーム終了前の全バッチフラッシュ\nauto& ctx = GX_Internal::CompatContext::Instance();\nctx.FlushAll();\n// 全描画コマンドが送信された',
  '• EndFrame 内部で自動的に呼ばれる\n• SpriteBatch, PrimitiveBatch の両方をEnd→DrawCallする\n• バッチ状態を ActiveBatch::None にリセット'
],

'CompatContext-BeginFrame': [
  'void BeginFrame()',
  'フレーム描画を開始する。コマンドリストのリセット、レンダーターゲットのクリア、背景色の適用を行う。ClearDrawScreen()から内部的に呼び出される。',
  '// ClearDrawScreen内部で呼ばれる\nauto& ctx = GX_Internal::CompatContext::Instance();\nctx.BeginFrame();\n// フレーム描画の準備完了',
  '• ClearDrawScreen() の内部実装で使用される\n• コマンドアロケータとコマンドリストをリセット\n• レンダーターゲットを背景色でクリア\n• frameActive フラグを true に設定'
],

'CompatContext-EndFrame': [
  'void EndFrame()',
  'フレーム描画を終了し、画面に表示する。全バッチのフラッシュ、ポストエフェクトの適用、コマンドリストの実行、SwapChainのPresent、フレーム同期を行う。ScreenFlip()から内部的に呼び出される。',
  '// ScreenFlip内部で呼ばれる\nauto& ctx = GX_Internal::CompatContext::Instance();\nctx.EndFrame();\n// 画面に描画結果が表示される',
  '• ScreenFlip() の内部実装で使用される\n• FlushAll → PostEffect.Resolve → CommandList.Close → Execute → Present → Fence Wait\n• frameActive フラグを false に設定\n• フレームインデックスを更新する'
],

// ============================================================
// CompatTypes  (Compat/CompatTypes.h)  —  page-CompatTypes
// ============================================================

'CompatTypes-GX_SCREEN_FRONT': [
  '#define GX_SCREEN_FRONT 0',
  '表画面（フロントバッファ）を示す定数。SetDrawScreenに渡すことで、直接表示される画面に描画する。通常はGX_SCREEN_BACKを使用するため、この定数を使うことは稀。',
  '// フロントバッファに直接描画（通常は使用しない）\nSetDrawScreen(GX_SCREEN_FRONT);\nDrawString(10, 10, "Direct draw", GetColor(255, 255, 255));',
  '• 値: 0\n• フロントバッファへの直接描画はちらつきの原因になる\n• 通常は GX_SCREEN_BACK を使用すること\n• DXLibの DX_SCREEN_FRONT に相当する定数'
],

'CompatTypes-GX_SCREEN_BACK': [
  '#define GX_SCREEN_BACK 1',
  '裏画面（バックバッファ）を示す定数。SetDrawScreenに渡すことで、バックバッファに描画する。ScreenFlipでフロントバッファに転送（表示）される。ダブルバッファリングの標準的な描画先。',
  '// バックバッファに描画（推奨）\nSetDrawScreen(GX_SCREEN_BACK);\n\nwhile (ProcessMessage() == 0) {\n    ClearDrawScreen();\n    DrawString(10, 10, "Hello", GetColor(255, 255, 255));\n    ScreenFlip();\n}',
  '• 値: 1\n• ダブルバッファリングの標準的な使い方\n• GX_Init直後に SetDrawScreen(GX_SCREEN_BACK) を呼ぶのが一般的\n• DXLibの DX_SCREEN_BACK に相当する定数'
],

'CompatTypes-GX_BLENDMODE_NOBLEND': [
  '#define GX_BLENDMODE_NOBLEND 0',
  'ブレンド無し（不透明描画）モードを示す定数。SetDrawBlendModeに渡すことで、アルファブレンドを無効にし完全不透明で描画する。デフォルトのブレンドモード。',
  '// ブレンドモードをリセット\nSetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);',
  '• 値: 0\n• デフォルトのブレンドモード\n• 半透明描画の後にリセットするために使用する\n• DXLibの DX_BLENDMODE_NOBLEND に相当する定数'
],

'CompatTypes-GX_BLENDMODE_ALPHA': [
  '#define GX_BLENDMODE_ALPHA 1',
  'アルファブレンドモードを示す定数。SetDrawBlendModeに渡すことで、アルファ値による半透明描画が有効になる。blendParamは不透明度（0=完全透明、255=完全不透明）として機能する。',
  '// 半透明描画\nSetDrawBlendMode(GX_BLENDMODE_ALPHA, 128);\nDrawBox(100, 100, 300, 200, GetColor(255, 0, 0), TRUE);\nSetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);',
  '• 値: 1\n• blendParam: 0（完全透明）〜 255（完全不透明）\n• フェードイン/フェードアウトに使用\n• DXLibの DX_BLENDMODE_ALPHA に相当する定数'
],

'CompatTypes-GX_BLENDMODE_ADD': [
  '#define GX_BLENDMODE_ADD 2',
  '加算ブレンドモードを示す定数。描画元と描画先のピクセル値を加算する。光のエフェクトやパーティクルの発光表現に使用する。',
  '// 加算ブレンド（光のエフェクト）\nSetDrawBlendMode(GX_BLENDMODE_ADD, 255);\nDrawGraph(x, y, hLight, TRUE);\nSetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);',
  '• 値: 2\n• 加算合成のため白に近づく（明るくなる）\n• パーティクルや爆発エフェクトに最適\n• DXLibの DX_BLENDMODE_ADD に相当する定数'
],

'CompatTypes-GX_BLENDMODE_SUB': [
  '#define GX_BLENDMODE_SUB 3',
  '減算ブレンドモードを示す定数。描画先のピクセル値から描画元の値を減算する。影や暗闘のエフェクト表現に使用する。',
  '// 減算ブレンド（影のエフェクト）\nSetDrawBlendMode(GX_BLENDMODE_SUB, 255);\nDrawBox(100, 100, 300, 200, GetColor(50, 50, 50), TRUE);\nSetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);',
  '• 値: 3\n• 減算合成のため黒に近づく（暗くなる）\n• DXLibの DX_BLENDMODE_SUB に相当する定数'
],

'CompatTypes-GX_BLENDMODE_MUL': [
  '#define GX_BLENDMODE_MUL 4',
  '乗算ブレンドモードを示す定数。描画元と描画先のピクセル値を乗算する。テクスチャの暗部強調や影の合成に使用する。',
  '// 乗算ブレンド\nSetDrawBlendMode(GX_BLENDMODE_MUL, 255);\nDrawGraph(x, y, hShadow, TRUE);\nSetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);',
  '• 値: 4\n• 乗算合成（色を掛け合わせる）\n• DXLibの DX_BLENDMODE_MULA に近い機能'
],

'CompatTypes-GX_BLENDMODE_SCREEN': [
  '#define GX_BLENDMODE_SCREEN 5',
  'スクリーンブレンドモードを示す定数。乗算の逆で、明るい部分を際立たせる合成を行う。発光や明るいオーバーレイ表現に使用する。',
  '// スクリーンブレンド\nSetDrawBlendMode(GX_BLENDMODE_SCREEN, 255);\nDrawGraph(x, y, hGlow, TRUE);\nSetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);',
  '• 値: 5\n• スクリーン合成（1 - (1-src)*(1-dst)）\n• 結果は常に元より明るくなる\n• DXLibには直接対応するモードがない GXLib独自の追加'
],

'CompatTypes-GX_PLAYTYPE_NORMAL': [
  '#define GX_PLAYTYPE_NORMAL 0',
  '通常再生タイプを示す定数。PlaySoundMemに渡すことで、再生完了まで処理をブロック（待機）する。短い効果音の同期再生に使用する。',
  '// 通常再生（再生完了まで待機）\nPlaySoundMem(hJingle, GX_PLAYTYPE_NORMAL);',
  '• 値: 0\n• 再生完了まで処理がブロックされる\n• ゲームループ内での使用は推奨しない（フリーズする）\n• DXLibの DX_PLAYTYPE_NORMAL に相当する定数'
],

'CompatTypes-GX_PLAYTYPE_BACK': [
  '#define GX_PLAYTYPE_BACK 1',
  'バックグラウンド再生タイプを示す定数。PlaySoundMemに渡すことで、再生開始後すぐに処理が返る。SE（効果音）の再生に最も一般的に使用される。',
  '// バックグラウンド再生（SE用）\nPlaySoundMem(hShot, GX_PLAYTYPE_BACK);\nPlaySoundMem(hExplosion, GX_PLAYTYPE_BACK);',
  '• 値: 1\n• 再生開始後すぐに次の処理に進む\n• SE（効果音）の再生に推奨\n• DXLibの DX_PLAYTYPE_BACK に相当する定数'
],

'CompatTypes-GX_PLAYTYPE_LOOP': [
  '#define GX_PLAYTYPE_LOOP 2',
  'ループ再生タイプを示す定数。PlaySoundMemに渡すことで、サウンドが終端に達したら自動的に先頭から再生し直す。BGM（背景音楽）の再生に使用する。',
  '// ループ再生（BGM用）\nPlaySoundMem(hBGM, GX_PLAYTYPE_LOOP);\n\n// 停止するまで永続的に再生される\nStopSoundMem(hBGM);',
  '• 値: 2\n• BGMの再生に推奨\n• StopSoundMem で明示的に停止するまで再生が続く\n• DXLibの DX_PLAYTYPE_LOOP に相当する定数'
],

'CompatTypes-KEY_INPUT': [
  '#define KEY_INPUT_* (各種キーコード)',
  'キーボードの各キーに対応するキーコード定数群。DirectInputのDIK_*コード体系と互換性がある。CheckHitKeyやGetHitKeyStateAllのインデックスとして使用する。主要なキー: KEY_INPUT_ESCAPE(0x01), KEY_INPUT_SPACE(0x39), KEY_INPUT_RETURN(0x1C), KEY_INPUT_UP/DOWN/LEFT/RIGHT(方向キー), KEY_INPUT_A〜Z(アルファベット)。',
  '// キーコードの使用例\nif (CheckHitKey(KEY_INPUT_ESCAPE)) break;\nif (CheckHitKey(KEY_INPUT_SPACE))  Jump();\nif (CheckHitKey(KEY_INPUT_UP))     playerY -= 5;\nif (CheckHitKey(KEY_INPUT_DOWN))   playerY += 5;\nif (CheckHitKey(KEY_INPUT_LEFT))   playerX -= 5;\nif (CheckHitKey(KEY_INPUT_RIGHT))  playerX += 5;\nif (CheckHitKey(KEY_INPUT_Z))      Attack();\nif (CheckHitKey(KEY_INPUT_X))      Defend();',
  '• DirectInput DIK_* コード体系と同一\n• アルファベット: KEY_INPUT_A(0x1E)〜KEY_INPUT_Z(0x2C)付近\n• 数字: KEY_INPUT_0(0x0B)〜KEY_INPUT_9(0x0A)付近\n• ファンクション: KEY_INPUT_F1(0x3B)〜KEY_INPUT_F12(0x58)\n• 方向キー: KEY_INPUT_UP(0xC8), DOWN(0xD0), LEFT(0xCB), RIGHT(0xCD)\n• 修飾キー: KEY_INPUT_LSHIFT(0x2A), KEY_INPUT_LCONTROL(0x1D), KEY_INPUT_LALT(0x38)'
],

'CompatTypes-MOUSE_INPUT': [
  '#define MOUSE_INPUT_LEFT 1\n#define MOUSE_INPUT_RIGHT 2\n#define MOUSE_INPUT_MIDDLE 4',
  'マウスボタンを示すビットフラグ定数。GetMouseInputの戻り値とビットAND演算で個別のボタン押下を判定する。複数ボタンの同時押しも検出可能。',
  '// マウスボタンの判定\nint mouse = GetMouseInput();\nbool leftClick   = (mouse & MOUSE_INPUT_LEFT)   != 0;\nbool rightClick  = (mouse & MOUSE_INPUT_RIGHT)  != 0;\nbool middleClick = (mouse & MOUSE_INPUT_MIDDLE) != 0;',
  '• MOUSE_INPUT_LEFT (1): 左ボタン\n• MOUSE_INPUT_RIGHT (2): 右ボタン\n• MOUSE_INPUT_MIDDLE (4): 中ボタン（ホイールクリック）\n• ビットフラグなので & 演算子で判定する\n• DXLibの MOUSE_INPUT_* と同一の値'
],

'CompatTypes-PAD_INPUT': [
  '#define PAD_INPUT_DOWN 0x01\n#define PAD_INPUT_LEFT 0x02\n#define PAD_INPUT_RIGHT 0x04\n#define PAD_INPUT_UP 0x08\n#define PAD_INPUT_A 0x10\n(他多数)',
  'ゲームパッドのボタンを示すビットフラグ定数群。GetJoypadInputStateの戻り値とビットAND演算で個別のボタン押下を判定する。方向キー（UP/DOWN/LEFT/RIGHT）とボタン（A〜Z, L, R, START, M）がある。',
  '// ゲームパッドの判定\nint pad = GetJoypadInputState(GX_INPUT_PAD1);\nif (pad & PAD_INPUT_UP)    MoveUp();\nif (pad & PAD_INPUT_DOWN)  MoveDown();\nif (pad & PAD_INPUT_A)     Jump();\nif (pad & PAD_INPUT_B)     Attack();\nif (pad & PAD_INPUT_START) Pause();',
  '• 方向: PAD_INPUT_UP(0x08), DOWN(0x01), LEFT(0x02), RIGHT(0x04)\n• ボタン: PAD_INPUT_A(0x10)〜Z(0x200)\n• トリガー: PAD_INPUT_L(0x400), R(0x800)\n• PAD_INPUT_START(0x1000), PAD_INPUT_M(0x2000)\n• DXLibの PAD_INPUT_* と同一の値'
],

'CompatTypes-VECTOR': [
  'struct VECTOR { float x; float y; float z; }',
  '3Dベクトルを表す構造体。x, y, zの3つのfloat成分を持つ。3D空間での位置、方向、速度、スケール等の表現に使用する。VGet関数で生成し、VAdd/VSub/VScale等の関数で演算する。DXLibのVECTOR型と互換性がある。',
  '// VECTOR の使用\nVECTOR pos = VGet(10.0f, 5.0f, 3.0f);\nVECTOR vel = VGet(1.0f, 0.0f, 0.0f);\npos = VAdd(pos, vel);\n\nfloat length = VSize(pos);\nVECTOR dir = VNorm(vel);\n\n// 直接メンバーアクセス\npos.x = 100.0f;\npos.y = 0.0f;\npos.z = 50.0f;',
  '• メンバー: float x, y, z\n• VGet() で生成するのが標準的な使い方\n• メンバーに直接代入も可能\n• DXLibの VECTOR 型と互換\n• 3D関数（SetModelPosition, SetCameraPositionAndTarget等）の引数として使用'
],

'CompatTypes-MATRIX': [
  'struct MATRIX { float m[4][4]; }',
  '4x4行列を表す構造体。m[行][列]の2次元配列で行列要素にアクセスする。3D変換（回転・平行移動・スケール）の合成や、座標変換に使用する。MGetIdent/MGetRotX/MGetRotY/MGetRotZ/MGetTranslate/MMult関数で操作する。DXLibのMATRIX型と互換性がある。',
  '// MATRIX の使用\nMATRIX ident = MGetIdent();\nMATRIX rot   = MGetRotY(3.14f);\nMATRIX trans = MGetTranslate(VGet(10.0f, 0.0f, 0.0f));\nMATRIX world = MMult(rot, trans);\n\n// 直接要素アクセス\nfloat val = world.m[0][0];',
  '• メンバー: float m[4][4] （m[行][列]）\n• MGetIdent() で単位行列を取得\n• MMult() で行列同士を乗算して変換を合成\n• DXLibの MATRIX 型と互換'
],

}
