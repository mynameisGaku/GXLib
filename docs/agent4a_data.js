// Agent 4a: Layer System (4 classes)
// RenderLayer, LayerStack, LayerCompositor, MaskScreen
{

// ============================================================
// RenderLayer (Graphics/Layer/RenderLayer.h)
// ============================================================
'RenderLayer-Create': [
  'bool Create(ID3D12Device* device, const std::string& name, int32_t zOrder, uint32_t width, uint32_t height)',
  '描画レイヤーを作成し、内部にRGBA8(LDR)のRenderTargetを生成します。nameはレイヤーの識別名、zOrderは合成時の描画順序（小さいほど先に描画）、width/heightはレンダーターゲットの解像度です。作成に成功するとtrueを返します。通常はLayerStack::CreateLayer()経由で呼び出されるため、直接使用する場面は少ないです。',
  '// レイヤーを直接作成\nGX::RenderLayer sceneLayer;\nbool ok = sceneLayer.Create(device, "Scene", 0, 1280, 720);\nif (!ok) {\n    GX_LOG_ERROR("Failed to create scene layer");\n}',
  '* 内部でRenderTarget::Create()を呼びRGBA8_UNORMフォーマットのRTを生成する\n* 通常はLayerStack::CreateLayer()を使用し、直接Createを呼ぶことは少ない\n* zOrderが同値の場合、合成順序は追加順に依存する'
],

'RenderLayer-SetZOrder': [
  'void SetZOrder(int32_t z)',
  'レイヤーのZ-order値を設定します。Z-orderは合成時の描画順序を決定し、値が小さいレイヤーが先に（背面に）描画されます。変更後はLayerStack::MarkDirty()を呼んでソートを再実行する必要があります。',
  '// UIレイヤーを最前面に変更\nuiLayer->SetZOrder(100);\nlayerStack.MarkDirty();  // ソート再実行を通知',
  '* 変更後はLayerStack::MarkDirty()を呼ばないとソート順が反映されない\n* 負の値も使用可能（マスクレイヤーは内部的に-1を使用）\n* 同じZ-order値を持つレイヤーの描画順は不定'
],

'RenderLayer-SetBlendMode': [
  'void SetBlendMode(LayerBlendMode mode)',
  'レイヤーの合成時のブレンドモードを設定します。LayerBlendModeにはAlpha（通常の透過合成）、Add（加算）、Sub（減算）、Mul（乗算）、Screen（スクリーン）、None（上書き）の6種類があります。デフォルトはAlphaです。',
  '// エフェクトレイヤーを加算合成に設定\neffectLayer->SetBlendMode(GX::LayerBlendMode::Add);\n\n// 背景レイヤーは上書き（ブレンドなし）\nbgLayer->SetBlendMode(GX::LayerBlendMode::None);',
  '* デフォルトはLayerBlendMode::Alpha\n* Addは発光エフェクト、Mulは影やカラーフィルタに適する\n* マスク付きレイヤーではAlphaとAddのみ対応（他はAlphaにフォールバック）'
],

'RenderLayer-SetVisible': [
  'void SetVisible(bool v)',
  'レイヤーの表示/非表示を切り替えます。非表示のレイヤーはLayerCompositor::Composite()で合成処理をスキップされます。レイヤーへの描画自体は引き続き可能ですが、最終出力に反映されません。',
  '// デバッグレイヤーの表示切替\nif (showDebug) {\n    debugLayer->SetVisible(true);\n} else {\n    debugLayer->SetVisible(false);\n}',
  '* デフォルトはtrue（表示）\n* 非表示でもBegin/Endによる描画は可能（バッファには書き込まれる）\n* Opacity=0とは異なり、合成処理自体をスキップするためパフォーマンスに優しい'
],

'RenderLayer-GetRenderTarget': [
  'RenderTarget& GetRenderTarget()',
  'レイヤーが内部に保持するRenderTargetの参照を返します。RTVハンドルの取得やリソース状態遷移など、低レベルなD3D12操作に使用します。constオーバーロードも提供されています。PostEffectPipelineのResolve先として使用する場合などにRTVHandleを取得できます。',
  '// レイヤーのRTをSRV状態に遷移\nauto& rt = sceneLayer->GetRenderTarget();\nrt.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);\n\n// RTVハンドルを取得\nD3D12_CPU_DESCRIPTOR_HANDLE rtv = sceneLayer->GetRTVHandle();',
  '* RGBA8_UNORM(LDR)フォーマットのRenderTarget\n* TransitionTo()でリソース状態を管理できる\n* GetRTVHandle()ショートカットメソッドも利用可能\n* PostEffectPipeline.Resolve()の出力先として指定可能'
],

'RenderLayer-SetName': [
  'const std::string& GetName() const',
  'レイヤーの識別名を取得します。名前はCreate()時に設定され、LayerStack::GetLayer()での検索キーとして使用されます。SetNameメソッドは提供されず、作成時にのみ名前を指定できます。',
  '// レイヤー名を取得\nconst std::string& name = layer->GetName();\nGX_LOG_INFO("Current layer: %s", name.c_str());',
  '* 名前はCreate()の引数でのみ設定可能（SetNameは存在しない）\n* LayerStack::GetLayer(name)の検索に使用される\n* 一意な名前を付けることを推奨'
],

// ============================================================
// LayerStack (Graphics/Layer/LayerStack.h)
// ============================================================
'LayerStack-AddLayer': [
  'RenderLayer* CreateLayer(ID3D12Device* device, const std::string& name, int32_t zOrder, uint32_t w, uint32_t h)',
  'RenderLayerを新規作成してスタックに追加し、そのポインタを返します。内部でunique_ptrとして所有権を管理するため、返されるポインタのdeleteは不要です。作成失敗時はnullptrを返します。追加後は自動的にソートが必要な状態(dirty)になります。',
  '// シーンレイヤーとUIレイヤーを作成\nGX::LayerStack layerStack;\nauto* sceneLayer = layerStack.CreateLayer(device, "Scene", 0, 1280, 720);\nauto* uiLayer    = layerStack.CreateLayer(device, "UI",    10, 1280, 720);\n\nif (!sceneLayer || !uiLayer) {\n    GX_LOG_ERROR("Failed to create layers");\n}',
  '* 内部でRenderLayer::Create()を呼び出す\n* LayerStackがunique_ptrで所有権を保持するため、返されたポインタをdeleteしないこと\n* 追加時に自動でm_needsSort=trueになる\n* 同名レイヤーの重複チェックは行わない'
],

'LayerStack-RemoveLayer': [
  'bool RemoveLayer(const std::string& name)',
  '指定した名前のレイヤーをスタックから削除します。削除に成功するとtrueを返し、該当レイヤーが見つからない場合はfalseを返します。削除されたレイヤーのunique_ptrも破棄されるため、以前に取得したポインタは無効になります。',
  '// デバッグレイヤーを削除\nbool removed = layerStack.RemoveLayer("Debug");\nif (removed) {\n    debugLayer = nullptr;  // ポインタを無効化\n}',
  '* 削除後はCreateLayer()で返されたポインタが無効になるため、nullptr代入を推奨\n* 削除時にソートフラグが自動設定される\n* 存在しないレイヤー名を指定してもエラーにはならない（falseを返すのみ）'
],

'LayerStack-Sort': [
  'void MarkDirty()',
  'レイヤーのソートが必要であることを通知します。SetZOrder()でZ-orderを変更した後に呼び出してください。実際のソートはGetSortedLayers()呼び出し時に遅延実行されます。CreateLayer/RemoveLayerでは内部で自動設定されるため不要です。',
  '// Z-order変更後にソートを通知\nsceneLayer->SetZOrder(5);\nuiLayer->SetZOrder(20);\nlayerStack.MarkDirty();  // 次のGetSortedLayers()でソートされる',
  '* 遅延ソート方式: MarkDirty()はフラグを立てるだけで、実際のソートはGetSortedLayers()呼出時\n* CreateLayer/RemoveLayerは内部でフラグを自動設定するため、手動呼出は不要\n* ソートはZ-order昇順（小→大 = 背面→前面）'
],

'LayerStack-GetLayers': [
  'const std::vector<RenderLayer*>& GetSortedLayers()',
  'Z-order昇順にソートされたレイヤーポインタのリストを返します。ソートが必要な場合は内部で自動的にソートを実行してから返します。LayerCompositor::Composite()が内部で使用しており、通常は直接呼び出す必要はありません。',
  '// 全レイヤーを列挙\nconst auto& layers = layerStack.GetSortedLayers();\nfor (auto* layer : layers) {\n    GX_LOG_INFO("Layer: %s (Z:%d, Visible:%d)",\n        layer->GetName().c_str(),\n        layer->GetZOrder(),\n        layer->IsVisible());\n}',
  '* ソートが必要な場合のみ内部でstd::sortを実行する（遅延評価）\n* 返されるvectorの参照はLayerStackが所有するため、要素の追加/削除はしないこと\n* LayerCompositor::Composite()が毎フレーム呼び出すため、通常は直接使用しない'
],

// ============================================================
// LayerCompositor (Graphics/Layer/LayerCompositor.h)
// ============================================================
'LayerCompositor-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t w, uint32_t h)',
  'レイヤーコンポジターを初期化します。ブレンドモード別のPSO（Alpha/Add/Sub/Mul/Screen/None の6種 + マスク付きAlpha/Add の2種）、ルートシグネチャ、定数バッファ、マスク用SRVヒープを作成します。シェーダーホットリロード用のPSO Rebuilderも登録されます。',
  '// コンポジター初期化\nGX::LayerCompositor compositor;\nif (!compositor.Initialize(device, 1280, 720)) {\n    GX_LOG_ERROR("Failed to initialize LayerCompositor");\n    return false;\n}',
  '* 内部で8個のPSO（マスクなし6 + マスクあり2）を作成する\n* LayerComposite.hlslシェーダーをコンパイルする\n* ShaderLibrary経由のホットリロードに対応\n* w, hはバックバッファの解像度に合わせること'
],

