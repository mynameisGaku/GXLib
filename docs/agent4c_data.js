// Auto-generated API reference data for PostEffect classes (SSR, DepthOfField, MotionBlur, OutlineEffect, VolumetricLight, TAA, AutoExposure)
// PageId-MemberName format

{

// ============================================================
// SSR (page-SSR)
// ============================================================

'SSR-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'Screen Space Reflections エフェクトを初期化する。D3D12デバイス、画面幅・高さを指定し、SSR用のルートシグネチャ・PSO・定数バッファ・SRVヒープを作成する。ビュー空間レイマーチングによる反射を実現するためのパイプラインが構築される。アプリケーション起動時に一度だけ呼び出す。',
  '// SSR の初期化\nGX::SSR ssr;\nbool ok = ssr.Initialize(device, 1920, 1080);\nif (!ok) {\n    GX::Logger::Error("SSR init failed");\n}',
  '• 内部で 2スロット x 2フレーム = 4エントリの専用SRVヒープが作成される\n• 初期化後 ShaderLibrary へ PSO リビルダーが自動登録される\n• デフォルトでは無効状態 (m_enabled=false) で初期化される\n• Forward+ レンダリング環境向け（GBuffer不要）'
],

'SSR-Execute': [
  'void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& srcHDR, RenderTarget& destHDR, DepthBuffer& depth, const Camera3D& camera)',
  'SSR エフェクトを実行する。ソースHDRテクスチャと深度バッファからビュー空間でレイマーチングを行い、反射色を取得して結果を destHDR に書き込む。法線は深度勾配から再構成されるため、法線バッファは不要。カメラの射影行列・ビュー行列が定数バッファに設定される。',
  '// PostEffectPipeline 内での SSR 実行\nif (ssr.IsEnabled()) {\n    ssr.Execute(cmdList, frameIndex,\n        srcHDR, destHDR, depthBuffer, camera);\n}',
  '• srcHDR と destHDR は異なる RenderTarget を指定すること（ピンポン方式）\n• 深度バッファから法線を再構成する方式のため、法線GBufferが不要\n• 深度勾配法線は中心差分と片側差分のブレンドで精度を確保している\n• スカイボックス (depth>=1.0) はレイマーチ対象から除外される'
],

'SSR-SetMaxDistance': [
  'void SetMaxDistance(float d)',
  'レイマーチングの最大距離を設定する。ビュー空間での最大レイ到達距離を制御し、値が大きいほど遠くの反射を拾えるが計算コストが増加する。デフォルトは 30.0f。',
  '// 遠距離の反射も拾う\nssr.SetMaxDistance(50.0f);',
  '• ビュー空間の単位で指定する\n• 値を大きくしすぎるとステップ間隔が粗くなり反射精度が低下する場合がある\n• SetStepSize / SetMaxSteps と合わせて調整すること'
],

'SSR-SetStepSize': [
  'void SetStepSize(float s)',
  'レイマーチングの1ステップあたりの進行距離を設定する。値が小さいほど精度が上がるが、同じ距離を探索するために多くのステップが必要になる。デフォルトは 0.15f。',
  '// 高精度なレイマーチング\nssr.SetStepSize(0.08f);\nssr.SetMaxSteps(400);',
  '• ビュー空間の単位で指定する\n• SetThickness と同程度の値にすると交差判定の漏れが少なくなる\n• 小さくしすぎると MaxSteps 内で十分な距離を探索できなくなる'
],

'SSR-SetMaxSteps': [
  'void SetMaxSteps(int n)',
  'レイマーチングの最大ステップ数を設定する。1ピクセルあたりのレイマーチングループ回数の上限を制御する。値が大きいほど遠くまで探索できるが、GPUコストが増加する。デフォルトは 200。',
  '// パフォーマンス重視\nssr.SetMaxSteps(100);\nssr.SetStepSize(0.3f);',
  '• GPU負荷に直結するため、ターゲットフレームレートに合わせて調整すること\n• maxDistance / stepSize よりも小さいと最大距離に達する前にレイが打ち切られる\n• バイナリリファインメント (8回) は maxSteps とは別カウント'
],

'SSR-SetThickness': [
  'void SetThickness(float t)',
  'レイと深度バッファの交差判定の厚み閾値を設定する。ビュー空間でのZ差がこの閾値以下のとき交差と判定する。値が大きいほど反射が出やすいが、偽反射も増える。デフォルトは 0.15f。',
  '// 厚みを調整して偽反射を抑制\nssr.SetThickness(0.1f);',
  '• StepSize と同程度の値にするのが推奨される\n• 小さすぎるとレイが交差を見逃して反射が消える\n• 大きすぎると背面のオブジェクトが反射に現れる'
],

'SSR-SetIntensity': [
  'void SetIntensity(float i)',
  'SSR 反射の強度を設定する。反射色にこの値が乗算され、最終的な反射の明るさを制御する。0.0 で反射なし、1.0 でフル反射。デフォルトは 1.0f。',
  '// 控えめな反射\nssr.SetIntensity(0.5f);',
  '• Fresnel 係数 (F0=0.3) と合わせて最終的な反射量が決まる\n• 0.0 に設定すると反射が完全に消える（エフェクト自体は実行される）\n• 無効化する場合は SetEnabled(false) の方が効率的'
],

'SSR-SetEnabled': [
  'void SetEnabled(bool enabled)',
  'SSR エフェクトの有効/無効を切り替える。無効の場合、Execute が呼ばれてもパスがスキップされる。デフォルトは無効 (false)。ランタイムで R キーによるトグルが可能。',
  '// SSR を有効化\nssr.SetEnabled(true);\n\n// トグル\nif (keyboard.IsKeyTriggered(VK_R)) {\n    ssr.SetEnabled(!ssr.IsEnabled());\n}',
  '• デフォルトは OFF (false)\n• PostEffectPipeline では R キーでトグルされる\n• 有効時は 2テクスチャ (scene+depth) の SRV ヒープが使用される'
],

