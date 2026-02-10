// Agent 4b: PostEffect classes (PostEffectPipeline, PostEffectSettings, Bloom, SSAO)
// Auto-generated API reference data
const AGENT4B_DATA = {

// ============================================================
// TonemapMode (Graphics/PostEffect/PostEffectPipeline.h)
// ============================================================
'PostEffectPipeline-TonemapMode': [
  'enum class TonemapMode : uint32_t { Reinhard = 0, ACES = 1, Uncharted2 = 2 }',
  'HDRからLDRへの変換に使用するトーンマッピングアルゴリズムを指定する列挙型です。Reinhardは最もシンプルで自然な明暗圧縮を行います。ACESは映画業界標準のフィルミックカーブで、コントラストと彩度のバランスに優れています。Uncharted2はゲーム「Uncharted 2」で使用されたフィルミックカーブで、暗部の詳細を保持しつつハイライトを圧縮します。デフォルトはACESです。',
  '// トーンマップモードを切り替える\npipeline.SetTonemapMode(GX::TonemapMode::ACES);\n\n// 現在のモードを取得\nGX::TonemapMode mode = pipeline.GetTonemapMode();\nif (mode == GX::TonemapMode::Reinhard) {\n    // Reinhard モード\n}',
  '• Reinhard: シンプルな輝度圧縮、自然な見た目\n• ACES: 映画業界標準、コントラストと彩度のバランスが良い（デフォルト）\n• Uncharted2: ゲーム向けフィルミックカーブ、暗部ディテール保持'
],

// ============================================================
// PostEffectPipeline (Graphics/PostEffect/PostEffectPipeline.h)
// ============================================================
'PostEffectPipeline-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'ポストエフェクトパイプライン全体を初期化します。HDR/LDR用のレンダーターゲット(各2枚のping-pong構成)、シェーダーのコンパイル、各エフェクト(SSAO, Bloom, DoF, MotionBlur, SSR, Outline, VolumetricLight, TAA, AutoExposure)の初期化、およびPSO(パイプラインステートオブジェクト)の作成を行います。ShaderHotReload用のPSO Rebuilderも登録されます。失敗時はfalseを返します。',
  '// ポストエフェクトパイプラインの初期化\nGX::PostEffectPipeline pipeline;\nif (!pipeline.Initialize(device, 1280, 720)) {\n    GX_LOG_ERROR("PostEffectPipeline initialization failed");\n    return false;\n}',
  '• GraphicsDevice初期化後に呼び出すこと\n• HDR RT (R16G16B16A16_FLOAT) x2 + LDR RT (R8G8B8A8_UNORM) x2 を内部で作成\n• 全サブエフェクト(SSAO, Bloom等)も連鎖的に初期化される\n• シェーダーホットリロード用のPSO Rebuilderが自動登録される'
],

'PostEffectPipeline-BeginScene': [
  'void BeginScene(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, Camera3D& camera)',
  'ポストエフェクトパイプラインのシーン描画を開始します。HDRレンダーターゲットをクリアし、描画先として設定します。TAA有効時はカメラにジッターオフセットを適用し、無効時はジッターをクリアします。Camera3Dは非constで渡す必要があります（TAA用のジッター適用のため）。',
  '// 毎フレームの描画開始\npipeline.BeginScene(cmdList, frameIndex, depthBuffer.GetDSVHandle(), camera);\n\n// 3Dシーンを描画\nrenderer3D.BeginScene(camera);\nrenderer3D.DrawMesh(mesh, transform);\nrenderer3D.EndScene();\n\npipeline.EndScene();',
  '• Camera3Dは非const参照（TAA有効時にジッターを適用するため）\n• 内部でHDR RTのクリアとビューポート/シザー矩形の設定を行う\n• BeginScene〜EndScene間に3Dシーンの描画コマンドを発行する\n• 深度バッファもクリアされる（depth=1.0, stencil=0）'
],

'PostEffectPipeline-EndScene': [
  'void EndScene()',
  'ポストエフェクトパイプラインのシーン描画を終了します。HDRレンダーターゲットをシェーダーリソース状態(SRV)に遷移させ、後続のResolve処理で読み取り可能にします。BeginSceneと対で呼び出す必要があります。',
  '// シーン描画終了\npipeline.EndScene();\n\n// ポストエフェクトを適用してバックバッファへ出力\npipeline.Resolve(backBufferRTV, depthBuffer, camera, deltaTime);',
  '• BeginSceneと必ず対で呼び出すこと\n• HDR RTの状態をRENDER_TARGET→PIXEL_SHADER_RESOURCEに遷移\n• EndScene後にResolveを呼び出してエフェクトチェーンを実行する'
],