'LayerCompositor-Composite': [
  'void Composite(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV, LayerStack& layerStack)',
  '全レイヤーをZ-order昇順にフルスクリーン三角形で合成し、バックバッファに出力します。まずバックバッファを黒でクリアし、各レイヤーのブレンドモードに応じたPSOを選択して描画します。非表示レイヤーとOpacity=0のレイヤーはスキップされます。マスク付きレイヤーは専用のマスク合成パスで処理されます。',
  '// メインループでの合成\n// 1. 各レイヤーに描画\nsceneLayer->Begin(cmdList);\nsceneLayer->Clear(cmdList);\n// ... シーン描画 ...\nsceneLayer->End();\n\nuiLayer->Begin(cmdList);\nuiLayer->Clear(cmdList);\n// ... UI描画 ...\nuiLayer->End();\n\n// 2. バックバッファに合成\ncompositor.Composite(cmdList, frameIndex, backBufferRTV, layerStack);',
  '* バックバッファは黒(0,0,0,1)でクリアされる\n* フルスクリーン三角形（SV_VertexID、VBなし、Draw(3,1,0,0)）で描画\n* 各レイヤーのRTは自動的にSRV状態に遷移される\n* マスク付きレイヤーは専用SRVヒープ（2スロット: layer+mask）を使用'
],