'SSR-IsEnabled': [
  'bool IsEnabled() const',
  'SSR エフェクトが有効かどうかを返す。true の場合、Execute でレイマーチングが実行される。',
  '// 状態表示\nif (ssr.IsEnabled()) {\n    textRenderer.Draw("SSR: ON", 10, 100);\n}',
  '• const メンバ関数'
],

'SSR-OnResize': [
  'void OnResize(ID3D12Device* device, uint32_t width, uint32_t height)',
  '画面サイズ変更時にリソースを再作成する。SRVヒープとパイプラインを新しいサイズに合わせて再構築する。ウィンドウリサイズ時にアプリケーション側から呼び出す。',
  '// ウィンドウリサイズ時\nssr.OnResize(device, newWidth, newHeight);',
  '• 内部の幅・高さメンバが更新される\n• SRV ヒープは再作成される\n• スワップチェーンのリサイズと合わせて呼ぶこと'
],

// ============================================================
// DepthOfField (page-DepthOfField)
// ============================================================

'DepthOfField-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  '被写界深度（Depth of Field）エフェクトを初期化する。CoC生成・ブラー（H/V分離）・合成の3段パイプラインを構築し、中間レンダーターゲット（CoC R16F full-res、ブラーHDR half-res x2）を作成する。アプリケーション起動時に一度だけ呼び出す。',
  '// DoF の初期化\nGX::DepthOfField dof;\nbool ok = dof.Initialize(device, 1920, 1080);\nif (!ok) {\n    GX::Logger::Error("DoF init failed");\n}',
  '• 内部で CoC (R16_FLOAT full-res)、ブラー中間 (HDR half-res)、ブラー結果 (HDR half-res) の3つの RT が作成される\n• 合成パス用に 3テクスチャ (sharp+blurred+CoC) をまとめた専用 SRV ヒープが作成される\n• 3つの独立したルートシグネチャ (CoC/Blur/Composite) が構築される\n• 初期化後 ShaderLibrary へ PSO リビルダーが自動登録される'
],

'DepthOfField-Execute': [
  'void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& srcHDR, RenderTarget& destHDR, DepthBuffer& depth, const Camera3D& camera)',
  'DoF エフェクトを実行する。(1) 深度バッファからビュー空間Zを復元し CoC マップを生成、(2) CoC 加重ガウシアンブラーを水平・垂直に分離実行（half-res）、(3) CoC 値でシャープ画像とブラー画像を線形補間して destHDR に出力する。',
  '// PostEffectPipeline 内での DoF 実行\nif (dof.IsEnabled()) {\n    dof.Execute(cmdList, frameIndex,\n        srcHDR, destHDR, depthBuffer, camera);\n}',
  '• 3パス構成: CoC生成 → H/Vブラー(half-res) → 合成\n• 合成パスは CopyDescriptors で 3テクスチャを1ヒープに配置\n• CoC は深度→ビュー空間Z→フォーカス距離との差分で計算\n• ブラーは half-res で実行されるためフルHDの半分のコスト'
],

'DepthOfField-SetFocalDistance': [
  'void SetFocalDistance(float d)',
  'フォーカス距離（ピントが合う中心位置）をビュー空間Zで設定する。カメラからこの距離にあるオブジェクトが最もシャープに描画される。デフォルトは 10.0f。',
  '// カメラから10m先にフォーカス\ndof.SetFocalDistance(10.0f);',
  '• ビュー空間のZ値（カメラからの距離）で指定する\n• SetFocalRange と合わせてシャープな範囲が決まる\n• ゲームプレイに合わせて動的に変更可能（オートフォーカス等）'
],

'DepthOfField-SetFocalRange': [
  'void SetFocalRange(float r)',
  'フォーカス鮮明範囲を設定する。フォーカス距離を中心にこの範囲内にあるオブジェクトが鮮明に描画される。値が小さいほどボケの効果が強くなる。デフォルトは 5.0f。',
  '// 狭い焦点範囲で強いボケ効果\ndof.SetFocalRange(2.0f);\ndof.SetFocalDistance(5.0f);',
  '• focalDistance ± focalRange/2 の範囲が鮮明領域\n• 0 に近い値にすると極めて浅い被写界深度になる\n• カットシーン等で被写体を強調する際に小さく設定する'
],

'DepthOfField-SetBokehRadius': [
  'void SetBokehRadius(float r)',
  'ボケの最大半径（ピクセル単位）を設定する。CoC のスケーリング係数として使用され、値が大きいほどフォーカス外の領域がより大きくぼける。デフォルトは 8.0f。',
  '// 大きなボケ効果\ndof.SetBokehRadius(16.0f);',
  '• ピクセル単位で指定する\n• ブラーは half-res で実行されるため、実効半径はこの値の半分程度になる\n• 大きすぎるとブラーのサンプル不足でアーティファクトが発生する場合がある'
],

'DepthOfField-SetEnabled': [
  'void SetEnabled(bool enabled)',
  'DoF エフェクトの有効/無効を切り替える。無効の場合、Execute が呼ばれてもパスがスキップされる。デフォルトは無効 (false)。',
  '// DoF を有効化\ndof.SetEnabled(true);',
  '• デフォルトは OFF (false)\n• 無効時は 3パスすべてがスキップされる'
],

'DepthOfField-IsEnabled': [
  'bool IsEnabled() const',
  'DoF エフェクトが有効かどうかを返す。true の場合、Execute で CoC 生成・ブラー・合成が実行される。',
  '// 状態確認\nbool active = dof.IsEnabled();',
  '• const メンバ関数'
],

