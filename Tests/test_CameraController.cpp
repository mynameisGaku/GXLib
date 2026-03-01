/// @file test_CameraController.cpp
/// @brief CameraController（シェイク、レール、オービット）のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/CameraController.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/Rendering/Camera2D.h"

using namespace gx;

// =========================================================================
// カメラシェイクテスト
// =========================================================================

TEST(CameraController_Shake, StartShake_BeginsShaking)
{
    CameraController ctrl;
    EXPECT_FALSE(ctrl.IsShaking());

    CameraShakeParams params;
    params.amplitude = 1.0f;
    params.duration = 0.5f;
    ctrl.StartShake(params);
    EXPECT_TRUE(ctrl.IsShaking());
}

TEST(CameraController_Shake, StartShake_SimpleOverload)
{
    CameraController ctrl;
    ctrl.StartShake(0.5f, 1.0f); // 振幅, 継続時間
    EXPECT_TRUE(ctrl.IsShaking());
}

TEST(CameraController_Shake, StopShake)
{
    CameraController ctrl;
    CameraShakeParams params;
    params.amplitude = 1.0f;
    params.duration = 2.0f;
    ctrl.StartShake(params);
    EXPECT_TRUE(ctrl.IsShaking());

    ctrl.StopShake();
    EXPECT_FALSE(ctrl.IsShaking());
}

TEST(CameraController_Shake, IsShaking_CorrectDuringLifetime)
{
    CameraController ctrl;
    EXPECT_FALSE(ctrl.IsShaking());

    CameraShakeParams params;
    params.amplitude = 0.5f;
    params.duration = 0.3f;
    ctrl.StartShake(params);
    EXPECT_TRUE(ctrl.IsShaking());

    // 途中まで更新
    ctrl.Update(0.1f);
    EXPECT_TRUE(ctrl.IsShaking());
}

TEST(CameraController_Shake, Update_ReducesTimer)
{
    CameraController ctrl;
    CameraShakeParams params;
    params.amplitude = 0.5f;
    params.duration = 0.5f;
    ctrl.StartShake(params);

    // 継続時間を超えて更新した後、シェイクは終了しているはず
    ctrl.Update(0.6f);
    EXPECT_FALSE(ctrl.IsShaking());
}

TEST(CameraController_Shake, ShakeOffset_NonZeroDuringShake)
{
    CameraController ctrl;
    CameraShakeParams params;
    params.amplitude = 2.0f;
    params.frequency = 20.0f;
    params.duration = 1.0f;
    params.dampingRate = 1.0f;
    ctrl.StartShake(params);
    ctrl.Update(0.05f);

    XMFLOAT3 offset = ctrl.GetShakeOffset();
    // シェイク中は少なくとも1つの成分がゼロでないはず
    bool anyNonZero = (offset.x != 0.0f) || (offset.y != 0.0f) || (offset.z != 0.0f);
    EXPECT_TRUE(anyNonZero);
}

TEST(CameraController_Shake, ShakeOffset_ZeroAfterDuration)
{
    CameraController ctrl;
    CameraShakeParams params;
    params.amplitude = 1.0f;
    params.duration = 0.2f;
    ctrl.StartShake(params);

    // 継続時間を大きく超えて更新
    ctrl.Update(0.5f);
    XMFLOAT3 offset = ctrl.GetShakeOffset();
    EXPECT_FLOAT_EQ(offset.x, 0.0f);
    EXPECT_FLOAT_EQ(offset.y, 0.0f);
    EXPECT_FLOAT_EQ(offset.z, 0.0f);
}

TEST(CameraController_Shake, ShakeOffset_ZeroWhenNotShaking)
{
    CameraController ctrl;
    XMFLOAT3 offset = ctrl.GetShakeOffset();
    EXPECT_FLOAT_EQ(offset.x, 0.0f);
    EXPECT_FLOAT_EQ(offset.y, 0.0f);
    EXPECT_FLOAT_EQ(offset.z, 0.0f);
}

TEST(CameraController_Shake, ApplyShakeToCamera2D)
{
    CameraController ctrl;
    Camera2D cam;
    cam.SetPosition(100.0f, 200.0f);

    CameraShakeParams params;
    params.amplitude = 5.0f;
    params.duration = 1.0f;
    params.frequency = 20.0f;
    ctrl.StartShake(params);
    ctrl.Update(0.05f);

    // 2Dカメラにシェイクを適用 -- クラッシュしないこと
    ctrl.ApplyShakeToCamera2D(cam);
    // カメラ位置がシェイクオフセットにより変更されている可能性あり
    SUCCEED();
}

// =========================================================================
// カメラレールテスト
// =========================================================================

