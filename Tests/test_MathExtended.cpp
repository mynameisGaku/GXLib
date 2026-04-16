/// @file test_MathExtended.cpp
/// @brief 数学ライブラリ強化 — 新規追加関数の単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/MathUtil.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"   // MathInline.h経由でQuaternionを含む
#include "Math/Color.h"
#include "Math/MathBatch.h"

using namespace gx;

// ============================================================================
// MathUtil 追加関数
// ============================================================================

TEST(MathUtilExtTest, MoveTowards)
{
    EXPECT_FLOAT_EQ(MathUtil::MoveTowards(0.0f, 10.0f, 5.0f), 5.0f);
    EXPECT_FLOAT_EQ(MathUtil::MoveTowards(0.0f, 10.0f, 15.0f), 10.0f);
    EXPECT_FLOAT_EQ(MathUtil::MoveTowards(5.0f, 5.0f, 1.0f), 5.0f);
    EXPECT_FLOAT_EQ(MathUtil::MoveTowards(10.0f, 0.0f, 3.0f), 7.0f);
}

TEST(MathUtilExtTest, DeltaAngle)
{
    EXPECT_NEAR(MathUtil::DeltaAngle(0.0f, MathUtil::PI), MathUtil::PI, 1e-5f);
    EXPECT_NEAR(MathUtil::DeltaAngle(0.0f, -MathUtil::PI), -MathUtil::PI, 1e-4f);
    EXPECT_NEAR(MathUtil::DeltaAngle(0.0f, MathUtil::TAU), 0.0f, 1e-5f);
    // 最短経路テスト
    float d = MathUtil::DeltaAngle(0.1f, MathUtil::TAU - 0.1f);
    EXPECT_NEAR(d, -0.2f, 1e-5f);
}

