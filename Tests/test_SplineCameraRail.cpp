/// @file test_SplineCameraRail.cpp
/// @brief スプラインカメラレール (CameraController拡張) のユニットテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/CameraController.h"
#include "Graphics/3D/Camera3D.h"

using namespace gx;

// ============================================================================
// SplineRailParams 構造体テスト
// ============================================================================

TEST(SplineCameraRail, Params_DefaultValues)
{
    SplineRailParams params;
    EXPECT_FLOAT_EQ(params.speed, 1.0f);
    EXPECT_FLOAT_EQ(params.tension, 0.5f);
    EXPECT_FALSE(params.loop);
    EXPECT_TRUE(params.easeInOut);
    EXPECT_FLOAT_EQ(params.easeDuration, 0.5f);
    EXPECT_EQ(params.controlPoints.size(), 0u);
    EXPECT_EQ(params.lookAtPoints.size(), 0u);
}

// ============================================================================
// StartSplineRail / StopSplineRail
// ============================================================================

TEST(SplineCameraRail, Start_NotEnoughPoints_DoesNotActivate)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 }); // 1点では不足
    ctrl.StartSplineRail(params);
    EXPECT_FALSE(ctrl.IsSplineRailActive());
}

TEST(SplineCameraRail, Start_TwoPoints_Activates)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    ctrl.StartSplineRail(params);
    EXPECT_TRUE(ctrl.IsSplineRailActive());
}

TEST(SplineCameraRail, Stop_Deactivates)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    ctrl.StartSplineRail(params);
    EXPECT_TRUE(ctrl.IsSplineRailActive());

    ctrl.StopSplineRail();
    EXPECT_FALSE(ctrl.IsSplineRailActive());
}

// ============================================================================
// GetSplineProgress
// ============================================================================

TEST(SplineCameraRail, Progress_StartsAtZero)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    ctrl.StartSplineRail(params);

    EXPECT_NEAR(ctrl.GetSplineProgress(), 0.0f, 0.01f);
}

TEST(SplineCameraRail, Progress_AdvancesWithUpdate)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.speed = 5.0f;
    params.easeInOut = false;  // イーズ無効で単純な進行をテスト
    ctrl.StartSplineRail(params);

    float initialProgress = ctrl.GetSplineProgress();
    ctrl.Update(0.5f);
    float laterProgress = ctrl.GetSplineProgress();

    EXPECT_GT(laterProgress, initialProgress);
}

TEST(SplineCameraRail, Progress_CompletesAndDeactivates)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.speed = 100.0f;  // 高速で素早く完了
    params.easeInOut = false;
    params.loop = false;
    ctrl.StartSplineRail(params);

    // 十分な時間更新して完了させる
    ctrl.Update(10.0f);

    EXPECT_FALSE(ctrl.IsSplineRailActive());
    EXPECT_NEAR(ctrl.GetSplineProgress(), 1.0f, 0.01f);
}

// ============================================================================
// ループ動作
// ============================================================================

TEST(SplineCameraRail, Loop_StaysActive)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.speed = 100.0f;
    params.easeInOut = false;
    params.loop = true;
    ctrl.StartSplineRail(params);

    // 高速で更新しても停止しないこと
    ctrl.Update(10.0f);
    EXPECT_TRUE(ctrl.IsSplineRailActive());

    // 進行度は0~1の範囲内
    float progress = ctrl.GetSplineProgress();
    EXPECT_GE(progress, 0.0f);
    EXPECT_LE(progress, 1.0f);
}

TEST(SplineCameraRail, Loop_WrapsAround)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 5, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.speed = 50.0f;
    params.easeInOut = false;
    params.loop = true;
    ctrl.StartSplineRail(params);

    // 複数回ループしてもアクティブ
    for (int i = 0; i < 10; ++i)
        ctrl.Update(1.0f);

    EXPECT_TRUE(ctrl.IsSplineRailActive());
}