'PostEffectPipeline-Resolve': [
  'void Resolve(D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV, DepthBuffer& depthBuffer, const Camera3D& camera, float deltaTime = 0.0f)',
  'ポストエフェクトチェーン全体を実行し、最終結果を指定されたRTVに出力します。HDR空間で[SSAO]→[SSR]→[VolumetricLight]→[Bloom]→[DoF]→[MotionBlur]→[Outline]→[TAA]→[ColorGrading]→[AutoExposure]→[Tonemapping]を適用し、LDR変換後に[FXAA]→[Vignette]を適用します。各エフェクトは個別のEnabled設定に従ってスキップされます。',
  '// ポストエフェクトを適用\nauto backBufferRTV = swapChain.GetCurrentRTV();\npipeline.Resolve(backBufferRTV, depthBuffer, camera, timer.GetDeltaTime());\n\n// レイヤーシステム使用時はレイヤーRTに出力\npipeline.Resolve(sceneLayer.GetRTVHandle(), depthBuffer, camera, dt);',
  '• deltaTimeはAutoExposureの時間適応スムージングに使用（デフォルト0.0f）\n• 出力先はバックバッファRTVまたはレイヤーRTの任意のRTVハンドル\n• ping-pongバッファを使用してHDRエフェクトチェーンを順次適用\n• 最後のLDRエフェクトは直接出力先RTVに描画される（余分なコピーなし）\n• MotionBlur/TAAの前フレームVP行列はResolve内で自動更新される'
],

'PostEffectPipeline-SetTonemapMode': [
  'void SetTonemapMode(TonemapMode mode)',
  'トーンマッピングアルゴリズムを設定します。HDRシーンをLDR（0〜1の輝度範囲）に変換する際のカーブを選択します。デフォルトはACESです。',
  '// ACESトーンマッピングを使用\npipeline.SetTonemapMode(GX::TonemapMode::ACES);\n\n// Reinhardに切り替え\npipeline.SetTonemapMode(GX::TonemapMode::Reinhard);',
  '• Reinhard / ACES / Uncharted2 の3種類から選択\n• ランタイムで切り替え可能（PSO再作成は不要）\n• SaveSettings/LoadSettingsで永続化される'
],

'PostEffectPipeline-GetTonemapMode': [
  'TonemapMode GetTonemapMode() const',
  '現在設定されているトーンマッピングモードを返します。デフォルト値はTonemapMode::ACESです。',
  '// 現在のトーンマップモードを確認\nGX::TonemapMode mode = pipeline.GetTonemapMode();\nif (mode == GX::TonemapMode::ACES) {\n    // ACES mode\n}',
  '• デフォルト値はTonemapMode::ACES'
],

'PostEffectPipeline-SetExposure': [
  'void SetExposure(float exposure)',
  'トーンマッピング時の露出値を設定します。値が大きいほどシーン全体が明るくなります。AutoExposure有効時はこの値は無視され、自動計算された露出が使用されます。デフォルトは1.0です。',
  '// 露出を調整\npipeline.SetExposure(1.5f);  // 少し明るめに\npipeline.SetExposure(0.5f);  // 暗めに',
  '• デフォルト値は1.0\n• AutoExposure有効時は自動露出値が優先される\n• 0以下の値を設定すると画面が真っ暗になる\n• SaveSettings/LoadSettingsで永続化される'
],

'PostEffectPipeline-GetExposure': [
  'float GetExposure() const',
  '現在設定されている手動露出値を返します。AutoExposureが有効でも、ここで返されるのは手動設定値であり、実際に適用されている自動露出値ではありません。',
  '// 現在の露出値を取得\nfloat exp = pipeline.GetExposure();',
  '• デフォルト値は1.0\n• AutoExposure有効時でもSetExposureで設定した手動値が返る'
],

'PostEffectPipeline-GetSSAO': [
  'SSAO& GetSSAO()\nconst SSAO& GetSSAO() const',
  '内部のSSAOエフェクトオブジェクトへの参照を返します。SSAOのパラメータ（半径、バイアス、強度）の調整や有効/無効の切り替えに使用します。constオーバーロードも提供されています。',
  '// SSAOのパラメータを調整\npipeline.GetSSAO().SetEnabled(true);\npipeline.GetSSAO().SetRadius(0.8f);\npipeline.GetSSAO().SetBias(0.03f);\npipeline.GetSSAO().SetPower(3.0f);',
  '• SSAOはデフォルトで有効(enabled=true)\n• 返される参照はPostEffectPipelineの寿命と同じ'
],