// ============================================================
// MotionBlur (page-MotionBlur)
// ============================================================

'MotionBlur-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'カメラベース Motion Blur エフェクトを初期化する。D3D12デバイスと画面サイズを指定し、ルートシグネチャ・PSO・定数バッファ・SRVヒープを作成する。深度リプロジェクション方式でカメラ移動による速度ベクトルを算出するパイプラインが構築される。',
  '// MotionBlur の初期化\nGX::MotionBlur motionBlur;\nbool ok = motionBlur.Initialize(device, 1920, 1080);\nif (!ok) {\n    GX::Logger::Error("MotionBlur init failed");\n}',
  '• 内部で 2スロット x 2フレーム = 4エントリの専用SRVヒープが作成される\n• 初期化後 ShaderLibrary へ PSO リビルダーが自動登録される\n• デフォルトでは無効状態 (m_enabled=false)\n• 前フレームVP行列が未設定の初回フレームは自動的にスキップされる'
],

'MotionBlur-Execute': [
  'void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& srcHDR, RenderTarget& destHDR, DepthBuffer& depth, const Camera3D& camera)',
  'Motion Blur エフェクトを実行する。深度バッファからワールド座標を逆VP行列で再構成し、前フレームのVP行列で再投影して画面空間速度ベクトルを計算する。その速度方向にHDRシーンをサンプリングしてブラーする。スカイボックス (depth>=1.0) はブラー対象から除外される。',
  '// PostEffectPipeline 内での MotionBlur 実行\nif (motionBlur.IsEnabled()) {\n    motionBlur.Execute(cmdList, frameIndex,\n        srcHDR, destHDR, depthBuffer, camera);\n}',
  '• 前フレームVP行列が必要なため、初回フレームはスキップされる\n• スカイボックス (depth>=1.0) はブラー対象外\n• HDR空間で実行されるため、明るい光源のブラーも正しく処理される\n• 速度ベクトルの大きさが intensity で制御される'
],

'MotionBlur-SetIntensity': [
  'void SetIntensity(float i)',
  'ブラーの強度を設定する。速度ベクトルにこの値が乗算され、ブラーの幅を制御する。値が大きいほどブラーが強くなる。デフォルトは 1.0f。',
  '// 控えめなモーションブラー\nmotionBlur.SetIntensity(0.5f);',
  '• 0.0 でブラーなし（エフェクト自体は実行される）\n• 1.0 で標準的なブラー幅\n• 大きすぎるとアーティファクトが目立つ場合がある'
],

'MotionBlur-SetSampleCount': [
  'void SetSampleCount(int n)',
  'ブラーのサンプル数を設定する。速度方向に沿って何回テクスチャをサンプリングするかを制御する。値が大きいほど滑らかなブラーになるが、GPUコストが増加する。デフォルトは 16。',
  '// 高品質なモーションブラー\nmotionBlur.SetSampleCount(32);',
  '• GPU負荷に直結する — ターゲットフレームレートに合わせて調整\n• 少なすぎるとバンディング（縞模様）が発生する\n• 16 が品質とパフォーマンスのバランスが良い'
],

'MotionBlur-SetEnabled': [
  'void SetEnabled(bool enabled)',
  'Motion Blur エフェクトの有効/無効を切り替える。無効の場合、Execute が呼ばれてもパスがスキップされる。デフォルトは無効 (false)。ランタイムで B キーによるトグルが可能。',
  '// MotionBlur を有効化\nmotionBlur.SetEnabled(true);\n\n// トグル\nif (keyboard.IsKeyTriggered(VK_B)) {\n    motionBlur.SetEnabled(!motionBlur.IsEnabled());\n}',
  '• デフォルトは OFF (false)\n• PostEffectPipeline では B キーでトグルされる\n• カメラが静止している場合はブラーが発生しない'
],

'MotionBlur-IsEnabled': [
  'bool IsEnabled() const',
  'Motion Blur エフェクトが有効かどうかを返す。true の場合、Execute で深度リプロジェクションブラーが実行される。',
  '// 状態確認\nbool active = motionBlur.IsEnabled();',
  '• const メンバ関数'
],

'MotionBlur-UpdatePreviousVP': [
  'void UpdatePreviousVP(const Camera3D& camera)',
  '前フレームのView-Projection行列を現在のカメラから保存する。フレーム末尾（Resolve の開始時）に呼び出し、次フレームでの速度ベクトル計算に使用する。呼び忘れるとブラーが正しく動作しない。',
  '// フレーム終了時に前フレームVPを保存\nmotionBlur.UpdatePreviousVP(camera);\n// TAA も同様に更新\ntaa.UpdatePreviousVP(camera);',
  '• PostEffectPipeline::Resolve() の開始時に自動的に呼ばれる\n• 初回フレームでは m_hasPreviousVP=false のため Execute がスキップされる\n• カメラのビュー行列と射影行列の積が保存される'
],

// ============================================================
// OutlineEffect (page-OutlineEffect)
// ============================================================

'OutlineEffect-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'エッジ検出アウトラインエフェクトを初期化する。D3D12デバイスと画面サイズを指定し、Sobel深度エッジ検出と法線エッジ検出を行うパイプラインを構築する。GBuffer 不要の Forward+ 環境で動作する。',
  '// OutlineEffect の初期化\nGX::OutlineEffect outline;\nbool ok = outline.Initialize(device, 1920, 1080);\nif (!ok) {\n    GX::Logger::Error("OutlineEffect init failed");\n}',
  '• 内部で 2スロット x 2フレーム = 4エントリの専用SRVヒープが作成される\n• 初期化後 ShaderLibrary へ PSO リビルダーが自動登録される\n• デフォルトでは無効状態 (m_enabled=false)\n• 法線は深度バッファから再構成される（法線GBuffer不要）'
],