// ============================================================================
// GetSplinePosition
// ============================================================================

TEST(SplineCameraRail, Position_AtStart_NearFirstPoint)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.controlPoints.push_back({ 20, 0, 0 });
    params.tension = 0.0f;  // 標準Catmull-Rom
    ctrl.StartSplineRail(params);

    Vector3 pos = ctrl.GetSplinePosition();
    EXPECT_NEAR(pos.x, 0.0f, 0.5f);
    EXPECT_NEAR(pos.y, 0.0f, 0.5f);
    EXPECT_NEAR(pos.z, 0.0f, 0.5f);
}

TEST(SplineCameraRail, Position_AtEnd_NearLastPoint)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.controlPoints.push_back({ 20, 0, 0 });
    params.speed = 1000.0f;
    params.easeInOut = false;
    params.loop = false;
    params.tension = 0.0f;
    ctrl.StartSplineRail(params);

    ctrl.Update(100.0f);  // 完了まで進める
    Vector3 pos = ctrl.GetSplinePosition();
    EXPECT_NEAR(pos.x, 20.0f, 0.5f);
    EXPECT_NEAR(pos.y, 0.0f, 0.5f);
    EXPECT_NEAR(pos.z, 0.0f, 0.5f);
}

TEST(SplineCameraRail, Position_Midway_BetweenPoints)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.tension = 1.0f;  // テンション=1は直線補間に近い
    params.easeInOut = false;
    ctrl.StartSplineRail(params);

    // 総距離の半分進める
    // テンション=1ではtangent=0なので直線的になる
    // 2点の場合の総距離は約10
    float totalLen = 10.0f;
    float halfTime = totalLen * 0.5f / params.speed;
    ctrl.Update(halfTime);

    Vector3 pos = ctrl.GetSplinePosition();
    // テンション1で2点だけなので、Hermite補間で中間付近になるはず
    EXPECT_GT(pos.x, 1.0f);
    EXPECT_LT(pos.x, 9.0f);
}

// ============================================================================
// GetSplineLookTarget
// ============================================================================

TEST(SplineCameraRail, LookTarget_WithLookAtPoints)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.controlPoints.push_back({ 20, 0, 0 });
    params.lookAtPoints.push_back({ 0, 5, 0 });
    params.lookAtPoints.push_back({ 10, 5, 0 });
    params.lookAtPoints.push_back({ 20, 5, 0 });
    params.tension = 0.0f;
    ctrl.StartSplineRail(params);

    Vector3 target = ctrl.GetSplineLookTarget();
    // 開始時は最初の注視点付近
    EXPECT_NEAR(target.y, 5.0f, 1.0f);
}

TEST(SplineCameraRail, LookTarget_WithoutLookAtPoints_UsesPathTangent)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.controlPoints.push_back({ 20, 0, 0 });
    // lookAtPointsは空
    params.tension = 0.0f;
    params.easeInOut = false;
    ctrl.StartSplineRail(params);

    // 少し進めてから注視点を取得
    ctrl.Update(0.1f);
    Vector3 pos = ctrl.GetSplinePosition();
    Vector3 target = ctrl.GetSplineLookTarget();

    // 注視点は位置の前方にあるはず
    float dx = target.x - pos.x;
    float dy = target.y - pos.y;
    float dz = target.z - pos.z;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    // 正規化された接線を位置に足したものなので距離は約1.0
    EXPECT_NEAR(dist, 1.0f, 0.5f);
}

// ============================================================================
// SetSplineSpeed
// ============================================================================

TEST(SplineCameraRail, SetSplineSpeed_AffectsProgress)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 100, 0, 0 });
    params.speed = 1.0f;
    params.easeInOut = false;
    ctrl.StartSplineRail(params);

    ctrl.Update(1.0f);
    float slowProgress = ctrl.GetSplineProgress();

    // リスタートして速度2倍
    ctrl.StartSplineRail(params);
    ctrl.SetSplineSpeed(2.0f);
    ctrl.Update(1.0f);
    float fastProgress = ctrl.GetSplineProgress();

    EXPECT_GT(fastProgress, slowProgress);
}