'PostEffectPipeline-GetBloom': [
  'Bloom& GetBloom()\nconst Bloom& GetBloom() const',
  '内部のBloomエフェクトオブジェクトへの参照を返します。Bloomの閾値や強度の調整、有効/無効の切り替えに使用します。constオーバーロードも提供されています。',
  '// Bloomのパラメータを調整\npipeline.GetBloom().SetEnabled(true);\npipeline.GetBloom().SetThreshold(0.8f);\npipeline.GetBloom().SetIntensity(0.6f);',
  '• Bloomはデフォルトで有効(enabled=true)\n• 返される参照はPostEffectPipelineの寿命と同じ'
],

'PostEffectPipeline-GetDoF': [
  'DepthOfField& GetDoF()\nconst DepthOfField& GetDoF() const',
  '内部の被写界深度(DoF)エフェクトオブジェクトへの参照を返します。フォーカス距離やボケ量の調整、有効/無効の切り替えに使用します。',
  '// DoFを有効化してフォーカス距離を設定\npipeline.GetDoF().SetEnabled(true);\npipeline.GetDoF().SetFocusDistance(10.0f);\npipeline.GetDoF().SetFocusRange(5.0f);',
  '• DoFはデフォルトで無効(enabled=false)\n• HDR空間でBloomの後に適用される'
],

'PostEffectPipeline-GetMotionBlur': [
  'MotionBlur& GetMotionBlur()\nconst MotionBlur& GetMotionBlur() const',
  '内部のモーションブラーエフェクトオブジェクトへの参照を返します。カメラの動きに基づく深度リプロジェクション方式のモーションブラーの調整や有効/無効の切り替えに使用します。',
  '// MotionBlurを有効化\npipeline.GetMotionBlur().SetEnabled(true);',
  '• デフォルトで無効(enabled=false)\n• HDR空間でDoFの後に適用される\n• 前フレームのVP行列はResolve内で自動更新される'
],

'PostEffectPipeline-GetSSR': [
  'SSR& GetSSR()\nconst SSR& GetSSR() const',
  '内部のスクリーンスペース反射(SSR)エフェクトオブジェクトへの参照を返します。画面空間のレイマーチングによるリアルタイム反射の調整や有効/無効の切り替えに使用します。',
  '// SSRを有効化\npipeline.GetSSR().SetEnabled(true);',
  '• デフォルトで無効(enabled=false)\n• HDR空間でSSAOの後に適用される\n• DDAレイマーチング + バイナリリファインメント方式'
],

'PostEffectPipeline-GetOutline': [
  'OutlineEffect& GetOutline()\nconst OutlineEffect& GetOutline() const',
  '内部のアウトラインエフェクトオブジェクトへの参照を返します。Sobelフィルタによる深度/法線エッジ検出ベースのアウトライン描画の調整や有効/無効の切り替えに使用します。',
  '// アウトラインを有効化\npipeline.GetOutline().SetEnabled(true);',
  '• デフォルトで無効(enabled=false)\n• HDR空間でMotionBlurの後に適用される\n• ビュー空間Zベースで距離に依存しないエッジ検出'
],

'PostEffectPipeline-GetVolumetricLight': [
  'VolumetricLight& GetVolumetricLight()\nconst VolumetricLight& GetVolumetricLight() const',
  '内部のボリュメトリックライト(ゴッドレイ)エフェクトオブジェクトへの参照を返します。GPU Gems 3のラジアルブラー方式による光芒エフェクトの調整や有効/無効の切り替えに使用します。',
  '// ゴッドレイを有効化\npipeline.GetVolumetricLight().SetEnabled(true);',
  '• デフォルトで無効(enabled=false)\n• HDR空間でSSRの後、Bloomの前に適用される\n• 太陽のスクリーン座標はカメラ情報から自動計算される'
],

'PostEffectPipeline-GetTAA': [
  'TAA& GetTAA()\nconst TAA& GetTAA() const',
  '内部のTemporal Anti-Aliasing(TAA)エフェクトオブジェクトへの参照を返します。Haltonシーケンスによるサブピクセルジッターと前フレーム合成によるアンチエイリアシングの調整や有効/無効の切り替えに使用します。',
  '// TAAを有効化\npipeline.GetTAA().SetEnabled(true);',
  '• デフォルトで無効(enabled=false)\n• HDR空間でOutlineの後、ColorGradingの前に適用される\n• Camera3DへのジッターはBeginScene内で自動適用される'
],

'PostEffectPipeline-GetAutoExposure': [
  'AutoExposure& GetAutoExposure()\nconst AutoExposure& GetAutoExposure() const',
  '内部の自動露出エフェクトオブジェクトへの参照を返します。シーン全体の平均輝度に基づく露出の自動調整の有効/無効や適応速度の調整に使用します。有効時はSetExposureの手動値は無視されます。',
  '// 自動露出を有効化\npipeline.GetAutoExposure().SetEnabled(true);',
  '• デフォルトで無効(enabled=false)\n• 有効時はSetExposureの手動値より自動露出値が優先される\n• PS-basedログ輝度ダウンサンプル + リードバックリングバッファ方式\n• deltaTime引数(Resolve)で時間適応スムージングが行われる'
],

