{

// ============================================================
// Collision2D — 2D形状定義
// ============================================================

'Collision2D-AABB2D': [
  'struct AABB2D { Vector2 min, max; AABB2D(); AABB2D(const Vector2& min, const Vector2& max); AABB2D(float x, float y, float w, float h); Vector2 Center() const; Vector2 Size() const; Vector2 HalfSize() const; bool Contains(const Vector2& point) const; AABB2D Expand(float margin) const; AABB2D Merged(const AABB2D& other) const; float Area() const; };',
  '2D軸整列バウンディングボックス（AABB）を表す構造体。min/maxの2点で矩形領域を定義する。コンストラクタは2点指定と(x,y,w,h)指定の2種類がある。Center(), Size(), HalfSize()で幾何情報を取得でき、Contains()で点の包含判定、Expand()でマージン拡張、Merged()で他のAABBとの統合、Area()で面積計算が可能。\n\n【用語】AABB（Axis-Aligned Bounding Box）: 軸整列バウンディングボックス。各辺が座標軸に平行な矩形（2D）または直方体（3D）。回転を持たないため衝突判定が非常に高速で、ブロードフェーズの空間分割や粗い衝突チェックに広く使用される。',
  `// 2点指定で作成
GX::AABB2D box(GX::Vector2(0, 0), GX::Vector2(100, 80));

// x,y,w,h指定で作成
GX::AABB2D box2(50.0f, 50.0f, 200.0f, 150.0f);

// 情報取得
GX::Vector2 c = box.Center();  // (50, 40)
GX::Vector2 s = box.Size();    // (100, 80)
float area = box.Area();       // 8000

// 点の包含判定
bool inside = box.Contains(GX::Vector2(10, 20)); // true

// マージン拡張
GX::AABB2D expanded = box.Expand(5.0f);

// 2つのAABBを統合
GX::AABB2D merged = box.Merged(box2);`,
  '• min/maxはそれぞれ左上・右下の座標を表す\n• (x,y,w,h)コンストラクタではmin=(x,y), max=(x+w, y+h)となる\n• Expand()は各辺をmargin分だけ外側に広げた新しいAABBを返す\n• Merged()は2つのAABBを包含する最小のAABBを返す（和集合バウンディング）'
],

'Collision2D-Circle': [
  'struct Circle { Vector2 center; float radius; Circle(); Circle(const Vector2& c, float r); Circle(float x, float y, float r); bool Contains(const Vector2& point) const; };',
  '2D円形コライダーを表す構造体。中心座標と半径で定義する。Contains()メソッドで点が円内に含まれるかを判定できる。距離の二乗比較を使用するため高速に動作する。',
  `// 円を作成
GX::Circle c1(GX::Vector2(100, 100), 50.0f);
GX::Circle c2(200.0f, 300.0f, 30.0f);

// 点が円内にあるか判定
bool inside = c1.Contains(GX::Vector2(120, 110)); // true`,
  '• 内部ではDistanceSquared()を使用しsqrt計算を回避している\n• radius=0の場合は点として扱われる'
],

'Collision2D-Line2D': [
  'struct Line2D { Vector2 start, end; Line2D(); Line2D(const Vector2& s, const Vector2& e); float Length() const; Vector2 Direction() const; Vector2 ClosestPoint(const Vector2& point) const; };',
  '2D線分を表す構造体。始点と終点で定義する。Length()で長さ、Direction()で正規化方向ベクトルを取得できる。ClosestPoint()は指定した点に最も近い線分上の点を返す。パラメトリック射影をクランプすることで線分の範囲内に制限される。',
  `GX::Line2D line(GX::Vector2(0, 0), GX::Vector2(100, 0));

float len = line.Length();                  // 100.0
GX::Vector2 dir = line.Direction();         // (1, 0)

// 点 (50, 30) に最も近い線分上の点
GX::Vector2 cp = line.ClosestPoint(GX::Vector2(50, 30)); // (50, 0)`,
  '• ClosestPoint()はパラメータtを[0,1]にクランプするため、線分の端点を超えない\n• Direction()は正規化済みのベクトルを返す'
],

'Collision2D-Polygon2D': [
  'struct Polygon2D { std::vector<Vector2> vertices; bool Contains(const Vector2& point) const; AABB2D GetBounds() const; };',
  '2D多角形を表す構造体。頂点配列で形状を定義する。Contains()はWinding Number法で点が多角形内部にあるかを判定する。GetBounds()は多角形を包含する最小のAABBを返す。凹多角形にも対応している。\n\n【用語】Winding Number（巻き数）: 多角形の辺が点の周りを何回巻くかを数える手法。点が内部にあれば巻き数が0以外になる。Ray Casting法と異なり、自己交差する多角形や凹多角形でも正確に内外判定できる。',
  `GX::Polygon2D poly;
poly.vertices = {
    GX::Vector2(0, 0),
    GX::Vector2(100, 0),
    GX::Vector2(100, 100),
    GX::Vector2(0, 100)
};

bool inside = poly.Contains(GX::Vector2(50, 50)); // true
GX::AABB2D bounds = poly.GetBounds();`,
  '• Winding Number法を使用するため凹多角形でも正しく判定できる\n• 頂点は反時計回り・時計回りどちらでもよい\n• GetBounds()は全頂点のmin/maxから計算される'
],

'Collision2D-HitResult2D': [
  'struct HitResult2D { bool hit; Vector2 point; Vector2 normal; float depth; operator bool() const; };',
  '2D衝突判定の詳細結果を格納する構造体。衝突の有無（hit）、衝突点（point）、衝突法線（normal）、めり込み深さ（depth）を保持する。operator boolが定義されているためif文で直接判定可能。Intersect系関数の戻り値として使用される。',
  `GX::AABB2D a(GX::Vector2(0,0), GX::Vector2(50,50));
GX::AABB2D b(GX::Vector2(40,40), GX::Vector2(90,90));

GX::HitResult2D result = GX::Collision2D::IntersectAABBvsAABB(a, b);
if (result) {
    // 衝突あり
    GX::Vector2 pushOut = result.normal * result.depth;
}`,
  '• operator bool()によりif(result)で衝突判定可能\n• depthはめり込み量を正の値で表す\n• normalはAからBへの分離方向を示す'
],

// ============================================================
// Collision2D — 衝突判定関数
// ============================================================

'Collision2D-AABBvsAABB': [
  'bool Collision2D::TestAABBvsAABB(const AABB2D& a, const AABB2D& b)',
  '2つのAABB同士の重なり判定を行う。各軸で分離軸テストを行い、全軸で重なっている場合にtrueを返す。最も高速な2D衝突判定の一つで、ブロードフェーズにも頻繁に使用される。',
  `GX::AABB2D a(GX::Vector2(0,0), GX::Vector2(50,50));
GX::AABB2D b(GX::Vector2(40,40), GX::Vector2(90,90));

if (GX::Collision2D::TestAABBvsAABB(a, b)) {
    // 衝突している
}`,
  '• 分離軸テスト（SAT）の最も単純な形式（X軸・Y軸の2軸のみ）\n• ブロードフェーズの衝突判定に最適\n• 詳細な衝突情報が必要な場合はIntersectAABBvsAABBを使用する\n\n【用語】SAT（Separating Axis Theorem / 分離軸定理）: 2つの凸形状が衝突しないなら、両者を分離する軸が必ず存在するという定理。全候補軸で射影の重なりをチェックし、1つでも分離軸が見つかれば非衝突と判定する。'
],

'Collision2D-CirclevsCircle': [
  'bool Collision2D::TestCirclevsCircle(const Circle& a, const Circle& b)',
  '2つの円同士の重なり判定を行う。中心間距離の二乗と半径の和の二乗を比較することで、sqrtを回避して高速に判定する。',
  `GX::Circle a(100.0f, 100.0f, 30.0f);
GX::Circle b(140.0f, 100.0f, 25.0f);

if (GX::Collision2D::TestCirclevsCircle(a, b)) {
    // 衝突している（距離40 < 半径合計55）
}`,
  '• 内部ではDistanceSquaredを使用しsqrt計算を回避\n• 詳細な衝突情報が必要な場合はIntersectCirclevsCircleを使用する'
],

'Collision2D-AABBvsCircle': [
  'bool Collision2D::TestAABBvsCircle(const AABB2D& aabb, const Circle& circle)',
  'AABBと円の重なり判定を行う。AABB上の最近接点と円の中心の距離を計算し、半径以内であればtrueを返す。矩形と円形のコライダーが混在するシーンで使用する。',
  `GX::AABB2D box(GX::Vector2(0,0), GX::Vector2(60,60));
GX::Circle circle(70.0f, 30.0f, 20.0f);

if (GX::Collision2D::TestAABBvsCircle(box, circle)) {
    // 衝突している
}`,
  '• AABB上の最近接点を求めてから距離判定する\n• 円がAABBの内部に完全に含まれる場合もtrueを返す'
],

'Collision2D-PointInAABB': [
  'bool Collision2D::TestPointInAABB(const Vector2& point, const AABB2D& aabb)',
  '点がAABB内に含まれるかを判定する。各軸のmin/max範囲内にあるかをチェックする。マウスカーソルのUI当たり判定やピッキングに使用する。',
  `GX::Vector2 mousePos(150.0f, 200.0f);
GX::AABB2D button(100.0f, 180.0f, 120.0f, 40.0f);

if (GX::Collision2D::TestPointInAABB(mousePos, button)) {
    // マウスがボタン領域内にある
}`,
  '• AABB2D::Contains()と同等の機能\n• 境界上の点もtrue（inclusive判定）'
],

'Collision2D-PointInCircle': [
  'bool Collision2D::TestPointInCircle(const Vector2& point, const Circle& circle)',
  '点が円内に含まれるかを判定する。点と中心の距離の二乗が半径の二乗以下であればtrueを返す。sqrtを使わない高速な判定で、円形のクリック判定等に使用する。',
  `GX::Vector2 clickPos(105.0f, 98.0f);
GX::Circle area(100.0f, 100.0f, 50.0f);

if (GX::Collision2D::TestPointInCircle(clickPos, area)) {
    // クリック位置が円形領域内
}`,
  '• Circle::Contains()と同等の機能\n• DistanceSquared比較によりsqrtを回避'
],

'Collision2D-PointInPolygon': [
  'bool Collision2D::TestPointInPolygon(const Vector2& point, const Polygon2D& polygon)',
  '点が多角形内に含まれるかをWinding Number法で判定する。凹多角形にも正しく対応する。複雑な形状のクリック判定や領域判定に使用する。',
  `GX::Polygon2D tri;
tri.vertices = {
    GX::Vector2(0, 0),
    GX::Vector2(200, 0),
    GX::Vector2(100, 150)
};

GX::Vector2 p(100.0f, 50.0f);
if (GX::Collision2D::TestPointInPolygon(p, tri)) {
    // 点が三角形内にある
}`,
  '• Winding Number法を使用（Ray Casting法より凹多角形に強い）\n• 頂点の巻き方向は問わない\n• 頂点数が多いと計算コストが増加する'
],

'Collision2D-LinevsAABB': [
  'bool Collision2D::TestLinevsAABB(const Line2D& line, const AABB2D& aabb)',
  '線分とAABBの交差判定を行う。線分がAABBの辺を横切る場合にtrueを返す。弾丸の軌跡判定やレーザーの遮蔽判定などに使用する。',
  `GX::Line2D laser(GX::Vector2(0, 50), GX::Vector2(200, 50));
GX::AABB2D wall(GX::Vector2(80, 0), GX::Vector2(120, 100));

if (GX::Collision2D::TestLinevsAABB(laser, wall)) {
    // レーザーが壁に当たっている
}`,
  '• パラメトリック交差判定を使用\n• 線分の両端がAABB内の場合もtrueを返す'
],

'Collision2D-LinevsCircle': [
  'bool Collision2D::TestLinevsCircle(const Line2D& line, const Circle& circle)',
  '線分と円の交差判定を行う。線分上の最近接点と円の中心の距離で判定する。弾丸と円形コライダーの当たり判定に使用する。',
  `GX::Line2D bullet(GX::Vector2(0, 100), GX::Vector2(300, 100));
GX::Circle enemy(150.0f, 100.0f, 20.0f);

if (GX::Collision2D::TestLinevsCircle(bullet, enemy)) {
    // 弾丸が敵に当たった
}`,
  '• 線分上の最近接点を計算してから距離判定する\n• 線分が円内を貫通する場合もtrue'
],

'Collision2D-LinevsLine': [
  'bool Collision2D::TestLinevsLine(const Line2D& a, const Line2D& b, Vector2* outPoint = nullptr)',
  '2つの線分の交差判定を行う。outPointを指定すると交差点を出力する。パラメトリック交差判定で、両線分のパラメータが[0,1]範囲内の場合のみtrueを返す。壁の遮蔽判定やレイキャストの基礎に使用する。',
  `GX::Line2D lineA(GX::Vector2(0, 0), GX::Vector2(100, 100));
GX::Line2D lineB(GX::Vector2(0, 100), GX::Vector2(100, 0));

GX::Vector2 crossPoint;
if (GX::Collision2D::TestLinevsLine(lineA, lineB, &crossPoint)) {
    // crossPoint = (50, 50)
}`,
  '• outPointはnullptrで省略可能\n• 平行な線分の場合はfalseを返す\n• パラメータtが[0,1]範囲外の場合は交差なし（線分の延長上で交わる場合）'
],

'Collision2D-Intersect_AABBvsAABB': [
  'HitResult2D Collision2D::IntersectAABBvsAABB(const AABB2D& a, const AABB2D& b)',
  'AABB同士の詳細な衝突判定を行い、衝突点・法線・めり込み深さを含むHitResult2Dを返す。衝突応答（押し出し処理）に必要な情報を一度に取得できる。最小めり込み軸を分離法線として選択する。',
  `GX::AABB2D player(GX::Vector2(90, 90), GX::Vector2(110, 110));
GX::AABB2D wall(GX::Vector2(100, 80), GX::Vector2(200, 120));

GX::HitResult2D hit = GX::Collision2D::IntersectAABBvsAABB(player, wall);
if (hit) {
    // 押し出し処理
    GX::Vector2 pushOut = hit.normal * hit.depth;
}`,
  '• normalは最小めり込み軸の方向（AからBへの分離方向）\n• depthは正の値でめり込み量を表す\n• pointは衝突面の中心付近'
],

'Collision2D-Intersect_CirclevsCircle': [
  'HitResult2D Collision2D::IntersectCirclevsCircle(const Circle& a, const Circle& b)',
  '円同士の詳細な衝突判定を行い、衝突点・法線・めり込み深さを返す。法線は中心間方向、pointは衝突面上の点となる。円同士の物理衝突応答に使用する。',
  `GX::Circle ball1(100.0f, 100.0f, 20.0f);
GX::Circle ball2(130.0f, 100.0f, 20.0f);

GX::HitResult2D hit = GX::Collision2D::IntersectCirclevsCircle(ball1, ball2);
if (hit) {
    // depth = 10 (合計半径40 - 距離30)
    // normal = (1, 0) (ball1→ball2方向)
}`,
  '• normalはAの中心からBの中心へ向かう正規化ベクトル\n• depthは(radiusA + radiusB) - 中心間距離\n• 中心が完全に一致する場合のゼロ除算に注意'
],

'Collision2D-Intersect_AABBvsCircle': [
  'HitResult2D Collision2D::IntersectAABBvsCircle(const AABB2D& aabb, const Circle& circle)',
  'AABBと円の詳細な衝突判定を行い、衝突情報を返す。AABB上の最近接点から法線・めり込み深さを計算する。矩形と円形の混合コライダーの衝突応答に使用する。',
  `GX::AABB2D platform(GX::Vector2(0, 200), GX::Vector2(300, 220));
GX::Circle ball(150.0f, 195.0f, 10.0f);

GX::HitResult2D hit = GX::Collision2D::IntersectAABBvsCircle(platform, ball);
if (hit) {
    // ボールをプラットフォームの上に押し出す
    ball.center += hit.normal * hit.depth;
}`,
  '• AABB上の最近接点と円中心の距離から法線・深さを計算\n• 円の中心がAABB内部にある場合も正しく処理する'
],

'Collision2D-Raycast2D_AABB': [
  'bool Collision2D::Raycast2D(const Vector2& origin, const Vector2& direction, const AABB2D& aabb, float& outT, Vector2* outNormal = nullptr)',
  'レイ（半直線）とAABBのレイキャスト判定を行う。ヒットした場合outTにレイのパラメータ値、outNormalにヒット面の法線を出力する。ヒット点はorigin + direction * outTで計算できる。シューティングゲームの弾道判定やライン・オブ・サイト判定に使用する。',
  `GX::Vector2 origin(0, 50);
GX::Vector2 dir(1, 0); // 右方向

GX::AABB2D wall(GX::Vector2(100, 0), GX::Vector2(120, 100));

float t;
GX::Vector2 normal;
if (GX::Collision2D::Raycast2D(origin, dir, wall, t, &normal)) {
    GX::Vector2 hitPoint = origin + dir * t;
    // hitPoint = (100, 50), normal = (-1, 0)
}`,
  '• directionは正規化推奨（outTが実際の距離に対応する）\n• outNormalはnullptrで省略可能\n• outTはレイの始点からヒット点までのパラメータ値\n• レイの始点がAABB内部の場合の挙動に注意'
],

'Collision2D-Raycast2D_Circle': [
  'bool Collision2D::Raycast2D(const Vector2& origin, const Vector2& direction, const Circle& circle, float& outT)',
  'レイと円のレイキャスト判定を行う。ヒットした場合outTにレイのパラメータ値を出力する。二次方程式の判別式で交差を判定し、最も近い正のtを返す。',
  `GX::Vector2 origin(0, 100);
GX::Vector2 dir(1, 0);

GX::Circle target(200.0f, 100.0f, 30.0f);

float t;
if (GX::Collision2D::Raycast2D(origin, dir, target, t)) {
    GX::Vector2 hitPoint = origin + dir * t;
    // hitPoint = (170, 100)  (円の手前側)
}`,
  '• 二次方程式の判別式でレイと円の交差を判定\n• 最も近い正のt（レイの進行方向側）を返す\n• レイの始点が円内部の場合もヒットする'
],

'Collision2D-SweepCirclevsCircle': [
  'bool Collision2D::SweepCirclevsCircle(const Circle& a, const Vector2& velA, const Circle& b, const Vector2& velB, float& outT)',
  '2つの移動する円のスイープ判定を行う。相対速度でレイキャストに帰着させ、最初に接触する時刻tを[0,1]範囲で返す。高速移動する弾丸と敵の衝突を漏れなく検出する連続衝突判定（CCD）に使用する。\n\n【用語】CCD（Continuous Collision Detection / 連続衝突判定）: フレーム間の移動軌跡全体で衝突を検出する手法。通常の離散判定では高速オブジェクトが障害物をすり抜ける「トンネリング」が発生するが、CCDはこれを防止する。',
  `GX::Circle bullet(0.0f, 100.0f, 5.0f);
GX::Vector2 bulletVel(500.0f, 0.0f);

GX::Circle enemy(200.0f, 100.0f, 20.0f);
GX::Vector2 enemyVel(0.0f, 0.0f);

float t;
if (GX::Collision2D::SweepCirclevsCircle(bullet, bulletVel, enemy, enemyVel, t)) {
    // t = 接触時刻 (0~1)
    GX::Vector2 contactPos = bullet.center + bulletVel * t;
}`,
  '• 相対速度を用いて静止円に対するレイキャストに変換\n• outTは[0,1]範囲で返される（0=フレーム開始、1=フレーム終了）\n• 高速移動オブジェクトのトンネリング防止に有効\n• 両方の速度を考慮するため、双方が動いていても正しく判定できる'
],

// ============================================================
// Collision3D — 3D形状定義
// ============================================================

'Collision3D-AABB3D': [
  'struct AABB3D { Vector3 min, max; AABB3D(); AABB3D(const Vector3& min, const Vector3& max); Vector3 Center() const; Vector3 Size() const; Vector3 HalfExtents() const; bool Contains(const Vector3& point) const; AABB3D Expand(float margin) const; AABB3D Merged(const AABB3D& other) const; float Volume() const; float SurfaceArea() const; };',
  '3D軸整列バウンディングボックスを表す構造体。min/maxの2点で直方体領域を定義する。HalfExtents()で各軸の半サイズ、Volume()で体積、SurfaceArea()で表面積を取得できる。BVHのSAHコスト計算にSurfaceArea()が使用される。Expand()とMerged()で空間的な操作が可能。',
  `GX::AABB3D box(GX::Vector3(-1,-1,-1), GX::Vector3(1,1,1));

GX::Vector3 center = box.Center();       // (0,0,0)
GX::Vector3 half = box.HalfExtents();    // (1,1,1)
float vol = box.Volume();                // 8.0
float sa = box.SurfaceArea();            // 24.0

bool inside = box.Contains(GX::Vector3(0.5f, 0.5f, 0.5f)); // true

GX::AABB3D bigger = box.Expand(0.5f);   // (-1.5,-1.5,-1.5) ~ (1.5,1.5,1.5)`,
  '• SurfaceArea()はBVH構築時のSAHコスト計算に使用される\n• Merged()は2つのAABBを包含する最小のAABBを返す\n• Expand()は全方向にmargin分拡張する'
],

'Collision3D-Sphere': [
  'struct Sphere { Vector3 center; float radius; Sphere(); Sphere(const Vector3& c, float r); bool Contains(const Vector3& point) const; };',
  '3D球体を表す構造体。中心座標と半径で定義する。Contains()で点の包含判定が可能。距離の二乗比較で高速に判定する。バウンディングスフィアやコライダーとして広く使用される。',
  `GX::Sphere s(GX::Vector3(0, 5, 0), 3.0f);

bool inside = s.Contains(GX::Vector3(1, 5, 1)); // true (距離sqrt(2) < 3)`,
  '• DistanceSquared比較によりsqrtを回避\n• フラスタムカリングのバウンディングボリュームとしても使用可能'
],

'Collision3D-Ray': [
  'struct Ray { Vector3 origin; Vector3 direction; Ray(); Ray(const Vector3& o, const Vector3& d); Vector3 GetPoint(float t) const; };',
  '3Dレイ（半直線）を表す構造体。始点と方向ベクトルで定義する。GetPoint(t)でレイ上の任意の点を取得できる。レイキャスト関数の入力として使用する。',
  `GX::Ray ray(GX::Vector3(0, 10, 0), GX::Vector3(0, -1, 0));

GX::Vector3 p = ray.GetPoint(5.0f); // (0, 5, 0)`,
  '• directionは正規化推奨（tが実際の距離に対応する）\n• GetPoint()はorigin + direction * tを返す'
],

'Collision3D-OBB': [
  'struct OBB { Vector3 center; Vector3 halfExtents; Vector3 axes[3]; OBB(); OBB(const Vector3& center, const Vector3& halfExtents, const Matrix4x4& rotation); };',
  '有向バウンディングボックス（OBB）を表す構造体。中心、各軸の半サイズ、3つの正規直交軸で定義する。回転行列からコンストラクタで軸を抽出できる。AABBでは対応できない回転したオブジェクトのタイトなバウンディングに使用する。\n\n【用語】OBB（Oriented Bounding Box）: 有向バウンディングボックス。AABBと異なり任意の回転を持つ直方体。回転したオブジェクトをより密に囲めるが、衝突判定コストはAABBより高い（15軸のSATが必要）。',
  `GX::Matrix4x4 rot;
// 45度Y軸回転行列を設定...
GX::OBB obb(GX::Vector3(0, 0, 0), GX::Vector3(2, 1, 3), rot);

// OBB同士の衝突判定
GX::OBB obb2(GX::Vector3(3, 0, 0), GX::Vector3(1, 1, 1), GX::Matrix4x4::Identity());
bool hit = GX::Collision3D::TestOBBVsOBB(obb, obb2);`,
  '• axes[3]は3つの正規直交軸（ローカルX,Y,Z軸の回転後の方向）\n• OBB vs OBBはSAT 15軸テスト（各OBBの3軸 + 外積9軸）\n• AABBより計算コストが高いが、回転オブジェクトにフィットする'
],

'Collision3D-Frustum': [
  'struct Frustum { Plane planes[6]; static Frustum FromViewProjection(const XMMATRIX& viewProj); };',
  '視錐台（フラスタム）を表す構造体。6つの平面（Near, Far, Left, Right, Top, Bottom）で定義する。FromViewProjection()でビュー・プロジェクション行列から自動生成できる。フラスタムカリングに使用し、描画不要なオブジェクトを除外する。',
  `// カメラのViewProjection行列からフラスタムを生成
XMMATRIX viewProj = XMLoadFloat4x4(&camera.GetViewProjectionMatrix());
GX::Frustum frustum = GX::Frustum::FromViewProjection(viewProj);

// 球がフラスタム内にあるか判定
GX::Sphere bounding(GX::Vector3(10, 0, 30), 5.0f);
if (GX::Collision3D::TestFrustumVsSphere(frustum, bounding)) {
    // 描画対象
}`,
  '• planes[6]はNear, Far, Left, Right, Top, Bottomの順\n• FromViewProjection()はビュー・プロジェクション行列の行から平面を抽出する\n• フラスタムカリングのブロードフェーズとして使用'
],

'Collision3D-Plane': [
  'struct Plane { Vector3 normal; float distance; Plane(); Plane(const Vector3& n, float d); Plane(const Vector3& n, const Vector3& point); float DistanceToPoint(const Vector3& point) const; };',
  '3D平面を表す構造体。法線ベクトルと原点からの距離で定義する。点と法線からも構築可能。DistanceToPoint()で点から平面までの符号付き距離を返す。正の値は法線側、負の値は裏側を意味する。',
  `// 法線+距離で定義
GX::Plane ground(GX::Vector3(0, 1, 0), 0.0f); // Y=0平面

// 法線+点で定義
GX::Plane wall(GX::Vector3(1, 0, 0), GX::Vector3(5, 0, 0)); // X=5平面

float dist = ground.DistanceToPoint(GX::Vector3(0, 10, 0)); // 10.0`,
  '• normal.Dot(point) - distance で符号付き距離を計算\n• 正の距離=法線方向側、負の距離=裏側\n• Frustumの各面をPlaneで表現する'
],

'Collision3D-Triangle': [
  'struct Triangle { Vector3 v0, v1, v2; Triangle(); Triangle(const Vector3& a, const Vector3& b, const Vector3& c); Vector3 Normal() const; };',
  '3D三角形を表す構造体。3頂点で定義する。Normal()は(v1-v0)×(v2-v0)の正規化ベクトルを返す。レイキャストやメッシュ衝突判定の基本プリミティブとして使用する。',
  `GX::Triangle tri(
    GX::Vector3(0, 0, 0),
    GX::Vector3(1, 0, 0),
    GX::Vector3(0, 1, 0)
);

GX::Vector3 n = tri.Normal(); // (0, 0, 1) (Z正方向)`,
  '• Normal()は外積による法線計算（反時計回りで正方向）\n• Moller-Trumboreアルゴリズムでレイとの交差判定に使用'
],

'Collision3D-HitResult3D': [
  'struct HitResult3D { bool hit; Vector3 point; Vector3 normal; float depth; float t; operator bool() const; };',
  '3D衝突判定の詳細結果を格納する構造体。衝突の有無（hit）、衝突点（point）、衝突法線（normal）、めり込み深さ（depth）、レイキャストパラメータ（t）を保持する。operator boolが定義されているためif文で直接判定可能。',
  `GX::Sphere a(GX::Vector3(0,0,0), 2.0f);
GX::Sphere b(GX::Vector3(3,0,0), 2.0f);

GX::HitResult3D result = GX::Collision3D::IntersectSphereVsSphere(a, b);
if (result) {
    // depth = 1.0 (半径合計4 - 距離3)
    GX::Vector3 pushOut = result.normal * result.depth;
}`,
  '• operator bool()によりif(result)で衝突判定可能\n• tはレイキャスト結果のパラメータ値（Intersect系では未使用の場合がある）\n• depthはめり込み量を正の値で表す'
],

// ============================================================
// Collision3D — 衝突判定関数
// ============================================================

'Collision3D-SphereVsSphere': [
  'bool Collision3D::TestSphereVsSphere(const Sphere& a, const Sphere& b)',
  '2つの球体の重なり判定を行う。中心間距離の二乗と半径の和の二乗を比較し、sqrtを回避して高速に判定する。3Dゲームで最も基本的な衝突判定の一つ。',
  `GX::Sphere a(GX::Vector3(0,0,0), 3.0f);
GX::Sphere b(GX::Vector3(4,0,0), 2.0f);

if (GX::Collision3D::TestSphereVsSphere(a, b)) {
    // 衝突（距離4 < 半径合計5）
}`,
  '• DistanceSquared比較によりsqrt計算を回避\n• 詳細情報が必要な場合はIntersectSphereVsSphereを使用'
],

'Collision3D-AABBVsAABB': [
  'bool Collision3D::TestAABBVsAABB(const AABB3D& a, const AABB3D& b)',
  '2つの3D AABB同士の重なり判定を行う。3軸すべてで重なりがある場合にtrueを返す。ブロードフェーズの衝突判定やOctree/BVHの内部判定に使用される。',
  `GX::AABB3D a(GX::Vector3(-1,-1,-1), GX::Vector3(1,1,1));
GX::AABB3D b(GX::Vector3(0,0,0), GX::Vector3(2,2,2));

if (GX::Collision3D::TestAABBVsAABB(a, b)) {
    // 衝突（全軸で重なりあり）
}`,
  '• 分離軸テスト（SAT）の3D版\n• ブロードフェーズに最適な高速判定\n• Octree/BVHのQuery内部でも使用される'
],

'Collision3D-SphereVsAABB': [
  'bool Collision3D::TestSphereVsAABB(const Sphere& sphere, const AABB3D& aabb)',
  '球体とAABBの重なり判定を行う。AABB上の最近接点と球の中心の距離で判定する。球形コライダーと矩形障害物の衝突判定に使用する。',
  `GX::Sphere ball(GX::Vector3(3, 0, 0), 1.5f);
GX::AABB3D box(GX::Vector3(0, -1, -1), GX::Vector3(2, 1, 1));

if (GX::Collision3D::TestSphereVsAABB(ball, box)) {
    // 衝突（AABB上の最近接点(2,0,0)と球中心の距離1 < 半径1.5）
}`,
  '• ClosestPointOnAABBで最近接点を求めてから距離判定\n• 球がAABB内部に完全に含まれる場合もtrue'
],

'Collision3D-OBBVsOBB': [
  'bool Collision3D::TestOBBVsOBB(const OBB& a, const OBB& b)',
  '2つのOBB同士の重なり判定を行う。SAT（分離軸テスト）で15軸をテストする。各OBBの3軸（計6軸）と外積による9軸の合計15軸で分離を確認する。回転したオブジェクト同士の精密な衝突判定に使用する。',
  `GX::Matrix4x4 rotA, rotB;
// rotAに45度Y回転、rotBに30度Z回転を設定...

GX::OBB a(GX::Vector3(0,0,0), GX::Vector3(2,1,1), rotA);
GX::OBB b(GX::Vector3(3,0,0), GX::Vector3(1,1,1), rotB);

if (GX::Collision3D::TestOBBVsOBB(a, b)) {
    // 回転したボックス同士が衝突
}`,
  '• SAT 15軸テスト（3+3+9軸）で正確な判定\n• AABBvsAABBより計算コストが高い\n• 外積軸で平行なエッジ間のすき間も検出する'
],

'Collision3D-FrustumVsSphere': [
  'bool Collision3D::TestFrustumVsSphere(const Frustum& frustum, const Sphere& sphere)',
  'フラスタムと球体の包含/交差判定を行う。球がフラスタムの6平面すべてに対して内側にある場合にtrueを返す。フラスタムカリングで球状バウンディングボリュームを持つオブジェクトのカリングに使用する。',
  `GX::Frustum frustum = GX::Frustum::FromViewProjection(viewProj);
GX::Sphere bv(GX::Vector3(10, 5, 30), 3.0f);

if (GX::Collision3D::TestFrustumVsSphere(frustum, bv)) {
    // カメラに映る可能性がある → 描画する
}`,
  '• 各平面のDistanceToPointで球の中心をテスト（半径分のマージンを考慮）\n• 偽陽性の可能性があるが、偽陰性はない（保守的判定）\n• フラスタムカリングのブロードフェーズとして使用'
],

'Collision3D-FrustumVsAABB': [
  'bool Collision3D::TestFrustumVsAABB(const Frustum& frustum, const AABB3D& aabb)',
  'フラスタムとAABBの包含/交差判定を行う。AABBの8頂点のうち少なくとも1つがフラスタム内にあるかをテストする。Octreeのフラスタムクエリ内部でも使用され、ノード単位のカリングを行う。',
  `GX::Frustum frustum = GX::Frustum::FromViewProjection(viewProj);
GX::AABB3D buildingBounds(GX::Vector3(-5, 0, 20), GX::Vector3(5, 10, 30));

if (GX::Collision3D::TestFrustumVsAABB(frustum, buildingBounds)) {
    // 建物が視界内 → 描画する
}`,
  '• 各平面に対してAABBの最遠頂点（P頂点）をテスト\n• Octree::QueryNodeFrustum内部でも使用される\n• FrustumVsSphereと組み合わせた階層的カリングが効果的'
],

'Collision3D-FrustumVsOBB': [
  'bool Collision3D::TestFrustumVsPoint(const Frustum& frustum, const Vector3& point)',
  'フラスタムと点の包含判定を行う。点がフラスタムの6平面すべてに対して内側にある場合にtrueを返す。パーティクルや小さなオブジェクトのフラスタムカリングに使用する。',
  `GX::Frustum frustum = GX::Frustum::FromViewProjection(viewProj);
GX::Vector3 particlePos(5.0f, 2.0f, 15.0f);

if (GX::Collision3D::TestFrustumVsPoint(frustum, particlePos)) {
    // パーティクルが視界内にある
}`,
  '• 全6平面のDistanceToPointが正であればtrue\n• 最も軽量なフラスタムカリング判定\n• バウンディングボリュームを持たない点オブジェクト用'
],

'Collision3D-Raycast_Sphere': [
  'bool Collision3D::RaycastSphere(const Ray& ray, const Sphere& sphere, float& outT)',
  'レイと球体のレイキャスト判定を行う。ヒットした場合、outTにレイの始点からヒット点までのパラメータ値を出力する。二次方程式の判別式で交差を判定し、最も近い正のtを返す。',
  `GX::Ray ray(GX::Vector3(0, 0, -10), GX::Vector3(0, 0, 1));
GX::Sphere target(GX::Vector3(0, 0, 0), 3.0f);

float t;
if (GX::Collision3D::RaycastSphere(ray, target, t)) {
    GX::Vector3 hitPoint = ray.GetPoint(t); // (0, 0, -3)
}`,
  '• 二次方程式の判別式で交差を判定\n• 最も近い正のtを返す（レイの進行方向側）\n• レイの始点が球内部の場合もヒットする'
],

'Collision3D-Raycast_AABB': [
  'bool Collision3D::RaycastAABB(const Ray& ray, const AABB3D& aabb, float& outT)',
  'レイとAABBのレイキャスト判定を行う。スラブ法（Slab Method）で各軸の交差区間を計算し、全軸の交差区間が重なる場合にヒットとする。BVHのRaycast内部でも使用される。',
  `GX::Ray ray(GX::Vector3(-5, 0.5f, 0.5f), GX::Vector3(1, 0, 0));
GX::AABB3D box(GX::Vector3(0, 0, 0), GX::Vector3(2, 2, 2));

float t;
if (GX::Collision3D::RaycastAABB(ray, box, t)) {
    GX::Vector3 hitPoint = ray.GetPoint(t); // (0, 0.5, 0.5)
}`,
  '• スラブ法（各軸のtmin/tmaxを計算して交差区間を求める）\n• BVH::RaycastNode内部でバウンディングボックスとのヒット判定に使用\n• 方向ベクトルの成分が0の場合も正しく処理する'
],

'Collision3D-Raycast_Triangle': [
  'bool Collision3D::RaycastTriangle(const Ray& ray, const Triangle& tri, float& outT, float& outU, float& outV)',
  'レイと三角形のレイキャスト判定を行う。Moller-Trumboreアルゴリズムで高速に計算する。outU, outVには重心座標を出力し、テクスチャ座標の補間等に利用できる。メッシュのピッキングやレイトレースに使用する。\n\n【用語】Moller-Trumboreアルゴリズム: レイと三角形の交差判定の高速アルゴリズム。クラメルの公式を利用して交差点のパラメータt（レイ上の距離）と重心座標(u,v)を同時に求める。前計算不要で三角形メッシュとのレイキャストに広く使われる。',
  `GX::Triangle tri(
    GX::Vector3(0, 0, 0),
    GX::Vector3(2, 0, 0),
    GX::Vector3(1, 2, 0)
);
GX::Ray ray(GX::Vector3(1, 0.5f, -5), GX::Vector3(0, 0, 1));

float t, u, v;
if (GX::Collision3D::RaycastTriangle(ray, tri, t, u, v)) {
    GX::Vector3 hitPoint = ray.GetPoint(t);
    // u, v: 重心座標 (テクスチャ補間等に使用)
}`,
  '• Moller-Trumboreアルゴリズムによる高速なレイ・三角形交差判定\n• u, vは重心座標（barycentric coordinates）で、ヒット点 = (1-u-v)*v0 + u*v1 + v*v2\n• メッシュのポリゴン単位のピッキングに使用'
],

'Collision3D-Raycast_OBB': [
  'bool Collision3D::RaycastOBB(const Ray& ray, const OBB& obb, float& outT)',
  'レイとOBBのレイキャスト判定を行う。レイをOBBのローカル空間に変換してスラブ法で判定する。回転したオブジェクトへのレイキャストに使用する。',
  `GX::Matrix4x4 rot; // 回転行列を設定...
GX::OBB box(GX::Vector3(5, 0, 0), GX::Vector3(1, 1, 1), rot);
GX::Ray ray(GX::Vector3(0, 0, 0), GX::Vector3(1, 0, 0));

float t;
if (GX::Collision3D::RaycastOBB(ray, box, t)) {
    GX::Vector3 hitPoint = ray.GetPoint(t);
}`,
  '• レイをOBBのローカル空間に変換してからスラブ法を適用\n• RaycastAABBをOBBローカル空間で実行する形'
],

'Collision3D-ClosestPointOnAABB': [
  'Vector3 Collision3D::ClosestPointOnAABB(const Vector3& point, const AABB3D& aabb)',
  'AABB上の指定した点に最も近い点を返す。各軸でmin/max範囲にクランプすることで計算する。SphereVsAABB判定の内部でも使用される基本的なユーティリティ関数。',
  `GX::AABB3D box(GX::Vector3(0, 0, 0), GX::Vector3(2, 2, 2));
GX::Vector3 p(3.0f, 1.0f, 1.0f);

GX::Vector3 closest = GX::Collision3D::ClosestPointOnAABB(p, box);
// closest = (2, 1, 1) — AABBの面上の最近接点`,
  '• 各軸を独立にClamp(point.axis, aabb.min.axis, aabb.max.axis)する\n• 点がAABB内部にある場合はそのまま返す\n• SphereVsAABBの内部処理にも使用'
],

'Collision3D-ClosestPointOnTriangle': [
  'Vector3 Collision3D::ClosestPointOnTriangle(const Vector3& point, const Triangle& tri)',
  '三角形上の指定した点に最も近い点を返す。重心座標を用いてVoronoi領域分類を行い、頂点・辺・面のうち最も近い領域の点を返す。メッシュとの距離計算やコライダー生成に使用する。',
  `GX::Triangle tri(
    GX::Vector3(0, 0, 0),
    GX::Vector3(4, 0, 0),
    GX::Vector3(2, 3, 0)
);
GX::Vector3 p(2.0f, 1.0f, 5.0f);

GX::Vector3 closest = GX::Collision3D::ClosestPointOnTriangle(p, tri);
// 三角形面上の(2, 1, 0)付近`,
  '• Voronoi領域分類で頂点・辺・面の最近接を判定\n• 重心座標に基づく正確な計算\n• 点と三角形メッシュの距離計算に使用'
],

'Collision3D-ClosestPointOnLine': [
  'Vector3 Collision3D::ClosestPointOnLine(const Vector3& point, const Vector3& lineA, const Vector3& lineB)',
  '線分上の指定した点に最も近い点を返す。線分のパラメータtを[0,1]にクランプして端点を超えないようにする。スケルトンのボーンやエッジとの距離計算に使用する。',
  `GX::Vector3 lineStart(0, 0, 0);
GX::Vector3 lineEnd(10, 0, 0);
GX::Vector3 p(5, 3, 0);

GX::Vector3 closest = GX::Collision3D::ClosestPointOnLine(p, lineStart, lineEnd);
// closest = (5, 0, 0)`,
  '• パラメータtをClamp(0,1)して線分範囲内に制限\n• 射影計算: t = dot(p-A, B-A) / dot(B-A, B-A)'
],

// ============================================================
// Quadtree — 2D空間分割
// ============================================================

'Quadtree-constructor': [
  'template<typename T> Quadtree<T>::Quadtree(const AABB2D& bounds, int maxDepth = 8, int maxObjects = 8)',
  '2D空間を再帰的に4分割するクワッドツリーを構築する。boundsにルートノードの範囲、maxDepthに最大分割深度、maxObjectsにノード内の最大オブジェクト数を指定する。ノード内のオブジェクト数がmaxObjectsを超えるとNW/NE/SW/SEの4象限に自動分割される。2Dゲームのブロードフェーズ衝突判定に使用する。\n\n【用語】Quadtree（四分木）: 2D空間を再帰的に4分割するツリー構造。広い空間で近傍オブジェクトだけを効率的に検索でき、衝突判定の候補ペアを大幅に削減する。N個のオブジェクトで全ペア判定O(N^2)を平均O(N log N)に改善。',
  `// ゲーム画面全体をカバーするクワッドツリー
GX::AABB2D worldBounds(GX::Vector2(0, 0), GX::Vector2(1280, 960));
GX::Quadtree<int> tree(worldBounds, 6, 4);`,
  '• テンプレート型Tは格納するオブジェクトの識別子（int, Entity*等）\n• maxDepth=8, maxObjects=8がデフォルト値\n• ノード分割はNW(左上), NE(右上), SW(左下), SE(右下)の4象限\n• Tはoperator==が必要（Remove内部で使用）'
],

'Quadtree-Insert': [
  'void Quadtree<T>::Insert(const T& object, const AABB2D& bounds)',
  'オブジェクトをクワッドツリーに挿入する。boundsはオブジェクトのAABBバウンディング。ツリーのノードと重なるすべての子ノードに再帰的に挿入される。ノード内のオブジェクト数がmaxObjectsを超えると自動的に分割が発生する。',
  `GX::Quadtree<int> tree(worldBounds);

// オブジェクトID=0をAABB(10,10)-(30,30)で挿入
tree.Insert(0, GX::AABB2D(GX::Vector2(10, 10), GX::Vector2(30, 30)));
tree.Insert(1, GX::AABB2D(GX::Vector2(500, 400), GX::Vector2(520, 420)));`,
  '• オブジェクトが複数のノードにまたがる場合、該当する全ノードに挿入される\n• ノード容量を超えると自動的にSubdivide()が呼ばれる\n• ルートノードの範囲外のオブジェクトは挿入されない'
],

'Quadtree-Remove': [
  'void Quadtree<T>::Remove(const T& object)',
  '指定したオブジェクトをクワッドツリーから削除する。全ノードを再帰的に走査し、一致するオブジェクトを除去する。オブジェクトが移動した際のUpdate処理（Remove→Insert）に使用する。',
  `tree.Remove(0); // オブジェクトID=0を全ノードから削除

// 移動後の更新パターン
tree.Remove(entityId);
tree.Insert(entityId, newBounds);`,
  '• 全ノードを再帰走査するため、大量のRemoveは負荷が高い\n• Tはoperator==で比較される\n• 移動オブジェクトはRemove→Insertで更新する'
],

'Quadtree-Clear': [
  'void Quadtree<T>::Clear()',
  'クワッドツリーの全オブジェクトとサブノードをクリアする。ルートノードのみ残り、初期状態に戻る。フレームごとに再構築するパターンで使用する。',
  `// 毎フレーム再構築パターン
tree.Clear();
for (auto& entity : entities) {
    tree.Insert(entity.id, entity.GetBounds());
}`,
  '• ルートノードのboundsは保持される\n• 子ノードはreset()で破棄される\n• 毎フレーム再構築する場合はClear→全Insert'
],

'Quadtree-Query': [
  'void Quadtree<T>::Query(const AABB2D& area, std::vector<T>& results) const\nvoid Quadtree<T>::Query(const Circle& area, std::vector<T>& results) const',
  '指定した範囲内のオブジェクトを検索する。AABB範囲とCircle範囲の2つのオーバーロードがある。ツリーを再帰的に走査し、範囲と重なるオブジェクトをresultsに追加する。近傍検索やローカルな衝突候補の取得に使用する。',
  `// AABB範囲クエリ
std::vector<int> nearby;
GX::AABB2D searchArea(GX::Vector2(90, 90), GX::Vector2(210, 210));
tree.Query(searchArea, nearby);

// 円形範囲クエリ
std::vector<int> inRange;
GX::Circle range(GX::Vector2(400, 300), 100.0f);
tree.Query(range, inRange);`,
  '• 結果はresultsに追加される（事前にclearが必要な場合がある）\n• Circle版は内部でAABBバウンディングで粗判定後、AABBvsCircleで精密判定\n• ツリー構造により範囲外のノードをスキップするため高速'
],

'Quadtree-GetPotentialPairs': [
  'void Quadtree<T>::GetPotentialPairs(std::vector<std::pair<T, T>>& pairs) const',
  'クワッドツリー内のすべての衝突候補ペアを取得する。同一ノード内および親子ノード間でAABBが重なるペアを列挙する。ブロードフェーズ衝突判定で全ペアを一括取得する場合に使用する。',
  `std::vector<std::pair<int, int>> pairs;
tree.GetPotentialPairs(pairs);

for (auto& [idA, idB] : pairs) {
    // ナローフェーズ判定を実行
    if (CheckDetailedCollision(entities[idA], entities[idB])) {
        ResolveCollision(entities[idA], entities[idB]);
    }
}`,
  '• 同一ノード内のペア + 祖先ノードとのペアを全列挙\n• 重複ペアは発生しない（祖先→子孫の方向のみ列挙）\n• ナローフェーズ判定の前段として使用する'
],

// ============================================================
// Octree — 3D空間分割
// ============================================================

'Octree-constructor': [
  'template<typename T> Octree<T>::Octree(const AABB3D& bounds, int maxDepth = 8, int maxObjects = 8)',
  '3D空間を再帰的に8分割するオクツリーを構築する。boundsにルートノードの範囲を指定する。ノード内のオブジェクト数がmaxObjectsを超えると8象限に自動分割される。3Dゲームのブロードフェーズ衝突判定やフラスタムカリングに使用する。',
  `// ワールド全体をカバーするオクツリー
GX::AABB3D worldBounds(GX::Vector3(-500, -100, -500), GX::Vector3(500, 200, 500));
GX::Octree<int> octree(worldBounds, 6, 8);`,
  '• 8象限（octant）に分割される（Quadtreeの3D版）\n• maxDepth=8, maxObjects=8がデフォルト\n• Tはoperator==が必要（Remove内部で使用）'
],

'Octree-Insert': [
  'void Octree<T>::Insert(const T& object, const AABB3D& bounds)',
  'オブジェクトをオクツリーに挿入する。boundsは3D AABBバウンディング。複数のoctantにまたがるオブジェクトは該当する全ノードに挿入される。ノード容量超過時に自動分割が発生する。',
  `octree.Insert(entityId, entity.GetBounds3D());

// 全オブジェクトを挿入
for (int i = 0; i < entityCount; ++i) {
    octree.Insert(i, entities[i].GetAABB());
}`,
  '• 複数のoctantにまたがるオブジェクトは複数ノードに挿入される\n• ルートノードの範囲外のオブジェクトは挿入されない'
],

'Octree-Remove': [
  'void Octree<T>::Remove(const T& object)',
  '指定したオブジェクトをオクツリーから削除する。全ノードを再帰的に走査して一致するオブジェクトを除去する。',
  `octree.Remove(entityId);

// 位置更新パターン
octree.Remove(id);
octree.Insert(id, newBounds);`,
  '• 全ノード再帰走査のため、頻繁なRemoveはClear→再構築の方が効率的な場合がある\n• Tはoperator==で比較される'
],

'Octree-Clear': [
  'void Octree<T>::Clear()',
  'オクツリーの全オブジェクトとサブノードをクリアする。ルートノードのみ残り初期状態に戻る。',
  `octree.Clear();

// 毎フレーム再構築
octree.Clear();
for (auto& obj : objects) {
    octree.Insert(obj.id, obj.GetAABB());
}`,
  '• ルートノードのboundsは保持される\n• 子ノードはreset()で解放される'
],

'Octree-Query': [
  'void Octree<T>::Query(const AABB3D& area, std::vector<T>& results) const\nvoid Octree<T>::Query(const Sphere& area, std::vector<T>& results) const\nvoid Octree<T>::Query(const Frustum& frustum, std::vector<T>& results) const',
  '指定した範囲内のオブジェクトを検索する。AABB、Sphere、Frustumの3つのオーバーロードがある。Frustum版はフラスタムカリングに使用し、カメラに映るオブジェクトのみを効率的に取得できる。ツリー構造により範囲外のノードを早期にスキップする。',
  `// AABB範囲クエリ
std::vector<int> nearby;
octree.Query(GX::AABB3D(GX::Vector3(-5,-5,-5), GX::Vector3(5,5,5)), nearby);

// 球体範囲クエリ（爆発範囲内のオブジェクト）
std::vector<int> inBlast;
octree.Query(GX::Sphere(explosionPos, 20.0f), inBlast);

// フラスタムカリング
std::vector<int> visible;
GX::Frustum frustum = GX::Frustum::FromViewProjection(viewProj);
octree.Query(frustum, visible);`,
  '• AABB版: TestAABBVsAABBでノード判定\n• Sphere版: SphereVsAABBでノード判定\n• Frustum版: TestFrustumVsAABBでノード判定（フラスタムカリングに最適）\n• 結果はresultsに追加される（事前clearが必要な場合がある）'
],

// ============================================================
// BVH — バウンディングボリューム階層
// ============================================================

'BVH-Build': [
  'void BVH<T>::Build(const std::vector<std::pair<T, AABB3D>>& objects)',
  'BVH（バウンディングボリューム階層）を構築する。オブジェクトとそのAABBのペア配列を受け取り、SAH（Surface Area Heuristic）に基づいて最適な分割を再帰的に行う。静的なシーンの高速レイキャストやAABBクエリに使用する。\n\n【用語】BVH（Bounding Volume Hierarchy）: バウンディングボリューム階層。オブジェクトのAABBを二分木構造で階層化し、レイキャストや範囲クエリで不要な部分を早期に枝刈りして高速検索を実現する。\n\n【用語】SAH（Surface Area Heuristic）: BVH構築時の分割最適化手法。子ノードの表面積比率でレイが通過する確率を推定し、期待コストが最小になる分割軸・位置を選択する。',
  `// オブジェクトリスト作成
std::vector<std::pair<int, GX::AABB3D>> objects;
for (int i = 0; i < meshCount; ++i) {
    objects.push_back({ i, meshes[i].GetBounds() });
}

GX::BVH<int> bvh;
bvh.Build(objects);`,
  '• SAH（Surface Area Heuristic）で最適な分割軸・位置を選択\n• 構築コストはO(n log^2 n)程度\n• 静的シーンに最適（動的オブジェクトが多い場合はOctreeの方が効率的）\n• Build()は既存のデータをクリアして再構築する'
],

'BVH-Clear': [
  'void BVH<T>::Clear()',
  'BVHの全ノードとオブジェクト参照をクリアする。Build()で再構築するまで空の状態になる。',
  `bvh.Clear();`,
  '• m_nodesとm_objectsの両方がクリアされる\n• Clear後のQuery/Raycastは何もヒットしない'
],

'BVH-Query': [
  'void BVH<T>::Query(const AABB3D& area, std::vector<T>& results) const\nvoid BVH<T>::Query(const Ray& ray, std::vector<T>& results) const',
  '指定した範囲またはレイと交差するオブジェクトを検索する。AABBクエリは範囲内の全オブジェクトを返し、Rayクエリはレイと交差する全オブジェクトを返す。BVHのノード階層をたどり、バウンディングボックスと交差しないサブツリーを早期にスキップする。',
  `// AABB範囲クエリ
std::vector<int> nearby;
bvh.Query(GX::AABB3D(GX::Vector3(-2,-2,-2), GX::Vector3(2,2,2)), nearby);

// レイクエリ（レイが貫通する全オブジェクト）
std::vector<int> hitObjects;
GX::Ray ray(GX::Vector3(0, 10, 0), GX::Vector3(0, -1, 0));
bvh.Query(ray, hitObjects);`,
  '• AABB版: TestAABBVsAABBでノード判定\n• Ray版: RaycastAABBでノードバウンディングボックスとの交差判定\n• BVH階層により平均O(log n)で検索可能\n• 結果はresultsに追加される'
],

'BVH-Raycast': [
  'bool BVH<T>::Raycast(const Ray& ray, float& outT, T* outObject = nullptr) const',
  'BVHに対してレイキャストを行い、最も近いヒットオブジェクトを返す。outTにヒットまでのパラメータ値、outObjectにヒットしたオブジェクトを出力する。BVHノードのバウンディングボックスで早期棄却し、リーフノードで個別のAABBと判定する。',
  `GX::Ray ray(GX::Vector3(0, 100, 0), GX::Vector3(0, -1, 0));

float t;
int hitMeshIndex;
if (bvh.Raycast(ray, t, &hitMeshIndex)) {
    GX::Vector3 hitPoint = ray.GetPoint(t);
    // hitMeshIndex が最も近いメッシュのインデックス
}`,
  '• 最も近い（最小t）ヒットのみを返す\n• outObjectはnullptrで省略可能\n• 早期棄却: 現在のclosestTより遠いノードをスキップ\n• Query(Ray)は全ヒット、Raycastは最近ヒットのみという違いがある'
],

// ============================================================
// PhysicsWorld2D — 2D物理ワールド
// ============================================================

'PhysicsWorld2D-AddBody': [
  'RigidBody2D* PhysicsWorld2D::AddBody()',
  '新しい2D剛体を作成してワールドに追加する。ワールドが所有権を持つため、返されたポインタをdeleteしてはならない。剛体のプロパティ（位置、質量、形状等）は返されたポインタ経由で設定する。',
  `GX::PhysicsWorld2D world;
world.SetGravity(GX::Vector2(0, 500)); // Y-down画面座標系

// 地面（Static）
GX::RigidBody2D* ground = world.AddBody();
ground->bodyType = GX::BodyType2D::Static;
ground->position = GX::Vector2(640, 700);
ground->shape.type = GX::ShapeType2D::AABB;
ground->shape.halfExtents = GX::Vector2(640, 20);

// ボール（Dynamic）
GX::RigidBody2D* ball = world.AddBody();
ball->position = GX::Vector2(640, 100);
ball->shape.type = GX::ShapeType2D::Circle;
ball->shape.radius = 15.0f;
ball->mass = 1.0f;
ball->restitution = 0.7f;`,
  '• ワールドがunique_ptrで所有権を管理する\n• 返されたポインタをdeleteしないこと\n• プロパティ（position, mass, shape等）は直接メンバーに設定する\n• RemoveBody()で削除するとポインタは無効になる'
],

'PhysicsWorld2D-RemoveBody': [
  'void PhysicsWorld2D::RemoveBody(RigidBody2D* body)',
  '指定した剛体をワールドから削除する。削除後はポインタが無効になるため、参照を保持している場合はnullptrに置き換えること。内部のunique_ptrが解放される。',
  `world.RemoveBody(ball);
ball = nullptr; // ポインタを無効化`,
  '• 削除後のポインタアクセスは未定義動作\n• 削除後は必ずポインタをnullptrにすること'
],

'PhysicsWorld2D-Step': [
  'void PhysicsWorld2D::Step(float deltaTime, int velocityIterations = 8, int positionIterations = 3)',
  '物理シミュレーションを1ステップ進める。重力の適用、ブロードフェーズ（O(n^2)全ペア判定）、ナローフェーズ、衝突応答、位置積分を順に実行する。velocityIterationsで衝突応答の精度、positionIterationsで位置補正の精度を制御する。',
  `// 毎フレームのゲームループ内で呼び出す
float dt = timer.GetDeltaTime();
world.Step(dt);

// 剛体の位置をスプライトに反映
sprite.SetPosition(ball->position.x, ball->position.y);`,
  '• deltaTimeは秒単位（60FPSなら約0.0167）\n• velocityIterations=8, positionIterations=3がデフォルト\n• ブロードフェーズはO(n^2)全ペア判定\n• 画面座標系（Y-down）では重力Yを正の値にすること\n• Step内でonCollision/onTriggerコールバックが発火する'
],

'PhysicsWorld2D-SetGravity': [
  'void PhysicsWorld2D::SetGravity(const Vector2& gravity)',
  '物理ワールドの重力ベクトルを設定する。画面座標系（Y軸下向き）で使用する場合、Yを正の値にする必要がある。デフォルトは(0, -9.81)。',
  `// 画面座標系（Y-down）の場合、下方向が正
world.SetGravity(GX::Vector2(0, 500.0f)); // ピクセル単位の重力

// 無重力（宇宙空間など）
world.SetGravity(GX::Vector2(0, 0));

// 横方向の重力（風の効果）
world.SetGravity(GX::Vector2(50.0f, 300.0f));`,
  '• デフォルトは(0, -9.81)（数学座標系）\n• 画面座標系（Y-down）ではYを正にすること\n• ピクセル単位の場合は値を大きくする必要がある（例: 500〜1000）'
],

'PhysicsWorld2D-GetGravity': [
  'Vector2 PhysicsWorld2D::GetGravity() const',
  '現在の重力ベクトルを取得する。',
  `GX::Vector2 g = world.GetGravity();`,
  '• デフォルト値は(0, -9.81)'
],

'PhysicsWorld2D-Raycast': [
  'bool PhysicsWorld2D::Raycast(const Vector2& origin, const Vector2& direction, float maxDistance, RigidBody2D** outBody = nullptr, Vector2* outPoint = nullptr, Vector2* outNormal = nullptr)',
  '2D物理ワールドに対してレイキャストを実行する。全ボディに対して判定し、最も近いヒットを返す。outBody, outPoint, outNormalでヒット情報を取得できる。射線判定やライン・オブ・サイト判定に使用する。',
  `GX::Vector2 origin(100, 300);
GX::Vector2 dir(1, 0); // 右方向

GX::RigidBody2D* hitBody = nullptr;
GX::Vector2 hitPoint, hitNormal;

if (world.Raycast(origin, dir, 500.0f, &hitBody, &hitPoint, &hitNormal)) {
    // hitBody: 最も近い衝突ボディ
    // hitPoint: 衝突点
    // hitNormal: 衝突法線
}`,
  '• directionは正規化推奨\n• maxDistanceはレイの最大到達距離\n• outBody/outPoint/outNormalはすべてnullptrで省略可能\n• 最も近いヒットのみを返す\n• Circle/AABBの両シェイプに対応'
],

'PhysicsWorld2D-QueryAABB': [
  'void PhysicsWorld2D::QueryAABB(const AABB2D& area, std::vector<RigidBody2D*>& results)',
  'AABB範囲内に存在するボディを検索する。範囲内のすべてのボディをresultsに追加する。爆発範囲内の敵検索やエリアトリガーに使用する。',
  `GX::AABB2D explosionArea(GX::Vector2(200, 200), GX::Vector2(400, 400));
std::vector<GX::RigidBody2D*> affected;
world.QueryAABB(explosionArea, affected);

for (auto* body : affected) {
    GX::Vector2 dir = (body->position - explosionCenter).Normalized();
    body->ApplyImpulse(dir * 500.0f);
}`,
  '• 各ボディのAABBとクエリ領域の交差判定を行う\n• 結果はresultsに追加される\n• Static/Dynamic/Kinematicすべてが対象'
],

'PhysicsWorld2D-onCollision': [
  'std::function<void(const ContactInfo2D&)> PhysicsWorld2D::onCollision',
  '衝突発生時に呼ばれるコールバック。ContactInfo2Dに衝突した2つのボディ、衝突点、法線、めり込み深さが含まれる。Step()内部の衝突応答時に発火する。ダメージ判定やエフェクト生成に使用する。',
  `world.onCollision = [](const GX::ContactInfo2D& contact) {
    // 衝突したボディを取得
    GX::RigidBody2D* a = contact.bodyA;
    GX::RigidBody2D* b = contact.bodyB;

    // 衝突の強さを速度差から計算
    float impactSpeed = (a->velocity - b->velocity).Length();
    if (impactSpeed > 100.0f) {
        // 強い衝突 → ダメージ処理
    }
};`,
  '• Step()内で発火する\n• ContactInfo2DにbodyA, bodyB, point, normal, depthが含まれる\n• isTrigger=trueのボディはonCollisionではなくonTriggerが発火する'
],

'PhysicsWorld2D-onTriggerEnter': [
  'std::function<void(RigidBody2D*, RigidBody2D*)> PhysicsWorld2D::onTriggerEnter',
  'トリガーボディとの重なり開始時に呼ばれるコールバック。isTrigger=trueのボディが他のボディと重なり始めたときに発火する。アイテム取得やエリアイベントのトリガーに使用する。',
  `world.onTriggerEnter = [](GX::RigidBody2D* trigger, GX::RigidBody2D* other) {
    // トリガーゾーンに侵入
    if (trigger->userData == itemZone) {
        // アイテム取得処理
    }
};`,
  '• isTrigger=trueのボディのみ発火する\n• トリガーボディは衝突応答を行わない（すり抜ける）\n• 重なっている間は発火しない（開始時のみ）'
],

'PhysicsWorld2D-onTriggerExit': [
  'std::function<void(RigidBody2D*, RigidBody2D*)> PhysicsWorld2D::onTriggerExit',
  'トリガーボディとの重なり終了時に呼ばれるコールバック。isTrigger=trueのボディから他のボディが離れたときに発火する。エリア退出イベントに使用する。',
  `world.onTriggerExit = [](GX::RigidBody2D* trigger, GX::RigidBody2D* other) {
    // トリガーゾーンから退出
};`,
  '• onTriggerEnterと対になるコールバック\n• ボディが削除された場合もExitが発火する場合がある'
],

// ============================================================
// PhysicsWorld3D — 3D物理ワールド (Jolt Physics)
// ============================================================

'PhysicsWorld3D-Initialize': [
  'bool PhysicsWorld3D::Initialize(uint32_t maxBodies = 10240)',
  '3D物理ワールドを初期化する。Jolt PhysicsエンジンのTempAllocator、JobSystem、PhysicsSystem等を内部で構築する。maxBodiesで最大ボディ数を指定する。使用前に必ず呼び出すこと。\n\n【用語】Jolt Physics: C++製のオープンソース3D物理エンジン。剛体シミュレーション、多様な衝突形状、ブロードフェーズ/ナローフェーズの衝突検出を提供する。GXLibではPIMPLパターンでラップし、Joltのヘッダー依存を隠蔽している。',
  `GX::PhysicsWorld3D physics;
if (!physics.Initialize(4096)) {
    // 初期化失敗
    return;
}`,
  '• 内部でRegisterDefaultAllocator(), RegisterTypes()等のJolt初期化を実行\n• TempAllocatorImplは32MB以上のメモリを確保する\n• maxBodiesはJoltのPhysicsSystem::Init引数に渡される\n• Shutdown()で対に解放すること'
],

'PhysicsWorld3D-Shutdown': [
  'void PhysicsWorld3D::Shutdown()',
  '3D物理ワールドを終了し、全リソースを解放する。Jolt Physicsの内部状態をすべてクリーンアップする。Initialize()と対に呼び出すこと。',
  `physics.Shutdown();`,
  '• 全ボディ・シェイプが解放される\n• Shutdown後のAPI呼び出しは未定義動作\n• デストラクタでも自動的に呼ばれる'
],

'PhysicsWorld3D-Step': [
  'void PhysicsWorld3D::Step(float deltaTime)',
  '3D物理シミュレーションを1ステップ進める。Jolt Physicsの内部ステップを実行し、重力・衝突・拘束を計算する。毎フレームのゲームループ内で呼び出す。',
  `// 毎フレーム
float dt = timer.GetDeltaTime();
physics.Step(dt);

// ボディの位置を取得して描画に反映
GX::Matrix4x4 worldMat = physics.GetWorldTransform(bodyId);`,
  '• deltaTimeは秒単位\n• 内部でCollisionSteps=1のUpdate()を実行\n• Step後にGetPosition/GetWorldTransformで更新後の状態を取得する'
],

'PhysicsWorld3D-SetGravity': [
  'void PhysicsWorld3D::SetGravity(const Vector3& gravity)',
  '物理ワールドの重力ベクトルを設定する。デフォルトは(0, -9.81, 0)。月面重力や無重力空間のシミュレーションに変更可能。',
  `physics.SetGravity(GX::Vector3(0, -9.81f, 0)); // 地球標準重力

physics.SetGravity(GX::Vector3(0, -1.62f, 0));  // 月面重力

physics.SetGravity(GX::Vector3(0, 0, 0));        // 無重力`,
  '• デフォルトは(0, -9.81, 0)\n• Jolt PhysicsSystemのSetGravity()に委譲する'
],

'PhysicsWorld3D-GetGravity': [
  'Vector3 PhysicsWorld3D::GetGravity() const',
  '現在の重力ベクトルを取得する。',
  `GX::Vector3 g = physics.GetGravity();`,
  '• デフォルト値は(0, -9.81, 0)'
],

'PhysicsWorld3D-CreateBoxShape': [
  'PhysicsShape* PhysicsWorld3D::CreateBoxShape(const Vector3& halfExtents)',
  'ボックス形状のコライダーシェイプを作成する。halfExtentsは各軸の半サイズ。使用後はDestroyShape()で解放すること。',
  `// 2x1x2のボックス（半サイズ 1, 0.5, 1）
GX::PhysicsShape* boxShape = physics.CreateBoxShape(GX::Vector3(1.0f, 0.5f, 1.0f));

GX::PhysicsBodySettings settings;
settings.position = GX::Vector3(0, 10, 0);
settings.motionType = GX::MotionType3D::Dynamic;
settings.mass = 5.0f;

GX::PhysicsBodyID id = physics.AddBody(boxShape, settings);`,
  '• halfExtentsは各軸の半サイズ（全体サイズの半分）\n• 作成されたシェイプは複数のボディで再利用可能\n• 使用後はDestroyShape()で解放すること'
],

'PhysicsWorld3D-CreateSphereShape': [
  'PhysicsShape* PhysicsWorld3D::CreateSphereShape(float radius)',
  '球体形状のコライダーシェイプを作成する。radiusは球の半径。最も計算効率の良いシェイプ。',
  `GX::PhysicsShape* sphere = physics.CreateSphereShape(0.5f);

GX::PhysicsBodySettings settings;
settings.position = GX::Vector3(0, 20, 0);
settings.mass = 1.0f;
settings.restitution = 0.8f; // 高い反発

GX::PhysicsBodyID ballId = physics.AddBody(sphere, settings);`,
  '• 球体は最も衝突判定が高速なシェイプ\n• 転がる物体（ボール、弾丸等）に最適'
],

'PhysicsWorld3D-CreateCapsuleShape': [
  'PhysicsShape* PhysicsWorld3D::CreateCapsuleShape(float halfHeight, float radius)',
  'カプセル形状のコライダーシェイプを作成する。halfHeightは球を除く円柱部分の半分の高さ、radiusは球と円柱の半径。キャラクターのコライダーに最適。',
  `// キャラクター用カプセル（高さ=2*halfHeight+2*radius = 1.6+0.6 = 2.2m）
GX::PhysicsShape* capsule = physics.CreateCapsuleShape(0.8f, 0.3f);

GX::PhysicsBodySettings settings;
settings.position = GX::Vector3(0, 1.1f, 0);
settings.motionType = GX::MotionType3D::Dynamic;
settings.mass = 70.0f;

GX::PhysicsBodyID charId = physics.AddBody(capsule, settings);`,
  '• 全体の高さ = 2 * halfHeight + 2 * radius\n• キャラクターコントローラーに最適（段差超えが自然）\n• Y軸方向に配置される'
],

'PhysicsWorld3D-CreateMeshShape': [
  'PhysicsShape* PhysicsWorld3D::CreateMeshShape(const Vector3* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)',
  'メッシュ形状のコライダーシェイプを作成する。三角形メッシュをそのままコライダーとして使用する。Staticボディ専用で、地形やレベルジオメトリに使用する。',
  `// メッシュデータからシェイプ作成
GX::PhysicsShape* meshShape = physics.CreateMeshShape(
    vertices.data(), static_cast<uint32_t>(vertices.size()),
    indices.data(), static_cast<uint32_t>(indices.size())
);

GX::PhysicsBodySettings settings;
settings.position = GX::Vector3(0, 0, 0);
settings.motionType = GX::MotionType3D::Static;
settings.layer = 0; // Staticレイヤー

physics.AddBody(meshShape, settings);`,
  '• Static専用（Dynamic/Kinematicでは使用不可）\n• 三角形メッシュのインデックスは3の倍数であること\n• 地形やレベルの静的コライダーに使用\n• 頂点数が多い場合は構築コストが高い'
],

'PhysicsWorld3D-AddBody': [
  'PhysicsBodyID PhysicsWorld3D::AddBody(PhysicsShape* shape, const PhysicsBodySettings& settings)',
  'ワールドにボディを追加する。シェイプとPhysicsBodySettingsで初期状態を指定する。返されたPhysicsBodyIDで以後の操作を行う。',
  `GX::PhysicsShape* box = physics.CreateBoxShape(GX::Vector3(1, 1, 1));

GX::PhysicsBodySettings settings;
settings.position = GX::Vector3(0, 10, 0);
settings.rotation = GX::Quaternion::Identity();
settings.motionType = GX::MotionType3D::Dynamic;
settings.mass = 10.0f;
settings.friction = 0.5f;
settings.restitution = 0.3f;
settings.linearDamping = 0.05f;
settings.angularDamping = 0.05f;
settings.layer = 1; // Moving layer

GX::PhysicsBodyID bodyId = physics.AddBody(box, settings);`,
  '• PhysicsBodyIDは内部IDのラッパー（IsValid()で有効性確認可能）\n• settings.layerは衝突フィルタリングに使用（0=Static, 1=Moving）\n• settings.userDataにゲームオブジェクトのポインタ等を格納可能'
],

'PhysicsWorld3D-RemoveBody': [
  'void PhysicsWorld3D::RemoveBody(PhysicsBodyID id)',
  '指定したボディをワールドから削除する。削除後のIDは無効になる。',
  `physics.RemoveBody(bodyId);
// bodyId は以後使用不可`,
  '• 削除後のIDでAPI呼び出しは未定義動作\n• シェイプは別途DestroyShape()で解放する必要がある'
],

'PhysicsWorld3D-DestroyShape': [
  'void PhysicsWorld3D::DestroyShape(PhysicsShape* shape)',
  'シェイプを破棄する。シェイプを使用している全ボディを削除してから呼び出すこと。',
  `physics.RemoveBody(bodyId);      // 先にボディを削除
physics.DestroyShape(boxShape);  // シェイプを解放
boxShape = nullptr;`,
  '• ボディが使用中のシェイプを破棄しないこと\n• 1つのシェイプを複数ボディで共有している場合は全ボディ削除後に呼ぶ'
],

'PhysicsWorld3D-SetPosition': [
  'void PhysicsWorld3D::SetPosition(PhysicsBodyID id, const Vector3& pos)',
  'ボディの位置を直接設定する。物理シミュレーションを無視してテレポートさせる場合に使用する。',
  `physics.SetPosition(bodyId, GX::Vector3(0, 5, 0));`,
  '• Kinematicボディの移動に使用する\n• Dynamicボディに頻繁に使用すると物理シミュレーションが不安定になる場合がある'
],

'PhysicsWorld3D-GetPosition': [
  'Vector3 PhysicsWorld3D::GetPosition(PhysicsBodyID id) const',
  'ボディの現在位置を取得する。Step()後に呼び出してシミュレーション結果を取得する。',
  `GX::Vector3 pos = physics.GetPosition(bodyId);`,
  '• Step()後の最新位置を返す\n• 描画位置の更新にはGetWorldTransform()の方が便利'
],

'PhysicsWorld3D-SetRotation': [
  'void PhysicsWorld3D::SetRotation(PhysicsBodyID id, const Quaternion& rot)',
  'ボディの回転をクォータニオンで直接設定する。',
  `GX::Quaternion rot = GX::Quaternion::FromEuler(0, 3.14f / 4, 0); // Y軸45度
physics.SetRotation(bodyId, rot);`,
  '• Kinematicボディの回転制御に使用\n• クォータニオンは正規化されていること'
],

'PhysicsWorld3D-GetRotation': [
  'Quaternion PhysicsWorld3D::GetRotation(PhysicsBodyID id) const',
  'ボディの現在の回転をクォータニオンで取得する。',
  `GX::Quaternion rot = physics.GetRotation(bodyId);`,
  '• 正規化されたクォータニオンが返される'
],

'PhysicsWorld3D-SetLinearVelocity': [
  'void PhysicsWorld3D::SetLinearVelocity(PhysicsBodyID id, const Vector3& vel)',
  'ボディの線速度を設定する。即座に速度が変化する。キャラクターのジャンプや発射体の初速設定に使用する。',
  `// ジャンプ
physics.SetLinearVelocity(charId, GX::Vector3(0, 8.0f, 0));`,
  '• 即座に速度が変化する（次のStepから反映）\n• ApplyImpulseと異なり、現在の速度を完全に上書きする'
],

'PhysicsWorld3D-SetAngularVelocity': [
  'void PhysicsWorld3D::SetAngularVelocity(PhysicsBodyID id, const Vector3& vel)',
  'ボディの角速度を設定する。各軸の回転速度をrad/sで指定する。',
  `// Y軸周りに毎秒1回転
physics.SetAngularVelocity(bodyId, GX::Vector3(0, 6.28f, 0));`,
  '• 単位はrad/s\n• 現在の角速度を完全に上書きする'
],

'PhysicsWorld3D-ApplyForce': [
  'void PhysicsWorld3D::ApplyForce(PhysicsBodyID id, const Vector3& force)',
  'ボディに力を加える。次のStep()で加速度として適用される（F=maの法則に従う）。継続的な推進力や風の力に使用する。',
  `// 前方に推進力を加え続ける
physics.ApplyForce(rocketId, GX::Vector3(0, 100.0f, 0));`,
  '• 力は次のStep()で消費される（毎フレーム再適用が必要）\n• 加速度 = force / mass\n• 継続的な力はApplyForce、瞬間的な力はApplyImpulse'
],

'PhysicsWorld3D-ApplyImpulse': [
  'void PhysicsWorld3D::ApplyImpulse(PhysicsBodyID id, const Vector3& impulse)',
  'ボディに衝撃を加える。即座に速度が変化する。爆発やヒット時の吹き飛ばしに使用する。',
  `// 爆発による吹き飛ばし
GX::Vector3 dir = (physics.GetPosition(bodyId) - explosionCenter).Normalized();
physics.ApplyImpulse(bodyId, dir * 500.0f);`,
  '• 即座に速度変化する（velocity += impulse / mass）\n• 1回の呼び出しで効果が完結する\n• 爆発やヒットなど瞬間的な力に使用'
],

'PhysicsWorld3D-ApplyTorque': [
  'void PhysicsWorld3D::ApplyTorque(PhysicsBodyID id, const Vector3& torque)',
  'ボディにトルク（回転力）を加える。次のStep()で角加速度として適用される。',
  `// Y軸周りに回転力を加える
physics.ApplyTorque(topId, GX::Vector3(0, 50.0f, 0));`,
  '• 次のStep()で消費される（毎フレーム再適用が必要）\n• 角加速度 = torque / inertia'
],

'PhysicsWorld3D-GetWorldTransform': [
  'Matrix4x4 PhysicsWorld3D::GetWorldTransform(PhysicsBodyID id) const',
  'ボディのワールド変換行列を取得する。位置と回転を含む4x4行列で、3Dレンダリングに直接使用できる。Renderer3DのDrawMesh等に渡す。',
  `GX::Matrix4x4 worldMat = physics.GetWorldTransform(bodyId);

// XMMATRIXに変換して描画
XMMATRIX xmWorld = worldMat.ToXMMATRIX();
renderer.DrawMesh(meshData, material, xmWorld);`,
  '• 位置+回転を含む4x4行列（スケールは含まない）\n• Renderer3D::DrawMesh(XMMATRIX)オーバーロードに直接渡せる\n• GetPosition+GetRotationを個別に取得するより効率的'
],

'PhysicsWorld3D-Raycast': [
  'PhysicsWorld3D::RaycastResult PhysicsWorld3D::Raycast(const Vector3& origin, const Vector3& direction, float maxDistance)',
  'Jolt Physicsのレイキャストを実行する。最も近いヒットのRaycastResult（hit, bodyID, point, normal, fraction）を返す。FPSゲームの射撃判定やマウスピッキングに使用する。',
  `GX::Vector3 rayOrigin = camera.GetPosition();
GX::Vector3 rayDir = camera.GetForward();

auto result = physics.Raycast(rayOrigin, rayDir, 100.0f);
if (result.hit) {
    GX::PhysicsBodyID hitId = result.bodyID;
    GX::Vector3 hitPoint = result.point;
    GX::Vector3 hitNormal = result.normal;
    float hitFraction = result.fraction; // [0,1]
}`,
  '• RaycastResult.fractionはレイ全長に対するヒット位置の割合[0,1]\n• hitPoint = origin + direction * (maxDistance * fraction)\n• directionは正規化推奨\n• 最も近いヒットのみを返す'
],

'PhysicsWorld3D-MotionType3D': [
  'enum class MotionType3D { Static, Kinematic, Dynamic }',
  '3Dボディのモーションタイプを定義する列挙型。Staticは移動しない固定オブジェクト（地面・壁）、Kinematicはコードで直接制御するオブジェクト（エレベーター・扉）、Dynamicは物理演算で動くオブジェクト。PhysicsBodySettingsで設定する。',
  `GX::PhysicsBodySettings settings;

// 地面（静的）
settings.motionType = GX::MotionType3D::Static;

// エレベーター（キネマティック）
settings.motionType = GX::MotionType3D::Kinematic;

// 落下する箱（動的）
settings.motionType = GX::MotionType3D::Dynamic;`,
  '• Static: 質量無限大、衝突応答で動かない、layer=0推奨\n• Kinematic: SetPosition/SetRotationで移動、Dynamicボディを押す\n• Dynamic: 力・重力・衝突で移動する、massの設定が必要'
],

'PhysicsWorld3D-PhysicsBodySettings': [
  'struct PhysicsBodySettings { Vector3 position; Quaternion rotation; MotionType3D motionType; float mass; float friction; float restitution; float linearDamping; float angularDamping; uint16_t layer; void* userData; };',
  '3D物理ボディの作成時設定をまとめた構造体。位置・回転・モーションタイプ・質量・摩擦・反発係数・減衰・衝突レイヤー・ユーザーデータを指定する。AddBody()の引数として渡す。',
  `GX::PhysicsBodySettings settings;
settings.position = GX::Vector3(5, 15, -3);            // 初期位置
settings.rotation = GX::Quaternion::Identity();          // 初期回転（なし）
settings.motionType = GX::MotionType3D::Dynamic;         // 力・衝突で動く
settings.mass = 2.5f;                                    // 質量（kg）
settings.friction = 0.6f;                                // 摩擦係数（0〜1）
settings.restitution = 0.4f;                             // 反発係数（0〜1）
settings.linearDamping = 0.05f;                          // 線形減衰
settings.angularDamping = 0.05f;                         // 角度減衰
settings.layer = 1;                                      // 衝突レイヤー（Moving）
settings.userData = static_cast<void*>(gameObject);       // ゲームオブジェクト参照

GX::PhysicsBodyID id = physics.AddBody(shape, settings);`,
  '• mass: Dynamic時の質量（kg）、Static/Kinematicでは無視される\n• friction: 摩擦係数（0=滑る, 1=強い摩擦）\n• restitution: 反発係数（0=非弾性, 1=完全弾性）\n• layer: 衝突フィルタリング（0=Static, 1=Moving）\n• userData: ゲームオブジェクトへのポインタ等を格納可能'
],

// ============================================================
// RigidBody2D — 2D剛体
// ============================================================

'RigidBody2D-BodyType2D': [
  'enum class BodyType2D { Static, Dynamic, Kinematic }',
  '2Dボディのタイプを定義する列挙型。Staticは固定オブジェクト（地面・壁）、Dynamicは力や衝突で動くオブジェクト、Kinematicはコードで直接制御するオブジェクト。bodyTypeメンバーに設定する。',
  `GX::RigidBody2D* body = world.AddBody();

body->bodyType = GX::BodyType2D::Static;    // 地面
body->bodyType = GX::BodyType2D::Dynamic;   // 落下する箱
body->bodyType = GX::BodyType2D::Kinematic; // 移動する足場`,
  '• Static: InverseMass()が0を返す（移動不可）\n• Dynamic: 力・重力・衝突で移動する\n• Kinematic: 衝突応答で動かないが、Dynamicボディを押す'
],

'RigidBody2D-ShapeType2D': [
  'enum class ShapeType2D { Circle, AABB }',
  '2Dコライダーの形状タイプを定義する列挙型。CircleとAABBの2種類。ColliderShape2Dのtypeメンバーに設定する。',
  `body->shape.type = GX::ShapeType2D::Circle;
body->shape.radius = 20.0f;

body->shape.type = GX::ShapeType2D::AABB;
body->shape.halfExtents = GX::Vector2(30.0f, 10.0f);`,
  '• Circle: radiusメンバーで半径を指定\n• AABB: halfExtentsメンバーで各軸の半サイズを指定\n• 形状タイプによって使用されるメンバーが異なる'
],

'RigidBody2D-members': [
  'Vector2 position; float rotation; Vector2 velocity; float angularVelocity; float mass; float restitution; float friction; float linearDamping; float angularDamping; bool fixedRotation; BodyType2D bodyType; ColliderShape2D shape; bool isTrigger; void* userData; uint32_t layer;',
  'RigidBody2Dの公開メンバー変数。position（ワールド座標）、velocity（線速度）、rotation（回転角ラジアン）、mass（質量）、restitution（反発係数）、friction（摩擦）、linearDamping/angularDamping（減衰）、fixedRotation（回転固定）、bodyType（ボディタイプ）、shape（コライダー形状）、isTrigger（トリガーフラグ）、userData（ユーザーデータ）、layer（衝突レイヤー）を直接読み書きできる。',
  `GX::RigidBody2D* body = world.AddBody();
body->position = GX::Vector2(400, 300);
body->velocity = GX::Vector2(100, -50);
body->rotation = 0.0f;
body->mass = 2.0f;
body->restitution = 0.5f;
body->friction = 0.3f;
body->linearDamping = 0.01f;
body->fixedRotation = true;  // 回転を防ぐ
body->bodyType = GX::BodyType2D::Dynamic;
body->shape.type = GX::ShapeType2D::Circle;
body->shape.radius = 15.0f;
body->isTrigger = false;
body->userData = static_cast<void*>(gameEntity);
body->layer = 0xFFFFFFFF; // 全レイヤーと衝突`,
  '• 全メンバーがpublic（直接アクセス可能）\n• restitution: 0=完全非弾性, 1=完全弾性\n• layer: ビットマスクで衝突フィルタリング（0xFFFFFFFFは全衝突）\n• isTrigger=trueのボディは衝突応答なし（コールバックのみ）\n• fixedRotation=trueで回転しない（ピンボールのフリッパー等）'
],

'RigidBody2D-ApplyForce': [
  'void RigidBody2D::ApplyForce(const Vector2& force)',
  '剛体に力を加える。蓄積された力は次のStep()で加速度として適用され、自動的にリセットされる。継続的な推力やジェット噴射に使用する。',
  `// 右方向に力を加え続ける
body->ApplyForce(GX::Vector2(200.0f, 0));`,
  '• 内部でm_forceAccumに加算される\n• Step()で加速度として適用後、蓄積はリセットされる\n• 毎フレーム呼び出す必要がある（力は蓄積されない）'
],

'RigidBody2D-ApplyImpulse': [
  'void RigidBody2D::ApplyImpulse(const Vector2& impulse)',
  '剛体に衝撃（インパルス）を加える。即座に速度が変化する。ジャンプや爆発の吹き飛ばしなど瞬間的な力に使用する。内部でvelocity += impulse * InverseMass()を計算する。',
  `// ジャンプ（Y-down画面座標系では上方向が負）
body->ApplyImpulse(GX::Vector2(0, -300.0f));

// 爆発による吹き飛ばし
GX::Vector2 dir = (body->position - explosionPos).Normalized();
body->ApplyImpulse(dir * 500.0f);`,
  '• 即座にvelocityが変化する\n• InverseMass()が0の場合（Static/Kinematic）は効果なし\n• velocity += impulse / mass で計算'
],

'RigidBody2D-ApplyTorque': [
  'void RigidBody2D::ApplyTorque(float torque)',
  '剛体にトルク（回転力）を加える。次のStep()で角加速度として適用される。fixedRotation=trueの場合は効果がない。',
  `body->ApplyTorque(10.0f); // 反時計回りのトルク`,
  '• 内部でm_torqueAccumに加算される\n• Step()で適用後リセットされる\n• fixedRotation=trueの場合は回転しない'
],

'RigidBody2D-GetAABB': [
  'float RigidBody2D::InverseMass() const',
  '逆質量（1/mass）を返す。Static/Kinematicの場合は0を返す（質量無限大＝動かない）。衝突応答の内部計算で使用される。',
  `float invMass = body->InverseMass();
if (invMass > 0.0f) {
    // Dynamic ボディ（移動可能）
    body->velocity += impulse * invMass;
}`,
  '• Dynamic && mass > 0 の場合のみ 1/mass を返す\n• Static/Kinematicは0を返す（衝突応答で動かない）\n• 衝突応答の内部でインパルス計算に使用される'
],

// ============================================================
// PhysicsShape — 3Dシェイプハンドル
// ============================================================

'PhysicsShape-struct': [
  'struct PhysicsShape { void* internal; };',
  'Jolt Physicsの内部シェイプ参照を保持するラッパー構造体。PhysicsWorld3D::CreateXxxShape()で作成し、AddBody()でボディに紐づける。使用後はDestroyShape()で解放する。internalにはJPH::ShapeRefC*が格納されるが、直接操作する必要はない。\n\n【用語】PIMPLパターン（Pointer to Implementation）: 実装詳細をvoid*やforward宣言で隠蔽するC++設計パターン。GXLibではJolt Physicsのヘッダー依存をPhysicsShapeのvoid* internalで隠し、コンパイル時間の短縮とAPI安定性を実現している。',
  `// シェイプの作成と使用
GX::PhysicsShape* box = physics.CreateBoxShape(GX::Vector3(1, 1, 1));   // 各辺2mの箱
GX::PhysicsShape* sphere = physics.CreateSphereShape(0.5f);              // 半径0.5mの球
GX::PhysicsShape* capsule = physics.CreateCapsuleShape(0.8f, 0.3f);     // 高さ0.8m, 半径0.3m

// ボディに紐づけ
GX::PhysicsBodyID id = physics.AddBody(box, settings);

// 使用後に解放（順序: ボディ→シェイプ）
physics.RemoveBody(id);
physics.DestroyShape(box);`,
  '• internalはJolt Physicsのシェイプ参照ポインタ（JPH::ShapeRefC*）\n• 直接操作せず、PhysicsWorld3DのAPI経由で使用すること\n• 1つのシェイプを複数ボディで共有可能\n• DestroyShape()で解放する（全ボディ削除後に呼ぶこと）'
]

}