// ============================================================================
// テンションパラメータ
// ============================================================================

TEST(SplineCameraRail, Tension_Zero_StandardCatmullRom)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 5, 0, 0 });
    params.controlPoints.push_back({ 10, 5, 0 });
    params.controlPoints.push_back({ 15, 0, 0 });
    params.tension = 0.0f;
    ctrl.StartSplineRail(params);

    // 標準Catmull-Romはp1とp2を通過する
    Vector3 pos0 = ctrl.GetSplinePosition();
    EXPECT_NEAR(pos0.x, 0.0f, 0.5f);
}

TEST(SplineCameraRail, Tension_One_LinearLike)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.controlPoints.push_back({ 20, 0, 0 });
    params.tension = 1.0f;  // 直線に近い
    params.easeInOut = false;
    ctrl.StartSplineRail(params);

    // 半分進めた位置
    float totalLen = 20.0f;
    float halfTime = totalLen * 0.5f / params.speed;
    ctrl.Update(halfTime);

    Vector3 pos = ctrl.GetSplinePosition();
    // テンション1でも制御点を通過する (Hermite基底の性質)
    // X方向に直線的な配置なので中間付近
    EXPECT_GT(pos.x, 2.0f);
    EXPECT_LT(pos.x, 18.0f);
}

// ============================================================================
// イーズイン・アウト
// ============================================================================

TEST(SplineCameraRail, EaseInOut_SlowerAtStartAndEnd)
{
    // イーズありの場合、最初と最後は遅く中間は速い
    CameraController ctrl;

    // イーズなし
    SplineRailParams paramsNoEase;
    paramsNoEase.controlPoints.push_back({ 0, 0, 0 });
    paramsNoEase.controlPoints.push_back({ 100, 0, 0 });
    paramsNoEase.speed = 10.0f;
    paramsNoEase.easeInOut = false;
    ctrl.StartSplineRail(paramsNoEase);
    ctrl.Update(0.5f);
    float progressNoEase = ctrl.GetSplineProgress();

    // イーズあり
    SplineRailParams paramsEase;
    paramsEase.controlPoints.push_back({ 0, 0, 0 });
    paramsEase.controlPoints.push_back({ 100, 0, 0 });
    paramsEase.speed = 10.0f;
    paramsEase.easeInOut = true;
    paramsEase.easeDuration = 2.0f;
    ctrl.StartSplineRail(paramsEase);
    ctrl.Update(0.5f);
    float progressEase = ctrl.GetSplineProgress();

    // イーズイン中なのでイーズありの方が進行度が小さいはず
    EXPECT_LT(progressEase, progressNoEase);
}

// ============================================================================
// 多数の制御点
// ============================================================================

TEST(SplineCameraRail, ManyControlPoints)
{
    CameraController ctrl;
    SplineRailParams params;

    // 20個の制御点を円状に配置
    constexpr int kPoints = 20;
    for (int i = 0; i < kPoints; ++i)
    {
        float angle = static_cast<float>(i) * 2.0f * 3.14159f / static_cast<float>(kPoints);
        params.controlPoints.push_back({
            10.0f * std::cos(angle),
            0.0f,
            10.0f * std::sin(angle)
        });
    }
    params.loop = true;
    params.speed = 5.0f;
    params.easeInOut = false;
    ctrl.StartSplineRail(params);

    // 何回更新してもクラッシュしないこと
    for (int i = 0; i < 100; ++i)
    {
        ctrl.Update(0.016f);
        Vector3 pos = ctrl.GetSplinePosition();
        // 円の半径10なので、位置は概ね (-15, 0, -15) ~ (15, 0, 15) の範囲
        EXPECT_LT(std::abs(pos.x), 20.0f);
        EXPECT_LT(std::abs(pos.z), 20.0f);
    }
    EXPECT_TRUE(ctrl.IsSplineRailActive());
}