'PostEffectPipeline-SetFXAAEnabled': [
  'void SetFXAAEnabled(bool enabled)',
  'FXAAアンチエイリアシングの有効/無効を切り替えます。FXAA(Fast Approximate Anti-Aliasing)はLDR空間でトーンマッピング後に適用される軽量なポストプロセスAAです。デフォルトは有効です。',
  '// FXAAを無効化（TAAを使う場合など）\npipeline.SetFXAAEnabled(false);\n\n// FXAAを有効化\npipeline.SetFXAAEnabled(true);',
  '• デフォルトで有効(enabled=true)\n• LDR空間でトーンマッピング後に適用される\n• TAAと併用可能だが、TAAのみで十分な場合はFXAAを無効にしてパフォーマンスを節約可能'
],

'PostEffectPipeline-IsFXAAEnabled': [
  'bool IsFXAAEnabled() const',
  'FXAAが有効かどうかを返します。デフォルトはtrueです。',
  '// FXAA有効状態を確認\nif (pipeline.IsFXAAEnabled()) {\n    // FXAAが有効\n}',
  '• デフォルト値はtrue'
],

'PostEffectPipeline-SetVignetteEnabled': [
  'void SetVignetteEnabled(bool enabled)',
  'ビネットエフェクト（画面周辺の減光効果）の有効/無効を切り替えます。色収差(Chromatic Aberration)もこのエフェクトに含まれます。LDRチェーンの最後に適用されます。デフォルトは無効です。',
  '// ビネットを有効化\npipeline.SetVignetteEnabled(true);\npipeline.SetVignetteIntensity(0.6f);',
  '• デフォルトで無効(enabled=false)\n• LDRチェーンの最終段で適用される（FXAA後）\n• 色収差(Chromatic Aberration)エフェクトと連動'
],

'PostEffectPipeline-SetVignetteIntensity': [
  'void SetVignetteIntensity(float v)',
  'ビネットエフェクトの強度を設定します。値が大きいほど画面周辺が暗くなります。デフォルトは0.5です。',
  '// ビネット強度を設定\npipeline.SetVignetteIntensity(0.8f);  // 強めのビネット\npipeline.SetVignetteIntensity(0.2f);  // 弱めのビネット',
  '• デフォルト値は0.5\n• SetVignetteEnabled(true)でビネットが有効でないと効果なし'
],

'PostEffectPipeline-GetVignetteIntensity': [
  'float GetVignetteIntensity() const',
  '現在設定されているビネット強度を返します。デフォルト値は0.5です。',
  '// ビネット強度を取得\nfloat intensity = pipeline.GetVignetteIntensity();',
  '• デフォルト値は0.5'
],

'PostEffectPipeline-SetColorGradingEnabled': [
  'void SetColorGradingEnabled(bool enabled)',
  'カラーグレーディングの有効/無効を切り替えます。コントラスト、彩度、色温度の調整をHDR空間で行います。トーンマッピング前のHDR段階で適用されるため、より自然な色調補正が可能です。デフォルトは無効です。',
  '// カラーグレーディングを有効化\npipeline.SetColorGradingEnabled(true);\npipeline.SetContrast(1.2f);\npipeline.SetSaturation(1.1f);\npipeline.SetTemperature(0.1f);  // 暖色方向',
  '• デフォルトで無効(enabled=false)\n• HDR空間で適用される（トーンマッピング前）\n• Contrast/Saturation/Temperatureの3パラメータを調整可能'
],

'PostEffectPipeline-SetContrast': [
  'void SetContrast(float v)',
  'カラーグレーディングのコントラスト値を設定します。1.0がニュートラル、1.0より大きいとコントラスト増加、小さいと減少します。デフォルトは1.0です。',
  '// コントラストを上げる\npipeline.SetContrast(1.3f);',
  '• デフォルト値は1.0（変化なし）\n• ColorGradingが有効でないと効果なし'
],

'PostEffectPipeline-GetContrast': [
  'float GetContrast() const',
  '現在設定されているコントラスト値を返します。デフォルト値は1.0です。',
  '// コントラスト値を取得\nfloat contrast = pipeline.GetContrast();',
  '• デフォルト値は1.0'
],

