// Auto-generated API reference data for Graphics/3D classes
// PageId-MemberName format

{

// ============================================================
// Renderer3D (page-Renderer3D)
// ============================================================

'Renderer3D-Initialize': [
  'bool Initialize(ID3D12Device*, DescriptorHeap* srvHeap, uint32_t w, uint32_t h)',
  'PBR 3Dレンダラーを初期化する。D3D12デバイス、SRVヒープ、画面サイズを指定し、PBRパイプライン（ルートシグネチャ・PSO）、定数バッファ、シャドウマップ、深度バッファ等すべてのリソースを生成する。アプリケーション起動時に一度だけ呼び出す。',
  '// Renderer3D の初期化\nGX::Renderer3D renderer3D;\nbool ok = renderer3D.Initialize(device, &srvHeap, 1920, 1080);\nif (!ok) {\n    GX::Logger::Error("Renderer3D init failed");\n}',
  '• 内部で CSM (4096x4096x4)、SpotShadow (2048x2048)、PointShadow (1024x1024x6) をすべて作成する\n• SRVヒープには連続した十分なスロットが必要（CSM t8-t11, Spot t12, Point t13）\n• 初期化後 ShaderLibrary へ PSO リビルダーが自動登録される\n• k_MaxObjectsPerFrame=512 で 1フレームの描画上限が決まる'
],

'Renderer3D-BeginScene': [
  'void BeginScene(ID3D12GraphicsCommandList*, uint32_t frameIndex, D3D12_CPU_DESCRIPTOR_HANDLE dsv, Camera3D&)',
  'シーン描画の開始を宣言する。コマンドリスト、フレームインデックス、深度バッファDSVハンドル、カメラを設定し、フレーム定数バッファを更新する。TAA が有効な場合はカメラにジッターオフセットが適用されるため、Camera3D は非const参照で渡す必要がある。',
  '// 毎フレームの描画ループ\nrenderer3D.BeginScene(cmdList, frameIndex,\n    depthBuffer.GetDSVHandle(), camera);\n// ここでライト設定・メッシュ描画\nrenderer3D.EndScene();',
  '• Camera3D& は非const — TAA ジッター適用のため\n• フレーム定数バッファ (b1) にビュー行列・射影行列・CSM情報・フォグ情報が書き込まれる\n• BeginScene → SetDirectionalLight/AddPointLight → DrawMesh → EndScene の順序で呼ぶ\n• シャドウパスは BeginScene の前に実行する'
],

'Renderer3D-EndScene': [
  'void EndScene()',
  'シーン描画を終了する。内部でスカイボックスの描画やプリミティブバッチのフラッシュ等の後処理を行う。BeginScene と対にして必ず呼び出す。',
  '// シーン終了\nrenderer3D.EndScene();\n// ここからポストエフェクトパイプラインへ',
  '• BeginScene の後に必ず呼ぶこと。呼び忘れるとコマンドリストが不正な状態になる\n• 内部でオブジェクトCBリングバッファのオフセットがリセットされる\n• PrimitiveBatch3D の End() も内部で呼ばれる'
],

'Renderer3D-DrawMesh': [
  'void DrawMesh(const Mesh&, const Transform3D&, const Material&)',
  'メッシュを指定のトランスフォームとマテリアルで描画する。Transform3D からワールド行列を計算し、オブジェクト定数バッファとマテリアル定数バッファを更新してドローコールを発行する。メインパスでもシャドウパスでも使用できる。',
  '// キューブを描画\nGX::Transform3D cubeTransform;\ncubeTransform.SetPosition(0.0f, 1.0f, 0.0f);\ncubeTransform.SetScale(2.0f);\nrenderer3D.DrawMesh(cubeMesh, cubeTransform, pbrMaterial);',
  '• k_MaxObjectsPerFrame=512 を超えるとアサーション失敗する\n• シャドウパスでも DrawMesh が呼ばれるため、影を落とすオブジェクト数に注意\n• マテリアルのテクスチャハンドルが有効な場合は自動的に SRV がバインドされる\n• VB/IB の冗長バインドは内部でスキップされる'
],

'Renderer3D-DrawMesh2': [
  'void DrawMesh(const Mesh&, const DirectX::XMMATRIX& world, const Material&)',
  'メッシュをワールド行列直接指定で描画する。物理シミュレーション等でクォータニオン回転を使用する場合、オイラー角では表現できない回転があるためこのオーバーロードを使う。Jolt Physics 等との連携に最適。',
  '// 物理シミュレーション結果のワールド行列で描画\nXMMATRIX world = XMMatrixRotationQuaternion(physQuat)\n    * XMMatrixTranslation(physPos.x, physPos.y, physPos.z);\nrenderer3D.DrawMesh(cubeMesh, world, pbrMaterial);',
  '• Transform3D 版と異なりオイラー角ジンバルロックの制約がない\n• 物理エンジンから取得したクォータニオン回転を直接使用できる\n• 内部処理は Transform3D 版と同一（オブジェクトCB更新→ドローコール）'
],

'Renderer3D-SetDirectionalLight': [
  'void SetDirectionalLight(const DirectionalLight&)',
  'ディレクショナルライト（太陽光）を設定する。シーン全体に影響する平行光源で、CSM シャドウマップの光源方向としても使用される。BeginScene の後、DrawMesh の前に呼び出す。',
  '// 太陽光の設定\nGX::DirectionalLight sunLight;\nsunLight.direction = { 0.3f, -1.0f, 0.5f };\nsunLight.color = { 1.0f, 0.95f, 0.9f };\nsunLight.intensity = 2.0f;\nrenderer3D.SetDirectionalLight(sunLight);',
  '• ディレクショナルライトは1つのみ設定可能\n• CSM シャドウの方向はこのライトの direction から自動計算される\n• intensity が 0 の場合はライト無効と同等になる'
],

'Renderer3D-AddPointLight': [
  'void AddPointLight(const PointLight&)',
  'ポイントライト（点光源）をシーンに追加する。複数のポイントライトを追加でき、最大8個まで対応。castShadow フラグが true のポイントライトは PointShadowMap で影を生成する。',
  '// ポイントライトの追加\nGX::PointLight lamp;\nlamp.position = { 5.0f, 3.0f, 0.0f };\nlamp.color = { 1.0f, 0.8f, 0.5f };\nlamp.intensity = 3.0f;\nlamp.radius = 15.0f;\nlamp.castShadow = true;\nrenderer3D.AddPointLight(lamp);',
  '• 最大8個まで追加可能（超過分は無視される）\n• castShadow=true のライトは1つだけ影を生成（最初の1つ）\n• PointShadowMap は Texture2DArray (6面) で全方位影を実現\n• 毎フレーム BeginScene 後にリセットされるため、毎回 Add し直す必要がある'
],

'Renderer3D-AddSpotLight': [
  'void AddSpotLight(const SpotLight&)',
  'スポットライト（集光灯）をシーンに追加する。最大4個まで対応。castShadow フラグが true のスポットライトは 2048x2048 のシャドウマップで影を生成する。',
  '// スポットライトの追加\nGX::SpotLight flashlight;\nflashlight.position = { 0.0f, 5.0f, -3.0f };\nflashlight.direction = { 0.0f, -1.0f, 1.0f };\nflashlight.color = { 1.0f, 1.0f, 1.0f };\nflashlight.intensity = 5.0f;\nflashlight.range = 20.0f;\nflashlight.innerAngle = 15.0f;\nflashlight.outerAngle = 30.0f;\nrenderer3D.AddSpotLight(flashlight);',
  '• 最大4個まで追加可能\n• castShadow=true のスポットライトは1つだけ影を生成\n• SRV スロット t12 にバインドされる\n• innerAngle と outerAngle の差でエッジのぼかし具合が変わる'
],

'Renderer3D-SetAmbientLight': [
  'void SetAmbientLight(float r, float g, float b)',
  'アンビエントライト（環境光）の色を設定する。シーン全体に均一に加算される基底照明で、影になっている部分が完全に黒くなるのを防ぐ。',
  '// 弱い青白いアンビエントライト\nrenderer3D.SetAmbientLight(0.1f, 0.12f, 0.15f);',
  '• デフォルトは (0, 0, 0) で環境光なし\n• 値が大きすぎるとシーン全体がフラットになる\n• HDR パイプラインではトーンマッピング後の見た目に影響する'
],

'Renderer3D-GetCSM': [
  'CascadedShadowMap& GetCSM()',
  'カスケードシャドウマップのインスタンスへの参照を取得する。CSM の分割比率調整やシャドウマップの直接操作が必要な場合に使用する。',
  '// CSM の分割比率をカスタマイズ\nauto& csm = renderer3D.GetCSM();\ncsm.SetCascadeSplits(0.05f, 0.15f, 0.4f, 1.0f);',
  '• CSM は 4096x4096 の深度テクスチャを4枚使用する\n• SRV は t8-t11 に配置される\n• 分割比率はシーンの規模に応じて調整すると品質が向上する'
],

'Renderer3D-GetPointShadowMap': [
  'PointShadowMap& GetPointShadowMap()',
  'ポイントシャドウマップのインスタンスへの参照を取得する。ポイントライト影の直接操作やデバッグ時に使用する。',
  '// ポイントシャドウマップのリソース確認\nauto& psm = renderer3D.GetPointShadowMap();\nauto* resource = psm.GetResource();',
  '• Texture2DArray (6スライス) で全方位の影を格納\n• 解像度は 1024x1024 per face\n• SRV は t13 にバインドされる'
],

'Renderer3D-OnResize': [
  'void OnResize(ID3D12Device*, uint32_t w, uint32_t h)',
  'ウィンドウリサイズ時に深度バッファや内部リソースを再作成する。SwapChain のリサイズ後に呼び出す。',
  '// ウィンドウリサイズ処理\nswapChain.Resize(newWidth, newHeight);\nrenderer3D.OnResize(device, newWidth, newHeight);',
  '• 深度バッファのサイズがスクリーンサイズに合わせて再作成される\n• シャドウマップのサイズは変更されない（固定解像度）\n• PostEffectPipeline の OnResize も別途呼ぶ必要がある'
],

// ============================================================
// Camera3D (page-Camera3D)
// ============================================================

'Camera3D-SetPosition': [
  'void SetPosition(const XMFLOAT3& pos)',
  'カメラのワールド位置を設定する。FPS/TPSモードではプレイヤー位置、Freeモードではエディタカメラ位置として使用される。位置変更後はビュー行列が自動的に再計算される。',
  '// カメラを原点の上空に配置\nGX::Camera3D camera;\ncamera.SetPosition({ 0.0f, 10.0f, -20.0f });',
  '• FreeモードではSetPositionで直接配置、FPS/TPSではSetTargetと連動\n• SetPosition(float x, float y, float z) のオーバーロードも使用可能\n• 方向ベクトル（Forward/Right/Up）は遅延計算（dirty flag）される'
],

'Camera3D-SetTarget': [
  'void SetTarget(const XMFLOAT3& target)',
  'カメラの注視点を設定する。カメラはこの点を向くようにビュー行列が構成される。TPSモードではターゲットオブジェクトの位置を毎フレーム更新する。',
  '// プレイヤーキャラクターを注視\ncamera.SetTarget(playerTransform.GetPosition());',
  '• Free/FPSモードではRotate()による回転が優先され、SetTargetは内部参照用\n• TPSモードではSetTPSDistance/SetTPSOffsetと組み合わせて使う\n• SetPosition と SetTarget が同一座標だとビュー行列が不正になる'
],

'Camera3D-SetUp': [
  'void SetUp(const XMFLOAT3& up)',
  'カメラの上方向ベクトルを設定する。通常は (0,1,0) で問題ないが、特殊なカメラワーク（宇宙空間や壁走り等）で上方向を変更する場合に使用する。',
  '// デフォルトの上方向\ncamera.SetUp({ 0.0f, 1.0f, 0.0f });',
  '• デフォルトは (0,1,0) の Y-up\n• 上方向と前方向が平行に近いとビュー行列が不安定になる\n• 正規化されていなくても内部で正規化される'
],

'Camera3D-SetFOV': [
  'void SetFOV(float fovY)',
  '垂直方向の視野角（Field of View）をラジアン単位で設定する。値が大きいほど広角になり、小さいほど望遠になる。ゲーム用途では通常 45度〜90度 (π/4〜π/2) を使用する。',
  '// 60度の視野角を設定\ncamera.SetFOV(XM_PI / 3.0f);',
  '• ラジアン単位で指定する（度数ではない）\n• XM_PIDIV4 (45度) がデフォルト値\n• SetPerspective() でも fovY を含めて一括設定可能'
],

'Camera3D-SetNearFar': [
  'void SetNearFar(float nearZ, float farZ)',
  'ニアクリップ面とファークリップ面の距離を設定する。この範囲外のオブジェクトはレンダリングされない。nearZ が小さすぎると深度バッファの精度が低下し、Z-fightingの原因になる。',
  '// 0.1〜500の範囲を設定\ncamera.SetNearFar(0.1f, 500.0f);',
  '• nearZ は 0 にできない（射影行列が破綻する）\n• farZ/nearZ の比が大きいほど深度精度が低下する\n• CSM のカスケード分割は nearZ〜farZ の範囲に基づいて計算される\n• デフォルトは nearZ=0.1, farZ=1000.0'
],

'Camera3D-SetAspectRatio': [
  'void SetAspectRatio(float aspect)',
  'アスペクト比（幅/高さ）を設定する。通常はウィンドウの幅と高さから計算して設定する。リサイズ時に更新が必要。',
  '// 16:9 のアスペクト比\ncamera.SetAspectRatio(1920.0f / 1080.0f);',
  '• SetPerspective() でも一括設定可能\n• リサイズイベント時に再設定すること\n• アスペクト比がウィンドウと一致しないと描画が歪む'
],

'Camera3D-GetViewMatrix': [
  'XMMATRIX GetViewMatrix() const',
  'ビュー行列を取得する。カメラの位置・方向からワールド空間→カメラ空間への変換行列を生成する。シェーダーの定数バッファ更新やカリング計算に使用する。',
  '// ビュー行列の取得\nXMMATRIX view = camera.GetViewMatrix();\nXMMATRIX vp = view * camera.GetProjectionMatrix();',
  '• Free/FPSモードではpitch/yawから前方ベクトルを計算\n• TPSモードではターゲット+距離+オフセットから位置を計算\n• 毎フレーム再計算される（キャッシュはdirty flagで管理）'
],

'Camera3D-GetProjectionMatrix': [
  'XMMATRIX GetProjectionMatrix() const',
  '射影行列を取得する。SetPerspective() または SetOrthographic() で設定されたパラメータから透視投影または正射影の行列を返す。TAA ジッターは含まれない。',
  '// 射影行列を取得\nXMMATRIX proj = camera.GetProjectionMatrix();',
  '• TAA ジッター無しの純粋な射影行列\n• ジッター付きが必要な場合は GetJitteredProjectionMatrix() を使う\n• デフォルトは透視投影（SetPerspective のパラメータ）'
],

'Camera3D-GetJitteredProjectionMatrix': [
  'XMMATRIX GetJitteredProjectionMatrix() const',
  'TAA ジッターオフセットを適用した射影行列を取得する。Temporal Anti-Aliasing (TAA) 使用時にサブピクセルオフセットを射影行列に加算し、フレームごとにわずかにズレたサンプリングを行う。',
  '// TAA 有効時の射影行列取得\ncamera.SetJitter(jitterX, jitterY);\nXMMATRIX jitteredProj = camera.GetJitteredProjectionMatrix();',
  '• SetJitter() で設定されたオフセットが射影行列の _31, _32 要素に加算される\n• TAA 無効時は GetProjectionMatrix() と同じ結果になる\n• Halton(2,3) シーケンスで 8 サンプルのジッターサイクルが一般的'
],

'Camera3D-GetViewProjectionMatrix': [
  'XMMATRIX GetViewProjectionMatrix() const',
  'ビュー行列と射影行列の積（VP行列）を取得する。ワールド空間からクリップ空間への変換を一度の行列乗算で行えるため、シェーダー定数バッファへの転送やカリング処理に便利。',
  '// VP行列でカリング判定\nXMMATRIX vp = camera.GetViewProjectionMatrix();\n// フラスタムカリングに使用',
  '• GetViewMatrix() * GetProjectionMatrix() の結果と同等\n• TAA ジッターは含まれない\n• PrimitiveBatch3D の Begin() に渡す用途にも使える'
],

'Camera3D-SetJitter': [
  'void SetJitter(float x, float y)',
  'TAA 用のサブピクセルジッターオフセットを NDC 空間で設定する。TAA エフェクトが毎フレーム自動で設定するため、通常ユーザーが直接呼び出す必要はない。',
  '// Halton シーケンスによるジッター設定\nfloat jx = (Halton(frameCount, 2) - 0.5f) / screenWidth;\nfloat jy = (Halton(frameCount, 3) - 0.5f) / screenHeight;\ncamera.SetJitter(jx, jy);',
  '• NDC 空間 (-1〜+1) での微小オフセット\n• TAA が無効なら SetJitter/ClearJitter は不要\n• 8 サンプルサイクルで繰り返される'
],

'Camera3D-GetPosition': [
  'XMFLOAT3 GetPosition() const',
  'カメラの現在のワールド位置を取得する。ライティング計算のスペキュラー反射や、オブジェクトのカメラ距離ソートなどに使用する。',
  '// カメラ位置からオブジェクトまでの距離を計算\nXMFLOAT3 camPos = camera.GetPosition();\nfloat dist = XMVectorGetX(XMVector3Length(\n    XMLoadFloat3(&objPos) - XMLoadFloat3(&camPos)));',
  '• const 参照で返されるためコピーコストが低い\n• FrameConstants の cameraPosition としてシェーダーに転送される'
],

'Camera3D-GetForward': [
  'XMFLOAT3 GetForward() const',
  'カメラの正規化された前方向ベクトルを取得する。カメラが向いている方向を表し、レイキャストやオーディオリスナー方向の設定に使用する。',
  '// カメラの前方に弾を発射\nXMFLOAT3 fwd = camera.GetForward();\nbulletVelocity = { fwd.x * speed, fwd.y * speed, fwd.z * speed };',
  '• pitch/yaw から計算される正規化ベクトル\n• dirty flag により必要なときだけ再計算\n• GetRight(), GetUp() も同様に利用可能'
],

'Camera3D-GetNearZ': [
  'float GetNearZ() / GetFarZ() const',
  'ニアクリップ面またはファークリップ面の距離を取得する。ポストエフェクト（DoF, SSR 等）で深度のリニアライズやレンジ計算に使用される。',
  '// 深度のリニアライズ計算\nfloat nearZ = camera.GetNearZ();\nfloat farZ = camera.GetFarZ();\nfloat linearDepth = nearZ * farZ / (farZ - depth * (farZ - nearZ));',
  '• デフォルト nearZ=0.1, farZ=1000.0\n• CSM のカスケード分割計算にも使用される\n• SetNearFar() または SetPerspective() で変更可能'
],

// ============================================================
// Transform3D (page-Transform3D)
// ============================================================

'Transform3D-SetPosition': [
  'void SetPosition(const XMFLOAT3&)',
  '3Dオブジェクトのワールド位置を設定する。XMFLOAT3 版のほか (float x, float y, float z) オーバーロードも利用可能。位置変更で dirty フラグが立ち、GetWorldMatrix() 時に再計算される。',
  '// オブジェクトを配置\nGX::Transform3D transform;\ntransform.SetPosition({ 5.0f, 0.0f, -3.0f });\n// または\ntransform.SetPosition(5.0f, 0.0f, -3.0f);',
  '• dirty フラグにより GetWorldMatrix() の呼び出し時にのみ行列が再計算される\n• 物理シミュレーション結果の反映にも使用する\n• デフォルト位置は原点 (0,0,0)'
],

'Transform3D-SetRotation': [
  'void SetRotation(const XMFLOAT3&)',
  '回転をオイラー角（ピッチ, ヨー, ロール）のラジアンで設定する。XMFLOAT3 版と (float pitch, float yaw, float roll) オーバーロードがある。内部ではジンバルロックの問題があるため、物理回転には DrawMesh の XMMATRIX 版を推奨する。',
  '// Y軸周りに45度回転\nGX::Transform3D transform;\ntransform.SetRotation(0.0f, XM_PI / 4.0f, 0.0f);',
  '• オイラー角の順序は pitch(X), yaw(Y), roll(Z)\n• ジンバルロックを避けるには Renderer3D::DrawMesh(mesh, XMMATRIX, material) を使用する\n• ラジアン単位（度数変換: XMConvertToRadians()）'
],

'Transform3D-SetScale': [
  'void SetScale(const XMFLOAT3&)',
  'オブジェクトのスケールを各軸ごとに設定する。均一スケール用に SetScale(float uniform) オーバーロードも利用可能。非均一スケールを使用する場合は法線変換に注意が必要。',
  '// 2倍の均一スケール\ntransform.SetScale(2.0f);\n// X方向のみ3倍に伸ばす\ntransform.SetScale({ 3.0f, 1.0f, 1.0f });',
  '• SetScale(float) で3軸均一スケール\n• 非均一スケールでは GetWorldInverseTranspose() が法線変換に必要\n• デフォルトスケールは (1,1,1)\n• 負のスケールはバックフェイスカリングに影響する'
],

'Transform3D-GetPosition': [
  'XMFLOAT3 GetPosition() const',
  '現在のワールド位置を取得する。const参照で返されるため効率的。オブジェクト間の距離計算やUI上の座標表示などに使用する。',
  '// 位置の取得\nconst XMFLOAT3& pos = transform.GetPosition();\nGX::Logger::Info("Pos: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);',
  '• const XMFLOAT3& を返すためコピーなし\n• GetRotation(), GetScale() も同様の形式で取得可能'
],

'Transform3D-GetRotation': [
  'XMFLOAT3 GetRotation() const',
  '現在のオイラー角回転を取得する。ピッチ(X回転), ヨー(Y回転), ロール(Z回転) のラジアン値が返される。',
  '// 回転値の取得\nconst XMFLOAT3& rot = transform.GetRotation();\nfloat yawDeg = XMConvertToDegrees(rot.y);',
  '• ラジアン単位で返される\n• オイラー角の順序は (pitch, yaw, roll)'
],

'Transform3D-GetScale': [
  'XMFLOAT3 GetScale() const',
  '現在のスケール値を取得する。各軸のスケール倍率が返される。',
  '// スケールの確認\nconst XMFLOAT3& s = transform.GetScale();\nbool isUniform = (s.x == s.y && s.y == s.z);',
  '• デフォルト値は (1, 1, 1)\n• const XMFLOAT3& を返す'
],

'Transform3D-GetWorldMatrix': [
  'XMMATRIX GetWorldMatrix() const',
  'ワールド行列を取得する。Scale * Rotation * Translation の順序（SRT）で合成される。dirty フラグにより前回のキャッシュが有効なら再計算をスキップする。',
  '// ワールド行列でバウンディングボックスを変換\nXMMATRIX world = transform.GetWorldMatrix();\nXMVECTOR worldPos = XMVector3Transform(localPos, world);',
  '• SRT 順: Scale → RotateX(pitch) → RotateY(yaw) → RotateZ(roll) → Translate\n• mutable dirty フラグで遅延再計算\n• GetWorldInverseTranspose() で法線変換用の行列も取得可能'
],

// ============================================================
// MeshData / MeshGenerator (page-MeshData)
// ============================================================

'MeshData-vertices': [
  'std::vector<Vertex3D> vertices',
  'CPU側の頂点データ配列。Vertex3D_PBR 型の頂点（位置・法線・UV・タンジェント）を格納する。MeshGenerator の各メソッドで自動生成されるほか、手動で頂点データを構築することも可能。',
  '// カスタム頂点データの作成\nGX::MeshData meshData;\nmeshData.vertices.push_back({{ 0,0,0 }, { 0,1,0 }, { 0,0 }, { 1,0,0,1 }});\nmeshData.vertices.push_back({{ 1,0,0 }, { 0,1,0 }, { 1,0 }, { 1,0,0,1 }});',
  '• 実際の型は Vertex3D_PBR（position, normal, texcoord, tangent）\n• MeshGenerator が生成するメッシュにはタンジェントも正しく設定される\n• indices と対で使用する'
],

'MeshData-indices': [
  'std::vector<uint32_t> indices',
  'CPU側のインデックスデータ配列。頂点の参照順序を定義し、三角形リストとして描画される。3つのインデックスで1つの三角形を構成する。',
  '// 三角形1つ分のインデックス\nmeshData.indices = { 0, 1, 2 };',
  '• uint32_t 型（DXGI_FORMAT_R32_UINT）\n• 反時計回り (CCW) がフロントフェイス\n• インデックスバッファを使うことで頂点の重複を削減できる'
],

'MeshData-CreateBox': [
  'static MeshData CreateBox(float w, float h, float d)',
  '中心原点のボックス（直方体）メッシュを生成する。各面に個別の法線・UV・タンジェントが設定されるため、PBR シェーディングに対応する。デバッグ表示やプレースホルダーオブジェクトに最適。',
  '// 2x2x2 のキューブを生成\nGX::MeshData cubeData = GX::MeshGenerator::CreateBox(2.0f, 2.0f, 2.0f);\nGX::GPUMesh cubeMesh = renderer3D.CreateGPUMesh(cubeData);',
  '• 中心が原点 (0,0,0) に配置される\n• 24頂点 (6面 x 4頂点)、36インデックス\n• UV は各面で 0-1 にマッピングされる'
],

'MeshData-CreateSphere': [
  'static MeshData CreateSphere(float radius, int slices, int stacks)',
  '中心原点の球体メッシュを生成する。slices（経線分割）と stacks（緯線分割）で精度を制御する。惑星、弾、パーティクル等の球体オブジェクトに使用する。',
  '// 半径1.0、32x16分割の球体\nGX::MeshData sphereData = GX::MeshGenerator::CreateSphere(1.0f, 32, 16);',
  '• slices が大きいほど滑らかになるがポリゴン数が増加する\n• 推奨: slices=32, stacks=16 で十分な品質\n• UV はメルカトル図法風にマッピングされる'
],

'MeshData-CreateCylinder': [
  'static MeshData CreateCylinder(float topR, float bottomR, float h, int slices)',
  '中心原点の円柱（またはコーン）メッシュを生成する。上部半径と下部半径を変えることで円錐台にもなる。topR=0 でコーンになる。柱、パイプ、木の幹等に使用する。',
  '// 半径1.0、高さ3.0の円柱\nGX::MeshData cylData = GX::MeshGenerator::CreateCylinder(1.0f, 1.0f, 3.0f, 24);\n// コーン（topR=0）\nGX::MeshData coneData = GX::MeshGenerator::CreateCylinder(0.0f, 1.0f, 2.0f, 24);',
  '• topRadius=0 でコーンメッシュになる\n• 側面の法線は Cross(Bitangent, Tangent) で外向きに生成\n• 底面キャップは CCW ワインディングで描画される\n• stackCount パラメータで側面の縦分割数も指定可能'
],

'MeshData-CreatePlane': [
  'static MeshData CreatePlane(float w, float d, int xDiv, int zDiv)',
  'XZ平面（Y=0）に水平な矩形メッシュを生成する。分割数を指定して細かいテッセレーションが可能。地面、水面、テストプレーンなどに使用する。',
  '// 50x50 の地面メッシュ（10x10分割）\nGX::MeshData ground = GX::MeshGenerator::CreatePlane(50.0f, 50.0f, 10, 10);',
  '• Y=0 の XZ 平面に配置される（法線は Y+ 方向）\n• 分割数が多いほど頂点ごとのライティング精度が上がる\n• テレインの代わりに簡易な平面が必要な場合に使用'
],

'MeshData-CreateCone': [
  'static MeshData CreateCone(float radius, float height, int slices)',
  'コーンメッシュを生成する。CreateCylinder の topRadius=0 のショートカット。矢印の先端やエフェクト表示などに使用する。',
  '// 半径0.5、高さ1.5のコーン\nGX::MeshData cone = GX::MeshGenerator::CreateCone(0.5f, 1.5f, 24);',
  '• 内部的には CreateCylinder(0, radius, height, slices) と同等\n• 底面キャップあり\n• スライス数 16-32 が推奨'
],

'MeshData-CreateTorus': [
  'static MeshData CreateTorus(float majorR, float minorR, int majorSegs, int minorSegs)',
  'トーラス（ドーナツ形状）メッシュを生成する。majorR がリングの中心からチューブ中心までの距離、minorR がチューブの半径。指輪、タイヤ、ポータルなどの表現に使用する。',
  '// トーラスの生成\nGX::MeshData torus = GX::MeshGenerator::CreateTorus(2.0f, 0.5f, 32, 16);',
  '• majorSegs はリング方向の分割数、minorSegs はチューブ断面の分割数\n• UV マッピングは (リング方向, チューブ方向) で 0-1\n• 推奨分割数: majorSegs=32, minorSegs=16'
],

// ============================================================
// Mesh (page-Mesh)
// ============================================================

'Mesh-Create': [
  'bool Create(ID3D12Device*, const MeshData&)',
  'MeshData（CPU側頂点+インデックス）から GPU バッファ（VB/IB）を作成する。内部で頂点バッファとインデックスバッファのリソースを D3D12 デフォルトヒープに配置し、アップロードヒープ経由でデータを転送する。',
  '// MeshData から GPU メッシュを作成\nGX::Mesh mesh;\nGX::MeshData data = GX::MeshGenerator::CreateBox(1, 1, 1);\nmesh.Create(device, data);',
  '• 内部で CreateVertexBuffer と CreateIndexBuffer を両方呼び出す\n• SubMesh 情報も自動設定される\n• 作成後は MeshData を破棄してよい（GPUにコピー済み）'
],

'Mesh-Draw': [
  'void Draw(ID3D12GraphicsCommandList*) const',
  'メッシュの IA（Input Assembler）ステージ設定と DrawIndexedInstanced コールを実行する。頂点バッファ・インデックスバッファのバインドとドローコールを一括で行う。通常は Renderer3D::DrawMesh() 経由で呼ばれる。',
  '// 低レベルでの描画（通常は Renderer3D 経由）\nmesh.Draw(cmdList);',
  '• Renderer3D::DrawMesh() が内部で呼ぶため、直接呼ぶ必要は少ない\n• IASetVertexBuffers + IASetIndexBuffer + DrawIndexedInstanced を実行\n• プリミティブトポロジーは D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST'
],

'Mesh-GetIndexCount': [
  'uint32_t GetIndexCount() const',
  'メッシュ全体のインデックス数を取得する。ドローコール数の見積もりやデバッグ情報表示に使用する。',
  '// ポリゴン数の表示\nuint32_t triCount = mesh.GetIndexCount() / 3;\nGX::Logger::Info("Triangles: %u", triCount);',
  '• 三角形数 = GetIndexCount() / 3\n• SubMesh ごとのインデックス数は SubMesh::indexCount で取得'
],

'Mesh-GetVertexCount': [
  'uint32_t GetVertexCount() const',
  'メッシュ全体の頂点数を取得する。メモリ使用量の把握やデバッグに使用する。',
  '// 頂点数の確認\nuint32_t verts = mesh.GetVertexCount();\nGX::Logger::Info("Vertices: %u", verts);',
  '• GPU メモリ使用量 = GetVertexCount() * sizeof(Vertex3D_PBR)\n• インデックスバッファにより実際の描画頂点数は異なる場合がある'
],

// ============================================================
// Vertex3D (page-Vertex3D)
// ============================================================

'Vertex3D-position': [
  'XMFLOAT3 position',
  '頂点のワールド空間での3D座標（x, y, z）。メッシュの形状を定義する基本要素。頂点シェーダーでワールド行列と乗算されて最終位置が決まる。',
  '// 頂点データの構築\nGX::Vertex3D_PBR v;\nv.position = { 1.0f, 2.0f, 3.0f };',
  '• InputLayout オフセット: 0バイト目\n• DXGI_FORMAT_R32G32B32_FLOAT (12バイト)\n• ローカル空間の座標を格納する'
],

'Vertex3D-normal': [
  'XMFLOAT3 normal',
  '頂点の法線ベクトル。ライティング計算（拡散反射・鏡面反射）に使用される。正規化されたベクトルでなければならない。MeshGenerator が自動で正しい法線を設定する。',
  '// 上向き法線\nv.normal = { 0.0f, 1.0f, 0.0f };',
  '• InputLayout オフセット: 12バイト目\n• 正規化が必要（長さ1.0）\n• 非均一スケール使用時は WorldInverseTranspose で変換される'
],

'Vertex3D-texCoord': [
  'XMFLOAT2 texCoord',
  'テクスチャUV座標。テクスチャマッピングに使用される。(0,0) が左上、(1,1) が右下。PBRマテリアルのアルベド、ノーマルマップ等すべてのテクスチャで共通のUV座標が使われる。',
  '// UV座標の設定\nv.texcoord = { 0.5f, 0.5f }; // テクスチャ中央',
  '• InputLayout オフセット: 24バイト目\n• DXGI_FORMAT_R32G32_FLOAT (8バイト)\n• 0-1 範囲外の値はサンプラーのラップモードに依存する'
],

'Vertex3D-tangent': [
  'XMFLOAT4 tangent',
  'タンジェントベクトル（接線ベクトル）。ノーマルマッピングで接空間（TBN行列）の構築に必要。w成分はバイタンジェントの符号（+1 or -1）を格納する。',
  '// タンジェント設定（w=バイタンジェント符号）\nv.tangent = { 1.0f, 0.0f, 0.0f, 1.0f };',
  '• InputLayout オフセット: 32バイト目\n• DXGI_FORMAT_R32G32B32A32_FLOAT (16バイト)\n• w = bitangent符号（cross(normal, tangent) * w = bitangent）\n• MeshGenerator が自動計算する'
],

'Vertex3D-boneWeights': [
  'XMFLOAT4 boneWeights',
  'スケルタルアニメーション用のボーンウェイト。最大4ボーンの影響度を格納し、合計が1.0になるよう正規化される。Vertex3D_Skinned 構造体のメンバー。',
  '// 2ボーンの影響\nGX::Vertex3D_Skinned sv;\nsv.weights = { 0.7f, 0.3f, 0.0f, 0.0f };',
  '• Vertex3D_Skinned 構造体でのみ使用（Vertex3D_PBR には含まれない）\n• InputLayout オフセット: 64バイト目\n• 4つのウェイトの合計は 1.0 であること\n• ウェイトが 0 のボーンは無視される'
],

'Vertex3D-boneIndices': [
  'uint32_t boneIndices[4]',
  'スケルタルアニメーション用のボーンインデックス。対応するボーンウェイトと組み合わせて、どのボーンからどの程度影響を受けるかを定義する。Vertex3D_Skinned の joints メンバーに対応。',
  '// 2ボーンのインデックス\nsv.joints = { 0, 3, 0, 0 }; // ボーン0と3',
  '• Vertex3D_Skinned 構造体でのみ使用\n• InputLayout オフセット: 48バイト目\n• DXGI_FORMAT_R32G32B32A32_UINT\n• k_MaxBones=128 の範囲内のインデックスを指定する'
],

// ============================================================
// Material / MaterialManager (page-Material)
// ============================================================

'Material-albedo': [
  'XMFLOAT4 albedo',
  'ベースカラー（アルベド）。RGB で基本色、A でアルファを指定する。テクスチャが設定されている場合はテクスチャ色と乗算される。PBR パイプラインの基本色パラメータ。',
  '// 赤いマテリアル\nGX::Material mat;\nmat.constants.albedoFactor = { 1.0f, 0.2f, 0.2f, 1.0f };',
  '• デフォルトは白 (1,1,1,1)\n• アルベドテクスチャがある場合は albedoFactor * textureColor\n• A=0 で完全透明（アルファブレンド有効時）'
],

'Material-metallic': [
  'float metallic',
  'メタリック度（0-1）。0で非金属（ディエレクトリック）、1で完全な金属。PBR のフレネル計算に大きく影響し、金属は反射色がアルベドに依存する。',
  '// 金属マテリアル\nmat.constants.metallicFactor = 1.0f;\n// 非金属マテリアル\nmat.constants.metallicFactor = 0.0f;',
  '• 0.0 = 非金属（プラスチック、木、布など）\n• 1.0 = 金属（金、銀、鉄など）\n• 中間値は通常使わない（物理的にはどちらか）\n• metRoughMapHandle が有効な場合はテクスチャの B チャンネルが使用される'
],

'Material-roughness': [
  'float roughness',
  'ラフネス（表面粗さ、0-1）。0で完全な鏡面反射、1で完全な拡散反射。スペキュラーハイライトのシャープさを制御する最も視覚的な影響が大きいパラメータ。',
  '// つるつるの金属\nmat.constants.roughnessFactor = 0.1f;\n// ざらざらの石\nmat.constants.roughnessFactor = 0.8f;',
  '• 0.0 = 完全鏡面（ミラー）\n• 1.0 = 完全拡散\n• metRoughMapHandle が有効な場合はテクスチャの G チャンネルが使用される\n• デフォルトは 0.5（中程度の粗さ）'
],

'Material-ao': [
  'float ao',
  'アンビエントオクルージョン強度。環境光の遮蔽度を制御する。AO マップが設定されている場合はこの値がテクスチャ値と乗算される。',
  '// AO 強度を最大に\nmat.constants.aoStrength = 1.0f;',
  '• デフォルトは 1.0（フル強度）\n• aoMapHandle が有効な場合はテクスチャ値 * aoStrength\n• SSAO ポストエフェクトとは別の、マテリアル単位の AO'
],

'Material-albedoTexture': [
  'int albedoTexture',
  'アルベドテクスチャのハンドル（TextureManager で管理）。-1 の場合はテクスチャ無しで albedoFactor の色がそのまま使用される。',
  '// テクスチャを設定\nint texHandle = texManager.LoadTexture(L"Assets/brick.png", device);\nmat.albedoMapHandle = texHandle;\nmat.constants.flags |= GX::MaterialFlags::HasAlbedoMap;',
  '• TextureManager::LoadTexture() で取得したハンドルを設定\n• -1 = テクスチャ無し\n• flags の HasAlbedoMap ビットも合わせて設定する必要がある'
],

'Material-normalTexture': [
  'int normalTexture',
  '法線マップのテクスチャハンドル。タンジェント空間の法線情報で表面の凹凸を表現する。-1 で無効。',
  '// 法線マップを設定\nmat.normalMapHandle = texManager.LoadTexture(L"Assets/brick_normal.png", device);\nmat.constants.flags |= GX::MaterialFlags::HasNormalMap;',
  '• 法線マップは RGB = XYZ のタンジェント空間法線\n• flags の HasNormalMap ビットも設定する\n• 頂点にタンジェント情報が必要（Vertex3D_PBR）'
],

'Material-metallicRoughnessTexture': [
  'int metallicRoughnessTexture',
  'メタリック/ラフネスの合成テクスチャハンドル。glTF 2.0 規格に準拠し、G チャンネルにラフネス、B チャンネルにメタリックが格納される。',
  '// メタリック/ラフネスマップを設定\nmat.metRoughMapHandle = texManager.LoadTexture(L"Assets/metal_rough.png", device);\nmat.constants.flags |= GX::MaterialFlags::HasMetRoughMap;',
  '• G = roughness, B = metallic（glTF 2.0 規格）\n• R チャンネルは未使用\n• テクスチャ値と constants の factor が乗算される'
],

'Material-CreateMaterial': [
  'int CreateMaterial(const Material&)',
  'マテリアルを MaterialManager に登録してハンドル（整数ID）を返す。登録したマテリアルはハンドル経由で参照・変更・解放できる。モデルローダーが内部で使用するほか、手動でマテリアルを管理する場合にも使用する。',
  '// マテリアル登録\nGX::Material mat;\nmat.constants.albedoFactor = { 0.8f, 0.2f, 0.1f, 1.0f };\nmat.constants.roughnessFactor = 0.3f;\nint handle = matManager.CreateMaterial(mat);',
  '• 最大 k_MaxMaterials=256 個まで登録可能\n• freelist による再利用で効率的なハンドル管理\n• ModelLoader が glTF 読み込み時に自動で CreateMaterial する'
],

'Material-GetMaterial': [
  'Material& GetMaterial(int handle)',
  'ハンドルからマテリアルの参照を取得する。取得した参照からパラメータをランタイムで変更できる。不正なハンドルの場合はデフォルトマテリアルを返す。',
  '// マテリアルパラメータの動的変更\nGX::Material* mat = matManager.GetMaterial(handle);\nif (mat) {\n    mat->constants.roughnessFactor = 0.5f;\n}',
  '• 不正なハンドルの場合は nullptr を返す\n• ポインタ版 (Material*) で返される\n• 返されたポインタ経由でリアルタイムにパラメータを変更可能'
],

'Material-ReleaseMaterial': [
  'void ReleaseMaterial(int handle)',
  'マテリアルを解放してハンドルを freelist に返す。モデルの破棄時やシーン切り替え時に不要になったマテリアルを解放する。',
  '// マテリアルの解放\nmatManager.ReleaseMaterial(handle);\nhandle = -1; // 無効化',
  '• 解放後のハンドルは次の CreateMaterial で再利用される\n• 解放済みハンドルを使ってアクセスすると不正な結果になる\n• テクスチャの解放は TextureManager で別途行う'
],

// ============================================================
// Model (page-Model)
// ============================================================

'Model-GetMeshes': [
  'const std::vector<Mesh>& GetMeshes() const',
  'モデルが保持する全メッシュの配列を取得する。glTF モデルは複数メッシュ（SubMesh）を持つことがあり、それぞれ異なるマテリアルが割り当てられる。Renderer3D::DrawModel() が内部で使用する。',
  '// メッシュ数の確認\nconst auto& meshes = model->GetMesh();\nauto& subMeshes = meshes.GetSubMeshes();\nGX::Logger::Info("SubMeshes: %zu", subMeshes.size());',
  '• Mesh クラスは内部に複数の SubMesh を持つ\n• SubMesh ごとに異なる materialHandle が設定される\n• 通常は Renderer3D::DrawModel() 経由で描画する'
],

'Model-GetMaterials': [
  'const std::vector<Material>& GetMaterials() const',
  'モデルに紐づくマテリアルハンドルの配列を取得する。各ハンドルは MaterialManager に登録されたマテリアルを参照する。SubMesh の materialHandle と対応する。',
  '// マテリアル一覧\nconst auto& handles = model->GetMaterialHandles();\nfor (int h : handles) {\n    auto* mat = matManager.GetMaterial(h);\n}',
  '• MaterialManager のハンドル (int) の配列\n• SubMesh::materialHandle がこの配列のインデックスに対応\n• ModelLoader が自動でマテリアルを登録する'
],

'Model-GetSkeleton': [
  'Skeleton* GetSkeleton()',
  'モデルのスケルトン（ボーン階層）を取得する。スケルトンがないモデルの場合は nullptr を返す。スケルタルアニメーション再生前に AnimationPlayer::SetSkeleton() に渡す。',
  '// スケルトンの取得\nGX::Skeleton* skel = model->GetSkeleton();\nif (skel) {\n    animPlayer.SetSkeleton(skel);\n    GX::Logger::Info("Bones: %u", skel->GetJointCount());\n}',
  '• スケルトンがない静的モデルでは nullptr を返す\n• HasSkeleton() で事前チェック可能\n• unique_ptr で所有されるため、Model の生存期間中のみ有効'
],

'Model-HasSkeleton': [
  'bool HasSkeleton() const',
  'モデルがスケルトン（ボーン階層データ）を持っているかどうかを返す。スケルタルアニメーション対応モデルかの判定に使用する。',
  '// スケルタルモデルかどうかで描画分岐\nif (model->HasSkeleton()) {\n    renderer3D.DrawSkinnedModel(*model, transform, animPlayer);\n} else {\n    renderer3D.DrawModel(*model, transform);\n}',
  '• true = Skeleton データあり（DrawSkinnedModel で描画可能）\n• false = 静的メッシュ（DrawModel で描画）\n• glTF にスキン情報がある場合に true になる'
],

// ============================================================
// ModelLoader (page-ModelLoader)
// ============================================================

'ModelLoader-LoadFromFile': [
  'std::unique_ptr<Model> LoadFromFile(const std::string& path, ID3D12Device*, TextureManager&)',
  'glTF 2.0 ファイル (.gltf/.glb) を読み込んで Model オブジェクトを生成する。メッシュ、マテリアル、テクスチャ、スケルトン、アニメーションを一括でロードする。テクスチャは自動的に TextureManager に登録される。',
  '// glTF モデルのロード\nGX::ModelLoader loader;\nauto model = loader.LoadFromFile(\n    L"Assets/Models/character.glb",\n    device,\n    renderer3D.GetTextureManager(),\n    renderer3D.GetMaterialManager());\nif (!model) {\n    GX::Logger::Error("Model load failed");\n}',
  '• 対応フォーマット: .gltf (テキスト JSON) と .glb (バイナリ)\n• cgltf ライブラリで解析\n• テクスチャは相対パスから自動ロード → TextureManager に登録\n• マテリアルは自動で MaterialManager に CreateMaterial される\n• 失敗時は nullptr を返す'
],

// ============================================================
// Light (page-Light)
// ============================================================

'Light-DirectionalLight': [
  'struct DirectionalLight { XMFLOAT3 direction, color; float intensity; }',
  'ディレクショナルライト（平行光源）の定義構造体。太陽光のような無限遠の光源を表す。direction は光の向き、color は光の色、intensity は強度を指定する。CSM シャドウの方向計算にも使用される。',
  '// 太陽光の定義\nauto sunLight = GX::Light::CreateDirectional(\n    { 0.3f, -1.0f, 0.5f },   // direction\n    { 1.0f, 0.95f, 0.9f },   // color\n    2.0f);                     // intensity',
  '• direction は正規化されていなくても内部で正規化される\n• intensity は HDR 値（1.0 以上可）\n• シーンに 1 つのみ設定可能\n• CSM の lightDirection として使用される'
],

'Light-PointLight': [
  'struct PointLight { XMFLOAT3 position, color; float intensity, radius; bool castShadow; }',
  'ポイントライト（点光源）の定義構造体。電球やトーチなどの全方位に光を放つ光源を表す。radius は光の到達範囲で、距離に応じた減衰計算に使用される。',
  '// 燭台のポイントライト\nauto torch = GX::Light::CreatePoint(\n    { 3.0f, 2.5f, 0.0f },    // position\n    10.0f,                     // range\n    { 1.0f, 0.7f, 0.3f },    // color (暖色)\n    3.0f);                     // intensity',
  '• 最大8個まで AddPointLight 可能\n• castShadow=true で PointShadowMap (6面) を使用\n• radius 外のフラグメントにはライトが影響しない\n• 減衰は物理ベースの距離二乗逆比例'
],

'Light-SpotLight': [
  'struct SpotLight { XMFLOAT3 position, direction, color; float intensity, range, innerAngle, outerAngle; bool castShadow; }',
  'スポットライト（集光灯）の定義構造体。懐中電灯やステージライトなど、円錐状の光を放つ光源を表す。innerAngle〜outerAngle の間でフォールオフが発生する。',
  '// 懐中電灯\nauto spot = GX::Light::CreateSpot(\n    { 0.0f, 5.0f, -3.0f },   // position\n    { 0.0f, -1.0f, 0.0f },   // direction (下向き)\n    20.0f,                     // range\n    30.0f,                     // spotAngle (度)\n    { 1.0f, 1.0f, 1.0f },    // color\n    5.0f);                     // intensity',
  '• 最大4個まで AddSpotLight 可能\n• spotAngle は度数で指定（内部で cos に変換）\n• innerAngle と outerAngle の差でエッジのソフトさが変わる\n• castShadow=true で 2048x2048 のシャドウマップ使用（SRV t12）'
],

// ============================================================
// Skeleton (page-Skeleton)
// ============================================================

'Skeleton-Bone': [
  'struct Bone { std::string name; int parentIndex; XMMATRIX inverseBindMatrix; }',
  'ボーン（ジョイント）情報の構造体。ボーン名、親ボーンインデックス、逆バインド行列を保持する。parentIndex=-1 はルートボーンを意味する。glTF のスキンデータから自動生成される。',
  '// ジョイント情報の参照\nconst auto& joints = skeleton->GetJoints();\nfor (const auto& j : joints) {\n    GX::Logger::Info("Joint: %s (parent=%d)", j.name.c_str(), j.parentIndex);\n}',
  '• parentIndex=-1 がルートボーン\n• inverseBindMatrix はバインドポーズの逆行列\n• ボーン行列 = globalTransform * inverseBindMatrix\n• k_MaxBones=128 が上限'
],

'Skeleton-GetBoneCount': [
  'int GetBoneCount() const',
  'スケルトンが保持するボーン（ジョイント）の総数を取得する。AnimationClip の Sample 呼び出し時にジョイント数を渡す必要がある。',
  '// ボーン数の確認\nuint32_t boneCount = skeleton->GetJointCount();\nGX::Logger::Info("Total bones: %u", boneCount);',
  '• k_MaxBones=128 を超えるスケルトンは未対応\n• GetJointCount() メソッドで取得\n• アニメーション計算のバッファ確保に使用'
],

'Skeleton-FindBone': [
  'int FindBone(const std::string& name) const',
  'ボーン名からインデックスを検索する。ボーンのアタッチメント（武器装着、エフェクト配置）やIK制御で特定のボーンを参照する際に使用する。見つからない場合は -1 を返す。',
  '// 右手ボーンのインデックスを取得\nint rightHand = skeleton->FindJointIndex("RightHand");\nif (rightHand >= 0) {\n    // 武器をアタッチ\n}',
  '• 見つからない場合は -1 を返す\n• ボーン名は glTF のノード名に依存\n• 線形探索なので頻繁に呼ぶ場合はキャッシュを推奨'
],

'Skeleton-GetBone': [
  'const Bone& GetBone(int index) const',
  '指定インデックスのボーン情報を取得する。ボーンの階層構造の走査やデバッグ表示に使用する。',
  '// ルートボーンからの階層表示\nconst auto& joints = skeleton->GetJoints();\nfor (uint32_t i = 0; i < skeleton->GetJointCount(); i++) {\n    const auto& j = joints[i];\n    GX::Logger::Info("[%d] %s parent=%d", i, j.name.c_str(), j.parentIndex);\n}',
  '• GetJoints() で全ジョイント配列を取得可能\n• インデックスの範囲チェックは呼び出し側で行う\n• Joint 構造体には name, parentIndex, inverseBindMatrix, localTransform が含まれる'
],

'Skeleton-GetFinalTransforms': [
  'const std::vector<XMMATRIX>& GetFinalTransforms() const',
  '最終的なボーン変換行列の配列を取得する。ComputeGlobalTransforms → ComputeBoneMatrices の結果が格納されており、スキニングシェーダーに転送される。AnimationPlayer が内部で計算・設定する。',
  '// ボーン行列を定数バッファに転送\nconst auto& boneConstants = animPlayer.GetBoneConstants();\n// boneConstants.boneMatrices[i] が各ボーンの最終行列',
  '• AnimationPlayer::GetBoneConstants() 経由で取得するのが一般的\n• BoneConstants::boneMatrices[k_MaxBones] に格納\n• globalTransform * inverseBindMatrix の結果\n• 頂点シェーダーの b4 スロットにバインドされる'
],

// ============================================================
// AnimationClip (page-AnimationClip)
// ============================================================

'AnimationClip-KeyFrame': [
  'struct KeyFrame { float time; XMFLOAT3 translation, scale; XMFLOAT4 rotation; }',
  'キーフレームデータの構造体。タイムスタンプ（秒）と、そのタイミングでの位置・回転（クォータニオン）・スケール値を保持する。補間タイプ（Linear/Step/CubicSpline）に従って中間値が計算される。',
  '// キーフレームの参照\nconst auto& channels = clip.GetChannels();\nfor (const auto& ch : channels) {\n    for (const auto& key : ch.translationKeys) {\n        float t = key.time;\n        XMFLOAT3 pos = key.value;\n    }\n}',
  '• AnimationChannel ごとにジョイント1つ分の TRS キーフレーム配列を持つ\n• rotation はクォータニオン（XMFLOAT4）で格納\n• 補間: Linear（線形）、Step（不連続）、CubicSpline（スプライン）\n• glTF から自動ロードされる'
],

'AnimationClip-GetDuration': [
  'float GetDuration() const',
  'アニメーションクリップの全体の長さを秒単位で取得する。ループ再生時の折り返し判定や、UI のプログレスバー表示に使用する。',
  '// アニメーション情報の表示\nfloat dur = clip.GetDuration();\nGX::Logger::Info("Animation: %s (%.2f sec)", clip.GetName().c_str(), dur);',
  '• 秒単位の float 値\n• キーフレームの最大タイムスタンプがデュレーションとなる\n• 0.0 の場合はアニメーションデータが空'
],

'AnimationClip-GetName': [
  'const std::string& GetName() const',
  'アニメーションクリップの名前を取得する。glTF 内のアニメーション名がそのまま設定される。Model::FindAnimationIndex() での検索キーとして使用する。',
  '// 名前でアニメーションを検索\nint idx = model->FindAnimationIndex("Walk");\nif (idx >= 0) {\n    animPlayer.Play(&model->GetAnimations()[idx]);\n}',
  '• glTF のアニメーション名がそのまま格納される\n• 名前が設定されていない場合は空文字列\n• Model::FindAnimationIndex() で名前から検索可能'
],

// ============================================================
// AnimationPlayer (page-AnimationPlayer)
// ============================================================

'AnimationPlayer-Play': [
  'void Play(const AnimationClip* clip, bool loop = true)',
  'アニメーションクリップの再生を開始する。loop=true でループ再生、false で 1 回再生後に停止する。再生中に別のクリップを Play() するとそのクリップに切り替わる。',
  '// 歩行アニメーションをループ再生\nconst auto& anims = model->GetAnimations();\nif (!anims.empty()) {\n    animPlayer.Play(&anims[0], true);\n}',
  '• クリップのポインタを保持するため、AnimationClip は再生中ずっと有効であること\n• 別のクリップを Play() すると即座に切り替わる（ブレンドなし）\n• loop=false の場合、最後のフレームで停止する'
],

'AnimationPlayer-Stop': [
  'void Stop()',
  'アニメーション再生を停止する。再生位置はリセットされ、ボーン行列はバインドポーズに戻る。',
  '// アニメーション停止\nanimPlayer.Stop();',
  '• IsPlaying() が false になる\n• Pause() と異なり再生位置がリセットされる\n• 停止後に Play() で再開すると最初から再生される'
],

'AnimationPlayer-Update': [
  'void Update(float deltaTime, Skeleton&)',
  '毎フレーム呼び出してアニメーションを更新する。deltaTime 分だけ時間を進め、現在時刻のボーン行列を再計算する。メインループの Update フェーズで呼び出す。',
  '// 毎フレームの更新\nfloat dt = timer.GetDeltaTime();\nanimPlayer.Update(dt);',
  '• deltaTime は秒単位\n• 内部で AnimationClip::Sample() → Skeleton::ComputeGlobalTransforms() → ComputeBoneMatrices() を実行\n• 再生速度は SetSpeed() で変更可能\n• Pause 中は時間が進まない'
],

'AnimationPlayer-SetSpeed': [
  'void SetSpeed(float speed)',
  'アニメーションの再生速度を設定する。1.0 が通常速度、2.0 で倍速、0.5 で半速。負の値は非対応。スローモーション演出やスピードアップに使用する。',
  '// 倍速再生\nanimPlayer.SetSpeed(2.0f);\n// スローモーション\nanimPlayer.SetSpeed(0.3f);',
  '• デフォルトは 1.0\n• 0.0 で実質 Pause と同等\n• 負の値（逆再生）は動作未保証\n• GetSpeed() で現在値を取得可能'
],

'AnimationPlayer-IsPlaying': [
  'bool IsPlaying() const',
  'アニメーションが再生中かどうかを返す。非ループアニメーションの終了判定や、UI でのステータス表示に使用する。',
  '// 再生終了の検出\nif (!animPlayer.IsPlaying()) {\n    // 次のアニメーションへ遷移\n    animPlayer.Play(&idleClip, true);\n}',
  '• Play() 後は true、Stop() 後は false\n• 非ループ再生で最後まで到達すると自動的に false になる\n• Pause() 中は true のまま（再生中だが一時停止）'
],

'AnimationPlayer-GetCurrentTime': [
  'float GetCurrentTime() const',
  '現在のアニメーション再生位置を秒単位で取得する。プログレスバーの表示やイベントトリガーのタイミング判定に使用する。',
  '// 再生進捗の表示\nfloat t = animPlayer.GetCurrentTime();\nfloat dur = clip->GetDuration();\nfloat progress = t / dur; // 0.0〜1.0',
  '• 秒単位の float 値\n• ループ再生時はデュレーションで折り返される\n• Stop() 後は 0.0 にリセットされる'
],

// ============================================================
// Skybox (page-Skybox)
// ============================================================

'Skybox-Initialize': [
  'bool Initialize(ID3D12Device*, DescriptorHeap* srvHeap)',
  'スカイボックスのパイプライン（ルートシグネチャ、PSO、キューブメッシュ VB/IB、定数バッファ）を初期化する。Renderer3D の内部で自動的に呼ばれるため、通常は直接呼び出す必要はない。',
  '// Renderer3D 内部で自動初期化される\n// 直接使用する場合:\nGX::Skybox skybox;\nskybox.Initialize(device);',
  '• Renderer3D::Initialize() 内で自動的に呼ばれる\n• 深度テスト有効・深度書き込み無効で描画される\n• HDR フォーマット (R16G16B16A16_FLOAT) の PSO を使用'
],

'Skybox-LoadCubemap': [
  'bool LoadCubemap(const std::string paths[6], ID3D12Device*, ID3D12CommandQueue*)',
  '6面のキューブマップテクスチャを読み込む。+X, -X, +Y, -Y, +Z, -Z の順序で6枚の画像パスを指定する。HDR 環境マップにも対応し、IBL 照明に使用できる。',
  '// キューブマップの読み込み\nstd::string faces[6] = {\n    "Assets/skybox/right.png", "Assets/skybox/left.png",\n    "Assets/skybox/top.png",   "Assets/skybox/bottom.png",\n    "Assets/skybox/front.png", "Assets/skybox/back.png"\n};\nskybox.LoadCubemap(faces, device, cmdQueue);',
  '• 現在の実装はプロシージャルスカイ（グラデーション+太陽）のみ\n• SetColors() と SetSun() でプロシージャルパラメータを設定する\n• キューブマップテクスチャ対応は将来拡張予定'
],

'Skybox-Draw': [
  'void Draw(ID3D12GraphicsCommandList*, uint32_t frameIndex, const Camera3D&)',
  'スカイボックスを描画する。カメラの VP 行列からカメラ位置を除いた回転のみの行列でキューブを描画し、常にカメラの周囲に表示されるようにする。Renderer3D::End() 内で自動的に呼ばれる。',
  '// 通常は Renderer3D が自動で描画する\n// 直接呼ぶ場合:\nXMFLOAT4X4 vp;\nXMStoreFloat4x4(&vp, camera.GetViewProjectionMatrix());\nskybox.Draw(cmdList, frameIndex, vp);',
  '• 深度テスト有効・深度書き込み無効（最遠に描画される）\n• ビュー行列の平行移動成分を除去して描画\n• プロシージャルスカイはグラデーション + 太陽ディスクで表現される\n• SetColors(top, bottom) と SetSun(direction, intensity) で外観を調整'
],

// ============================================================
// PrimitiveBatch3D (page-PrimitiveBatch3D)
// ============================================================

'PrimitiveBatch3D-Initialize': [
  'bool Initialize(ID3D12Device*)',
  '3Dプリミティブバッチを初期化する。ライン描画用のパイプライン（PSO/ルートシグネチャ）と動的頂点バッファを作成する。Renderer3D 内部で自動的に呼ばれる。',
  '// Renderer3D 経由で取得して使用\nauto& primBatch = renderer3D.GetPrimitiveBatch3D();',
  '• k_MaxVertices=65536 個の頂点バッファが確保される\n• D3D_PRIMITIVE_TOPOLOGY_LINELIST で描画\n• Renderer3D::Initialize() 内で自動初期化'
],

'PrimitiveBatch3D-Begin': [
  'void Begin(ID3D12GraphicsCommandList*, uint32_t frameIndex, const Camera3D&)',
  'プリミティブバッチの描画を開始する。PSO のバインドと VP 行列の定数バッファ更新を行う。End() との間に DrawLine/DrawBox 等の描画コマンドを発行する。',
  '// プリミティブ描画開始\nauto& pb = renderer3D.GetPrimitiveBatch3D();\npb.Begin(cmdList, frameIndex, vp);\npb.DrawGrid(50.0f, 50, { 0.3f, 0.3f, 0.3f, 1.0f });\npb.End();',
  '• Begin/End ペアの間に描画コマンドを発行する\n• VP 行列は XMFLOAT4X4 で渡す\n• Renderer3D のメインパス内で使用可能'
],

'PrimitiveBatch3D-End': [
  'void End()',
  'バッチに蓄積された頂点データを一括描画する。DynamicBuffer の頂点を GPU に転送し、DrawInstanced でドローコールを発行する。',
  '// バッチ終了（ドローコール実行）\npb.End();',
  '• 内部で蓄積された全ラインを 1 回のドローコールで描画\n• End() 後に頂点カウンタはリセットされる\n• Renderer3D::End() 内で自動的に呼ばれる'
],

'PrimitiveBatch3D-DrawLine': [
  'void DrawLine(const XMFLOAT3& a, const XMFLOAT3& b, uint32_t color)',
  '3D空間に線分を描画する。始点 a から終点 b まで指定色のラインを引く。デバッグ用のレイ表示、オブジェクト間の接続線、パス可視化などに使用する。',
  '// レイの可視化\nXMFLOAT3 start = { 0, 1, 0 };\nXMFLOAT3 end = { 10, 1, 5 };\npb.DrawLine(start, end, { 1.0f, 0.0f, 0.0f, 1.0f });',
  '• color は XMFLOAT4 (R,G,B,A) で指定\n• k_MaxVertices を超えると描画されない\n• ライン幅は 1px 固定（D3D12 の制限）'
],

'PrimitiveBatch3D-DrawBox': [
  'void DrawBox(const XMFLOAT3& center, const XMFLOAT3& size, uint32_t color)',
  'ワイヤーフレームのボックスを描画する。バウンディングボックスの可視化やコリジョン範囲の表示に使用する。12本のライン（辺）で構成される。',
  '// バウンディングボックスの表示\npb.DrawWireBox({ 0, 1, 0 }, { 2, 2, 2 }, { 0, 1, 0, 1 });',
  '• center は中心座標、extents は半径（半分のサイズ）\n• 12ライン = 24頂点を消費する\n• 回転なし（AABB のみ対応）'
],

'PrimitiveBatch3D-DrawSphere': [
  'void DrawSphere(const XMFLOAT3& center, float radius, uint32_t color)',
  'ワイヤーフレームの球体を描画する。3つの円（XY, YZ, XZ 平面）で球を表現する。コリジョン範囲やポイントライトの影響半径の可視化に使用する。',
  '// ポイントライトの範囲表示\npb.DrawWireSphere(lightPos, lightRadius, { 1, 1, 0, 1 }, 24);',
  '• segments パラメータで円の分割数を指定（デフォルト16）\n• 3つの大円で描画（segments*2 * 3 = 96頂点、デフォルト）\n• 精度が必要な場合は segments を増やす'
],

'PrimitiveBatch3D-DrawGrid': [
  'void DrawGrid(float size, int divisions, uint32_t color)',
  'XZ平面にグリッドを描画する。エディタのフロアグリッドやデバッグ用の位置ガイドとして使用する。size はグリッド全体の半径、divisions は分割数。',
  '// 50x50 のグリッドを描画\npb.DrawGrid(50.0f, 50, { 0.3f, 0.3f, 0.3f, 0.5f });',
  '• Y=0 の XZ 平面に描画される\n• (divisions+1) * 2 本のラインが描画される\n• 半透明色を指定すると背景に溶け込む表示になる'
],

'PrimitiveBatch3D-DrawAxis': [
  'void DrawAxis(const XMFLOAT3& origin, float length)',
  '原点から XYZ 軸を描画する。X=赤、Y=緑、Z=青の色分けで方向を示す。シーンの方向確認やトランスフォームの可視化に便利。',
  '// 原点にXYZ軸を表示\npb.DrawAxis({ 0, 0, 0 }, 5.0f);',
  '• X軸 = 赤 (1,0,0)、Y軸 = 緑 (0,1,0)、Z軸 = 青 (0,0,1)\n• length が各軸の長さ\n• 6頂点（3ライン）を消費する'
],

'PrimitiveBatch3D-DrawFrustum': [
  'void DrawFrustum(const XMMATRIX& viewProj, uint32_t color)',
  'ビュー・プロジェクション行列から視錐台（フラスタム）をワイヤーフレームで描画する。カメラの可視範囲やシャドウカスケードの確認に使用する。',
  '// カメラのフラスタムを可視化\nXMMATRIX vp = camera.GetViewProjectionMatrix();\npb.DrawFrustum(vp, { 1, 1, 0, 1 });',
  '• VP 行列の逆行列から 8 つの角点を計算して 12 辺を描画\n• CSM のカスケード分割の確認に便利\n• 24頂点（12ライン）を消費する'
],

// ============================================================
// Terrain (page-Terrain)
// ============================================================

'Terrain-LoadFromHeightmap': [
  'bool LoadFromHeightmap(const std::string& path, float width, float depth, float maxHeight, ID3D12Device*)',
  'ハイトマップ画像から地形メッシュを生成する。グレースケール画像の輝度値を高さに変換し、法線・UV・タンジェントを自動計算して GPU バッファを作成する。CreateFromHeightmap() と CreateProcedural() の 2 種類がある。',
  '// ハイトマップからテレイン生成\nGX::Terrain terrain;\nstd::vector<float> heightData(256 * 256);\n// heightData にグレースケール値を設定...\nterrain.CreateFromHeightmap(device, heightData.data(), 256, 256,\n    100.0f, 100.0f, 20.0f);',
  '• heightmapData は 0.0〜1.0 のグレースケール float 配列\n• 0.0 = 最低地点、1.0 = maxHeight の高さ\n• 法線は隣接頂点から自動計算される\n• CreateProcedural() でパーリンノイズ風の地形も生成可能'
],

'Terrain-GetHeight': [
  'float GetHeight(float x, float z) const',
  '指定ワールド座標 (x, z) での地形の高さを取得する。キャラクターの接地判定や、オブジェクトの地面配置に使用する。双線形補間で滑らかな高さ値を返す。',
  '// キャラクターを地面に接地\nfloat y = terrain.GetHeight(playerX, playerZ);\nplayer.SetPosition(playerX, y, playerZ);',
  '• 地形範囲外の座標では 0.0f を返す\n• 内部でグリッドの双線形補間を行う\n• 高さデータは CPU 側にキャッシュされている（GPU アクセス不要）'
],

'Terrain-GetMesh': [
  'const Mesh& GetMesh() const',
  '地形のメッシュデータを取得する。Renderer3D::DrawTerrain() が内部で使用する。頂点バッファとインデックスバッファへのアクセスに使用する。',
  '// テレインの描画（Renderer3D 経由が推奨）\nGX::Transform3D terrainTransform;\nrenderer3D.DrawTerrain(terrain, terrainTransform);',
  '• GetVertexBuffer(), GetIndexBuffer(), GetIndexCount() で低レベルアクセス可能\n• 通常は Renderer3D::DrawTerrain() 経由で描画する\n• マテリアルは Renderer3D::SetMaterial() で別途設定'
],

'Terrain-SetMaterial': [
  'void SetMaterial(const Material&)',
  '地形に適用するマテリアルを設定する。テレインに草地テクスチャや岩のマテリアルを適用する際に使用する。',
  '// テレインにマテリアルを設定\nGX::Material terrainMat;\nterrainMat.albedoMapHandle = texManager.LoadTexture(L"Assets/grass.png", device);\nterrainMat.constants.flags |= GX::MaterialFlags::HasAlbedoMap;\nrenderer3D.SetMaterial(terrainMat);\nrenderer3D.DrawTerrain(terrain, transform);',
  '• Renderer3D::SetMaterial() で描画前にマテリアルを設定する\n• テレイン専用のマテリアルスロットはなく、通常の PBR マテリアルを使用\n• 複数テクスチャのスプラッティングは未対応（単一マテリアル）'
],

// ============================================================
// Fog (page-Fog)
// ============================================================

'Fog-enabled': [
  'bool enabled',
  'フォグの有効/無効フラグ。FogMode で制御され、FogMode::None が無効に相当する。Renderer3D::SetFog() で設定する。',
  '// フォグを有効化\nrenderer3D.SetFog(GX::FogMode::Linear,\n    { 0.6f, 0.65f, 0.7f }, // color\n    50.0f,                  // start\n    200.0f,                 // end\n    0.01f);                 // density',
  '• FogMode::None で無効、Linear/Exp/Exp2 で有効\n• Renderer3D::SetFog() で一括設定\n• デフォルトは無効 (FogMode::None)'
],

'Fog-color': [
  'XMFLOAT3 color',
  'フォグの色。通常は空やスカイボックスの色に合わせて設定する。遠景がこの色にフェードアウトする。',
  '// 朝焼け風のフォグ色\nrenderer3D.SetFog(GX::FogMode::Linear,\n    { 0.9f, 0.7f, 0.5f }, 30.0f, 150.0f);',
  '• RGB (0-1) で指定\n• スカイボックスの色に合わせると自然な見た目になる\n• HDR パイプラインではトーンマッピング前の値を設定する'
],

'Fog-start': [
  'float start, end',
  'Linear フォグの開始距離と終了距離。start 以下の距離ではフォグなし、end 以上の距離で完全にフォグに覆われる。その間は線形補間される。',
  '// 近距離フォグ（室内の煙など）\nrenderer3D.SetFog(GX::FogMode::Linear,\n    { 0.5f, 0.5f, 0.5f }, 5.0f, 30.0f);',
  '• Linear モードでのみ使用される\n• start < end でなければならない\n• カメラからの距離（ビュー空間Z）で計算される'
],

'Fog-density': [
  'float density',
  '指数フォグ (Exp/Exp2) の密度パラメータ。値が大きいほど近距離からフォグがかかる。Exp では e^(-density*d)、Exp2 では e^(-(density*d)^2) で計算される。',
  '// 濃い霧\nrenderer3D.SetFog(GX::FogMode::Exp2,\n    { 0.7f, 0.7f, 0.75f }, 0, 0, 0.03f);',
  '• Exp/Exp2 モードでのみ使用される\n• 推奨範囲: 0.001〜0.1\n• Exp2 は Exp より急激にフォグがかかる'
],

'Fog-mode': [
  'FogMode mode',
  'フォグの計算モード。Linear（線形）、Exponential（指数）、ExponentialSquared（二乗指数）の3種類。それぞれ距離に対するフォグ密度の変化曲線が異なる。',
  '// 各フォグモードの設定\nrenderer3D.SetFog(GX::FogMode::Linear, fogColor, 50.0f, 200.0f);\nrenderer3D.SetFog(GX::FogMode::Exp, fogColor, 0, 0, 0.01f);\nrenderer3D.SetFog(GX::FogMode::Exp2, fogColor, 0, 0, 0.02f);',
  '• FogMode::None = フォグ無効\n• FogMode::Linear = 線形フォグ（start/end で制御）\n• FogMode::Exp = 指数フォグ（density で制御）\n• FogMode::Exp2 = 二乗指数フォグ（density で制御、より急激な減衰）'
],

// ============================================================
// CascadedShadowMap (page-CascadedShadowMap)
// ============================================================

'CascadedShadowMap-Initialize': [
  'bool Initialize(ID3D12Device*, DescriptorHeap* dsvHeap, DescriptorHeap* srvHeap, uint32_t resolution = 2048)',
  'カスケードシャドウマップを初期化する。4つのカスケードそれぞれに resolution x resolution の深度テクスチャを作成し、DSV/SRV ディスクリプタを配置する。Renderer3D 内部で自動的に呼ばれる。',
  '// Renderer3D 経由で CSM にアクセス\nauto& csm = renderer3D.GetCSM();\ncsm.SetCascadeSplits(0.05f, 0.15f, 0.4f, 1.0f);',
  '• デフォルト解像度は k_ShadowMapSize=4096\n• 4カスケード x 4096x4096 = 約256MBの深度バッファ\n• SRV はシェーダーの t8-t11 にバインドされる\n• Renderer3D::Initialize() 内で自動的に初期化される'
],

'CascadedShadowMap-Update': [
  'void Update(const Camera3D&, const XMFLOAT3& lightDir)',
  'カメラとライト方向から各カスケードのライト空間 VP 行列を計算する。カメラのフラスタムを分割し、各カスケードに最適なライトVP行列を生成する。毎フレーム Renderer3D::UpdateShadow() 経由で呼ばれる。',
  '// シャドウの更新（Renderer3D 経由が推奨）\nrenderer3D.UpdateShadow(camera);',
  '• カスケード分割は nearZ〜farZ を cascadeRatios で分割\n• 各カスケードのバウンディング球からライトVP行列を計算\n• シャドウが安定するようスナッピング処理が含まれる\n• ライト方向は正規化されていなくても内部で正規化される'
],

'CascadedShadowMap-BeginShadowPass': [
  'void BeginShadowPass(ID3D12GraphicsCommandList*, int cascade)',
  '指定カスケードのシャドウパスを開始する。深度テクスチャのクリアとビューポート/シザーの設定を行い、シャドウ PSO をバインドする。この後に DrawMesh でシーンの深度を書き込む。',
  '// 4カスケードのシャドウパス\nfor (int i = 0; i < 4; i++) {\n    renderer3D.BeginShadowPass(cmdList, frameIndex, i);\n    // DrawMesh でシーンオブジェクトを描画\n    renderer3D.EndShadowPass(i);\n}',
  '• cascade は 0〜3（k_NumCascades-1）\n• 深度テクスチャを D3D12_RESOURCE_STATE_DEPTH_WRITE にトランジション\n• シャドウパスでは色は書き込まれない（深度のみ）\n• BeginShadowPass / DrawMesh / EndShadowPass の順で呼ぶ'
],

'CascadedShadowMap-EndShadowPass': [
  'void EndShadowPass(ID3D12GraphicsCommandList*, int cascade)',
  '指定カスケードのシャドウパスを終了する。深度テクスチャを PIXEL_SHADER_RESOURCE 状態にトランジションし、メインパスでサンプリング可能にする。',
  '// シャドウパスの終了\nrenderer3D.EndShadowPass(cascadeIndex);',
  '• DEPTH_WRITE → PIXEL_SHADER_RESOURCE のバリアを発行\n• 全4カスケードの EndShadowPass 後にメインパスを開始する\n• バリアの発行順序を守らないと GPU ハザードが発生する'
],

'CascadedShadowMap-BindSRVs': [
  'void BindSRVs(ID3D12GraphicsCommandList*)',
  'CSM の 4 枚の深度テクスチャ SRV をシェーダーにバインドする。メインパスの PBR シェーダーでシャドウサンプリングに使用する。t8-t11 スロットに配置される。',
  '// SRV のバインド（Renderer3D が自動で行う）\ncsm.BindSRVs(cmdList);',
  '• ディスクリプタテーブルで連続4スロット (t8-t11) をバインド\n• Renderer3D::Begin() 内で自動的に呼ばれる\n• PCF (Percentage Closer Filtering) でサンプリングされる'
],

'CascadedShadowMap-GetLightViewProjection': [
  'XMMATRIX GetLightViewProjection(int cascade) const',
  '指定カスケードのライト空間ビュー・プロジェクション行列を取得する。シャドウ計算で頂点をライト空間に変換する際に使用する。FrameConstants にも転送される。',
  '// カスケード0のライトVP行列\nXMFLOAT4X4 lvp;\nauto& constants = csm.GetShadowConstants();\nlvp = constants.lightVP[0];',
  '• GetShadowConstants() で全カスケードの情報を一括取得可能\n• FrameConstants の lightVP[4] にコピーされてシェーダーに転送される\n• カスケード選択はピクセルシェーダーでビュー空間Zに基づいて行われる'
],

// ============================================================
// PointShadowMap (page-PointShadowMap)
// ============================================================

'PointShadowMap-Initialize': [
  'bool Initialize(ID3D12Device*, DescriptorHeap* dsvHeap, DescriptorHeap* srvHeap, uint32_t resolution = 1024)',
  'ポイントシャドウマップを初期化する。Texture2DArray (6スライス) の深度リソースと 6 つの DSV を作成する。Renderer3D 内部で自動的に初期化される。',
  '// Renderer3D 経由でアクセス\nauto& psm = renderer3D.GetPointShadowMap();',
  '• k_Size=1024 のデフォルト解像度\n• Texture2DArray で 6 面を 1 リソースに格納\n• 専用の DSV ヒープ（6スロット）を内部で作成\n• SRV は t13 にバインドされる'
],

'PointShadowMap-BeginShadowPass': [
  'void BeginShadowPass(ID3D12GraphicsCommandList*, int face, const XMFLOAT3& lightPos)',
  '指定面（0-5）のシャドウパスを開始する。ライト位置から各面のビュー行列を計算し、90度 FOV の射影行列と合わせて VP 行列を設定する。ドミナント軸方式で面を選択する。',
  '// 6面のポイントシャドウパス\nfor (uint32_t face = 0; face < 6; face++) {\n    renderer3D.BeginPointShadowPass(cmdList, frameIndex, face);\n    // DrawMesh でシーンオブジェクトを描画\n    renderer3D.EndPointShadowPass(face);\n}',
  '• face: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z\n• 各面は 90 度の視野角で正方形のビューポートを使用\n• 6パス必要なため CSM より描画コストが高い\n• 物理オブジェクトはシャドウパスではスキップを推奨（DrawMesh 上限対策）'
],

'PointShadowMap-EndShadowPass': [
  'void EndShadowPass(ID3D12GraphicsCommandList*, int face)',
  '指定面のシャドウパスを終了する。最後の面（face=5）の終了時にリソース全体を PIXEL_SHADER_RESOURCE にトランジションする。',
  '// シャドウパスの終了\nrenderer3D.EndPointShadowPass(face);',
  '• 全6面のパス終了後にメインパスで読み取り可能になる\n• バリア管理は内部で自動処理される\n• SetCurrentState() でリソース状態を追跡'
],

'PointShadowMap-BindSRV': [
  'void BindSRV(ID3D12GraphicsCommandList*)',
  'ポイントシャドウマップの SRV (Texture2DArray) をシェーダーにバインドする。t13 スロットに配置され、PBR シェーダーでポイントライトの影サンプリングに使用される。',
  '// SRV バインド（Renderer3D が自動で行う）\n// シェーダー側: Texture2DArray t13\n// ドミナント軸でスライスを選択してサンプリング',
  '• Texture2DArray として 6 スライスを 1 つの SRV でバインド\n• シェーダー側でライト→フラグメントのベクトルのドミナント軸からスライスを選択\n• Renderer3D::Begin() 内で自動的にバインドされる'
],

// ============================================================
// ShadowMap (page-ShadowMap)
// ============================================================

'ShadowMap-Initialize': [
  'bool Initialize(ID3D12Device*, DescriptorHeap* dsvHeap, DescriptorHeap* srvHeap, uint32_t resolution = 2048)',
  'スポットライト用シャドウマップを初期化する。指定解像度の深度バッファと DSV/SRV を作成する。Renderer3D 内部で自動的に初期化される。',
  '// Renderer3D が内部で管理するスポットシャドウ\n// k_SpotShadowMapSize = 2048',
  '• デフォルト解像度は 2048x2048\n• DepthBuffer ベースの深度テクスチャ\n• SRV は t12 にバインドされる\n• Renderer3D::Initialize() で自動作成'
],

'ShadowMap-BeginShadowPass': [
  'void BeginShadowPass(ID3D12GraphicsCommandList*, const SpotLight&)',
  'スポットライトシャドウパスを開始する。スポットライトの位置と方向からライトVP行列を計算し、シャドウマップへの深度描画を開始する。',
  '// スポットシャドウパス\nrenderer3D.BeginSpotShadowPass(cmdList, frameIndex);\n// DrawMesh でシーンオブジェクトを描画\nrenderer3D.EndSpotShadowPass();',
  '• スポットライトの outerAngle から FOV を計算\n• ライト位置からの透視投影で描画\n• CSM/Point の前後どちらでも実行可能'
],

'ShadowMap-EndShadowPass': [
  'void EndShadowPass(ID3D12GraphicsCommandList*)',
  'スポットシャドウパスを終了する。深度テクスチャを PIXEL_SHADER_RESOURCE 状態にトランジションする。',
  '// シャドウパスの終了\nrenderer3D.EndSpotShadowPass();',
  '• DEPTH_WRITE → PIXEL_SHADER_RESOURCE のバリア\n• メインパスでのサンプリングが可能になる'
],

'ShadowMap-BindSRV': [
  'void BindSRV(ID3D12GraphicsCommandList*)',
  'スポットシャドウマップの SRV をシェーダーにバインドする。t12 スロットに配置され、PBR シェーダーでスポットライトの影サンプリングに使用される。',
  '// SRV バインド（Renderer3D が自動で行う）\n// シェーダー側: Texture2D t12',
  '• t12 スロットにバインド\n• PCF サンプリングでソフトシャドウを実現\n• Renderer3D::Begin() 内で自動的にバインドされる'
]

}