TEST(MathUtilExtTest, Repeat)
{
    EXPECT_NEAR(MathUtil::Repeat(5.5f, 3.0f), 2.5f, 1e-5f);
    EXPECT_NEAR(MathUtil::Repeat(3.0f, 3.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(MathUtil::Repeat(-1.0f, 3.0f), 2.0f, 1e-5f);
    EXPECT_NEAR(MathUtil::Repeat(0.0f, 5.0f), 0.0f, 1e-5f);
}

TEST(MathUtilExtTest, PingPong)
{
    EXPECT_NEAR(MathUtil::PingPong(0.0f, 3.0f), 0.0f, 1e-5f);
    EXPECT_NEAR(MathUtil::PingPong(1.5f, 3.0f), 1.5f, 1e-5f);
    EXPECT_NEAR(MathUtil::PingPong(3.0f, 3.0f), 3.0f, 1e-5f);
    EXPECT_NEAR(MathUtil::PingPong(4.5f, 3.0f), 1.5f, 1e-5f);
    EXPECT_NEAR(MathUtil::PingPong(6.0f, 3.0f), 0.0f, 1e-5f);
}

TEST(MathUtilExtTest, SmoothDamp)
{
    float vel = 0.0f;
    float current = 0.0f;
    // 何回かステップして目標に近づくことを確認
    for (int i = 0; i < 100; ++i)
        current = MathUtil::SmoothDamp(current, 10.0f, vel, 0.3f, 1.0f / 60.0f);
    EXPECT_NEAR(current, 10.0f, 0.01f);
}

TEST(MathUtilExtTest, SmoothDampAngle)
{
    float vel = 0.0f;
    float current = 0.0f;
    float target = MathUtil::PI * 1.5f;  // 270度 → 最短経路は-90度
    for (int i = 0; i < 100; ++i)
        current = MathUtil::SmoothDampAngle(current, target, vel, 0.3f, 1.0f / 60.0f);
    // 最終値は目標角度に近いはず
    float delta = MathUtil::DeltaAngle(current, target);
    EXPECT_NEAR(delta, 0.0f, 0.02f);
}

// ============================================================================
// Vector2 追加関数
// ============================================================================

TEST(Vector2ExtTest, Perpendicular)
{
    Vector2 v(3.0f, 4.0f);
    Vector2 p = v.Perpendicular();
    EXPECT_FLOAT_EQ(p.x, -4.0f);
    EXPECT_FLOAT_EQ(p.y, 3.0f);
    // 直交確認
    EXPECT_NEAR(v.Dot(p), 0.0f, 1e-5f);
}

TEST(Vector2ExtTest, Scale)
{
    Vector2 a(2.0f, 3.0f), b(4.0f, 5.0f);
    Vector2 s = Vector2::Scale(a, b);
    EXPECT_FLOAT_EQ(s.x, 8.0f);
    EXPECT_FLOAT_EQ(s.y, 15.0f);
}

TEST(Vector2ExtTest, MoveTowards)
{
    Vector2 a(0, 0), b(3, 4);  // 距離5
    Vector2 r = Vector2::MoveTowards(a, b, 2.5f);
    EXPECT_NEAR(r.Length(), 2.5f, 1e-5f);
    // 方向が正しいか
    EXPECT_NEAR(r.x / r.Length(), 3.0f / 5.0f, 1e-5f);

    // maxDelta >= 距離 → ターゲットに到達
    r = Vector2::MoveTowards(a, b, 10.0f);
    EXPECT_FLOAT_EQ(r.x, 3.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
}

TEST(Vector2ExtTest, ClampMagnitude)
{
    Vector2 v(3.0f, 4.0f);  // 長さ5
    Vector2 c = Vector2::ClampMagnitude(v, 2.5f);
    EXPECT_NEAR(c.Length(), 2.5f, 1e-5f);

    // 既に短い場合はそのまま
    c = Vector2::ClampMagnitude(v, 10.0f);
    EXPECT_FLOAT_EQ(c.x, 3.0f);
    EXPECT_FLOAT_EQ(c.y, 4.0f);
}

TEST(Vector2ExtTest, Angle)
{
    Vector2 a(1, 0), b(0, 1);
    EXPECT_NEAR(Vector2::Angle(a, b), 90.0f, 0.01f);  // FastAcos精度 ~0.004°

    EXPECT_NEAR(Vector2::Angle(a, a), 0.0f, 0.01f);

    Vector2 c(-1, 0);
    EXPECT_NEAR(Vector2::Angle(a, c), 180.0f, 0.01f);
}

TEST(Vector2ExtTest, SignedAngle)
{
    Vector2 a(1, 0), b(0, 1);
    EXPECT_NEAR(Vector2::SignedAngle(a, b), 90.0f, 0.01f);
    EXPECT_NEAR(Vector2::SignedAngle(b, a), -90.0f, 0.01f);
}

// ============================================================================
// Vector3 追加関数
// ============================================================================

TEST(Vector3ExtTest, Scale)
{
    Vector3 a(2, 3, 4), b(5, 6, 7);
    Vector3 s = Vector3::Scale(a, b);
    EXPECT_FLOAT_EQ(s.x, 10.0f);
    EXPECT_FLOAT_EQ(s.y, 18.0f);
    EXPECT_FLOAT_EQ(s.z, 28.0f);
}

TEST(Vector3ExtTest, Project)
{
    Vector3 v(3, 4, 0);
    Vector3 n(1, 0, 0);
    Vector3 p = Vector3::Project(v, n);
    EXPECT_NEAR(p.x, 3.0f, 1e-5f);
    EXPECT_NEAR(p.y, 0.0f, 1e-5f);
    EXPECT_NEAR(p.z, 0.0f, 1e-5f);
}

TEST(Vector3ExtTest, ProjectOnPlane)
{
    Vector3 v(3, 4, 5);
    Vector3 n(0, 1, 0);
    Vector3 p = Vector3::ProjectOnPlane(v, n);
    EXPECT_NEAR(p.x, 3.0f, 1e-5f);
    EXPECT_NEAR(p.y, 0.0f, 1e-5f);
    EXPECT_NEAR(p.z, 5.0f, 1e-5f);
}

TEST(Vector3ExtTest, MoveTowards)
{
    Vector3 a(0, 0, 0), b(3, 4, 0);
    Vector3 r = Vector3::MoveTowards(a, b, 2.5f);
    EXPECT_NEAR(r.Length(), 2.5f, 1e-5f);

    r = Vector3::MoveTowards(a, b, 100.0f);
    EXPECT_FLOAT_EQ(r.x, 3.0f);
    EXPECT_FLOAT_EQ(r.y, 4.0f);
}

TEST(Vector3ExtTest, ClampMagnitude)
{
    Vector3 v(3, 4, 0);  // 長さ5
    Vector3 c = Vector3::ClampMagnitude(v, 2.5f);
    EXPECT_NEAR(c.Length(), 2.5f, 1e-5f);

    c = Vector3::ClampMagnitude(v, 10.0f);
    EXPECT_FLOAT_EQ(c.x, 3.0f);
    EXPECT_FLOAT_EQ(c.y, 4.0f);
}

TEST(Vector3ExtTest, Slerp)
{
    Vector3 a(1, 0, 0), b(0, 1, 0);
    Vector3 mid = Vector3::Slerp(a, b, 0.5f);
    EXPECT_NEAR(mid.Length(), 1.0f, 1e-4f);
    EXPECT_NEAR(mid.x, mid.y, 1e-4f);  // 45度 → x == y

    // t=0 → a
    Vector3 s0 = Vector3::Slerp(a, b, 0.0f);
    EXPECT_NEAR(s0.x, 1.0f, 1e-4f);
    EXPECT_NEAR(s0.y, 0.0f, 1e-4f);

    // t=1 → b
    Vector3 s1 = Vector3::Slerp(a, b, 1.0f);
    EXPECT_NEAR(s1.x, 0.0f, 1e-4f);
    EXPECT_NEAR(s1.y, 1.0f, 1e-4f);
}

TEST(Vector3ExtTest, Angle)
{
    Vector3 a(1, 0, 0), b(0, 1, 0);
    EXPECT_NEAR(Vector3::Angle(a, b), 90.0f, 0.01f);  // FastAcos精度
    EXPECT_NEAR(Vector3::Angle(a, a), 0.0f, 0.01f);

    Vector3 c(-1, 0, 0);
    EXPECT_NEAR(Vector3::Angle(a, c), 180.0f, 0.01f);
}

TEST(Vector3ExtTest, SignedAngle)
{
    Vector3 a(1, 0, 0), b(0, 1, 0), axis(0, 0, 1);
    EXPECT_NEAR(Vector3::SignedAngle(a, b, axis), 90.0f, 0.01f);
    EXPECT_NEAR(Vector3::SignedAngle(b, a, axis), -90.0f, 0.01f);
}

TEST(Vector3ExtTest, LerpSIMD)
{
    Vector3 a(1, 2, 3), b(5, 6, 7);
    Vector3 r = Vector3::Lerp(a, b, 0.25f);
    EXPECT_NEAR(r.x, 2.0f, 1e-5f);
    EXPECT_NEAR(r.y, 3.0f, 1e-5f);
    EXPECT_NEAR(r.z, 4.0f, 1e-5f);
}

// ============================================================================
// Vector4 追加関数
// ============================================================================

TEST(Vector4ExtTest, Min)
{
    Vector4 a(1, 5, 3, 8), b(4, 2, 6, 1);
    Vector4 r = Vector4::Min(a, b);
    EXPECT_FLOAT_EQ(r.x, 1.0f);
    EXPECT_FLOAT_EQ(r.y, 2.0f);
    EXPECT_FLOAT_EQ(r.z, 3.0f);
    EXPECT_FLOAT_EQ(r.w, 1.0f);
}

TEST(Vector4ExtTest, Max)
{
    Vector4 a(1, 5, 3, 8), b(4, 2, 6, 1);
    Vector4 r = Vector4::Max(a, b);
    EXPECT_FLOAT_EQ(r.x, 4.0f);
    EXPECT_FLOAT_EQ(r.y, 5.0f);
    EXPECT_FLOAT_EQ(r.z, 6.0f);
    EXPECT_FLOAT_EQ(r.w, 8.0f);
}

TEST(Vector4ExtTest, LerpSIMD)
{
    Vector4 a(0, 0, 0, 0), b(10, 20, 30, 40);
    Vector4 r = Vector4::Lerp(a, b, 0.5f);
    EXPECT_NEAR(r.x, 5.0f, 1e-5f);
    EXPECT_NEAR(r.y, 10.0f, 1e-5f);
    EXPECT_NEAR(r.z, 15.0f, 1e-5f);
    EXPECT_NEAR(r.w, 20.0f, 1e-5f);
}

// ============================================================================
// Quaternion 追加関数
// ============================================================================

TEST(QuaternionExtTest, Euler)
{
    // Euler は FromEuler のエイリアス
    Quaternion a = Quaternion::Euler(0.5f, 1.0f, 0.3f);
    Quaternion b = Quaternion::FromEuler(0.5f, 1.0f, 0.3f);
    EXPECT_FLOAT_EQ(a.x, b.x);
    EXPECT_FLOAT_EQ(a.y, b.y);
    EXPECT_FLOAT_EQ(a.z, b.z);
    EXPECT_FLOAT_EQ(a.w, b.w);
}

TEST(QuaternionExtTest, LookRotation)
{
    // Z方向を向く → 単位回転
    Quaternion q = Quaternion::LookRotation(Vector3::Forward());
    // Forward=(0,0,1)をqで回転すると(0,0,1)になる
    Vector3 fwd = q.RotateVector(Vector3::Forward());
    EXPECT_NEAR(fwd.x, 0.0f, 1e-4f);
    EXPECT_NEAR(fwd.y, 0.0f, 1e-4f);
    EXPECT_NEAR(fwd.z, 1.0f, 1e-4f);

    // X方向を向く
    Quaternion qx = Quaternion::LookRotation(Vector3::Right());
    Vector3 fwdX = qx.RotateVector(Vector3::Forward());
    EXPECT_NEAR(fwdX.x, 1.0f, 1e-4f);
    EXPECT_NEAR(fwdX.y, 0.0f, 1e-4f);
    EXPECT_NEAR(fwdX.z, 0.0f, 1e-4f);
}

TEST(QuaternionExtTest, FromToRotation)
{
    Vector3 from(1, 0, 0), to(0, 1, 0);
    Quaternion q = Quaternion::FromToRotation(from, to);
    Vector3 result = q.RotateVector(from);
    EXPECT_NEAR(result.x, to.x, 1e-4f);
    EXPECT_NEAR(result.y, to.y, 1e-4f);
    EXPECT_NEAR(result.z, to.z, 1e-4f);

    // 同じ方向 → 単位回転
    Quaternion id = Quaternion::FromToRotation(from, from);
    EXPECT_NEAR(id.w, 1.0f, 1e-4f);
}

TEST(QuaternionExtTest, FromToRotation180)
{
    // 180度回転
    Vector3 from(1, 0, 0), to(-1, 0, 0);
    Quaternion q = Quaternion::FromToRotation(from, to);
    Vector3 result = q.RotateVector(from);
    EXPECT_NEAR(result.x, -1.0f, 1e-3f);
    EXPECT_NEAR(result.y, 0.0f, 1e-3f);
    EXPECT_NEAR(result.z, 0.0f, 1e-3f);
}

TEST(QuaternionExtTest, AngleBetween)
{
    Quaternion a = Quaternion::Identity();
    Quaternion b = Quaternion::FromAxisAngle(Vector3::Up(), MathUtil::PI / 2.0f);
    float angle = Quaternion::Angle(a, b);
    EXPECT_NEAR(angle, MathUtil::PI / 2.0f, 1e-4f);
}

TEST(QuaternionExtTest, RotateTowards)
{
    Quaternion a = Quaternion::Identity();
    Quaternion b = Quaternion::FromAxisAngle(Vector3::Up(), MathUtil::PI);
    // 小ステップ
    Quaternion r = Quaternion::RotateTowards(a, b, 0.1f);
    float angle = Quaternion::Angle(a, r);
    EXPECT_NEAR(angle, 0.1f, 5e-4f);  // FastAcos/FastSin精度

    // 大きなステップ → 目標到達
    r = Quaternion::RotateTowards(a, b, 100.0f);
    float finalAngle = Quaternion::Angle(r, b);
    EXPECT_NEAR(finalAngle, 0.0f, 5e-4f);
}

// ============================================================================
// Matrix4x4 追加関数
// ============================================================================

TEST(Matrix4x4ExtTest, TRS)
{
    Vector3 pos(10, 20, 30);
    Quaternion rot = Quaternion::FromAxisAngle(Vector3::Up(), MathUtil::PI / 2.0f);
    Vector3 scale(2, 3, 4);

    Matrix4x4 m = Matrix4x4::TRS(pos, rot, scale);

    // 平行移動の検証
    EXPECT_NEAR(m._41, 10.0f, 1e-4f);
    EXPECT_NEAR(m._42, 20.0f, 1e-4f);
    EXPECT_NEAR(m._43, 30.0f, 1e-4f);

    // 原点をTRSで変換すると平行移動分になる
    Vector3 origin = m.TransformPoint(Vector3::Zero());
    EXPECT_NEAR(origin.x, 10.0f, 1e-4f);
    EXPECT_NEAR(origin.y, 20.0f, 1e-4f);
    EXPECT_NEAR(origin.z, 30.0f, 1e-4f);

    // 単位ベクトルの変換で回転+スケールの検証
    Vector3 xAxis = m.TransformVector(Vector3(1, 0, 0));
    EXPECT_NEAR(xAxis.Length(), 2.0f, 1e-3f);  // x方向のスケール
}

TEST(Matrix4x4ExtTest, TRSIdentity)
{
    Matrix4x4 m = Matrix4x4::TRS(Vector3::Zero(), Quaternion::Identity(), Vector3::One());
    Matrix4x4 id = Matrix4x4::Identity();
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            EXPECT_NEAR(m.m[i][j], id.m[i][j], 1e-5f);
}

TEST(Matrix4x4ExtTest, DeterminantIdentity)
{
    EXPECT_NEAR(Matrix4x4::Identity().Determinant(), 1.0f, 1e-5f);
}

TEST(Matrix4x4ExtTest, DeterminantScaling)
{
    Matrix4x4 s = Matrix4x4::Scaling(2.0f, 3.0f, 4.0f);
    EXPECT_NEAR(s.Determinant(), 24.0f, 1e-4f);
}

// ============================================================================
// Color SIMD 演算
// ============================================================================

TEST(ColorSIMDTest, ScalarMul)
{
    Color c(0.5f, 0.25f, 0.125f, 1.0f);
    Color r = c * 2.0f;
    EXPECT_NEAR(r.r, 1.0f, 1e-5f);
    EXPECT_NEAR(r.g, 0.5f, 1e-5f);
    EXPECT_NEAR(r.b, 0.25f, 1e-5f);
    EXPECT_NEAR(r.a, 2.0f, 1e-5f);
}

TEST(ColorSIMDTest, Add)
{
    Color a(0.1f, 0.2f, 0.3f, 0.4f);
    Color b(0.5f, 0.6f, 0.7f, 0.6f);
    Color r = a + b;
    EXPECT_NEAR(r.r, 0.6f, 1e-5f);
    EXPECT_NEAR(r.g, 0.8f, 1e-5f);
    EXPECT_NEAR(r.b, 1.0f, 1e-5f);
    EXPECT_NEAR(r.a, 1.0f, 1e-5f);
}

TEST(ColorSIMDTest, Lerp)
{
    Color a = Color::Black();
    Color b = Color::White();
    Color mid = Color::Lerp(a, b, 0.5f);
    EXPECT_NEAR(mid.r, 0.5f, 1e-5f);
    EXPECT_NEAR(mid.g, 0.5f, 1e-5f);
    EXPECT_NEAR(mid.b, 0.5f, 1e-5f);
    EXPECT_NEAR(mid.a, 1.0f, 1e-5f);  // alpha: 1→1 = 1
}

// ============================================================================
// Batch API
// ============================================================================

TEST(BatchTest, TransformPointsBasic)
{
    Matrix4x4 m = Matrix4x4::Translation(10, 0, 0);
    Vector3 src[] = { {0,0,0}, {1,2,3} };
    Vector3 dst[2];
    gx::batch::TransformPoints(dst, src, m, 2);
    EXPECT_NEAR(dst[0].x, 10.0f, 1e-4f);
    EXPECT_NEAR(dst[1].x, 11.0f, 1e-4f);
    EXPECT_NEAR(dst[1].y, 2.0f, 1e-4f);
    EXPECT_NEAR(dst[1].z, 3.0f, 1e-4f);
}

TEST(BatchTest, TransformPointsZeroCount)
{
    Matrix4x4 m = Matrix4x4::Identity();
    gx::batch::TransformPoints(nullptr, nullptr, m, 0);
    // ゼロ個でクラッシュしないことを確認
    SUCCEED();
}

TEST(BatchTest, TransformVectors)
{
    Matrix4x4 m = Matrix4x4::Translation(100, 200, 300);
    Vector3 src[] = { {1,0,0} };
    Vector3 dst[1];
    gx::batch::TransformVectors(dst, src, m, 1);
    // 平行移動の影響を受けない
    EXPECT_NEAR(dst[0].x, 1.0f, 1e-4f);
    EXPECT_NEAR(dst[0].y, 0.0f, 1e-4f);
    EXPECT_NEAR(dst[0].z, 0.0f, 1e-4f);
}

TEST(BatchTest, NormalizeArray)
{
    Vector3 src[] = { {3,4,0}, {0,0,5} };
    Vector3 dst[2];
    gx::batch::NormalizeArray(dst, src, 2);
    EXPECT_NEAR(dst[0].Length(), 1.0f, 1e-4f);
    EXPECT_NEAR(dst[0].x, 0.6f, 1e-4f);
    EXPECT_NEAR(dst[0].y, 0.8f, 1e-4f);
    EXPECT_NEAR(dst[1].z, 1.0f, 1e-4f);
}

TEST(BatchTest, NormalizeArrayZeroCount)
{
    gx::batch::NormalizeArray(nullptr, nullptr, 0);
    SUCCEED();
}

TEST(BatchTest, LerpArray)
{
    Vector3 a[] = { {0,0,0}, {10,10,10} };
    Vector3 b[] = { {10,20,30}, {20,30,40} };
    Vector3 dst[2];
    gx::batch::LerpArray(dst, a, b, 0.5f, 2);
    EXPECT_NEAR(dst[0].x, 5.0f, 1e-4f);
    EXPECT_NEAR(dst[0].y, 10.0f, 1e-4f);
    EXPECT_NEAR(dst[0].z, 15.0f, 1e-4f);
    EXPECT_NEAR(dst[1].x, 15.0f, 1e-4f);
}

TEST(BatchTest, DotArray)
{
    Vector3 a[] = { {1,0,0}, {0,1,0} };
    Vector3 b[] = { {1,0,0}, {0,0,1} };
    float dst[2];
    gx::batch::DotArray(dst, a, b, 2);
    EXPECT_NEAR(dst[0], 1.0f, 1e-5f);
    EXPECT_NEAR(dst[1], 0.0f, 1e-5f);
}

TEST(BatchTest, SingleElement)
{
    Vector3 src = {3, 4, 0};
    Vector3 dst;
    gx::batch::NormalizeArray(&dst, &src, 1);
    EXPECT_NEAR(dst.Length(), 1.0f, 1e-4f);
}