'PostEffectPipeline-SetSaturation': [
  'void SetSaturation(float v)',
  'カラーグレーディングの彩度を設定します。1.0がニュートラル、0.0でモノクロ、1.0以上で彩度増加です。デフォルトは1.0です。',
  '// 彩度を上げる\npipeline.SetSaturation(1.2f);\n\n// モノクロにする\npipeline.SetSaturation(0.0f);',
  '• デフォルト値は1.0（変化なし）\n• 0.0で完全なグレースケール\n• ColorGradingが有効でないと効果なし'
],

'PostEffectPipeline-GetSaturation': [
  'float GetSaturation() const',
  '現在設定されている彩度値を返します。デフォルト値は1.0です。',
  '// 彩度値を取得\nfloat saturation = pipeline.GetSaturation();',
  '• デフォルト値は1.0'
],

'PostEffectPipeline-SetTemperature': [
  'void SetTemperature(float v)',
  'カラーグレーディングの色温度を設定します。0.0がニュートラル、正の値で暖色（黄〜赤方向）、負の値で寒色（青方向）にシフトします。デフォルトは0.0です。',
  '// 暖色方向にシフト（夕暮れ風）\npipeline.SetTemperature(0.3f);\n\n// 寒色方向にシフト（冷たい雰囲気）\npipeline.SetTemperature(-0.2f);',
  '• デフォルト値は0.0（変化なし）\n• 正の値で暖色、負の値で寒色にシフト\n• ColorGradingが有効でないと効果なし'
],

'PostEffectPipeline-GetTemperature': [
  'float GetTemperature() const',
  '現在設定されている色温度値を返します。デフォルト値は0.0です。',
  '// 色温度値を取得\nfloat temp = pipeline.GetTemperature();',
  '• デフォルト値は0.0'
],

'PostEffectPipeline-LoadSettings': [
  'bool LoadSettings(const std::string& filePath)',
  '指定されたJSONファイルからポストエフェクトの全設定を読み込みます。内部でPostEffectSettings::Loadに委譲し、トーンマッピングモード、露出、各エフェクトの有効/無効とパラメータを復元します。ファイルが存在しない場合やパース失敗時はfalseを返します。',
  '// 起動時に設定を読み込む\npipeline.LoadSettings("post_effects.json");\n\n// VFS経由のパスも使用可能\npipeline.LoadSettings("Assets/settings/post_effects.json");',
  '• JSON形式（nlohmann/json使用）\n• 存在しないファイルの場合はfalseを返すがクラッシュしない\n• SaveSettingsで保存したファイルを読み込む\n• F12キーでの保存と起動時の自動読み込みパターンが一般的'
],

'PostEffectPipeline-SaveSettings': [
  'bool SaveSettings(const std::string& filePath) const',
  '現在のポストエフェクト設定をJSON形式でファイルに保存します。トーンマッピングモード、露出、FXAA、ビネット、カラーグレーディング、各エフェクトの有効/無効とパラメータが全て保存されます。書き込み失敗時はfalseを返します。',
  '// 設定をファイルに保存\npipeline.SaveSettings("post_effects.json");\n\n// F12キーで保存するパターン\nif (keyboard.IsKeyTriggered(VK_F12)) {\n    pipeline.SaveSettings("post_effects.json");\n}',
  '• JSON形式で人間が読める形式で保存される\n• 既存ファイルは上書きされる\n• LoadSettingsと対になるメソッド'
],

'PostEffectPipeline-GetHDRRTVHandle': [
  'D3D12_CPU_DESCRIPTOR_HANDLE GetHDRRTVHandle() const',
  '内部HDRレンダーターゲットのRTVハンドルを返します。レイヤーシステム等でシーンレイヤーの出力先として使用する場合など、外部からHDR RTに直接アクセスする必要がある場合に使用します。',
  '// HDR RTのRTVハンドルを取得\nauto hdrRTV = pipeline.GetHDRRTVHandle();',
  '• フォーマットはR16G16B16A16_FLOAT\n• BeginScene〜EndScene間の描画先と同じRT\n• 通常のユーザーコードでは直接使用する必要はない'
],

'PostEffectPipeline-OnResize': [
  'void OnResize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'ウィンドウサイズ変更時にパイプライン内の全レンダーターゲットを再作成します。HDR/LDR RT、および全サブエフェクト(SSAO, Bloom, DoF, MotionBlur, SSR, Outline, VolumetricLight, TAA, AutoExposure)のOnResizeも連鎖的に呼び出します。',
  '// ウィンドウリサイズ時\nvoid OnResize(uint32_t w, uint32_t h) {\n    // GPU完了を待ってから\n    graphicsDevice.WaitForGPU();\n    swapChain.Resize(device, w, h);\n    depthBuffer.Create(device, w, h);\n    pipeline.OnResize(device, w, h);\n}',
  '• 呼び出し前にGPUの処理完了を待つこと\n• SwapChain, DepthBufferのリサイズと合わせて呼び出す\n• 内部で全サブエフェクトのOnResizeも呼ばれる'
],

