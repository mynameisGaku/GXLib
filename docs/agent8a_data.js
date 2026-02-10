{

// ============================================================
// Vector2 (page-Vector2)
// ============================================================

'Vector2-ctor_default': [
  'Vector2()',
  'ゼロベクトル (0, 0) で初期化するデフォルトコンストラクタです。XMFLOAT2を継承しており、DirectXMathとゼロオーバーヘッドで相互運用できます。明示的に値を指定しない場合、x, y ともに 0.0f で初期化されます。',
  `GX::Vector2 v;
// v.x == 0.0f, v.y == 0.0f`,
  '• XMFLOAT2を継承しているため、DirectXMath関数にそのまま渡せる\n• デフォルト状態はゼロベクトル'
],

'Vector2-ctor_xy': [
  'Vector2(float x, float y)',
  '指定した x, y 成分で初期化するコンストラクタです。2D空間上の位置や方向を表現する際に最も頻繁に使用します。各成分は float 型で指定します。',
  `GX::Vector2 pos(100.0f, 200.0f);
GX::Vector2 dir(1.0f, 0.0f);`,
  '• 暗黙変換コンストラクタではないため、{x, y} のブレース初期化も可能'
],

'Vector2-ctor_xmfloat2': [
  'Vector2(const XMFLOAT2& v)',
  'DirectXMath の XMFLOAT2 からの変換コンストラクタです。既存の DirectXMath コードとの橋渡しに使用します。XMFLOAT2 の x, y 成分がそのままコピーされます。',
  `XMFLOAT2 xmf(3.0f, 4.0f);
GX::Vector2 v(xmf);
// v.x == 3.0f, v.y == 4.0f`,
  '• XMFLOAT2 継承のため暗黙変換としても機能する'
],

'Vector2-x': [
  'float x',
  'ベクトルの X 成分です。XMFLOAT2 から継承されたパブリックメンバ変数で、直接読み書きが可能です。2D空間では水平方向の座標値を表します。',
  `GX::Vector2 v(10.0f, 20.0f);
v.x = 50.0f;
float horizontal = v.x; // 50.0f`,
  '• XMFLOAT2 から継承されたメンバ\n• 直接代入可能'
],

'Vector2-y': [
  'float y',
  'ベクトルの Y 成分です。XMFLOAT2 から継承されたパブリックメンバ変数で、直接読み書きが可能です。2D空間では垂直方向の座標値を表します。',
  `GX::Vector2 v(10.0f, 20.0f);
v.y = 80.0f;
float vertical = v.y; // 80.0f`,
  '• XMFLOAT2 から継承されたメンバ\n• 直接代入可能'
],

'Vector2-operatorPlus': [
  'Vector2 operator+(const Vector2& v) const',
  '2つのベクトルを成分ごとに加算し、結果を新しい Vector2 として返します。元のベクトルは変更されません。位置の移動やベクトルの合成に使用します。',
  `GX::Vector2 a(1.0f, 2.0f);
GX::Vector2 b(3.0f, 4.0f);
GX::Vector2 c = a + b; // (4.0f, 6.0f)`,
  '• 元のオブジェクトは変更されない (const)\n• += 演算子も別途利用可能'
],

'Vector2-operatorMinus': [
  'Vector2 operator-(const Vector2& v) const',
  '2つのベクトルを成分ごとに減算し、結果を新しい Vector2 として返します。2点間の方向ベクトルの計算などに使用します。元のベクトルは変更されません。',
  `GX::Vector2 a(5.0f, 7.0f);
GX::Vector2 b(2.0f, 3.0f);
GX::Vector2 diff = a - b; // (3.0f, 4.0f)`,
  '• 2点間の方向ベクトル算出に頻用\n• -= 演算子も別途利用可能'
],

'Vector2-operatorMul': [
  'Vector2 operator*(float s) const',
  'ベクトルの各成分にスカラー値を乗算し、結果を新しい Vector2 として返します。ベクトルの拡大・縮小に使用します。逆順の s * v も利用可能です。',
  `GX::Vector2 v(2.0f, 3.0f);
GX::Vector2 scaled = v * 2.0f; // (4.0f, 6.0f)
GX::Vector2 scaled2 = 0.5f * v; // (1.0f, 1.5f)`,
  '• フリー関数 operator*(float, Vector2) により s * v の順序でも使用可能\n• *= 演算子も利用可能'
],

'Vector2-Dot': [
  'float Dot(const Vector2& v) const',
  '2つのベクトルの内積（ドット積）を計算します。内部で DirectXMath の XMVector2Dot を使用するため高速です。2つのベクトルの類似度や射影量の計算に利用します。結果が正なら同方向、負なら逆方向、0なら直交です。',
  `GX::Vector2 a(1.0f, 0.0f);
GX::Vector2 b(0.0f, 1.0f);
float d = a.Dot(b); // 0.0f (直交)

GX::Vector2 c(1.0f, 0.0f);
float d2 = a.Dot(c); // 1.0f (同方向)`,
  '• 戻り値はスカラー (float)\n• DirectXMath の SIMD 演算を内部使用'
],

'Vector2-Cross': [
  'float Cross(const Vector2& v) const',
  '2Dベクトルの外積（スカラー値）を計算します。結果は x*v.y - y*v.x で、2つのベクトルが張る平行四辺形の符号付き面積に相当します。正の値は反時計回り、負の値は時計回りの関係を示します。',
  `GX::Vector2 a(1.0f, 0.0f);
GX::Vector2 b(0.0f, 1.0f);
float cross = a.Cross(b); // 1.0f (反時計回り)`,
  '• 2D外積はスカラー値を返す（3D外積とは異なる）\n• 回転方向の判定や面積計算に有用'
],

'Vector2-Length': [
  'float Length() const',
  'ベクトルの長さ（ユークリッドノルム）を計算して返します。内部で DirectXMath の XMVector2Length を使用します。平方根の計算を含むため、長さの比較だけなら LengthSquared の方が高速です。',
  `GX::Vector2 v(3.0f, 4.0f);
float len = v.Length(); // 5.0f`,
  '• sqrt を含むため頻繁な呼び出しにはコスト注意\n• 比較目的には LengthSquared() を推奨'
],

'Vector2-Normalized': [
  'Vector2 Normalized() const',
  'ベクトルを正規化（長さ1に変換）した新しい Vector2 を返します。元のベクトルは変更されません。方向ベクトルとして使用する場合や、単位ベクトルが必要な場合に使用します。ゼロベクトルに対して呼び出した場合の結果は不定です。',
  `GX::Vector2 v(3.0f, 4.0f);
GX::Vector2 dir = v.Normalized();
// dir.Length() ≈ 1.0f`,
  '• 自身を変更しない (const)\n• 自身を正規化する場合は Normalize() を使用\n• ゼロベクトルでの呼び出しに注意'
],

'Vector2-Distance': [
  'float Distance(const Vector2& v) const',
  '自身と指定ベクトルの間の距離（ユークリッド距離）を計算します。内部的には差分ベクトルの Length() を呼び出します。2点間の距離測定に使用します。',
  `GX::Vector2 a(0.0f, 0.0f);
GX::Vector2 b(3.0f, 4.0f);
float dist = a.Distance(b); // 5.0f`,
  '• (this - v).Length() と同等\n• 比較のみなら DistanceSquared() を推奨'
],

'Vector2-Lerp': [
  'static Vector2 Lerp(const Vector2& a, const Vector2& b, float t)',
  '2つのベクトル間を線形補間します。t=0 のとき a、t=1 のとき b を返します。アニメーションやスムーズな移動の実装に使用します。t は 0〜1 の範囲に限定されず、外挿も可能です。',
  `GX::Vector2 start(0.0f, 0.0f);
GX::Vector2 end(100.0f, 200.0f);
GX::Vector2 mid = GX::Vector2::Lerp(start, end, 0.5f);
// mid == (50.0f, 100.0f)`,
  '• 静的メンバ関数\n• t を [0, 1] にクランプしないため、外挿にも使用可能'
],

'Vector2-Zero': [
  'static Vector2 Zero()',
  'ゼロベクトル (0, 0) を返す静的メンバ関数です。初期化やリセット時の定数として使用します。デフォルトコンストラクタと同じ結果ですが、意図を明示するために使い分けます。',
  `GX::Vector2 v = GX::Vector2::Zero();
// v.x == 0.0f, v.y == 0.0f`,
  '• Vector2() と等価だが意図が明確'
],

'Vector2-One': [
  'static Vector2 One()',
  '全成分が 1 のベクトル (1, 1) を返す静的メンバ関数です。スケーリングの基準値やデフォルトサイズとして使用します。',
  `GX::Vector2 v = GX::Vector2::One();
// v.x == 1.0f, v.y == 1.0f`,
  '• スケール初期値等に便利'
],

'Vector2-Up': [
  'static Vector2 UnitY()',
  'Y軸正方向の単位ベクトル (0, 1) を返す静的メンバ関数です。上方向を示す定数として使用します。画面座標系では Y が下向きの場合があるので注意が必要です。',
  `GX::Vector2 up = GX::Vector2::UnitY();
// up.x == 0.0f, up.y == 1.0f`,
  '• 画面座標系ではY軸の向きに注意\n• 関数名は UnitY'
],

'Vector2-Right': [
  'static Vector2 UnitX()',
  'X軸正方向の単位ベクトル (1, 0) を返す静的メンバ関数です。右方向を示す定数として使用します。',
  `GX::Vector2 right = GX::Vector2::UnitX();
// right.x == 1.0f, right.y == 0.0f`,
  '• 関数名は UnitX'
],

// ============================================================
// Vector3 (page-Vector3)
// ============================================================

'Vector3-ctor_default': [
  'Vector3()',
  'ゼロベクトル (0, 0, 0) で初期化するデフォルトコンストラクタです。XMFLOAT3を継承しており、DirectXMathとゼロオーバーヘッドで相互運用できます。',
  `GX::Vector3 v;
// v.x == 0.0f, v.y == 0.0f, v.z == 0.0f`,
  '• XMFLOAT3 継承によりDirectXMath関数にそのまま渡せる'
],

'Vector3-ctor_xyz': [
  'Vector3(float x, float y, float z)',
  '指定した x, y, z 成分で初期化するコンストラクタです。3D空間上の位置、方向、スケールなどの表現に使用します。',
  `GX::Vector3 pos(10.0f, 5.0f, -3.0f);
GX::Vector3 scale(2.0f, 2.0f, 2.0f);`,
  '• ブレース初期化 {x, y, z} も使用可能'
],

'Vector3-ctor_xmfloat3': [
  'Vector3(const XMFLOAT3& v)',
  'DirectXMath の XMFLOAT3 からの変換コンストラクタです。既存の DirectXMath コードとの相互運用に使用します。',
  `XMFLOAT3 xmf(1.0f, 2.0f, 3.0f);
GX::Vector3 v(xmf);`,
  '• 暗黙変換としても機能する'
],

'Vector3-x': [
  'float x',
  'ベクトルの X 成分です。XMFLOAT3 から継承されたパブリックメンバ変数です。3D空間では左右方向の座標値を表します。直接読み書きが可能です。',
  `GX::Vector3 v(1.0f, 2.0f, 3.0f);
v.x = 10.0f;`,
  '• XMFLOAT3 から継承'
],

'Vector3-y': [
  'float y',
  'ベクトルの Y 成分です。XMFLOAT3 から継承されたパブリックメンバ変数です。3D空間では上下方向の座標値を表します。直接読み書きが可能です。',
  `GX::Vector3 v(1.0f, 2.0f, 3.0f);
v.y = 20.0f;`,
  '• XMFLOAT3 から継承'
],

'Vector3-z': [
  'float z',
  'ベクトルの Z 成分です。XMFLOAT3 から継承されたパブリックメンバ変数です。3D空間では前後方向（奥行き）の座標値を表します。直接読み書きが可能です。',
  `GX::Vector3 v(1.0f, 2.0f, 3.0f);
v.z = 30.0f;`,
  '• XMFLOAT3 から継承'
],

'Vector3-operatorPlus': [
  'Vector3 operator+(const Vector3& v) const',
  '2つの3Dベクトルを成分ごとに加算し、新しい Vector3 を返します。位置の移動や力の合成に使用します。元のベクトルは変更されません。',
  `GX::Vector3 a(1.0f, 2.0f, 3.0f);
GX::Vector3 b(4.0f, 5.0f, 6.0f);
GX::Vector3 c = a + b; // (5.0f, 7.0f, 9.0f)`,
  '• += 演算子も利用可能'
],

'Vector3-operatorMinus': [
  'Vector3 operator-(const Vector3& v) const',
  '2つの3Dベクトルを成分ごとに減算し、新しい Vector3 を返します。2点間の方向ベクトル計算や差分の取得に使用します。',
  `GX::Vector3 target(10.0f, 0.0f, 10.0f);
GX::Vector3 eye(0.0f, 5.0f, 0.0f);
GX::Vector3 toTarget = target - eye;`,
  '• 2点間の方向ベクトル算出に頻用\n• -= 演算子も利用可能'
],

'Vector3-Dot': [
  'float Dot(const Vector3& v) const',
  '2つの3Dベクトルの内積を計算します。内部で DirectXMath の XMVector3Dot を使用します。ライティングの拡散反射計算（法線と光源方向の内積）やベクトル間の角度判定に必須です。',
  `GX::Vector3 normal(0.0f, 1.0f, 0.0f);
GX::Vector3 lightDir(0.0f, -1.0f, 0.0f);
float nDotL = normal.Dot(lightDir); // -1.0f`,
  '• ライティング計算の基本演算\n• SIMD最適化済み'
],

'Vector3-Cross': [
  'Vector3 Cross(const Vector3& v) const',
  '2つの3Dベクトルの外積（クロス積）を計算し、両方に垂直なベクトルを返します。内部で DirectXMath の XMVector3Cross を使用します。法線ベクトルの算出やカメラの右方向ベクトルの計算に使用します。',
  `GX::Vector3 forward(0.0f, 0.0f, 1.0f);
GX::Vector3 up(0.0f, 1.0f, 0.0f);
GX::Vector3 right = forward.Cross(up);
// right == (-1.0f, 0.0f, 0.0f)`,
  '• 結果は this x v の順で計算される（順序で符号が反転）\n• SIMD最適化済み'
],

'Vector3-Length': [
  'float Length() const',
  'ベクトルの長さ（ユークリッドノルム）を返します。内部で DirectXMath の XMVector3Length を使用します。3D空間での距離や速度の大きさの取得に使用します。',
  `GX::Vector3 v(1.0f, 2.0f, 2.0f);
float len = v.Length(); // 3.0f`,
  '• sqrt を含むため、比較目的には LengthSquared() を推奨'
],

'Vector3-Normalized': [
  'Vector3 Normalized() const',
  'ベクトルを正規化（長さ1）にした新しい Vector3 を返します。元のベクトルは変更されません。方向ベクトルの取得に使用します。ゼロベクトルに対して呼び出した場合の結果は不定です。',
  `GX::Vector3 v(3.0f, 0.0f, 4.0f);
GX::Vector3 dir = v.Normalized();
// dir.Length() ≈ 1.0f`,
  '• 自身を変更しない (const)\n• 自身を正規化するには Normalize() を使用'
],

'Vector3-Distance': [
  'float Distance(const Vector3& v) const',
  '自身と指定ベクトルの間の3D空間上の距離を計算します。内部的に差分ベクトルの Length() を呼び出します。オブジェクト間の距離判定に使用します。',
  `GX::Vector3 a(0.0f, 0.0f, 0.0f);
GX::Vector3 b(1.0f, 2.0f, 2.0f);
float dist = a.Distance(b); // 3.0f`,
  '• (this - v).Length() と同等\n• 比較のみなら DistanceSquared() を推奨'
],

'Vector3-Lerp': [
  'static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)',
  '2つの3Dベクトル間を線形補間します。t=0 のとき a、t=1 のとき b を返します。3Dアニメーションやカメラのスムーズ移動に使用します。',
  `GX::Vector3 start(0.0f, 0.0f, 0.0f);
GX::Vector3 end(10.0f, 20.0f, 30.0f);
GX::Vector3 mid = GX::Vector3::Lerp(start, end, 0.5f);
// mid == (5.0f, 10.0f, 15.0f)`,
  '• 静的メンバ関数\n• t を [0, 1] にクランプしない'
],

'Vector3-Zero': [
  'static Vector3 Zero()',
  'ゼロベクトル (0, 0, 0) を返す静的メンバ関数です。原点や初期位置として使用します。',
  `GX::Vector3 origin = GX::Vector3::Zero();`,
  '• Vector3() と等価'
],

'Vector3-One': [
  'static Vector3 One()',
  '全成分が 1 のベクトル (1, 1, 1) を返す静的メンバ関数です。デフォルトスケール値として頻繁に使用します。',
  `GX::Vector3 scale = GX::Vector3::One();`,
  '• スケールの初期値として定番'
],

'Vector3-Up': [
  'static Vector3 Up()',
  '上方向の単位ベクトル (0, 1, 0) を返す静的メンバ関数です。Y軸正方向をワールドの上方向として使用します。カメラのアップベクトルとして使うのが一般的です。',
  `GX::Vector3 up = GX::Vector3::Up();
// ビュー行列の作成時に使用
auto view = GX::Matrix4x4::LookAtLH(eye, target, up);`,
  '• 左手座標系の Y-up 規約に対応'
],

'Vector3-Forward': [
  'static Vector3 Forward()',
  '前方向の単位ベクトル (0, 0, 1) を返す静的メンバ関数です。左手座標系においてZ軸正方向がカメラの前方を表します。',
  `GX::Vector3 fwd = GX::Vector3::Forward();
// fwd == (0.0f, 0.0f, 1.0f)`,
  '• 左手座標系 (LH) でのZ正方向'
],

'Vector3-Right': [
  'static Vector3 Right()',
  '右方向の単位ベクトル (1, 0, 0) を返す静的メンバ関数です。X軸正方向を右方向として使用します。',
  `GX::Vector3 right = GX::Vector3::Right();
// right == (1.0f, 0.0f, 0.0f)`,
  '• X軸正方向'
],

// ============================================================
// Vector4 (page-Vector4)
// ============================================================

'Vector4-ctor_default': [
  'Vector4()',
  'ゼロベクトル (0, 0, 0, 0) で初期化するデフォルトコンストラクタです。XMFLOAT4を継承しており、DirectXMathとゼロオーバーヘッドで相互運用できます。',
  `GX::Vector4 v;
// v.x == 0, v.y == 0, v.z == 0, v.w == 0`,
  '• XMFLOAT4 継承'
],

'Vector4-ctor_xyzw': [
  'Vector4(float x, float y, float z, float w)',
  '指定した x, y, z, w 成分で初期化するコンストラクタです。同次座標やシェーダーへの4成分データ受け渡しに使用します。',
  `GX::Vector4 v(1.0f, 2.0f, 3.0f, 1.0f);`,
  '• 同次座標 (w=1 で位置、w=0 で方向) の表現に便利'
],

'Vector4-ctor_v3w': [
  'Vector4(const Vector3& v, float w)',
  'Vector3 と W 成分から初期化するコンストラクタです。3Dベクトルに W 成分を追加して4Dベクトルを作成します。同次座標の生成に便利です。',
  `GX::Vector3 pos(1.0f, 2.0f, 3.0f);
GX::Vector4 homogeneous(pos, 1.0f); // 位置として
GX::Vector4 direction(pos, 0.0f);   // 方向として`,
  '• w=1.0f で位置、w=0.0f で方向ベクトルを表現'
],

'Vector4-ctor_xmfloat4': [
  'Vector4(const XMFLOAT4& v)',
  'DirectXMath の XMFLOAT4 からの変換コンストラクタです。XMFLOAT4 との相互運用に使用します。',
  `XMFLOAT4 xmf(1.0f, 2.0f, 3.0f, 4.0f);
GX::Vector4 v(xmf);`,
  '• 暗黙変換としても機能する'
],

'Vector4-x': [
  'float x',
  'ベクトルの X 成分です。XMFLOAT4 から継承されたパブリックメンバ変数で、直接読み書きが可能です。',
  `GX::Vector4 v(1.0f, 2.0f, 3.0f, 4.0f);
float val = v.x; // 1.0f`,
  '• XMFLOAT4 から継承'
],

'Vector4-y': [
  'float y',
  'ベクトルの Y 成分です。XMFLOAT4 から継承されたパブリックメンバ変数で、直接読み書きが可能です。',
  `GX::Vector4 v(1.0f, 2.0f, 3.0f, 4.0f);
float val = v.y; // 2.0f`,
  '• XMFLOAT4 から継承'
],

'Vector4-z': [
  'float z',
  'ベクトルの Z 成分です。XMFLOAT4 から継承されたパブリックメンバ変数で、直接読み書きが可能です。',
  `GX::Vector4 v(1.0f, 2.0f, 3.0f, 4.0f);
float val = v.z; // 3.0f`,
  '• XMFLOAT4 から継承'
],

'Vector4-w': [
  'float w',
  'ベクトルの W 成分です。XMFLOAT4 から継承されたパブリックメンバ変数で、直接読み書きが可能です。同次座標での位置(w=1)と方向(w=0)の区別に使用します。',
  `GX::Vector4 v(1.0f, 2.0f, 3.0f, 4.0f);
float val = v.w; // 4.0f`,
  '• XMFLOAT4 から継承\n• 同次座標での意味付けに重要'
],

'Vector4-ToXMVECTOR': [
  'XMVECTOR (XMLoadFloat4 経由)',
  'Vector4 は XMFLOAT4 を継承しているため、XMLoadFloat4(this) により XMVECTOR に変換できます。DirectXMath の SIMD 関数に渡す際に使用します。専用の ToXMVECTOR メンバ関数はなく、XMLoadFloat4 を直接使用します。',
  `GX::Vector4 v(1.0f, 2.0f, 3.0f, 4.0f);
XMVECTOR xmv = XMLoadFloat4(&v);`,
  '• XMFLOAT4 継承のため XMLoadFloat4 に直接渡せる\n• 明示的な変換関数ではなく DirectXMath API を使用'
],

// ============================================================
// Matrix4x4 (page-Matrix4x4)
// ============================================================

'Matrix4x4-ctor_default': [
  'Matrix4x4()',
  '単位行列で初期化するデフォルトコンストラクタです。XMFLOAT4X4を継承しており、DirectXMathとゼロオーバーヘッドで相互運用できます。変換なしの初期状態として安全に使用できます。',
  `GX::Matrix4x4 m;
// 単位行列 (対角要素が1、それ以外が0)`,
  '• XMFLOAT4X4 継承\n• デフォルトは単位行列（ゼロ行列ではない）'
],

'Matrix4x4-ctor_xmfloat4x4': [
  'Matrix4x4(const XMFLOAT4X4& m)',
  'DirectXMath の XMFLOAT4X4 からの変換コンストラクタです。既存の DirectXMath コードの行列データを変換する際に使用します。',
  `XMFLOAT4X4 xmf;
XMStoreFloat4x4(&xmf, XMMatrixIdentity());
GX::Matrix4x4 m(xmf);`,
  '• 暗黙変換としても機能する'
],

'Matrix4x4-ToXMMATRIX': [
  'XMMATRIX ToXMMATRIX() const',
  '内部データを DirectXMath の XMMATRIX 型に変換して返します。DirectXMath の行列演算関数に渡す際に使用します。XMLoadFloat4x4 を内部で呼び出します。',
  `GX::Matrix4x4 m = GX::Matrix4x4::Identity();
XMMATRIX xmm = m.ToXMMATRIX();`,
  '• DirectXMath API との橋渡しに必須'
],

'Matrix4x4-FromXMMATRIX': [
  'static Matrix4x4 FromXMMATRIX(const XMMATRIX& m)',
  'DirectXMath の XMMATRIX から Matrix4x4 を生成する静的ファクトリ関数です。DirectXMath の計算結果を格納する際に使用します。',
  `XMMATRIX xmm = XMMatrixRotationY(GX::MathUtil::PI);
GX::Matrix4x4 m = GX::Matrix4x4::FromXMMATRIX(xmm);`,
  '• 静的メンバ関数\n• XMStoreFloat4x4 を内部で呼び出す'
],

'Matrix4x4-Identity': [
  'static Matrix4x4 Identity()',
  '単位行列を返す静的メンバ関数です。変換なしの基準行列として使用します。対角成分が 1、それ以外が 0 の行列です。',
  `GX::Matrix4x4 m = GX::Matrix4x4::Identity();`,
  '• デフォルトコンストラクタも単位行列を返す'
],

'Matrix4x4-Transpose': [
  'Matrix4x4 Transpose() const',
  '転置行列を計算して返します。行と列を入れ替えた行列を生成します。法線変換に使用する逆転置行列の作成時などに使用します。元の行列は変更されません。',
  `GX::Matrix4x4 m = GX::Matrix4x4::RotationY(0.5f);
GX::Matrix4x4 mt = m.Transpose();`,
  '• 自身を変更しない (const)\n• 直交行列の転置は逆行列と等しい'
],

'Matrix4x4-Inverse': [
  'Matrix4x4 Inverse() const',
  '逆行列を計算して返します。行列による変換を元に戻す変換行列を生成します。ビュー行列の逆行列からカメラのワールド位置を取得する際などに使用します。元の行列は変更されません。',
  `GX::Matrix4x4 view = GX::Matrix4x4::LookAtLH(eye, target, up);
GX::Matrix4x4 invView = view.Inverse();`,
  '• 特異行列（行列式が0）の場合は不正な結果になる\n• DirectXMath の XMMatrixInverse を内部使用'
],

'Matrix4x4-Multiply': [
  'Matrix4x4 operator*(const Matrix4x4& m) const',
  '2つの行列を乗算して合成行列を返します。変換の合成（スケール→回転→平行移動など）に使用します。行列の乗算順序は右から左に適用されます。',
  `GX::Matrix4x4 scale = GX::Matrix4x4::Scaling(2.0f);
GX::Matrix4x4 rot = GX::Matrix4x4::RotationY(GX::MathUtil::PI / 4.0f);
GX::Matrix4x4 trans = GX::Matrix4x4::Translation(0.0f, 5.0f, 0.0f);
GX::Matrix4x4 world = scale * rot * trans;`,
  '• 乗算順序に注意: S * R * T が一般的\n• DirectXMath の XMMatrixMultiply を内部使用'
],

'Matrix4x4-Translation': [
  'static Matrix4x4 Translation(float x, float y, float z)',
  '平行移動行列を作成する静的メンバ関数です。指定した x, y, z 方向にオブジェクトを移動させる変換行列を生成します。Vector3 を引数に取るオーバーロードもあります。',
  `GX::Matrix4x4 t = GX::Matrix4x4::Translation(10.0f, 0.0f, 5.0f);

// Vector3 オーバーロード
GX::Vector3 offset(10.0f, 0.0f, 5.0f);
GX::Matrix4x4 t2 = GX::Matrix4x4::Translation(offset);`,
  '• Vector3 引数のオーバーロードあり\n• Translation(const Vector3& t) も利用可能'
],

'Matrix4x4-Scaling': [
  'static Matrix4x4 Scaling(float x, float y, float z)',
  '拡大縮小行列を作成する静的メンバ関数です。各軸方向に異なるスケール値を指定できます。均一スケール用の Scaling(float uniform) オーバーロードもあります。',
  `GX::Matrix4x4 s = GX::Matrix4x4::Scaling(2.0f, 1.0f, 2.0f);

// 均一スケール
GX::Matrix4x4 s2 = GX::Matrix4x4::Scaling(0.5f);`,
  '• 均一スケール版 Scaling(float) も利用可能\n• 負のスケールで反転が可能'
],

'Matrix4x4-RotationX': [
  'static Matrix4x4 RotationX(float radians)',
  'X軸周りの回転行列を作成する静的メンバ関数です。角度はラジアン単位で指定します。ピッチ回転（上下の傾き）に使用します。',
  `float angle = GX::MathUtil::DegreesToRadians(45.0f);
GX::Matrix4x4 rx = GX::Matrix4x4::RotationX(angle);`,
  '• 角度はラジアン単位\n• DegreesToRadians で度数法から変換可能'
],

'Matrix4x4-RotationY': [
  'static Matrix4x4 RotationY(float radians)',
  'Y軸周りの回転行列を作成する静的メンバ関数です。角度はラジアン単位で指定します。ヨー回転（左右の回転）に使用します。',
  `float angle = GX::MathUtil::DegreesToRadians(90.0f);
GX::Matrix4x4 ry = GX::Matrix4x4::RotationY(angle);`,
  '• 角度はラジアン単位'
],

'Matrix4x4-RotationZ': [
  'static Matrix4x4 RotationZ(float radians)',
  'Z軸周りの回転行列を作成する静的メンバ関数です。角度はラジアン単位で指定します。ロール回転（傾き回転）に使用します。2Dゲームでのスプライト回転にも利用できます。',
  `float angle = GX::MathUtil::DegreesToRadians(30.0f);
GX::Matrix4x4 rz = GX::Matrix4x4::RotationZ(angle);`,
  '• 角度はラジアン単位\n• 2D回転にも利用可能'
],

'Matrix4x4-LookAt': [
  'static Matrix4x4 LookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up)',
  '左手座標系のビュー行列（LookAt行列）を作成する静的メンバ関数です。カメラの位置、注視点、上方向ベクトルを指定します。3Dシーンのカメラ設定に必須です。',
  `GX::Vector3 eye(0.0f, 5.0f, -10.0f);
GX::Vector3 target(0.0f, 0.0f, 0.0f);
GX::Vector3 up = GX::Vector3::Up();
GX::Matrix4x4 view = GX::Matrix4x4::LookAtLH(eye, target, up);`,
  '• 左手座標系 (LH) 専用\n• DirectXMath の XMMatrixLookAtLH を内部使用'
],

'Matrix4x4-Perspective': [
  'static Matrix4x4 PerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ)',
  '左手座標系の透視投影行列を作成する静的メンバ関数です。垂直視野角、アスペクト比、ニア/ファークリップ面を指定します。3Dシーンの遠近感を表現するために使用します。',
  `float fov = GX::MathUtil::DegreesToRadians(60.0f);
float aspect = 1280.0f / 720.0f;
GX::Matrix4x4 proj = GX::Matrix4x4::PerspectiveFovLH(fov, aspect, 0.1f, 1000.0f);`,
  '• fovY はラジアン単位\n• nearZ は 0 より大きい値を指定すること\n• 左手座標系 (LH) 専用'
],

'Matrix4x4-Orthographic': [
  'static Matrix4x4 OrthographicLH(float width, float height, float nearZ, float farZ)',
  '左手座標系の正射影行列を作成する静的メンバ関数です。遠近感のない投影で、UIやミニマップ、シャドウマップの描画に使用します。',
  `GX::Matrix4x4 ortho = GX::Matrix4x4::OrthographicLH(1280.0f, 720.0f, 0.0f, 1.0f);`,
  '• 遠近感のない平行投影\n• UIやシャドウマップ用途に最適'
],

// ============================================================
// Quaternion (page-Quaternion)
// ============================================================

'Quaternion-ctor_default': [
  'Quaternion()',
  '単位クォータニオン (0, 0, 0, 1) で初期化するデフォルトコンストラクタです。回転なしの状態を表します。XMFLOAT4 を継承しています。',
  `GX::Quaternion q;
// q.x == 0, q.y == 0, q.z == 0, q.w == 1 (回転なし)`,
  '• 単位クォータニオン = 回転なし\n• XMFLOAT4 継承'
],

'Quaternion-ctor_xyzw': [
  'Quaternion(float x, float y, float z, float w)',
  '指定した x, y, z, w 成分で直接初期化するコンストラクタです。通常は FromAxisAngle や FromEuler などのファクトリ関数を使用することを推奨しますが、シリアライズからの復元などで直接値を指定する場合に使用します。',
  `GX::Quaternion q(0.0f, 0.7071f, 0.0f, 0.7071f);`,
  '• 通常はファクトリ関数の使用を推奨\n• x,y,z が虚部、w が実部'
],

'Quaternion-ctor_xmfloat4': [
  'Quaternion(const XMFLOAT4& q)',
  'DirectXMath の XMFLOAT4 からの変換コンストラクタです。DirectXMath のクォータニオン演算結果を格納する際に使用します。',
  `XMFLOAT4 xmf;
XMStoreFloat4(&xmf, XMQuaternionIdentity());
GX::Quaternion q(xmf);`,
  '• 暗黙変換としても機能する'
],

'Quaternion-x': [
  'float x',
  'クォータニオンの X 成分（虚部）です。XMFLOAT4 から継承されたパブリックメンバ変数です。直接アクセスは可能ですが、通常はファクトリ関数で生成することを推奨します。',
  `GX::Quaternion q = GX::Quaternion::FromEuler(0.5f, 0.0f, 0.0f);
float xVal = q.x;`,
  '• XMFLOAT4 から継承\n• 虚部の X 成分'
],

'Quaternion-y': [
  'float y',
  'クォータニオンの Y 成分（虚部）です。XMFLOAT4 から継承されたパブリックメンバ変数です。',
  `GX::Quaternion q = GX::Quaternion::FromEuler(0.0f, 1.0f, 0.0f);
float yVal = q.y;`,
  '• XMFLOAT4 から継承\n• 虚部の Y 成分'
],

'Quaternion-z': [
  'float z',
  'クォータニオンの Z 成分（虚部）です。XMFLOAT4 から継承されたパブリックメンバ変数です。',
  `GX::Quaternion q = GX::Quaternion::FromEuler(0.0f, 0.0f, 0.5f);
float zVal = q.z;`,
  '• XMFLOAT4 から継承\n• 虚部の Z 成分'
],

'Quaternion-w': [
  'float w',
  'クォータニオンの W 成分（実部）です。XMFLOAT4 から継承されたパブリックメンバ変数です。単位クォータニオンでは w=1 です。',
  `GX::Quaternion q = GX::Quaternion::Identity();
float wVal = q.w; // 1.0f`,
  '• XMFLOAT4 から継承\n• 実部 (scalar part)'
],

'Quaternion-FromAxisAngle': [
  'static Quaternion FromAxisAngle(const Vector3& axis, float radians)',
  '任意の軸周りの回転を表すクォータニオンを作成します。回転軸は正規化されている必要があります。角度はラジアン単位です。ジンバルロックを回避できる回転表現です。',
  `GX::Vector3 axis = GX::Vector3::Up();
float angle = GX::MathUtil::DegreesToRadians(90.0f);
GX::Quaternion q = GX::Quaternion::FromAxisAngle(axis, angle);`,
  '• 軸は正規化されていること\n• 角度はラジアン単位\n• ジンバルロックが発生しない'
],

'Quaternion-FromEuler': [
  'static Quaternion FromEuler(float pitch, float yaw, float roll)',
  'オイラー角（ピッチ・ヨー・ロール）から回転クォータニオンを作成します。ピッチはX軸回転、ヨーはY軸回転、ロールはZ軸回転です。角度はすべてラジアン単位です。',
  `float pitch = GX::MathUtil::DegreesToRadians(30.0f); // X軸
float yaw   = GX::MathUtil::DegreesToRadians(45.0f); // Y軸
float roll  = 0.0f;                                   // Z軸
GX::Quaternion q = GX::Quaternion::FromEuler(pitch, yaw, roll);`,
  '• 角度はすべてラジアン単位\n• 内部で XMQuaternionRotationRollPitchYaw を使用'
],

'Quaternion-Slerp': [
  'static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)',
  '2つのクォータニオン間を球面線形補間（Slerp）します。t=0 のとき a、t=1 のとき b を返します。回転アニメーションにおいて一定の角速度で滑らかに補間する場合に使用します。Lerp より高品質ですが計算コストが高いです。',
  `GX::Quaternion start = GX::Quaternion::Identity();
GX::Quaternion end = GX::Quaternion::FromAxisAngle(GX::Vector3::Up(), GX::MathUtil::PI);
GX::Quaternion mid = GX::Quaternion::Slerp(start, end, 0.5f);`,
  '• 一定角速度の補間が可能\n• Lerp より高品質だが計算コスト大\n• スケルタルアニメーションの回転補間に最適'
],

'Quaternion-Lerp': [
  'static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t)',
  '2つのクォータニオン間を正規化線形補間（NLerp）します。線形補間後に正規化するため、角速度が一定にはなりませんが Slerp より高速です。回転差が小さい場合はSlerpとほぼ同等の結果になります。',
  `GX::Quaternion a = GX::Quaternion::FromEuler(0.0f, 0.0f, 0.0f);
GX::Quaternion b = GX::Quaternion::FromEuler(0.0f, 0.5f, 0.0f);
GX::Quaternion mid = GX::Quaternion::Lerp(a, b, 0.5f);`,
  '• NLerp（Normalized Linear Interpolation）\n• Slerp より高速だが角速度は一定でない\n• 回転差が小さい場合に推奨'
],

'Quaternion-Identity': [
  'static Quaternion Identity()',
  '単位クォータニオン (0, 0, 0, 1) を返す静的メンバ関数です。回転なしの状態を表します。初期化やリセット時に使用します。',
  `GX::Quaternion q = GX::Quaternion::Identity();
// 回転なしの状態`,
  '• デフォルトコンストラクタと等価'
],

'Quaternion-ToMatrix': [
  'Vector3 ToEuler() const / RotateVector(const Vector3& v) const',
  'クォータニオンをオイラー角に変換する ToEuler() と、ベクトルを回転する RotateVector() を提供します。ToEuler は (pitch, yaw, roll) をラジアンで返します。RotateVector は指定ベクトルにクォータニオンの回転を適用します。',
  `GX::Quaternion q = GX::Quaternion::FromEuler(0.5f, 1.0f, 0.0f);

// オイラー角に変換
GX::Vector3 euler = q.ToEuler();

// ベクトルを回転
GX::Vector3 forward = q.RotateVector(GX::Vector3::Forward());`,
  '• ToEuler の戻り値: x=pitch, y=yaw, z=roll（ラジアン）\n• RotateVector は内部で回転行列経由で変換'
],

'Quaternion-Conjugate': [
  'Quaternion Conjugate() const',
  '共役クォータニオンを返します。虚部 (x, y, z) の符号を反転し、実部 (w) はそのまま保持します。単位クォータニオンの場合、共役は逆クォータニオンと等しくなります。元のオブジェクトは変更されません。',
  `GX::Quaternion q = GX::Quaternion::FromAxisAngle(GX::Vector3::Up(), 1.0f);
GX::Quaternion conj = q.Conjugate();
// conj は逆回転を表す（単位クォータニオンの場合）`,
  '• 単位クォータニオンでは Conjugate == Inverse\n• 自身を変更しない (const)'
],

'Quaternion-Normalize': [
  'Quaternion Normalized() const / void Normalize()',
  'クォータニオンを正規化（長さ1に）します。Normalized() は新しいオブジェクトを返し、Normalize() は自身を変更します。累積的な回転合成でドリフトした場合の補正に使用します。',
  `GX::Quaternion q(0.1f, 0.2f, 0.3f, 0.9f);
GX::Quaternion norm = q.Normalized();
// norm.Length() ≈ 1.0f

q.Normalize(); // q 自身を正規化`,
  '• 回転の累積でノルムがドリフトした場合に補正\n• Normalized() は const、Normalize() は自身を変更'
],

// ============================================================
// Color (page-Color)
// ============================================================

'Color-ctor_default': [
  'Color()',
  '白色 (1, 1, 1, 1) で初期化するデフォルトコンストラクタです。r, g, b, a の各成分は float 型で 0.0〜1.0 の範囲の値を持ちます。デフォルトが白色である点に注意してください。',
  `GX::Color c;
// c.r == 1.0f, c.g == 1.0f, c.b == 1.0f, c.a == 1.0f`,
  '• デフォルトは白色 (Vector 系のゼロ初期化とは異なる)\n• XMFLOAT4 を継承しない独自構造体'
],

'Color-ctor_float': [
  'Color(float r, float g, float b, float a = 1.0f)',
  'float 成分で色を初期化するコンストラクタです。各成分は 0.0〜1.0 の範囲で指定します。アルファ値のデフォルトは 1.0（不透明）です。HDR レンダリングでは 1.0 を超える値も使用可能です。',
  `GX::Color red(1.0f, 0.0f, 0.0f);        // 不透明な赤
GX::Color semiRed(1.0f, 0.0f, 0.0f, 0.5f); // 半透明な赤`,
  '• a のデフォルトは 1.0f（不透明）\n• float リテラルを使用すること (1.0f であって 1 ではない)'
],

'Color-ctor_uint32': [
  'Color(uint32_t rgba)',
  '32ビット RGBA 整数値 (0xRRGGBBAA) から色を初期化するコンストラクタです。各チャンネル 8 ビットの値を内部で 0.0〜1.0 の float に変換します。Web カラーコードに近い形式で色を指定できます。',
  `GX::Color c(0xFF0000FFu); // 赤 (R=255, G=0, B=0, A=255)
GX::Color blue(0x0000FFFFu); // 青`,
  '• 形式: 0xRRGGBBAA\n• uint32_t 型のため u サフィックスの使用を推奨\n• float コンストラクタとの曖昧さを避けるため明示的なキャストが必要な場合がある'
],

'Color-ctor_uint8': [
  'Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)',
  '8ビット整数成分 (0〜255) で色を初期化するコンストラクタです。内部で 255.0f で除算して float に変換します。整数値での色指定に慣れている場合に便利です。',
  `GX::Color c(static_cast<uint8_t>(255), static_cast<uint8_t>(128), static_cast<uint8_t>(0));`,
  '• 内部で 0.0〜1.0 の float に変換される\n• float コンストラクタとの曖昧さを避けるため explicit キャストが必要\n• a のデフォルトは 255'
],

'Color-r': [
  'float r',
  '赤成分 (0.0〜1.0) です。パブリックメンバ変数で直接読み書きが可能です。デフォルト値は 1.0f です。',
  `GX::Color c = GX::Color::White();
c.r = 0.5f; // 赤成分を半分に`,
  '• デフォルト値 1.0f'
],

'Color-g': [
  'float g',
  '緑成分 (0.0〜1.0) です。パブリックメンバ変数で直接読み書きが可能です。デフォルト値は 1.0f です。',
  `GX::Color c = GX::Color::White();
c.g = 0.0f;`,
  '• デフォルト値 1.0f'
],

'Color-b': [
  'float b',
  '青成分 (0.0〜1.0) です。パブリックメンバ変数で直接読み書きが可能です。デフォルト値は 1.0f です。',
  `GX::Color c = GX::Color::White();
c.b = 0.0f;`,
  '• デフォルト値 1.0f'
],

'Color-a': [
  'float a',
  'アルファ（透明度）成分 (0.0〜1.0) です。0.0 が完全透明、1.0 が完全不透明です。パブリックメンバ変数で直接読み書きが可能です。',
  `GX::Color c = GX::Color::Red();
c.a = 0.5f; // 半透明の赤`,
  '• 0.0 = 完全透明、1.0 = 完全不透明\n• デフォルト値 1.0f'
],

'Color-ToRGBA': [
  'uint32_t ToRGBA() const',
  '色を 32 ビット RGBA 整数値 (0xRRGGBBAA) に変換します。各成分を 0〜255 にクランプして変換します。テクスチャピクセルデータの生成やシリアライズに使用します。',
  `GX::Color c(1.0f, 0.0f, 0.0f, 1.0f);
uint32_t rgba = c.ToRGBA(); // 0xFF0000FF`,
  '• 出力形式: 0xRRGGBBAA\n• 内部で Clamp(0, 1) を適用'
],

'Color-ToABGR': [
  'uint32_t ToABGR() const',
  '色を 32 ビット ABGR 整数値 (0xAABBGGRR) に変換します。一部のグラフィックス API やフォーマットで使用されるバイトオーダーに対応します。',
  `GX::Color c(1.0f, 0.0f, 0.0f, 1.0f);
uint32_t abgr = c.ToABGR(); // 0xFF0000FF`,
  '• 出力形式: 0xAABBGGRR\n• RGBA と比較してR/Bが入れ替わる'
],

'Color-ToXMFLOAT4': [
  'XMFLOAT4 ToXMFLOAT4() const',
  'DirectXMath の XMFLOAT4 形式に変換します。シェーダーの定数バッファに色値を渡す際に使用します。r, g, b, a がそのまま x, y, z, w にマッピングされます。',
  `GX::Color c = GX::Color::Red();
XMFLOAT4 xmf = c.ToXMFLOAT4();
// xmf.x == 1.0f, xmf.y == 0.0f, xmf.z == 0.0f, xmf.w == 1.0f`,
  '• r→x, g→y, b→z, a→w のマッピング'
],

'Color-FromHSV': [
  'static Color FromHSV(float h, float s, float v, float a = 1.0f)',
  'HSV（色相・彩度・明度）色空間から Color を生成する静的メンバ関数です。色相 h は 0〜360 度、彩度 s と明度 v は 0〜1 の範囲で指定します。色相環ベースの色選択やカラーパレット生成に便利です。',
  `// 色相120度 = 緑
GX::Color green = GX::Color::FromHSV(120.0f, 1.0f, 1.0f);

// レインボーカラー生成
for (int i = 0; i < 7; ++i) {
    GX::Color c = GX::Color::FromHSV(i * 51.4f, 1.0f, 1.0f);
}`,
  '• h: [0, 360)、s: [0, 1]、v: [0, 1]\n• h は 360 で自動ラップ'
],

'Color-White': [
  'static Color White()',
  '白色 (1, 1, 1, 1) を返す静的メンバ関数です。デフォルトカラーやテキスト色として頻繁に使用します。',
  `GX::Color c = GX::Color::White();`,
  '• デフォルトコンストラクタと同じ値'
],

'Color-Black': [
  'static Color Black()',
  '黒色 (0, 0, 0, 1) を返す静的メンバ関数です。背景色やクリアカラーとして使用します。アルファは 1.0（不透明）です。',
  `GX::Color c = GX::Color::Black();`,
  '• アルファは 1.0（不透明な黒）'
],

'Color-Red': [
  'static Color Red()',
  '赤色 (1, 0, 0, 1) を返す静的メンバ関数です。',
  `GX::Color c = GX::Color::Red();`,
  '• 純粋な赤 (R=1, G=0, B=0, A=1)'
],

'Color-Green': [
  'static Color Green()',
  '緑色 (0, 1, 0, 1) を返す静的メンバ関数です。',
  `GX::Color c = GX::Color::Green();`,
  '• 純粋な緑 (R=0, G=1, B=0, A=1)'
],

'Color-Blue': [
  'static Color Blue()',
  '青色 (0, 0, 1, 1) を返す静的メンバ関数です。',
  `GX::Color c = GX::Color::Blue();`,
  '• 純粋な青 (R=0, G=0, B=1, A=1)'
],

'Color-Yellow': [
  'static Color Yellow()',
  '黄色 (1, 1, 0, 1) を返す静的メンバ関数です。赤と緑の混色です。',
  `GX::Color c = GX::Color::Yellow();`,
  '• R=1, G=1, B=0, A=1'
],

'Color-Lerp': [
  'static Color Lerp(const Color& a, const Color& b, float t)',
  '2つの色間を線形補間します。t=0 のとき a、t=1 のとき b を返します。グラデーション表現やフェードイン/アウトに使用します。全成分（r, g, b, a）が個別に補間されます。',
  `GX::Color start = GX::Color::Red();
GX::Color end = GX::Color::Blue();
GX::Color mid = GX::Color::Lerp(start, end, 0.5f);
// mid ≈ (0.5, 0.0, 0.5, 1.0) 紫色`,
  '• 全成分 (RGBA) を個別に線形補間\n• HSV 空間の補間ではない（RGB空間）'
],

// ============================================================
// MathUtil (page-MathUtil)
// ============================================================

'MathUtil-PI': [
  'constexpr float PI = 3.14159265358979323846f',
  '円周率 (pi) の定数です。角度のラジアン変換やトリゴノメトリ計算に使用します。constexpr で定義されているためコンパイル時定数として使用可能です。',
  `float halfCircle = GX::MathUtil::PI; // 180度相当
float quarterCircle = GX::MathUtil::PI / 2.0f; // 90度相当`,
  '• constexpr float\n• PI/2 = 90度、PI/4 = 45度'
],

'MathUtil-TAU': [
  'constexpr float TAU = PI * 2.0f',
  '円周率の2倍 (2 * pi = tau) の定数です。完全な1回転を表します。回転角度の計算で 2*PI と書く代わりに使用できます。',
  `float fullRotation = GX::MathUtil::TAU; // 360度相当`,
  '• TAU = 2 * PI ≈ 6.283\n• 1回転の角度'
],

'MathUtil-EPSILON': [
  'constexpr float EPSILON = 1e-6f',
  '浮動小数点比較に使用する微小値です。float の丸め誤差を考慮した等値比較に使用します。ApproximatelyEqual のデフォルト許容誤差としても使用されます。',
  `float a = 0.1f + 0.2f;
float b = 0.3f;
bool close = std::abs(a - b) < GX::MathUtil::EPSILON;`,
  '• 1e-6f = 0.000001f\n• 浮動小数点の比較に使用'
],

'MathUtil-Clamp': [
  'inline float Clamp(float value, float minVal, float maxVal)',
  '値を指定した最小値と最大値の範囲にクランプします。value が minVal より小さければ minVal を、maxVal より大きければ maxVal を返します。int 版のオーバーロードもあります。',
  `float clamped = GX::MathUtil::Clamp(1.5f, 0.0f, 1.0f); // 1.0f
float clamped2 = GX::MathUtil::Clamp(-0.5f, 0.0f, 1.0f); // 0.0f

int iClamped = GX::MathUtil::Clamp(150, 0, 100); // 100`,
  '• float 版と int 版のオーバーロードあり\n• minVal <= maxVal であること'
],

'MathUtil-Lerp': [
  'inline float Lerp(float a, float b, float t)',
  '2つの float 値間を線形補間します。a + (b - a) * t の計算を行います。アニメーション、カラー補間、パラメータのスムーズ変化など幅広い用途に使用します。',
  `float mid = GX::MathUtil::Lerp(0.0f, 100.0f, 0.5f); // 50.0f
float quarter = GX::MathUtil::Lerp(0.0f, 100.0f, 0.25f); // 25.0f`,
  '• t をクランプしないため外挿も可能\n• ベクトル版は Vector2/3/4::Lerp を使用'
],

'MathUtil-InverseLerp': [
  'inline float InverseLerp(float a, float b, float value)',
  '逆線形補間。value が a〜b の範囲のどの位置にあるかを 0〜1 の補間係数 t として返します。Lerp の逆操作です。a == b の場合は 0 を返します。',
  `float t = GX::MathUtil::InverseLerp(0.0f, 100.0f, 50.0f); // 0.5f
float t2 = GX::MathUtil::InverseLerp(10.0f, 20.0f, 15.0f); // 0.5f`,
  '• Lerp の逆関数\n• a == b の場合は 0.0f を返す（ゼロ除算回避）'
],

'MathUtil-Remap': [
  'inline float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)',
  'ある範囲の値を別の範囲にリマップします。内部で InverseLerp と Lerp を組み合わせています。入力範囲と出力範囲が異なるパラメータの変換に使用します。',
  `// 0-100 の値を 0.0-1.0 に変換
float normalized = GX::MathUtil::Remap(75.0f, 0.0f, 100.0f, 0.0f, 1.0f); // 0.75f

// 温度: 摂氏→華氏
float fahrenheit = GX::MathUtil::Remap(100.0f, 0.0f, 100.0f, 32.0f, 212.0f); // 212.0f`,
  '• InverseLerp + Lerp の組み合わせ\n• 入力範囲外の値にも対応（クランプなし）'
],

'MathUtil-SmoothStep': [
  'inline float SmoothStep(float edge0, float edge1, float x)',
  'Hermite 補間によるスムーズステップ関数（3次多項式）です。edge0〜edge1 の範囲で 0〜1 に滑らかに変化する値を返します。線形補間より自然な遷移（イーズイン/アウト）が必要な場合に使用します。',
  `float t = GX::MathUtil::SmoothStep(0.0f, 1.0f, 0.5f);
// t ≈ 0.5f (中間点では Lerp と同じだが、端で緩やかに変化)`,
  '• 3次多項式: 3t^2 - 2t^3\n• 端点で1次微分が0になる\n• 汎用的なイージング関数'
],

'MathUtil-SmootherStep': [
  'inline float SmootherStep(float edge0, float edge1, float x)',
  'Ken Perlin の改良スムーズステップ関数（5次多項式）です。SmoothStep より滑らかな補間を提供します。端点で1次微分と2次微分がともに0になります。',
  `float t = GX::MathUtil::SmootherStep(0.0f, 1.0f, 0.5f);`,
  '• 5次多項式: 6t^5 - 15t^4 + 10t^3\n• SmoothStep より滑らか（2次微分も連続）\n• Perlin ノイズなどで使用'
],

'MathUtil-DegreesToRadians': [
  'inline float DegreesToRadians(float degrees)',
  '度数法（度）からラジアンに変換します。GXLib の回転関数はすべてラジアン入力のため、度数法で角度を指定する場合にこの関数で変換が必要です。',
  `float rad90 = GX::MathUtil::DegreesToRadians(90.0f);  // ≈ 1.5708f
float rad180 = GX::MathUtil::DegreesToRadians(180.0f); // ≈ 3.1416f`,
  '• degrees * (PI / 180)\n• 全回転関数がラジアン入力のため頻繁に使用'
],

'MathUtil-RadiansToDegrees': [
  'inline float RadiansToDegrees(float radians)',
  'ラジアンから度数法（度）に変換します。デバッグ表示や UI でのラジアン値の可読化に使用します。',
  `float deg = GX::MathUtil::RadiansToDegrees(GX::MathUtil::PI); // 180.0f`,
  '• radians * (180 / PI)'
],

'MathUtil-IsPowerOfTwo': [
  'inline bool IsPowerOfTwo(uint32_t x)',
  '指定した値が 2 の累乗かどうかを判定します。テクスチャサイズのバリデーションやメモリアライメントの確認に使用します。0 は false を返します。',
  `bool result = GX::MathUtil::IsPowerOfTwo(256);  // true
bool result2 = GX::MathUtil::IsPowerOfTwo(100); // false
bool result3 = GX::MathUtil::IsPowerOfTwo(0);   // false`,
  '• ビット演算 (x & (x-1)) == 0 による高速判定\n• 0 は false を返す'
],

'MathUtil-NextPowerOfTwo': [
  'inline uint32_t NextPowerOfTwo(uint32_t x)',
  '指定した値以上の最小の 2 の累乗を返します。テクスチャサイズの切り上げやバッファサイズの計算に使用します。入力が 0 の場合は 1 を返します。',
  `uint32_t n = GX::MathUtil::NextPowerOfTwo(100); // 128
uint32_t n2 = GX::MathUtil::NextPowerOfTwo(256); // 256
uint32_t n3 = GX::MathUtil::NextPowerOfTwo(0);   // 1`,
  '• 入力が既に2の累乗ならそのまま返す\n• ビットシフトによる高速計算'
],

'MathUtil-ApproximatelyEqual': [
  'inline bool ApproximatelyEqual(float a, float b, float epsilon = EPSILON)',
  '2つの float 値がほぼ等しいかを判定します。浮動小数点の丸め誤差を考慮した比較で、差の絶対値が epsilon 以下なら true を返します。デフォルトの epsilon は EPSILON (1e-6f) です。',
  `float a = 0.1f + 0.2f;
float b = 0.3f;
bool eq = GX::MathUtil::ApproximatelyEqual(a, b); // true

// カスタム許容誤差
bool eq2 = GX::MathUtil::ApproximatelyEqual(1.0f, 1.01f, 0.1f); // true`,
  '• abs(a - b) <= epsilon で判定\n• デフォルト epsilon = 1e-6f\n• float の == 演算子の代替として使用推奨'
],

// ============================================================
// Random (page-Random)
// ============================================================

'Random-ctor_default': [
  'Random()',
  'ランダムなシードで初期化するデフォルトコンストラクタです。内部で Mersenne Twister (std::mt19937) を使用します。毎回異なるシーケンスを生成します。',
  `GX::Random rng;
int value = rng.Int(1, 100);`,
  '• std::mt19937 ベース\n• 実行ごとに異なるシーケンス'
],

'Random-ctor_seed': [
  'explicit Random(uint32_t seed)',
  '指定したシード値で初期化するコンストラクタです。同じシードを指定すれば同じ乱数シーケンスが再現されます。テストやリプレイ機能の実装に便利です。',
  `GX::Random rng(12345);
int a = rng.Int(1, 100); // 毎回同じ値`,
  '• explicit 修飾されているため暗黙変換なし\n• 同じシード → 同じシーケンス（再現性）'
],

'Random-SetSeed': [
  'void SetSeed(uint32_t seed)',
  '乱数シードを再設定します。呼び出し後は指定したシードに基づく新しいシーケンスが開始されます。ゲームのリスタート時にシードをリセットする場合などに使用します。',
  `GX::Random rng;
rng.SetSeed(42);
int first = rng.Int(); // シード42からの最初の乱数`,
  '• 内部の mt19937 エンジンをシードで再初期化'
],

'Random-Int': [
  'int32_t Int() / int32_t Int(int32_t min, int32_t max)',
  'ランダムな整数を生成します。引数なし版は 32 ビットの乱数値を返します。min, max を指定する版は [min, max]（両端含む）の範囲の一様分布乱数を返します。',
  `GX::Random rng;
int raw = rng.Int();           // 任意の32ビット整数
int dice = rng.Int(1, 6);     // サイコロ: 1〜6
int damage = rng.Int(10, 50); // ダメージ: 10〜50`,
  '• 範囲指定版は [min, max] の閉区間\n• min <= max であること'
],

'Random-Float': [
  'float Float() / float Float(float min, float max)',
  'ランダムな浮動小数点数を生成します。引数なし版は [0.0, 1.0] の範囲の値を返します。min, max を指定する版は [min, max] の範囲の一様分布乱数を返します。',
  `GX::Random rng;
float normalized = rng.Float();          // 0.0〜1.0
float speed = rng.Float(1.0f, 10.0f);   // 速度: 1.0〜10.0
float angle = rng.Float(0.0f, GX::MathUtil::TAU); // 角度: 0〜2PI`,
  '• 引数なし版は [0.0, 1.0] の閉区間\n• 確率計算やパラメータのランダム化に便利'
],

'Random-Vector2InRange': [
  'Vector2 Vector2InRange(float minX, float maxX, float minY, float maxY)',
  '指定した矩形範囲内のランダムな 2D ベクトルを生成します。各軸の最小値・最大値を個別に指定できます。パーティクルの初期位置やランダムな 2D 座標の生成に使用します。',
  `GX::Random rng;
// 画面内のランダムな位置
GX::Vector2 pos = rng.Vector2InRange(0.0f, 1280.0f, 0.0f, 720.0f);`,
  '• 各軸が独立した一様分布'
],

'Random-PointInCircle': [
  'Vector2 PointInCircle(float radius = 1.0f)',
  '円の内部に均一分布するランダムな点を生成します。単純な角度+距離のランダムとは異なり、面積に基づく均一分布を使用するため中心部に偏りません。パーティクルの拡散や円形領域の敵配置に使用します。',
  `GX::Random rng;
GX::Vector2 p = rng.PointInCircle(50.0f); // 半径50の円内`,
  '• 均一分布（中心に偏らない）\n• デフォルト半径は 1.0f'
],

'Random-Vector3InRange': [
  'Vector3 Vector3InRange(float minX, float maxX, float minY, float maxY, float minZ, float maxZ)',
  '指定した直方体範囲内のランダムな 3D ベクトルを生成します。各軸の最小値・最大値を個別に指定できます。3D空間でのパーティクル生成やオブジェクト配置に使用します。',
  `GX::Random rng;
// 空間内のランダムな位置
GX::Vector3 pos = rng.Vector3InRange(-10.0f, 10.0f, 0.0f, 20.0f, -10.0f, 10.0f);`,
  '• 各軸が独立した一様分布'
],

'Random-PointInSphere': [
  'Vector3 PointInSphere(float radius = 1.0f)',
  '球の内部に均一分布するランダムな点を生成します。体積に基づく均一分布を使用するため中心部に偏りません。3D パーティクルの爆発エフェクトや球形領域の配置に使用します。',
  `GX::Random rng;
GX::Vector3 p = rng.PointInSphere(5.0f); // 半径5の球内`,
  '• 均一分布（中心に偏らない）\n• デフォルト半径は 1.0f'
],

'Random-Direction2D': [
  'Vector2 Direction2D()',
  'ランダムな方向の 2D 単位ベクトルを生成します。長さは常に 1.0 で、方向が均一に分布します。全方位に等確率で飛散するパーティクルの実装などに使用します。',
  `GX::Random rng;
GX::Vector2 dir = rng.Direction2D();
// dir.Length() ≈ 1.0f`,
  '• 長さ 1.0 の単位ベクトル\n• 全方向に均一分布'
],

'Random-Direction3D': [
  'Vector3 Direction3D()',
  'ランダムな方向の 3D 単位ベクトルを生成します。球面上に均一に分布する方向ベクトルを返します。3D パーティクルの放出方向やランダムな光線の生成に使用します。',
  `GX::Random rng;
GX::Vector3 dir = rng.Direction3D();
// dir.Length() ≈ 1.0f`,
  '• 球面上に均一分布\n• 長さ 1.0 の単位ベクトル'
],

'Random-RandomColor': [
  'Color RandomColor(float alpha = 1.0f)',
  'ランダムな色を生成します。RGB 各成分が 0.0〜1.0 の範囲でランダムに決定されます。アルファ値はパラメータで指定できます。デバッグ描画やパーティクルの色付けに便利です。',
  `GX::Random rng;
GX::Color c = rng.RandomColor();       // 不透明なランダム色
GX::Color c2 = rng.RandomColor(0.5f); // 半透明なランダム色`,
  '• RGB が個別にランダム\n• alpha のデフォルトは 1.0f'
],

'Random-Global': [
  'static Random& Global()',
  'グローバル共有の Random インスタンスへの参照を返す静的メンバ関数です。個別のインスタンスを管理する必要がない場合の簡易アクセス用です。スレッドセーフではないため、マルチスレッド環境ではスレッドごとにインスタンスを作成してください。',
  `int value = GX::Random::Global().Int(1, 100);
float f = GX::Random::Global().Float();
GX::Color c = GX::Random::Global().RandomColor();`,
  '• シングルトン的なグローバルインスタンス\n• スレッドセーフではない\n• マルチスレッド環境では個別インスタンスを推奨'
],

}