TEST(CameraController_Rail, AddRailPoint_IncreasesCount)
{
    CameraController ctrl;
    EXPECT_EQ(ctrl.GetRailPointCount(), 0u);

    CameraRailPoint p1;
    p1.position = { 0, 0, 0 };
    p1.lookTarget = { 0, 0, 1 };
    ctrl.AddRailPoint(p1);
    EXPECT_EQ(ctrl.GetRailPointCount(), 1u);

    CameraRailPoint p2;
    p2.position = { 10, 0, 0 };
    p2.lookTarget = { 10, 0, 1 };
    ctrl.AddRailPoint(p2);
    EXPECT_EQ(ctrl.GetRailPointCount(), 2u);
}

TEST(CameraController_Rail, ClearRail_Resets)
{
    CameraController ctrl;
    CameraRailPoint p;
    p.position = { 0, 0, 0 };
    ctrl.AddRailPoint(p);
    ctrl.AddRailPoint(p);
    EXPECT_EQ(ctrl.GetRailPointCount(), 2u);

    ctrl.ClearRail();
    EXPECT_EQ(ctrl.GetRailPointCount(), 0u);
}

TEST(CameraController_Rail, StartRail_StopRail)
{
    CameraController ctrl;
    CameraRailPoint p1; p1.position = { 0, 0, 0 };
    CameraRailPoint p2; p2.position = { 10, 0, 0 };
    ctrl.AddRailPoint(p1);
    ctrl.AddRailPoint(p2);

    EXPECT_FALSE(ctrl.IsOnRail());
    ctrl.StartRail();
    EXPECT_TRUE(ctrl.IsOnRail());

    ctrl.StopRail();
    EXPECT_FALSE(ctrl.IsOnRail());
}

TEST(CameraController_Rail, SetRailProgress)
{
    CameraController ctrl;
    CameraRailPoint p1; p1.position = { 0, 0, 0 };
    CameraRailPoint p2; p2.position = { 10, 0, 0 };
    ctrl.AddRailPoint(p1);
    ctrl.AddRailPoint(p2);
    ctrl.StartRail();

    ctrl.SetRailProgress(0.5f);
    EXPECT_FLOAT_EQ(ctrl.GetRailProgress(), 0.5f);

    ctrl.SetRailProgress(0.0f);
    EXPECT_FLOAT_EQ(ctrl.GetRailProgress(), 0.0f);

    ctrl.SetRailProgress(1.0f);
    EXPECT_FLOAT_EQ(ctrl.GetRailProgress(), 1.0f);
}

TEST(CameraController_Rail, GetRailPosition_Interpolates)
{
    CameraController ctrl;
    CameraRailPoint p1; p1.position = { 0, 0, 0 }; p1.lookTarget = { 0, 0, 1 };
    CameraRailPoint p2; p2.position = { 10, 0, 0 }; p2.lookTarget = { 10, 0, 1 };
    ctrl.AddRailPoint(p1);
    ctrl.AddRailPoint(p2);
    ctrl.StartRail();

    // 進行度0では、位置は最初のポイントにあるはず
    ctrl.SetRailProgress(0.0f);
    XMFLOAT3 pos0 = ctrl.GetRailPosition();
    EXPECT_NEAR(pos0.x, 0.0f, 0.5f);

    // 進行度1では、位置は最後のポイントにあるはず
    ctrl.SetRailProgress(1.0f);
    XMFLOAT3 pos1 = ctrl.GetRailPosition();
    EXPECT_NEAR(pos1.x, 10.0f, 0.5f);

    // 進行度0.5では、位置はおおよそ中間にあるはず
    ctrl.SetRailProgress(0.5f);
    XMFLOAT3 posHalf = ctrl.GetRailPosition();
    EXPECT_NEAR(posHalf.x, 5.0f, 2.0f);
}

TEST(CameraController_Rail, LoopWrapping)
{
    CameraController ctrl;
    CameraRailPoint p1; p1.position = { 0, 0, 0 };
    CameraRailPoint p2; p2.position = { 10, 0, 0 };
    ctrl.AddRailPoint(p1);
    ctrl.AddRailPoint(p2);
    ctrl.StartRail(true); // ループ=true

    // 1.0を超える進行度を設定するとラップアラウンドするはず
    ctrl.SetRailProgress(1.5f);
    float progress = ctrl.GetRailProgress();
    // ループモードでは、進行度はラップされるはず（クランプまたはモジュロ）
    EXPECT_GE(progress, 0.0f);
    EXPECT_LE(progress, 1.0f);
}

// =========================================================================
// オービットカメラテスト
// =========================================================================