// ============================================================
// PostEffectSettings (Graphics/PostEffect/PostEffectSettings.h)
// ============================================================
'PostEffectSettings-Load': [
  'static bool Load(const std::string& filePath, PostEffectPipeline& pipeline)',
  'JSONファイルからポストエフェクトの全設定を読み込み、PostEffectPipelineに適用するstaticメソッドです。nlohmann/jsonライブラリを使用してパースし、トーンマッピングモード、露出、各エフェクトの有効/無効とパラメータを設定します。ファイルが存在しないか、パース失敗時はfalseを返します。',
  '// 設定をJSONから読み込む\nGX::PostEffectPipeline pipeline;\npipeline.Initialize(device, width, height);\nGX::PostEffectSettings::Load("post_effects.json", pipeline);',
  '• staticメソッド（インスタンス不要）\n• nlohmann/json (ThirdParty/json.hpp) を使用\n• 通常はPostEffectPipeline::LoadSettings経由で呼び出す\n• 各キーが存在しない場合はデフォルト値が維持される'
],

'PostEffectSettings-Save': [
  'static bool Save(const std::string& filePath, const PostEffectPipeline& pipeline)',
  'PostEffectPipelineの現在の設定をJSON形式でファイルに保存するstaticメソッドです。全エフェクトの有効/無効フラグとパラメータ値を網羅的に出力します。書き込み失敗時はfalseを返します。',
  '// 設定をJSONに保存\nGX::PostEffectSettings::Save("post_effects.json", pipeline);',
  '• staticメソッド（インスタンス不要）\n• 通常はPostEffectPipeline::SaveSettings経由で呼び出す\n• JSON出力は整形(pretty print)される'
],

// ============================================================
// Bloom (Graphics/PostEffect/Bloom.h)
// ============================================================
'Bloom-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'Bloomエフェクトを初期化します。5段階のMIPレベルRTを作成し、閾値抽出・ダウンサンプル・Gaussianブラー(水平/垂直)・アディティブ合成用のシェーダーとPSOをコンパイルします。失敗時はfalseを返します。通常はPostEffectPipeline::Initializeから自動的に呼び出されます。',
  '// 通常はPostEffectPipeline経由で初期化される\nGX::PostEffectPipeline pipeline;\npipeline.Initialize(device, 1280, 720);\n\n// 直接初期化する場合（特殊用途）\nGX::Bloom bloom;\nif (!bloom.Initialize(device, 1280, 720)) {\n    // エラー処理\n}',
  '• PostEffectPipeline::Initialize内で自動的に呼ばれる\n• 5段階MIPレベル(1/2, 1/4, 1/8, 1/16, 1/32)のRTが作成される\n• HDRフォーマット(R16G16B16A16_FLOAT)のRTを使用\n• ShaderHotReload用のPSO Rebuilderも登録される'
],

'Bloom-Execute': [
  'void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& hdrRT, RenderTarget& destRT)',
  'Bloomエフェクトを実行します。HDRシーンから明るい部分を閾値抽出し、5段階のダウンサンプル→Gaussianブラー→アップサンプル合成を行い、最終結果を出力先RTに書き込みます。hdrRTはSRV状態で渡し、destRTにはhdrRTの内容+Bloom結果がアディティブブレンドで合成されます。',
  '// 通常はPostEffectPipeline::Resolve内で自動的に呼ばれる\n// 直接使用する場合:\nbloom.Execute(cmdList, frameIndex, hdrRT, destRT);',
  '• IsEnabled()がfalseの場合はPostEffectPipeline側でスキップされる\n• hdrRTはSRV状態で渡すこと\n• destRTにはシーン+Bloomが合成出力される\n• 内部で閾値抽出→5段ダウンサンプル→H/Vブラー→アップサンプル→加算合成を実行'
],

'Bloom-SetThreshold': [
  'void SetThreshold(float threshold)',
  'Bloom抽出の輝度閾値を設定します。この値を超える輝度のピクセルのみがBloom対象となります。値が低いほど多くのピクセルが光り、高いほど非常に明るい部分のみが光ります。デフォルトは1.0です。',
  '// Bloom閾値を下げて広範囲に光らせる\npipeline.GetBloom().SetThreshold(0.5f);\n\n// 閾値を上げてハイライトのみ\npipeline.GetBloom().SetThreshold(2.0f);',
  '• デフォルト値は1.0\n• HDR値が閾値を超えるピクセルが抽出される\n• 0に近いとほぼ全ピクセルが光り、性能に影響はないが見た目が白飛びする'
],

