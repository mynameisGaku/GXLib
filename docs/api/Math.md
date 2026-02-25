# Math API リファレンス

名前空間: `GX`。全型は DirectXMath の XMFLOAT 系を継承し、ゼロオーバーヘッドで相互変換できる。

## Vector2

2D 浮動小数点ベクトル (`XMFLOAT2` 継承)。

| コンストラクタ | 説明 |
|---|---|
| `Vector2()` | (0, 0) で初期化 |
| `Vector2(float x, float y)` | 指定成分で初期化 |
| `Vector2(const XMFLOAT2& v)` | XMFLOAT2 から変換 |

| メソッド | 戻り値 | 説明 |
|---|---|---|
| `Length()` | `float` | ベクトルの長さ |
| `LengthSquared()` | `float` | 長さの2乗 (sqrt 不要で高速) |
| `Normalized()` | `Vector2` | 正規化したコピーを返す |
| `Normalize()` | `void` | 自身を正規化する |
| `Dot(v)` | `float` | 内積 |
| `Cross(v)` | `float` | 2D 外積 (スカラー) |
| `Distance(v)` | `float` | 2点間の距離 |

| 静的メソッド | 説明 |
|---|---|
| `Vector2::Zero()` | (0, 0) |
| `Vector2::One()` | (1, 1) |
| `Vector2::Lerp(a, b, t)` | 線形補間 |
| `Vector2::Min(a, b)` / `Max(a, b)` | 各成分ごとの最小/最大 |

演算子: `+ - * / += -= *= == != -` (単項)

## Vector3

3D 浮動小数点ベクトル (`XMFLOAT3` 継承)。

| コンストラクタ | 説明 |
|---|---|
| `Vector3()` | (0, 0, 0) で初期化 |
| `Vector3(float x, float y, float z)` | 指定成分で初期化 |

| メソッド | 戻り値 | 説明 |
|---|---|---|
| `Length()` / `LengthSquared()` | `float` | 長さ / 長さの2乗 |
| `Normalized()` / `Normalize()` | `Vector3`/`void` | 正規化 |
| `Dot(v)` | `float` | 内積 |
| `Cross(v)` | `Vector3` | 外積 |
| `Distance(v)` / `DistanceSquared(v)` | `float` | 2点間距離 |

| 静的メソッド | 説明 |
|---|---|
| `Zero()` / `One()` / `Up()` / `Down()` / `Forward()` / `Right()` | 定数ベクトル |
| `Lerp(a, b, t)` | 線形補間 |
| `Reflect(direction, normal)` | 反射ベクトル |
| `Transform(v, matrix)` | 行列で座標変換 (w=1) |
| `TransformNormal(v, matrix)` | 行列で法線変換 (w=0) |

## Vector4

4D 浮動小数点ベクトル (`XMFLOAT4` 継承)。Vector3 + float w から構築可能。

## Matrix4x4

4x4 行列 (`XMFLOAT4X4` 継承)。既定値は単位行列。

| メソッド | 説明 |
|---|---|
| `ToXMMATRIX()` | `XMMATRIX` に変換する |
| `operator*(m)` | 行列乗算 |
| `Inverse()` | 逆行列 |
| `Transpose()` | 転置行列 |
| `Determinant()` | 行列式 |
| `TransformPoint(v)` | 点を変換 (w=1) |
| `TransformVector(v)` | 方向を変換 (w=0) |

| 静的ファクトリ | 説明 |
|---|---|
| `Identity()` | 単位行列 |
| `Translation(x, y, z)` | 平行移動行列 |
| `Scaling(x, y, z)` / `Scaling(uniform)` | 拡大縮小行列 |
| `RotationX(rad)` / `RotationY(rad)` / `RotationZ(rad)` | 軸回転行列 |
| `RotationRollPitchYaw(pitch, yaw, roll)` | オイラー角回転行列 |
| `LookAtLH(eye, target, up)` | ビュー行列 |
| `PerspectiveFovLH(fovY, aspect, near, far)` | 透視投影行列 |
| `OrthographicLH(w, h, near, far)` | 正射影行列 |

## Quaternion

回転クォータニオン (`XMFLOAT4` 継承)。既定値は単位クォータニオン (0,0,0,1)。

| メソッド | 説明 |
|---|---|
| `operator*(q)` | 回転の合成 |
| `Normalized()` / `Normalize()` | 正規化 |
| `Conjugate()` | 共役 |
| `Inverse()` | 逆クォータニオン |
| `ToEuler()` | オイラー角 (pitch, yaw, roll) に変換 |
| `RotateVector(v)` | ベクトルを回転する |

| 静的メソッド | 説明 |
|---|---|
| `Identity()` | 単位クォータニオン |
| `FromAxisAngle(axis, radians)` | 任意軸回転 |
| `FromEuler(pitch, yaw, roll)` | オイラー角から生成 |
| `FromRotationMatrix(m)` | 回転行列から生成 |
| `Slerp(a, b, t)` | 球面線形補間 |
| `Lerp(a, b, t)` | 正規化線形補間 (NLerp) |

## Color

RGBA 色 (float4、0.0-1.0)。

| コンストラクタ | 説明 |
|---|---|
| `Color()` | 白 (1,1,1,1) |
| `Color(float r, g, b, a=1.0f)` | float 成分指定 |
| `Color(uint32_t rgba)` | 0xRRGGBBAA から生成 |
| `Color(uint8_t r, g, b, a=255)` | 整数成分指定 |

| メソッド/静的メソッド | 説明 |
|---|---|
| `ToRGBA()` / `ToABGR()` | 32bit 整数に変換する |
| `ToXMFLOAT4()` | XMFLOAT4 に変換する |
| `FromHSV(h, s, v, a)` | HSV 色空間から生成する |
| `Lerp(a, b, t)` | 線形補間 |
| `White()` / `Black()` / `Red()` / `Green()` / `Blue()` | プリセット色 |

## Spline

スプライン曲線。`SplineType`: `Linear`, `CatmullRom`, `CubicBezier`。

| メソッド | 説明 |
|---|---|
| `AddPoint(point)` | 制御点を追加する |
| `SetPoint(index, point)` | 制御点を設定する |
| `SetType(type)` | 補間タイプを設定する |
| `SetClosed(bool)` | 閉曲線にするかを設定する |
| `Evaluate(t)` | パラメータ t (0-1) で位置を評価する |
| `EvaluateTangent(t)` | 接線方向を評価する |
| `GetTotalLength(subdivisions)` | 近似全長を取得する |
| `EvaluateByDistance(distance)` | 弧長パラメータで位置を評価する |
| `FindClosestParameter(point)` | 最近接点のパラメータを求める |

## MathUtil

`GX::MathUtil` 名前空間のユーティリティ関数。

| 関数 | 説明 |
|---|---|
| `Clamp(value, min, max)` | 値をクランプする |
| `Lerp(a, b, t)` | 線形補間 |
| `InverseLerp(a, b, value)` | 逆線形補間 |
| `Remap(value, fromMin, fromMax, toMin, toMax)` | 範囲リマップ |
| `SmoothStep(edge0, edge1, x)` | 3次スムーズステップ |
| `DegreesToRadians(deg)` / `RadiansToDegrees(rad)` | 角度変換 |
| `NormalizeAngle(rad)` | 角度を [-PI, PI] に正規化する |
| `ApproximatelyEqual(a, b, eps)` | 浮動小数点近似比較 |

定数: `PI`, `TAU` (2*PI), `EPSILON` (1e-6f)
