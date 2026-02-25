# 06 - GXEasy::App で始める 2D ゲーム

GXEasy::App を使って、最小限のコードで 2D ゲームを作る方法を解説します。

## このチュートリアルで学ぶこと

- GXEasy::App の概要と利点
- AppConfig によるウィンドウ設定
- Start / Update / Draw ライフサイクル
- CompatContext を使った 3D シーンへのアクセス
- deltaTime を使ったフレームレート非依存の移動
- FormatT によるテキスト表示

## 前提知識

- [01_GettingStarted.md](01_GettingStarted.md) の内容（ビルドとウィンドウ表示）
- [03_Game2D.md](03_Game2D.md) の内容（入力処理の基本）

## GXEasy::App とは

従来の Compat API (`GX_Init` / `ProcessMessage` / `ScreenFlip`) を使う場合、
初期化やメインループの雛形コードを毎回書く必要がありました。
GXEasy::App はその定型処理をラッパークラスに隠蔽し、
ゲームロジックだけに集中できるようにした仕組みです。

| 方式 | 特徴 |
|------|------|
| Compat API (従来) | `WinMain` にループを自分で書く。細かい制御が可能 |
| **GXEasy::App** | クラスを継承するだけ。初期化・ループ・終了を自動管理 |

> **GXEasy.h をインクルードするだけで使えます。**
> Compat API (`DrawString`, `CheckHitKey` 等) も同時に利用可能です。

## 最小サンプル

```cpp
#include "GXEasy.h"

class MyApp : public GXEasy::App
{
public:
    void Start() override
    {
        // 初期化処理（1回だけ呼ばれる）
    }

    void Update(float dt) override
    {
        // 更新処理（毎フレーム呼ばれる）
        // dt = 前フレームからの経過時間（秒）
    }

    void Draw() override
    {
        // 描画処理（毎フレーム、Update の後に呼ばれる）
    }
};

// エントリーポイント: WinMain を自動生成するマクロ
GX_EASY_APP(MyApp)
```

`GX_EASY_APP(MyApp)` マクロが `WinMain` を生成し、
`MyApp` のインスタンスを作成してエンジンを起動します。
`GX_Init()` / `GX_End()` / メインループは全て内部で処理されるため、
ユーザーが書く必要はありません。

## AppConfig でウィンドウを設定する

`GetConfig()` をオーバーライドして、ウィンドウの設定を変更できます。

```cpp
GXEasy::AppConfig GetConfig() const override
{
    GXEasy::AppConfig config;
    config.title    = L"My 2D Game";   // ウィンドウタイトル
    config.width    = 1280;            // 画面幅 (px)
    config.height   = 720;             // 画面高さ (px)
    config.windowed = true;            // true=ウィンドウモード, false=フルスクリーン
    config.bgR = 10;                   // 背景色 R (0-255)
    config.bgG = 10;                   // 背景色 G
    config.bgB = 30;                   // 背景色 B
    config.allowEscapeExit = true;     // true=ESCキーで終了
    config.maxDeltaTime    = 0.1f;     // dt の上限 (秒)。処理落ち時の暴走防止
    config.targetFps       = 60;       // FPS 上限 (0=無制限)
    return config;
}
```

> **maxDeltaTime はなぜ必要か？**
>
> ウィンドウのドラッグ中やブレークポイント停止後に dt が極端に大きくなり、
> オブジェクトが画面外に吹き飛ぶことがあります。
> `maxDeltaTime = 0.1f` に設定すると、dt は最大でも 0.1 秒に制限されます。

## ライフサイクル

GXEasy::App の各メソッドは以下の順序で呼ばれます。

```
GetConfig()     ← ウィンドウ設定を取得
    |
OnBoot()        ← GX_Init() の前（特殊な初期設定用）
    |
GX_Init()       ← エンジン初期化（自動）
    |
Start()         ← 初期化処理（1回だけ）
    |
+-- Update(dt)  ← 毎フレーム更新
|   Draw()      ← 毎フレーム描画
|   ScreenFlip  ← 画面更新（自動）
+-- ループ
    |
Release()       ← 終了処理（1回だけ）
    |
GX_End()        ← エンジン終了（自動）
```

`autoClear` と `autoPresent` がデフォルトで `true` なので、
`ClearDrawScreen()` と `ScreenFlip()` を自分で呼ぶ必要はありません。

## 移動するオブジェクトの例

矢印キーで円を動かし、画面上に情報を表示するサンプルです。