'OutlineEffect-Execute': [
  'void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& srcHDR, RenderTarget& destHDR, DepthBuffer& depth, const Camera3D& camera)',
  'アウトラインエフェクトを実行する。深度バッファからビュー空間Z値を読み取り Sobel エッジ検出を行い、同時に深度勾配から法線を再構成して法線ドット積によるエッジ検出も行う。検出されたエッジに指定色のアウトラインを合成して destHDR に出力する。',
  '// PostEffectPipeline 内での Outline 実行\nif (outline.IsEnabled()) {\n    outline.Execute(cmdList, frameIndex,\n        srcHDR, destHDR, depthBuffer, camera);\n}',
  '• ビュー空間Z変換により距離非依存のエッジ検出が実現されている\n• 深度エッジと法線エッジの2つの検出結果を組み合わせる\n• HDR空間で実行されるため、アウトライン色も HDR 値を指定可能'
],

'OutlineEffect-SetDepthThreshold': [
  'void SetDepthThreshold(float t)',
  'Sobel 深度エッジ検出の閾値を設定する。隣接ピクセルとのビュー空間Z差がこの閾値を超えるとエッジと判定される。値が小さいほど敏感にエッジを検出する。デフォルトは 0.5f。',
  '// エッジ検出を敏感にする\noutline.SetDepthThreshold(0.2f);',
  '• ビュー空間Z値の差分で判定するため、距離に依存しない\n• 小さすぎると法線の微妙な変化もエッジとして検出してしまう\n• 大きすぎると明確なシルエットしか検出されなくなる'
],

'OutlineEffect-SetNormalThreshold': [
  'void SetNormalThreshold(float t)',
  '法線エッジ検出の閾値を設定する。隣接ピクセルの法線ドット積が (1 - threshold) 以下の場合にエッジと判定される。値が小さいほど緩やかな曲面もエッジとして検出する。デフォルトは 0.3f。',
  '// 法線エッジも敏感に検出\noutline.SetNormalThreshold(0.15f);',
  '• 法線は深度勾配から再構成される（GBuffer不要）\n• 0.0 にすると法線エッジが無効化される\n• 1.0 に近い値では90度近い角度差のみ検出される'
],

'OutlineEffect-SetIntensity': [
  'void SetIntensity(float i)',
  'アウトラインの強度を設定する。エッジ検出結果にこの値が乗算され、アウトラインの濃さを制御する。0.0 でアウトラインなし、1.0 でフル強度。デフォルトは 1.0f。',
  '// 半透明のアウトライン\noutline.SetIntensity(0.5f);',
  '• 0.0 に設定するとアウトラインが消える（エフェクト自体は実行される）\n• 無効化する場合は SetEnabled(false) の方が効率的'
],

'OutlineEffect-SetLineColor': [
  'void SetLineColor(const XMFLOAT4& color)',
  'アウトラインの色を RGBA で設定する。エッジとして検出されたピクセルにこの色が合成される。アルファ値も有効。デフォルトは黒 (0, 0, 0, 1)。',
  '// 白いアウトライン\noutline.SetLineColor({ 1.0f, 1.0f, 1.0f, 1.0f });\n\n// 赤い半透明アウトライン\noutline.SetLineColor({ 1.0f, 0.0f, 0.0f, 0.7f });',
  '• RGBA の float4 で指定する\n• HDR パイプラインで実行されるため、1.0 を超える値も指定可能\n• デフォルトは黒 {0.0f, 0.0f, 0.0f, 1.0f}'
],

'OutlineEffect-SetEnabled': [
  'void SetEnabled(bool enabled)',
  'アウトラインエフェクトの有効/無効を切り替える。無効の場合、Execute が呼ばれてもパスがスキップされる。デフォルトは無効 (false)。ランタイムで O キーによるトグルが可能。',
  '// Outline を有効化\noutline.SetEnabled(true);\n\n// トグル\nif (keyboard.IsKeyTriggered(VK_O)) {\n    outline.SetEnabled(!outline.IsEnabled());\n}',
  '• デフォルトは OFF (false)\n• PostEffectPipeline では O キーでトグルされる'
],

'OutlineEffect-IsEnabled': [
  'bool IsEnabled() const',
  'アウトラインエフェクトが有効かどうかを返す。true の場合、Execute で Sobel エッジ検出が実行される。',
  '// 状態確認\nbool active = outline.IsEnabled();',
  '• const メンバ関数'
],

// ============================================================
// VolumetricLight (page-VolumetricLight)
// ============================================================

'VolumetricLight-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'ボリューメトリックライト（ゴッドレイ）エフェクトを初期化する。GPU Gems 3 のスクリーン空間ラディアルブラー方式で、ディレクショナルライトからの光の筋を合成するパイプラインを構築する。アプリケーション起動時に一度だけ呼び出す。',
  '// VolumetricLight の初期化\nGX::VolumetricLight volumetric;\nbool ok = volumetric.Initialize(device, 1920, 1080);\nif (!ok) {\n    GX::Logger::Error("VolumetricLight init failed");\n}',
  '• 内部で 2スロット x 2フレーム = 4エントリの専用SRVヒープが作成される\n• 初期化後 ShaderLibrary へ PSO リビルダーが自動登録される\n• デフォルトでは無効状態 (m_enabled=false)\n• 太陽のスクリーン座標は UpdateSunInfo() で毎フレーム計算する必要がある'
],