'Bloom-GetThreshold': [
  'float GetThreshold() const',
  '現在設定されているBloom閾値を返します。デフォルト値は1.0です。',
  '// 現在の閾値を取得\nfloat threshold = pipeline.GetBloom().GetThreshold();',
  '• デフォルト値は1.0'
],

'Bloom-SetIntensity': [
  'void SetIntensity(float intensity)',
  'Bloom合成の強度を設定します。値が大きいほどBloom効果が強くなり、光の滲みが目立ちます。デフォルトは0.5です。',
  '// Bloom強度を上げる\npipeline.GetBloom().SetIntensity(0.8f);\n\n// 控えめなBloom\npipeline.GetBloom().SetIntensity(0.2f);',
  '• デフォルト値は0.5\n• 定数バッファで毎フレームGPUに送信される'
],

'Bloom-GetIntensity': [
  'float GetIntensity() const',
  '現在設定されているBloom強度を返します。デフォルト値は0.5です。',
  '// 現在の強度を取得\nfloat intensity = pipeline.GetBloom().GetIntensity();',
  '• デフォルト値は0.5'
],

'Bloom-SetEnabled': [
  'void SetEnabled(bool enabled)',
  'Bloomエフェクトの有効/無効を切り替えます。無効時はPostEffectPipeline::ResolveでBloom処理全体がスキップされ、パフォーマンスコストは発生しません。デフォルトは有効です。',
  '// Bloomを無効化\npipeline.GetBloom().SetEnabled(false);\n\n// 有効化\npipeline.GetBloom().SetEnabled(true);',
  '• デフォルトで有効(enabled=true)\n• 無効時はExecuteが呼ばれないため、GPUコストなし'
],

'Bloom-IsEnabled': [
  'bool IsEnabled() const',
  'Bloomが有効かどうかを返します。デフォルトはtrueです。',
  '// Bloom有効状態を確認\nif (pipeline.GetBloom().IsEnabled()) {\n    // Bloomが有効\n}',
  '• デフォルト値はtrue'
],

'Bloom-OnResize': [
  'void OnResize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'ウィンドウサイズ変更時にBloom用の全MIPレベルRTを再作成します。5段階のダウンサンプルRT(1/2〜1/32)とブラー中間RTを新しいサイズに合わせて再生成します。PostEffectPipeline::OnResizeから自動的に呼び出されます。',
  '// 通常はPostEffectPipeline::OnResize経由で呼ばれる\npipeline.OnResize(device, newWidth, newHeight);',
  '• PostEffectPipeline::OnResize内で自動的に呼ばれる\n• 5段階のMIP RT + ブラー中間RTが再作成される\n• 呼び出し前にGPUの処理完了を待つこと'
],

// ============================================================
// SSAO (Graphics/PostEffect/SSAO.h)
// ============================================================
'SSAO-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'SSAOエフェクトを初期化します。AO出力RT(R8_UNORM)とブラー中間RTの作成、64サンプルの半球カーネル生成、シェーダーのコンパイル、AO生成・バイラテラルブラー(水平/垂直)・乗算合成用PSOの作成を行います。失敗時はfalseを返します。',
  '// 通常はPostEffectPipeline経由で初期化される\nGX::PostEffectPipeline pipeline;\npipeline.Initialize(device, 1280, 720);\n\n// 直接初期化する場合\nGX::SSAO ssao;\nif (!ssao.Initialize(device, 1280, 720)) {\n    // エラー処理\n}',
  '• PostEffectPipeline::Initialize内で自動的に呼ばれる\n• 64サンプルの半球カーネルが乱数で生成される（ノイズテクスチャは不使用、ハッシュベース回転）\n• AO出力はR8_UNORMフォーマット（1チャンネル）'
],

'SSAO-Execute': [
  'void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, RenderTarget& hdrRT, DepthBuffer& depthBuffer, const Camera3D& camera)',
  'SSAOエフェクトを実行します。深度バッファからビュー空間の位置を復元し、半球サンプリングによる遮蔽計算を行います。バイラテラルブラー(水平+垂直2パス)でノイズを除去し、最終的にHDRシーンにAO値を乗算合成(MultiplyBlend)します。hdrRTにインプレースで合成されるため、出力先RTは不要です。',
  '// 通常はPostEffectPipeline::Resolve内で自動的に呼ばれる\n// 直接使用する場合:\nssao.Execute(cmdList, frameIndex, hdrRT, depthBuffer, camera);',
  '• hdrRTにインプレース乗算合成される（出力先RTは別途不要）\n• 深度→ビュー空間位置復元→半球64サンプリング→遮蔽率計算\n• バイラテラルブラーでエッジ保持しつつノイズ除去\n• カメラの射影行列/逆射影行列/near/farが定数バッファで送信される'
],