'LayerCompositor-SetMaskTexture': [
  'void DrawLayerMasked(RenderLayer& layer, RenderLayer& mask)',
  'マスク付きレイヤーを合成するための内部メソッドです。レイヤーのRTとマスクのRTを専用SRVヒープに書き込み、マスク対応PSOで合成します。直接呼び出すのではなく、RenderLayer::SetMask()でマスクを設定すると、Composite()が自動的にこのパスを使用します。',
  '// マスクの設定（Composite内で自動使用される）\nGX::MaskScreen maskScreen;\nmaskScreen.Create(device, 1280, 720);\nmaskScreen.SetEnabled(true);\n\n// マスクを描画\nmaskScreen.Clear(cmdList, 0);  // 全面透過\nmaskScreen.DrawCircle(cmdList, frameIndex, 640, 360, 200);  // 円形マスク\n\n// レイヤーにマスクを設定\nsceneLayer->SetMask(maskScreen.GetAsLayer());\n\n// Composite()がマスク合成を自動処理\ncompositor.Composite(cmdList, frameIndex, backBufferRTV, layerStack);',
  '* マスク付き合成はAlphaとAddブレンドモードのみ対応\n* 専用SRVヒープは2スロット x 2フレーム = 4スロットで管理\n* マスクRTのRチャンネルがマスク値（0=透過、1=不透明）として使用される\n* RenderLayer::SetMask(nullptr)でマスクを解除できる'
],

// ============================================================
// MaskScreen (Graphics/Layer/MaskScreen.h)
// ============================================================
'MaskScreen-Initialize': [
  'bool Create(ID3D12Device* device, uint32_t w, uint32_t h)',
  'マスクスクリーンを作成します。内部にRGBA8フォーマットのRenderLayer（"_Mask", Z=-1）を生成し、Rチャンネルをマスク値として使用します。矩形・円描画用のシェーダー、ルートシグネチャ、PSO、頂点バッファ、定数バッファも同時に初期化されます。',
  '// マスクスクリーンの作成\nGX::MaskScreen maskScreen;\nif (!maskScreen.Create(device, 1280, 720)) {\n    GX_LOG_ERROR("Failed to create MaskScreen");\n    return false;\n}',
  '* 内部RenderLayerは"_Mask"の名前でZ-order=-1で作成される\n* MaskDraw.hlslシェーダーをコンパイルする\n* 頂点バッファは2048バイト（矩形6頂点+円64セグメント対応）\n* ShaderLibrary経由のホットリロードに対応'
],