'VolumetricLight-Execute': [
  'void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& srcHDR, RenderTarget& destHDR, DepthBuffer& depth, const Camera3D& camera)',
  'ゴッドレイエフェクトを実行する。各ピクセルから太陽のスクリーン位置へ向かってレイマーチングし、深度バッファで空（depth>=1.0）と判定された箇所のみ光の寄与として加算する。結果を destHDR に出力する。太陽がカメラの背面にある場合は sunVisible=0 で自動的に効果が消える。',
  '// PostEffectPipeline 内での VolumetricLight 実行\nvolumetric.UpdateSunInfo(camera);\nif (volumetric.IsEnabled()) {\n    volumetric.Execute(cmdList, frameIndex,\n        srcHDR, destHDR, depthBuffer, camera);\n}',
  '• Execute の前に UpdateSunInfo() を呼んでおくこと\n• スカイボックス (depth>=1.0) のみが光源として寄与する\n• 太陽がカメラ背面の場合は sunVisible=0 で自動フェードアウト\n• 画面端での太陽位置はフェード処理で急なポップアップを防止'
],

'VolumetricLight-SetLightDirection': [
  'void SetLightDirection(const XMFLOAT3& dir)',
  'ゴッドレイの光源方向をワールド空間で設定する。ディレクショナルライト（太陽光）の方向と一致させる必要がある。太陽のスクリーン座標計算に使用される。デフォルトは (0.3, -1.0, 0.5)。',
  '// Renderer3D のディレクショナルライトと同期\nXMFLOAT3 sunDir = { 0.3f, -1.0f, 0.5f };\nvolumetric.SetLightDirection(sunDir);\nrenderer3D.SetDirectionalLight({ sunDir, {1,1,1}, 2.0f });',
  '• Renderer3D のディレクショナルライト方向と一致させること\n• 正規化は内部で行われないため、正規化済みの方向を渡すのが推奨\n• UpdateSunInfo() でこの方向からスクリーン座標が計算される'
],

'VolumetricLight-SetLightColor': [
  'void SetLightColor(const XMFLOAT3& color)',
  'ゴッドレイの色を RGB で設定する。光の筋にこの色が適用される。暖色系にすると夕焼けの光、白色だと昼間の太陽光の表現になる。デフォルトは (1.0, 0.98, 0.95)。',
  '// 夕焼け色のゴッドレイ\nvolumetric.SetLightColor({ 1.0f, 0.6f, 0.3f });',
  '• HDR 空間で適用されるため、1.0 を超える値も指定可能\n• シーンのディレクショナルライトの色と合わせるのが自然\n• RGB のみ (アルファなし)'
],

'VolumetricLight-SetDensity': [
  'void SetDensity(float v)',
  '散乱密度を設定する。大気中の粒子密度をシミュレートするパラメータで、値が大きいほど光の散乱が強くなる。デフォルトは 1.0f。',
  '// 濃い霧の中のゴッドレイ\nvolumetric.SetDensity(1.5f);',
  '• GPU Gems 3 アルゴリズムの density パラメータに対応\n• 1.0 が標準的な密度'
],

'VolumetricLight-SetDecay': [
  'void SetDecay(float v)',
  'レイマーチングの減衰率を設定する。各サンプルステップごとに光の寄与がこの係数で減衰する。1.0 に近いほど遠くまで光が届き、小さいほど近距離で減衰する。デフォルトは 0.97f。',
  '// 遠くまで光が届く設定\nvolumetric.SetDecay(0.99f);',
  '• 各レイステップでサンプル値に decay^step が乗算される\n• 0.97 前後が自然な見た目になる\n• 1.0 にすると減衰なし（不自然な結果になる可能性）'
],

'VolumetricLight-SetWeight': [
  'void SetWeight(float v)',
  '各サンプルのウェイトを設定する。レイマーチングの各ステップで取得した光の寄与にこの値が乗算される。デフォルトは 0.04f。',
  '// サンプルウェイトを調整\nvolumetric.SetWeight(0.06f);',
  '• GPU Gems 3 アルゴリズムの weight パラメータに対応\n• numSamples と合わせて最終的な明るさが決まる\n• 大きすぎると光が過剰に明るくなる'
],

'VolumetricLight-SetExposure': [
  'void SetExposure(float v)',
  'ゴッドレイの露出を設定する。レイマーチングの結果全体にこの値が乗算され、最終的な光の筋の明るさを制御する。デフォルトは 0.35f。',
  '// 露出を上げて明るいゴッドレイ\nvolumetric.SetExposure(0.5f);',
  '• 最終出力の全体的なスケーリング係数\n• AutoExposure とは独立して動作する'
],

'VolumetricLight-SetNumSamples': [
  'void SetNumSamples(int n)',
  'レイマーチングのサンプル数を設定する。ピクセルから太陽位置へのレイ上で何回サンプリングするかを制御する。値が大きいほど品質が上がるが、GPUコストが増加する。デフォルトは 96。',
  '// 高品質なゴッドレイ\nvolumetric.SetNumSamples(128);\n\n// パフォーマンス重視\nvolumetric.SetNumSamples(48);',
  '• GPU負荷に直結する\n• 少なすぎるとバンディング（縞模様）が発生する\n• 96 が品質とパフォーマンスのバランスが良い'
],

'VolumetricLight-SetIntensity': [
  'void SetIntensity(float v)',
  'ゴッドレイの全体強度を設定する。すべてのパラメータの最終出力にこの値が乗算される。他のパラメータを変更せずに全体の効果の強さだけを調整したい場合に便利。デフォルトは 1.0f。',
  '// 控えめなゴッドレイ\nvolumetric.SetIntensity(0.5f);',
  '• 0.0 でゴッドレイが完全に消える（エフェクト自体は実行される）\n• 無効化する場合は SetEnabled(false) の方が効率的\n• exposure や weight とは独立した最終乗算係数'
],