'SSAO-SetRadius': [
  'void SetRadius(float r)',
  'SSAOのサンプリング半径を設定します。半球カーネルのサンプル範囲を決定し、大きいほど広範囲の遮蔽を検出しますが、ディテールが減少します。デフォルトは0.5です。',
  '// SSAO半径を調整\npipeline.GetSSAO().SetRadius(0.8f);  // 広範囲の遮蔽\npipeline.GetSSAO().SetRadius(0.3f);  // 細かいディテール重視',
  '• デフォルト値は0.5\n• ワールド空間の単位で指定\n• 大きすぎると不自然なハロー(光輪)が発生する'
],

'SSAO-GetRadius': [
  'float GetRadius() const',
  '現在設定されているSSAOサンプリング半径を返します。デフォルト値は0.5です。',
  '// SSAO半径を取得\nfloat radius = pipeline.GetSSAO().GetRadius();',
  '• デフォルト値は0.5'
],

'SSAO-SetBias': [
  'void SetBias(float b)',
  'SSAOのバイアス値を設定します。深度の自己遮蔽(self-occlusion)アーティファクトを防ぐためのオフセットです。値が大きいほどアーティファクトは減りますが、細かい遮蔽が検出されにくくなります。デフォルトは0.025です。',
  '// バイアスを調整\npipeline.GetSSAO().SetBias(0.03f);\n\n// アーティファクトが多い場合は大きくする\npipeline.GetSSAO().SetBias(0.05f);',
  '• デフォルト値は0.025\n• 平面でのアクネ(ちらつき)が発生する場合は値を大きくする\n• 大きすぎると遮蔽が弱くなる'
],

'SSAO-GetBias': [
  'float GetBias() const',
  '現在設定されているSSAOバイアス値を返します。デフォルト値は0.025です。',
  '// SSAOバイアスを取得\nfloat bias = pipeline.GetSSAO().GetBias();',
  '• デフォルト値は0.025'
],

'SSAO-SetPower': [
  'void SetPower(float p)',
  'SSAOの強度(べき乗)を設定します。AO値にpow(ao, power)を適用し、遮蔽効果のコントラストを調整します。値が大きいほど遮蔽が暗くなり、はっきりとした陰影が出ます。デフォルトは2.0です。',
  '// SSAO強度を上げる\npipeline.GetSSAO().SetPower(3.0f);\n\n// 控えめなSSAO\npipeline.GetSSAO().SetPower(1.0f);',
  '• デフォルト値は2.0\n• pow(ao, power)として適用される\n• 1.0でリニア、2.0以上でコントラスト増加'
],

'SSAO-GetPower': [
  'float GetPower() const',
  '現在設定されているSSAO強度(べき乗値)を返します。デフォルト値は2.0です。',
  '// SSAO強度を取得\nfloat power = pipeline.GetSSAO().GetPower();',
  '• デフォルト値は2.0'
],

'SSAO-SetEnabled': [
  'void SetEnabled(bool enabled)',
  'SSAOエフェクトの有効/無効を切り替えます。無効時はPostEffectPipeline::ResolveでSSAO処理全体がスキップされ、パフォーマンスコストは発生しません。デフォルトは有効です。',
  '// SSAOを無効化（パフォーマンス向上）\npipeline.GetSSAO().SetEnabled(false);\n\n// 有効化\npipeline.GetSSAO().SetEnabled(true);',
  '• デフォルトで有効(enabled=true)\n• 無効時はExecuteが呼ばれないため、GPUコストなし\n• SSAOはポストエフェクトチェーンの最初に実行される'
],

'SSAO-IsEnabled': [
  'bool IsEnabled() const',
  'SSAOが有効かどうかを返します。デフォルトはtrueです。',
  '// SSAO有効状態を確認\nif (pipeline.GetSSAO().IsEnabled()) {\n    // SSAOが有効\n}',
  '• デフォルト値はtrue'
],

'SSAO-OnResize': [
  'void OnResize(ID3D12Device* device, uint32_t width, uint32_t height)',
  'ウィンドウサイズ変更時にSSAO用のRTを再作成します。AO出力RT(R8_UNORM)とブラー中間RTを新しいサイズで再生成します。PostEffectPipeline::OnResizeから自動的に呼び出されます。',
  '// 通常はPostEffectPipeline::OnResize経由で呼ばれる\npipeline.OnResize(device, newWidth, newHeight);',
  '• PostEffectPipeline::OnResize内で自動的に呼ばれる\n• AO出力RT + ブラー中間RTが再作成される\n• 呼び出し前にGPUの処理完了を待つこと'
],

};