'MaskScreen-Begin': [
  'void SetEnabled(bool enabled)',
  'マスクスクリーンの有効/無効を切り替えます。無効時はマスク処理がスキップされ、レイヤーは通常合成されます。DXLib互換のSetUseMaskScreenFlag相当の機能です。',
  '// マスクの有効化\nmaskScreen.SetEnabled(true);\n\n// Lキーでマスクのトグル\nif (keyboard.IsKeyTriggered(VK_L)) {\n    maskScreen.SetEnabled(!maskScreen.IsEnabled());\n}',
  '* デフォルトはfalse（無効）\n* IsEnabled()で現在の状態を取得可能\n* 有効/無効の切替はフレーム間で自由に行える'
],

'MaskScreen-End': [
  'void Clear(ID3D12GraphicsCommandList* cmdList, uint8_t fill)',
  'マスクスクリーン全体を指定値でクリアします。fill=0で全面透過（マスクなし）、fill=255で全面不透明になります。マスク描画の前に呼び出して初期状態を設定します。内部ではfill値を0.0-1.0に正規化してRenderLayer::Clear()に渡します。',
  '// 全面透過でクリアしてから円形マスクを描画\nmaskScreen.Clear(cmdList, 0);\nmaskScreen.DrawCircle(cmdList, frameIndex, 640, 360, 200, 1.0f);\n\n// 全面不透明でクリアしてから矩形を透過に\nmaskScreen.Clear(cmdList, 255);\nmaskScreen.DrawFillRect(cmdList, frameIndex, 100, 100, 200, 200, 0.0f);',
  '* fill=0: 全面透過（描画した部分のみ表示）\n* fill=255: 全面不透明（描画した部分を透過にする反転マスク用）\n* 内部でfill/255.0fに正規化される\n* デフォルト引数はfill=0'
],

'MaskScreen-DrawRect': [
  'void DrawFillRect(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, float x, float y, float w, float h, float value)',
  'マスクスクリーンに矩形を描画します。(x, y)は左上座標、(w, h)はサイズ、valueはマスク値（0.0=透過、1.0=不透明）です。内部で2三角形（6頂点）を生成し、正射影行列（ピクセル座標系）で描画します。',
  '// 画面中央に矩形マスクを描画\nmaskScreen.Clear(cmdList, 0);\nmaskScreen.DrawFillRect(cmdList, frameIndex,\n    340.0f, 160.0f, 600.0f, 400.0f, 1.0f);\n\n// レイヤーにマスクを適用\nsceneLayer->SetMask(maskScreen.GetAsLayer());',
  '* value=1.0fで不透明（マスク領域として表示）、0.0fで透過\n* 座標系はピクセル座標（左上原点）\n* デフォルトvalue=1.0f\n* 1回のDrawFillRectで6頂点を使用'
],

'MaskScreen-DrawCircle': [
  'void DrawCircle(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex, float cx, float cy, float radius, float value)',
  'マスクスクリーンに円を描画します。(cx, cy)は中心座標、radiusは半径、valueはマスク値です。内部で64セグメントの三角形ファン（192頂点）を生成して円を近似描画します。スポットライト風マスクやビネット効果に使用できます。',
  '// 円形マスクでスポットライト効果\nmaskScreen.Clear(cmdList, 0);\nmaskScreen.DrawCircle(cmdList, frameIndex,\n    640.0f, 360.0f, 250.0f, 1.0f);\nsceneLayer->SetMask(maskScreen.GetAsLayer());\n\n// 複数の円を重ねて描画も可能\nmaskScreen.DrawCircle(cmdList, frameIndex, 200.0f, 200.0f, 100.0f, 1.0f);\nmaskScreen.DrawCircle(cmdList, frameIndex, 1000.0f, 500.0f, 150.0f, 1.0f);',
  '* 64セグメントで円を近似（192頂点 = 64三角形 x 3頂点）\n* 座標系はピクセル座標（左上原点）\n* デフォルトvalue=1.0f\n* 複数のDrawCircle/DrawFillRectを組み合わせて複雑なマスク形状を作成可能'
],

'MaskScreen-GetSRVIndex': [
  'RenderLayer* GetAsLayer()',
  'マスクスクリーンの内部RenderLayerへのポインタを返します。RenderLayer::SetMask()に渡すことで、対象レイヤーにマスクを適用できます。LayerCompositorがComposite時にマスクRTのSRVを参照するために使用されます。',
  '// マスクレイヤーを取得してシーンに適用\nGX::RenderLayer* maskLayer = maskScreen.GetAsLayer();\nsceneLayer->SetMask(maskLayer);\n\n// マスク解除\nsceneLayer->SetMask(nullptr);',
  '* 返されるポインタはMaskScreenが内部で保持するRenderLayerのもの\n* SetMask(nullptr)でマスクを解除できる\n* マスクRTのRチャンネルがマスク値として使用される\n* MaskScreenの寿命中は有効なポインタが保証される'
],

}