'VolumetricLight-UpdateSunInfo': [
  'void UpdateSunInfo(const Camera3D& camera)',
  '太陽のスクリーン座標と可視性を毎フレーム計算する。カメラの位置からライト方向の逆方向に仮想太陽を配置し、VP行列でスクリーン空間に変換する。enabled に関係なく呼ぶ必要がある（有効切替時に即座に反映されるように）。',
  '// 毎フレーム、Execute の前に呼ぶ\nvolumetric.UpdateSunInfo(camera);\nif (volumetric.IsEnabled()) {\n    volumetric.Execute(cmdList, frameIndex,\n        srcHDR, destHDR, depthBuffer, camera);\n}',
  '• camPos - lightDir * 1000 の位置を VP 行列で NDC → UV 変換\n• 太陽がカメラ背面の場合は sunVisible=0\n• 画面端ではフェード処理により急なポップアップを防止する\n• SetEnabled() の状態に関係なく毎フレーム呼ぶこと'
],

'VolumetricLight-SetEnabled': [
  'void SetEnabled(bool enabled)',
  'ゴッドレイエフェクトの有効/無効を切り替える。無効の場合、Execute が呼ばれてもパスがスキップされる。デフォルトは無効 (false)。ランタイムで V キーによるトグルが可能。',
  '// VolumetricLight を有効化\nvolumetric.SetEnabled(true);\n\n// トグル\nif (keyboard.IsKeyTriggered(VK_V)) {\n    volumetric.SetEnabled(!volumetric.IsEnabled());\n}',
  '• デフォルトは OFF (false)\n• PostEffectPipeline では V キーでトグルされる\n• UpdateSunInfo() は enabled に関係なく呼ぶべき'
],

'VolumetricLight-IsEnabled': [
  'bool IsEnabled() const',
  'ゴッドレイエフェクトが有効かどうかを返す。true の場合、Execute でラディアルブラーが実行される。',
  '// 状態確認\nbool active = volumetric.IsEnabled();',
  '• const メンバ関数'
],

// ============================================================
// TAA (page-TAA)
// ============================================================

'TAA-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'Temporal Anti-Aliasing エフェクトを初期化する。D3D12デバイスと画面サイズを指定し、TAA パイプライン（ルートシグネチャ・PSO）、履歴バッファ（R16G16B16A16_FLOAT）、定数バッファ、3テクスチャ用SRVヒープを作成する。Halton(2,3) 8サンプルジッターサイクルでサブピクセル情報を蓄積する。',
  '// TAA の初期化\nGX::TAA taa;\nbool ok = taa.Initialize(device, 1920, 1080);\nif (!ok) {\n    GX::Logger::Error("TAA init failed");\n}',
  '• 内部で 3スロット x 2フレーム = 6エントリの専用SRVヒープが作成される（scene+history+depth）\n• 履歴RT は R16G16B16A16_FLOAT で HDR 精度を維持\n• 初期化後 ShaderLibrary へ PSO リビルダーが自動登録される\n• デフォルトでは無効状態 (m_enabled=false)'
],

'TAA-Execute': [
  'void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& srcHDR, RenderTarget& destHDR, DepthBuffer& depth, const Camera3D& camera)',
  'TAA エフェクトを実行する。現フレームのHDRシーンと深度リプロジェクションで取得した前フレーム履歴をブレンドし、近傍分散クランプでゴースティングを抑制する。実行後、destHDR の内容が自動的に履歴RTにコピーされる。',
  '// PostEffectPipeline 内での TAA 実行\nif (taa.IsEnabled()) {\n    taa.Execute(cmdList, frameIndex,\n        srcHDR, destHDR, depthBuffer, camera);\n}',
  '• 前フレーム履歴が存在しない初回フレームはブレンドなし（現フレームをそのまま使用）\n• 分散クランプ (mu ± gamma * sigma) でゴースティングを抑制\n• 実行後に CopyResource で destHDR → historyRT に自動コピー\n• Camera3D の GetJitteredProjectionMatrix() が使用される'
],

'TAA-SetBlendFactor': [
  'void SetBlendFactor(float f)',
  '履歴バッファとのブレンド係数を設定する。値が大きいほど過去のフレーム情報を多く保持し、時間的な安定性が向上するが、動きの速いシーンでゴースティングが発生しやすくなる。デフォルトは 0.9f。',
  '// 安定性重視\ntaa.SetBlendFactor(0.95f);\n\n// レスポンス重視\ntaa.SetBlendFactor(0.8f);',
  '• 0.9 = 現フレーム10% + 履歴90% のブレンド\n• 近傍分散クランプにより実際のブレンド比は動的に調整される\n• 0.0 に設定すると現フレームのみ使用（TAA効果なし）\n• 1.0 に近いほど安定するがゴースティングのリスクが増加'
],

'TAA-GetCurrentJitter': [
  'XMFLOAT2 GetCurrentJitter() const',
  '現フレームのジッターオフセットを NDC 空間で返す。Halton(2,3) 数列から生成され、8サンプルサイクルで繰り返される。Camera3D にサブピクセルシフトとして適用されるオフセット値。',
  '// 現在のジッター値を取得\nXMFLOAT2 jitter = taa.GetCurrentJitter();\nGX::Logger::Info("Jitter: %.4f, %.4f", jitter.x, jitter.y);',
  '• Halton(2,3) 数列に基づく 8サンプルサイクル\n• NDC空間（-1 to 1）のオフセット値\n• Camera3D.SetJitter() を通じてプロジェクション行列に適用される\n• BeginScene 内で自動的にカメラに設定される'
],

