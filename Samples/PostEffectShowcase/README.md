# PostEffectShowcase

ポストエフェクトの ON/OFF を切り替えながら確認できる 3D サンプルです。

## 操作
- WASD / QE: 移動
- Shift: 高速移動
- 右クリック: マウスキャプチャ切り替え
- 1〜0: エフェクト切り替え（Bloom / SSAO / FXAA / Vignette / ColorGrading / DoF / MotionBlur / SSR / Outline / TAA）
- R: カメラ自動回転
- ESC: 終了

## ビルド/実行
- ルートで `cmake -B build -S .`
- `cmake --build build --config Debug --target PostEffectShowcase`
- `build/` 配下に生成される `PostEffectShowcase.exe` を実行
