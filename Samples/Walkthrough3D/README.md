# Walkthrough3D

3D シーン内を自由に移動できるウォークスルーのサンプルです。
PBR レンダリング、ライティング、ポストエフェクトを実際に体験できます。

## このサンプルで学べること

- Camera3D によるFPS視点の3Dカメラ制御
- PBR (物理ベースレンダリング) マテリアルの設定
- 複数ライト（Directional / Point / Spot）の配置
- glTF モデルの読み込みと描画
- PostEffectPipeline によるポストエフェクト適用
- Skybox の表示

## コードのポイント

- **FPSカメラ**: マウスでカメラ回転、WASD で前後左右移動
- **PBR マテリアル**: `metallic` / `roughness` パラメータでリアルな質感
- **ポストエフェクト**: Bloom, SSAO, TAA, Tonemap 等を組み合わせた HDR パイプライン
- **CSM (カスケードシャドウマップ)**: 太陽光による広範囲の影を自動生成

## 操作

- WASD / QE: 移動（前後左右 / 上下）
- Shift: 高速移動
- 右クリック: マウスキャプチャ切り替え（カメラ回転有効/無効）
- ESC: 終了

## ビルド/実行

```bash
cmake -B build -S .    # ルートで実行
cmake --build build --config Debug --target Walkthrough3D
```

`build/` 配下に生成される `Walkthrough3D.exe` を実行してください。

## 関連チュートリアル

- [04_Rendering3D](../../docs/tutorials/04_Rendering3D.md) — 3D 描画・PBR・ライティング
- [01_GettingStarted](../../docs/tutorials/01_GettingStarted.md) — ビルドとネイティブ API の基本