'TAA-AdvanceFrame': [
  'void AdvanceFrame()',
  'フレームカウントを1進める。Halton 数列の次のジッターサンプルに移行する。毎フレーム呼び出す必要がある。',
  '// フレーム終了時\ntaa.AdvanceFrame();',
  '• 内部の m_frameCount をインクリメントする\n• 8サンプルサイクルで Halton ジッターが繰り返される\n• PostEffectPipeline::Resolve() 内で自動的に呼ばれる'
],

'TAA-UpdatePreviousVP': [
  'void UpdatePreviousVP(const Camera3D& camera)',
  '前フレームのView-Projection行列を現在のカメラから保存する。フレーム末尾に呼び出し、次フレームでの深度リプロジェクション（速度ベクトル計算）に使用する。非ジッターのVP行列が保存される。',
  '// フレーム終了時に前フレームVPを保存\ntaa.UpdatePreviousVP(camera);',
  '• PostEffectPipeline::Resolve() の開始時に自動的に呼ばれる\n• 非ジッターの VP 行列が保存される（ジッターは分離して管理）\n• 初回フレームでは m_hasPreviousVP=false のためブレンドがスキップされる'
],

'TAA-SetEnabled': [
  'void SetEnabled(bool enabled)',
  'TAA エフェクトの有効/無効を切り替える。有効時は Camera3D にジッターオフセットが適用され、サブピクセル情報が蓄積される。無効の場合、Execute がスキップされジッターも適用されない。デフォルトは無効 (false)。ランタイムで T キーによるトグルが可能。',
  '// TAA を有効化\ntaa.SetEnabled(true);\n\n// トグル\nif (keyboard.IsKeyTriggered(VK_T)) {\n    taa.SetEnabled(!taa.IsEnabled());\n}',
  '• デフォルトは OFF (false)\n• PostEffectPipeline では T キーでトグルされる\n• 有効時は BeginScene で Camera3D にジッターが自動適用される\n• 無効化すると履歴バッファはクリアされないが使用もされない'
],

'TAA-IsEnabled': [
  'bool IsEnabled() const',
  'TAA エフェクトが有効かどうかを返す。true の場合、Execute でテンポラルブレンドが実行される。',
  '// 状態確認\nbool active = taa.IsEnabled();',
  '• const メンバ関数'
],

'TAA-Halton': [
  'static float Halton(int index, int base)',
  'Halton 低差分数列の値を計算する。TAA ジッターオフセットの生成に使用される。base=2 と base=3 でそれぞれ X/Y 方向のオフセットを生成する。0-1 の範囲の準乱数を返す。',
  '// Halton 数列でサンプルポイントを生成\nfor (int i = 0; i < 8; i++) {\n    float x = GX::TAA::Halton(i + 1, 2);  // base 2\n    float y = GX::TAA::Halton(i + 1, 3);  // base 3\n    // (x, y) は [0, 1) の準乱数\n}',
  '• static メンバ関数（インスタンス不要で呼び出し可能）\n• base=2, base=3 の組み合わせで 2D の低差分列を生成\n• index は 1-based で指定する（0 は常に 0.0 を返す）\n• 8サンプルサイクルで TAA ジッターに使用される'
],

// ============================================================
// AutoExposure (page-AutoExposure)
// ============================================================

'AutoExposure-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  '自動露出（Eye Adaptation）エフェクトを初期化する。ピクセルシェーダーベースの対数輝度ダウンサンプルチェーン（256->64->16->4->1 R16_FLOAT）と、2フレームリングバッファのCPUリードバックリソースを作成する。コンピュートシェーダーインフラ不要の PS ベース方式。',
  '// AutoExposure の初期化\nGX::AutoExposure autoExposure;\nbool ok = autoExposure.Initialize(device, 1920, 1080);\nif (!ok) {\n    GX::Logger::Error("AutoExposure init failed");\n}',
  '• ダウンサンプルチェーン: 5レベル (256, 64, 16, 4, 1) の R16_FLOAT RT\n• 2フレームリングバッファでGPUストールを回避\n• 初期化後 ShaderLibrary へ PSO リビルダーが自動登録される\n• デフォルトでは無効状態 (m_enabled=false)'
],

'AutoExposure-ComputeExposure': [
  'float ComputeExposure(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& hdrScene, float deltaTime)',
  'HDRシーンの平均輝度を計算し、時間的に滑らかに適応した露出値を返す。対数輝度ダウンサンプルチェーンで平均輝度を計算し、2フレーム前のリードバック結果を読み取り、指数関数的スムージングで露出を更新する。返り値はトーンマッピングパスに渡す露出倍率。',
  '// 毎フレームの露出計算\nfloat exposure = 1.0f;\nif (autoExposure.IsEnabled()) {\n    exposure = autoExposure.ComputeExposure(\n        cmdList, frameIndex, hdrScene, deltaTime);\n}\n// トーンマッピングに露出を適用\ntonemapping.SetExposure(exposure);',
  '• 2フレーム前の結果を使用するため初期化直後は lastAvgLuminance=0.5 がフォールバック値\n• 指数関数的スムージング: exposure += (target - exposure) * (1 - exp(-speed * dt))\n• 返り値は keyValue / exp(avgLogLuminance) で計算された露出倍率\n• GPUストールは2フレームリングバッファで回避されている'
],

'AutoExposure-SetAdaptationSpeed': [
  'void SetAdaptationSpeed(float s)',
  '露出の適応速度を設定する。値が大きいほど素早く露出が変化し、小さいほど緩やかに変化する。暗い部屋から明るい屋外に出た際の目の順応をシミュレートする。デフォルトは 1.5f。',
  '// ゆっくり適応（映画的演出）\nautoExposure.SetAdaptationSpeed(0.5f);\n\n// 素早く適応（FPS向け）\nautoExposure.SetAdaptationSpeed(3.0f);',
  '• 指数関数的スムージングの速度パラメータ\n• 0 に近い値では露出がほとんど変化しなくなる\n• deltaTime と組み合わせてフレームレート非依存の適応が行われる'
],