TEST(CameraController_Orbit, SetOrbitConfig)
{
    CameraController ctrl;
    OrbitConfig cfg;
    cfg.distance = 10.0f;
    cfg.minDistance = 2.0f;
    cfg.maxDistance = 100.0f;
    cfg.pitchMin = -1.0f;
    cfg.pitchMax = 1.0f;
    ctrl.SetOrbitConfig(cfg);

    const auto& retrieved = ctrl.GetOrbitConfig();
    EXPECT_FLOAT_EQ(retrieved.distance, 10.0f);
    EXPECT_FLOAT_EQ(retrieved.minDistance, 2.0f);
    EXPECT_FLOAT_EQ(retrieved.maxDistance, 100.0f);
    EXPECT_FLOAT_EQ(retrieved.pitchMin, -1.0f);
    EXPECT_FLOAT_EQ(retrieved.pitchMax, 1.0f);
}

TEST(CameraController_Orbit, OrbitRotate_ChangesYaw)
{
    CameraController ctrl;
    float initialYaw = ctrl.GetOrbitYaw();
    ctrl.OrbitRotate(0.5f, 0.0f);
    EXPECT_NE(ctrl.GetOrbitYaw(), initialYaw);
}

TEST(CameraController_Orbit, OrbitRotate_ChangesPitch)
{
    CameraController ctrl;
    float initialPitch = ctrl.GetOrbitPitch();
    ctrl.OrbitRotate(0.0f, 0.3f);
    EXPECT_NE(ctrl.GetOrbitPitch(), initialPitch);
}

TEST(CameraController_Orbit, PitchClamping)
{
    CameraController ctrl;
    OrbitConfig cfg;
    cfg.pitchMin = -1.0f;
    cfg.pitchMax = 1.0f;
    ctrl.SetOrbitConfig(cfg);

    // 非常に大きなピッチ回転を適用
    ctrl.OrbitRotate(0.0f, 100.0f);
    float pitch = ctrl.GetOrbitPitch();
    EXPECT_LE(pitch, cfg.pitchMax + 0.01f);

    // 非常に大きな負のピッチを適用
    ctrl.OrbitRotate(0.0f, -200.0f);
    pitch = ctrl.GetOrbitPitch();
    EXPECT_GE(pitch, cfg.pitchMin - 0.01f);
}

TEST(CameraController_Orbit, OrbitZoom_ChangesDistance)
{
    CameraController ctrl;
    OrbitConfig cfg;
    cfg.distance = 5.0f;
    cfg.minDistance = 1.0f;
    cfg.maxDistance = 50.0f;
    ctrl.SetOrbitConfig(cfg);

    float initialDist = ctrl.GetOrbitDistance();
    ctrl.OrbitZoom(-1.0f); // ズームイン
    EXPECT_NE(ctrl.GetOrbitDistance(), initialDist);
}

TEST(CameraController_Orbit, DistanceClamping)
{
    CameraController ctrl;
    OrbitConfig cfg;
    cfg.distance = 5.0f;
    cfg.minDistance = 2.0f;
    cfg.maxDistance = 20.0f;
    cfg.zoomSpeed = 1.0f;
    ctrl.SetOrbitConfig(cfg);

    // 非常に遠くまでズームイン
    for (int i = 0; i < 100; ++i)
        ctrl.OrbitZoom(-1.0f);
    EXPECT_GE(ctrl.GetOrbitDistance(), cfg.minDistance - 0.01f);

    // 非常に遠くまでズームアウト
    for (int i = 0; i < 200; ++i)
        ctrl.OrbitZoom(1.0f);
    EXPECT_LE(ctrl.GetOrbitDistance(), cfg.maxDistance + 0.01f);
}

TEST(CameraController_Orbit, SetOrbitTarget)
{
    CameraController ctrl;
    XMFLOAT3 target = { 5.0f, 3.0f, -2.0f };
    ctrl.SetOrbitTarget(target);
    XMFLOAT3 retrieved = ctrl.GetOrbitTarget();
    EXPECT_FLOAT_EQ(retrieved.x, 5.0f);
    EXPECT_FLOAT_EQ(retrieved.y, 3.0f);
    EXPECT_FLOAT_EQ(retrieved.z, -2.0f);
}

TEST(CameraController_Orbit, ApplyOrbitToCamera_SetsPosition)
{
    CameraController ctrl;
    OrbitConfig cfg;
    cfg.distance = 10.0f;
    ctrl.SetOrbitConfig(cfg);
    ctrl.SetOrbitTarget({ 0.0f, 0.0f, 0.0f });

    Camera3D camera;
    camera.SetPosition(0.0f, 0.0f, 0.0f);

    ctrl.ApplyOrbitToCamera(camera);

    // カメラは原点からある距離にあるはず
    const auto& pos = camera.GetPosition();
    float dist = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    EXPECT_NEAR(dist, 10.0f, 1.0f);
}

// =========================================================================
// 一般的な更新
// =========================================================================

TEST(CameraController, Update_NoActiveFeature_NoCrash)
{
    CameraController ctrl;
    // アクティブな機能がない状態で更新を呼んでもクラッシュしないこと
    ctrl.Update(0.016f);
    SUCCEED();
}