```cpp
#include "GXEasy.h"

class MovingCircleApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"Moving Circle";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 12; config.bgG = 12; config.bgB = 28;
        return config;
    }

    void Start() override
    {
        m_x = 640.0f;  // 画面中央付近
        m_y = 360.0f;
        m_score = 0;
    }

    void Update(float dt) override
    {
        // deltaTime を掛けてフレームレート非依存にする
        const float speed = 300.0f;  // 1秒あたり 300 ピクセル移動
        if (CheckHitKey(KEY_INPUT_LEFT))  m_x -= speed * dt;
        if (CheckHitKey(KEY_INPUT_RIGHT)) m_x += speed * dt;
        if (CheckHitKey(KEY_INPUT_UP))    m_y -= speed * dt;
        if (CheckHitKey(KEY_INPUT_DOWN))  m_y += speed * dt;

        // 画面外に出ないよう制限
        m_x = (std::max)(30.0f, (std::min)(1250.0f, m_x));
        m_y = (std::max)(30.0f, (std::min)(690.0f, m_y));

        m_totalTime += dt;
    }

    void Draw() override
    {
        // 円を描画 (x, y, 半径, 色, 塗りつぶし)
        DrawCircle(
            static_cast<int>(m_x), static_cast<int>(m_y),
            30, GetColor(255, 200, 80), TRUE
        );

        // FormatT でテキストを整形して表示
        DrawString(10, 10,
            FormatT(TEXT("Pos: ({:.0f}, {:.0f})  Time: {:.1f}s"),
                    m_x, m_y, m_totalTime).c_str(),
            GetColor(255, 255, 255));

        DrawString(10, 35,
            TEXT("Arrow keys: Move  ESC: Quit"),
            GetColor(140, 180, 220));
    }

private:
    float m_x = 0, m_y = 0;
    float m_totalTime = 0;
    int   m_score = 0;
};

GX_EASY_APP(MovingCircleApp)
```

## FormatT について

`FormatT` は `std::format` をラップした関数で、
UNICODE / ANSI ビルドの両方に対応しています。

```cpp
// 数値の整形表示
TString text = FormatT(TEXT("Score: {}  HP: {}/{}"), score, hp, maxHp);

// 浮動小数点の桁数指定
TString fps = FormatT(TEXT("FPS: {:.1f}"), 1.0f / dt);

// 複数の値を組み合わせ
TString info = FormatT(TEXT("Player ({:.0f}, {:.0f}) - Level {}"), x, y, level);

// DrawString で画面に表示
DrawString(10, 10, text.c_str(), GetColor(255, 255, 255));
```

> **注意:** `FormatT` の引数は値渡し（by-value）です。
> これは MSVC の `std::make_format_args` が lvalue を要求するための制約です。

## CompatContext で 3D 機能にアクセスする

GXEasy::App の内部には `CompatContext` というシングルトンがあり、
Renderer3D / Camera3D / PostEffectPipeline 等の 3D オブジェクトを保持しています。
3D 描画が必要な場合は、このコンテキストを経由してアクセスします。

```cpp
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"

void Start() override
{
    auto& ctx      = GX_Internal::CompatContext::Instance();
    auto& renderer = ctx.renderer3D;
    auto& camera   = ctx.camera;
    auto& postFX   = ctx.postEffect;

    // ポストエフェクト設定
    postFX.SetTonemapMode(GX::TonemapMode::ACES);
    postFX.GetBloom().SetEnabled(true);
    postFX.SetFXAAEnabled(true);

    // カメラ設定
    float aspect = (float)ctx.swapChain.GetWidth() / ctx.swapChain.GetHeight();
    camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
    camera.SetPosition(0.0f, 5.0f, -10.0f);
    camera.LookAt({ 0.0f, 0.0f, 0.0f });

    // メッシュとマテリアルの作成
    m_mesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));
    m_material.constants.albedoFactor = { 0.8f, 0.2f, 0.1f, 1.0f };
}
```

> 3D 描画の完全な手順は [04_Rendering3D.md](04_Rendering3D.md) を参照してください。

## ビルドと実行

GXEasy::App を使ったサンプルは `Samples/EasyHello/` にあります。

```bash
cmake --build build --config Debug --target EasyHello
```

## よくある問題

### dt が大きすぎてオブジェクトが飛ぶ

- `AppConfig::maxDeltaTime` を `0.1f` に設定してください（デフォルトで設定済み）

### ESC で終了しない

- `AppConfig::allowEscapeExit` が `true` になっているか確認してください

### 3D 描画が表示されない

- `ctx.FlushAll()` を 3D 描画前に呼んでいるか確認してください（2D バッチとの競合を防ぐ）
- `postEffect.BeginScene()` / `EndScene()` / `Resolve()` の呼び出し順を確認してください

## 次のステップ

- [07_3DScene.md](07_3DScene.md) -- Scene/Entity システムで 3D シーンを構築する
- [08_AssetPipeline.md](08_AssetPipeline.md) -- gxconv/gxpak でアセットを管理する
- [04_Rendering3D.md](04_Rendering3D.md) -- PBR/ライティング/ポストエフェクトの詳細