'AutoExposure-GetAdaptationSpeed': [
  'float GetAdaptationSpeed() const',
  '現在の適応速度を返す。SetAdaptationSpeed で設定された値を取得する。',
  '// 現在の適応速度を確認\nfloat speed = autoExposure.GetAdaptationSpeed();',
  '• const メンバ関数\n• デフォルト値は 1.5f'
],

'AutoExposure-SetMinExposure': [
  'void SetMinExposure(float v)',
  '露出の最小値を設定する。自動露出の結果がこの値を下回らないようクランプされる。暗いシーンで過剰に明るくなるのを防ぐ。デフォルトは 0.1f。',
  '// 最小露出を制限\nautoExposure.SetMinExposure(0.2f);',
  '• 明るいシーンでの露出下限\n• maxExposure とともに露出の有効範囲を定義する'
],

'AutoExposure-GetMinExposure': [
  'float GetMinExposure() const',
  '露出の最小値を返す。SetMinExposure で設定された値を取得する。',
  '// 最小露出を確認\nfloat minExp = autoExposure.GetMinExposure();',
  '• const メンバ関数\n• デフォルト値は 0.1f'
],

'AutoExposure-SetMaxExposure': [
  'void SetMaxExposure(float v)',
  '露出の最大値を設定する。自動露出の結果がこの値を超えないようクランプされる。暗いシーンで過剰に明るくなるのを防ぐ。デフォルトは 10.0f。',
  '// 最大露出を制限\nautoExposure.SetMaxExposure(5.0f);',
  '• 暗いシーンでの露出上限\n• minExposure とともに露出の有効範囲を定義する\n• ゲームのアートディレクションに合わせて調整する'
],

'AutoExposure-GetMaxExposure': [
  'float GetMaxExposure() const',
  '露出の最大値を返す。SetMaxExposure で設定された値を取得する。',
  '// 最大露出を確認\nfloat maxExp = autoExposure.GetMaxExposure();',
  '• const メンバ関数\n• デフォルト値は 10.0f'
],

'AutoExposure-SetKeyValue': [
  'void SetKeyValue(float v)',
  '目標中間灰の値を設定する。シーンの平均輝度に対してどの程度の明るさを目標とするかを制御する。値が大きいほどシーン全体が明るくなる。写真露出における「18%グレー」に相当。デフォルトは 0.18f。',
  '// やや明るめの露出目標\nautoExposure.SetKeyValue(0.25f);',
  '• 露出 = keyValue / exp(avgLogLuminance) で計算される\n• 0.18 は写真のゾーンシステムにおける中間灰に対応\n• シーンの雰囲気に合わせて調整する（暗いゲーム→低め、明るいゲーム→高め）'
],

'AutoExposure-GetKeyValue': [
  'float GetKeyValue() const',
  '目標中間灰の値を返す。SetKeyValue で設定された値を取得する。',
  '// キー値を確認\nfloat key = autoExposure.GetKeyValue();',
  '• const メンバ関数\n• デフォルト値は 0.18f'
],

'AutoExposure-GetCurrentExposure': [
  'float GetCurrentExposure() const',
  '現在の露出値を返す。ComputeExposure で最後に計算された適応後の露出値。時間的スムージングが適用された値であり、目標露出とは異なる場合がある。初期値は 1.0f。',
  '// HUD に露出値を表示\nfloat exp = autoExposure.GetCurrentExposure();\nchar buf[64];\nsprintf_s(buf, "Exposure: %.2f", exp);\ntextRenderer.Draw(buf, 10, 40);',
  '• const メンバ関数\n• ComputeExposure が呼ばれるたびに更新される\n• 指数関数的スムージングにより目標値に徐々に近づく\n• 初期値は 1.0f（中間露出）'
],

'AutoExposure-SetEnabled': [
  'void SetEnabled(bool enabled)',
  '自動露出エフェクトの有効/無効を切り替える。無効の場合、ComputeExposure が呼ばれても固定露出 (1.0) が返される。デフォルトは無効 (false)。ランタイムで X キーによるトグルが可能。',
  '// AutoExposure を有効化\nautoExposure.SetEnabled(true);\n\n// トグル\nif (keyboard.IsKeyTriggered(VK_X)) {\n    autoExposure.SetEnabled(!autoExposure.IsEnabled());\n}',
  '• デフォルトは OFF (false)\n• PostEffectPipeline では X キーでトグルされる\n• 無効時はダウンサンプルチェーンが実行されない'
],

'AutoExposure-IsEnabled': [
  'bool IsEnabled() const',
  '自動露出エフェクトが有効かどうかを返す。true の場合、ComputeExposure で輝度ダウンサンプルチェーンが実行される。',
  '// 状態確認\nbool active = autoExposure.IsEnabled();',
  '• const メンバ関数'
],

'AutoExposure-OnResize': [
  'void OnResize(ID3D12Device* device, uint32_t width, uint32_t height)',
  '画面サイズ変更時にリソースを再作成する。ダウンサンプルチェーンの入力解像度は固定(256)だが、初段の輝度計算で参照するシーンサイズが変わるためパイプラインを更新する。ウィンドウリサイズ時にアプリケーション側から呼び出す。',
  '// ウィンドウリサイズ時\nautoExposure.OnResize(device, newWidth, newHeight);',
  '• ダウンサンプルチェーン自体のサイズは固定 (256, 64, 16, 4, 1)\n• スワップチェーンのリサイズと合わせて呼ぶこと\n• リードバックバッファは再作成されない'
],

}