// ============================================================================
// 既存機能との共存
// ============================================================================

TEST(SplineCameraRail, CoexistsWithShake)
{
    CameraController ctrl;

    // シェイクとスプラインレールを同時にアクティブ化
    CameraShakeParams shake;
    shake.amplitude = 1.0f;
    shake.duration = 2.0f;
    ctrl.StartShake(shake);

    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.speed = 2.0f;
    params.easeInOut = false;
    ctrl.StartSplineRail(params);

    EXPECT_TRUE(ctrl.IsShaking());
    EXPECT_TRUE(ctrl.IsSplineRailActive());

    ctrl.Update(0.5f);

    // 両方がアクティブなままであること
    EXPECT_TRUE(ctrl.IsShaking());
    EXPECT_TRUE(ctrl.IsSplineRailActive());
}

TEST(SplineCameraRail, CoexistsWithRail)
{
    CameraController ctrl;

    // 旧レールとスプラインレールは独立
    CameraRailPoint rp1, rp2;
    rp1.position = { 0, 0, 0 };
    rp2.position = { 10, 0, 0 };
    ctrl.AddRailPoint(rp1);
    ctrl.AddRailPoint(rp2);
    ctrl.StartRail();

    SplineRailParams params;
    params.controlPoints.push_back({ 0, 5, 0 });
    params.controlPoints.push_back({ 10, 5, 0 });
    params.easeInOut = false;
    ctrl.StartSplineRail(params);

    EXPECT_TRUE(ctrl.IsOnRail());
    EXPECT_TRUE(ctrl.IsSplineRailActive());
}

// ============================================================================
// エッジケース
// ============================================================================

TEST(SplineCameraRail, InactiveByDefault)
{
    CameraController ctrl;
    EXPECT_FALSE(ctrl.IsSplineRailActive());
    EXPECT_NEAR(ctrl.GetSplineProgress(), 0.0f, 0.01f);
}

TEST(SplineCameraRail, Update_WhenInactive_NoCrash)
{
    CameraController ctrl;
    ctrl.Update(0.016f);
    EXPECT_FALSE(ctrl.IsSplineRailActive());
}

TEST(SplineCameraRail, GetPosition_WhenInactive_ReturnsZero)
{
    CameraController ctrl;
    Vector3 pos = ctrl.GetSplinePosition();
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
    EXPECT_FLOAT_EQ(pos.z, 0.0f);
}

TEST(SplineCameraRail, Restart_ResetsProgress)
{
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.speed = 2.0f;
    params.easeInOut = false;
    ctrl.StartSplineRail(params);
    ctrl.Update(1.0f);
    float midProgress = ctrl.GetSplineProgress();
    EXPECT_GT(midProgress, 0.0f);

    // 再開始すると進行度がリセットされる
    ctrl.StartSplineRail(params);
    EXPECT_NEAR(ctrl.GetSplineProgress(), 0.0f, 0.01f);
}

TEST(SplineCameraRail, CollinearPoints)
{
    // X軸上に並んだ3点（退化ケース）
    CameraController ctrl;
    SplineRailParams params;
    params.controlPoints.push_back({ 0, 0, 0 });
    params.controlPoints.push_back({ 5, 0, 0 });
    params.controlPoints.push_back({ 10, 0, 0 });
    params.tension = 0.0f;
    params.easeInOut = false;
    params.speed = 5.0f;
    ctrl.StartSplineRail(params);

    // 直線上なのでY,Zは常に0付近
    for (int i = 0; i < 20; ++i)
    {
        ctrl.Update(0.1f);
        Vector3 pos = ctrl.GetSplinePosition();
        EXPECT_NEAR(pos.y, 0.0f, 0.5f);
        EXPECT_NEAR(pos.z, 0.0f, 0.5f);
    }
}
