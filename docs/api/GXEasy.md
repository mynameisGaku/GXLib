# GXEasy API リファレンス

GXEasy は GXLib の簡易アプリケーションラッパーと DXLib 互換関数を提供する。

## GXEasy::App クラス

アプリケーションの基底クラス。継承してライフサイクルメソッドをオーバーライドする。

| メソッド | 説明 |
|---|---|
| `virtual AppConfig GetConfig() const` | ウィンドウ設定を返す。既定値を変えたい場合にオーバーライドする |
| `virtual void OnBoot()` | `GX_Init()` の前に呼ばれる |
| `virtual void Start()` | `GX_Init()` の直後に1回だけ呼ばれる（初期化処理） |
| `virtual void Update(float dt)` | 毎フレーム呼ばれる。`dt` は前フレームからの経過秒数 |
| `virtual void Draw()` | `Update()` の後に毎フレーム呼ばれる（描画処理） |
| `virtual void Release()` | `GX_End()` の前に1回だけ呼ばれる（後片付け） |

## AppConfig 構造体

| フィールド | 型 | 既定値 | 説明 |
|---|---|---|---|
| `title` | `std::wstring` | `L"GXLib Easy App"` | ウィンドウタイトル |
| `width` | `int` | `1280` | 画面幅 |
| `height` | `int` | `720` | 画面高さ |
| `windowed` | `bool` | `true` | ウィンドウモード |
| `autoClear` | `bool` | `true` | 自動画面クリア |
| `autoPresent` | `bool` | `true` | 自動ScreenFlip |
| `allowEscapeExit` | `bool` | `true` | ESCキーで終了 |
| `targetFps` | `int` | `0` | FPS上限（0=無制限） |
| `bgR/bgG/bgB` | `int` | `0` | 背景色 RGB (0-255) |

### 使用例

```cpp
#include "GXEasy.h"

class MyApp : public GXEasy::App {
    AppConfig GetConfig() const override {
        AppConfig c;
        c.title = L"My Game";
        c.width = 1920;
        c.height = 1080;
        return c;
    }
    void Start() override { /* 初期化 */ }
    void Update(float dt) override { /* 更新 */ }
    void Draw() override { /* 描画 */ }
};

GX_EASY_APP(MyApp)
```

## DXLib 互換関数

### システム

| 関数 | 説明 |
|---|---|
| `int GX_Init()` | GXLib を初期化する。成功時 0、失敗時 -1 |
| `int GX_End()` | 全リソースを解放して終了する |
| `int ProcessMessage()` | ウィンドウメッセージ処理。ウィンドウが閉じられた場合 -1 |
| `int SetMainWindowText(const TCHAR* title)` | ウィンドウタイトルを設定する |
| `int ChangeWindowMode(int flag)` | TRUE でウィンドウモード |
| `int SetGraphMode(int w, int h, int bits)` | 画面解像度と色深度を設定する（GX_Init 前に呼ぶ） |
| `unsigned int GetColor(int r, int g, int b)` | RGB 値から 0xFFRRGGBB 形式のカラー値を生成する |
| `int SetDrawScreen(int screen)` | 描画先スクリーン設定 (`GX_SCREEN_BACK`) |
| `int ClearDrawScreen()` | 画面クリアしてフレーム開始 |
| `int ScreenFlip()` | バックバッファを表示してフレーム終了 |
| `int SetBackgroundColor(int r, int g, int b)` | 背景色を設定する |

### 2D 描画

| 関数 | 説明 |
|---|---|
| `int LoadGraph(const TCHAR* path)` | 画像を読み込みハンドルを返す |
| `int DeleteGraph(int handle)` | グラフィックハンドルを解放する |
| `int LoadDivGraph(path, allNum, xNum, yNum, xSize, ySize, handleBuf)` | 画像を分割読み込みする |
| `int DrawGraph(int x, int y, int handle, int trans)` | 画像を描画する |
| `int DrawRotaGraph(cx, cy, ext, angle, handle, trans)` | 回転拡縮描画する |
| `int DrawExtendGraph(x1, y1, x2, y2, handle, trans)` | 拡大縮小描画する |
| `int DrawLine(x1, y1, x2, y2, color, thickness)` | 直線を描画する |
| `int DrawBox(x1, y1, x2, y2, color, fill)` | 矩形を描画する |
| `int DrawCircle(cx, cy, r, color, fill)` | 円を描画する |
| `int SetDrawBlendMode(int mode, int param)` | ブレンドモードを設定する |

### テキスト

| 関数 | 説明 |
|---|---|
| `int DrawString(int x, int y, const TCHAR* str, unsigned int color)` | 文字列を描画する |
| `int DrawFormatString(x, y, color, format, ...)` | 書式付き文字列を描画する |
| `int CreateFontToHandle(name, size, thick, type)` | フォントハンドルを作成する |
| `int DrawStringToHandle(x, y, str, color, fontHandle)` | 指定フォントで描画する |

### 入力

| 関数 | 説明 |
|---|---|
| `int CheckHitKey(int keyCode)` | キーが押されているか (1=押下、0=非押下) |
| `int GetHitKeyStateAll(char* buf)` | 全256キーの押下状態を配列に取得する |
| `int GetMouseInput()` | マウスボタン状態をビットフラグで取得する |
| `int GetMousePoint(int* x, int* y)` | マウス座標を取得する |
| `int GetJoypadInputState(int inputType)` | ゲームパッド入力をビットフラグで取得する |

### サウンド

| 関数 | 説明 |
|---|---|
| `int LoadSoundMem(const TCHAR* path)` | サウンドを読み込みハンドルを返す |
| `int PlaySoundMem(int handle, int playType, int resume)` | サウンドを再生する |
| `int StopSoundMem(int handle)` | サウンドを停止する |
| `int ChangeVolumeSoundMem(int volume, int handle)` | 音量を変更する (0-255) |
| `int PlayMusic(const TCHAR* path, int playType)` | BGM を再生する |

### FormatT ヘルパー

```cpp
// UNICODE/ANSI 両対応の std::format ラッパー
TString text = FormatT(TEXT("Score: %d"), score);
```
