# PostEffectShowcase

ポストエフェクトの ON/OFF を切り替えながら確認できる 3D サンプルです。
各エフェクトの効果を視覚的に比較できます。

## このサンプルで学べること

- PostEffectPipeline の初期化と使い方
- 各ポストエフェクトの視覚的な効果
- HDR (高ダイナミックレンジ) パイプラインの流れ
- JSON 設定ファイルによるパラメータの保存/読み込み

## コードのポイント

- **エフェクトチェーン**: Begin() → 3D描画 → Resolve() でポストエフェクトが自動適用
- **個別制御**: `SetBloomEnabled(true)` のように各エフェクトを個別に ON/OFF
- **パラメータ調整**: 強度・閾値などをリアルタイムに変更可能
- **JSON 保存**: F12 で現在の設定を `post_effects.json` に書き出し

## エフェクト対応キー早見表

| キー | エフェクト | 正式名称 | 効果 |
|------|-----------|---------|------|
| **1** | Bloom | ブルーム | 明るい部分から光がにじみ出す |
| **2** | SSAO | Screen Space Ambient Occlusion | 隅に自然な影を追加 |
| **3** | FXAA | Fast Approximate Anti-Aliasing | ジャギー（ギザギザ）を軽減 |
| **4** | Vignette | ビネット | 画面の四隅を暗くする |
| **5** | ColorGrading | カラーグレーディング | 映像全体の色調を調整 |
| **6** | DoF | Depth of Field (被写界深度) | ピント外をぼかす |
| **7** | MotionBlur | モーションブラー | 動きに残像を付ける |
| **8** | SSR | Screen Space Reflections | 床や水面の映り込み |
| **9** | Outline | アウトライン | 物体の輪郭線を描画 |
| **0** | TAA | Temporal Anti-Aliasing | 高品質なアンチエイリアシング |
| **R** | — | カメラ自動回転 | シーンを360度回転して確認 |
| **F12** | — | 設定保存 | 現在のエフェクト設定を JSON に保存 |

## 操作

- WASD / QE: 移動
- Shift: 高速移動
- 右クリック: マウスキャプチャ切り替え
- 1〜0: 上記エフェクト切り替え
- R: カメラ自動回転
- F12: 設定を JSON に保存
- ESC: 終了

## ビルド/実行

```bash
cmake -B build -S .    # ルートで実行
cmake --build build --config Debug --target PostEffectShowcase
```

`build/` 配下に生成される `PostEffectShowcase.exe` を実行してください。

## 関連チュートリアル

- [04_Rendering3D](../../docs/tutorials/04_Rendering3D.md) — 3D 描画とポストエフェクト
- [01_GettingStarted](../../docs/tutorials/01_GettingStarted.md) — ビルドの基本
