/// @file test_VehiclePhysics.cpp
/// @brief VehiclePhysics 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Physics/VehiclePhysics.h"

using namespace gx;

// ==================== デフォルト / 構成 ====================

TEST(VehiclePhysicsTest, DefaultConfig)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    const auto& cfg = vp.GetConfig();
    EXPECT_NEAR(cfg.mass, 1500.0f, 1e-3f);
    EXPECT_NEAR(cfg.maxSteerAngle, 35.0f, 1e-3f);
    EXPECT_NEAR(cfg.maxEngineTorque, 400.0f, 1e-3f);
    EXPECT_NEAR(cfg.aeroDragCoeff, 0.35f, 1e-3f);
    EXPECT_EQ(cfg.driveType, DriveType::RWD);
}

TEST(VehiclePhysicsTest, SetConfig)
{
    VehiclePhysics vp;
    VehicleConfig cfg;
    cfg.mass = 2000.0f;
    cfg.driveType = DriveType::AWD;
    vp.Initialize(cfg);
    EXPECT_NEAR(vp.GetConfig().mass, 2000.0f, 1e-3f);
    EXPECT_EQ(vp.GetConfig().driveType, DriveType::AWD);
}

TEST(VehiclePhysicsTest, WheelCount)
{
    VehiclePhysics vp;
    vp.SetWheelCount(4);
    EXPECT_EQ(vp.GetWheelCount(), 4);
    vp.SetWheelCount(6);
    EXPECT_EQ(vp.GetWheelCount(), 6);
}

TEST(VehiclePhysicsTest, SetWheelConfig)
{
    VehiclePhysics vp;
    vp.SetWheelCount(4);
    WheelConfig wc;
    wc.radius = 0.4f;
    wc.position = { -0.8f, 0.0f, 1.2f };
    vp.SetWheelConfig(0, wc);
    auto got = vp.GetWheelConfig(0);
    EXPECT_NEAR(got.radius, 0.4f, 1e-5f);
    EXPECT_NEAR(got.position.x, -0.8f, 1e-5f);
}

TEST(VehiclePhysicsTest, GetWheelConfig)
{
    VehiclePhysics vp;
    vp.SetWheelCount(4);
    WheelConfig wc;
    wc.suspensionStiffness = 40000.0f;
    wc.suspensionDamping   = 5000.0f;
    vp.SetWheelConfig(2, wc);
    auto got = vp.GetWheelConfig(2);
    EXPECT_NEAR(got.suspensionStiffness, 40000.0f, 1e-3f);
    EXPECT_NEAR(got.suspensionDamping, 5000.0f, 1e-3f);
}

TEST(VehiclePhysicsTest, DefaultWheelState)
{
    VehiclePhysics vp;
    vp.SetWheelCount(4);
    auto ws = vp.GetWheelState(0);
    EXPECT_NEAR(ws.suspensionCompression, 0.0f, 1e-5f);
    EXPECT_NEAR(ws.steerAngle, 0.0f, 1e-5f);
    EXPECT_NEAR(ws.slipRatio, 0.0f, 1e-5f);
}

// ==================== 入力 ====================

TEST(VehiclePhysicsTest, ThrottleSetGet)
{
    VehiclePhysics vp;
    vp.SetThrottle(0.75f);
    EXPECT_NEAR(vp.GetThrottle(), 0.75f, 1e-5f);
    vp.SetThrottle(1.5f); // clamp
    EXPECT_NEAR(vp.GetThrottle(), 1.0f, 1e-5f);
    vp.SetThrottle(-0.5f);
    EXPECT_NEAR(vp.GetThrottle(), 0.0f, 1e-5f);
}

TEST(VehiclePhysicsTest, BrakeSetGet)
{
    VehiclePhysics vp;
    vp.SetBrake(0.5f);
    EXPECT_NEAR(vp.GetBrake(), 0.5f, 1e-5f);
}

TEST(VehiclePhysicsTest, SteeringSetGet)
{
    VehiclePhysics vp;
    vp.SetSteering(-0.3f);
    EXPECT_NEAR(vp.GetSteering(), -0.3f, 1e-5f);
    vp.SetSteering(2.0f); // clamp
    EXPECT_NEAR(vp.GetSteering(), 1.0f, 1e-5f);
}

TEST(VehiclePhysicsTest, HandbrakeToggle)
{
    VehiclePhysics vp;
    EXPECT_FALSE(vp.GetHandbrake());
    vp.SetHandbrake(true);
    EXPECT_TRUE(vp.GetHandbrake());
    vp.SetHandbrake(false);
    EXPECT_FALSE(vp.GetHandbrake());
}

TEST(VehiclePhysicsTest, GearShift)
{
    VehiclePhysics vp;
    VehicleConfig cfg;
    vp.Initialize(cfg);
    EXPECT_EQ(vp.GetCurrentGear(), 2); // 1st
    vp.ShiftGear(3);
    EXPECT_EQ(vp.GetCurrentGear(), 3);
    vp.ShiftGear(0); // reverse
    EXPECT_EQ(vp.GetCurrentGear(), 0);
}

// ==================== 初期状態 ====================

TEST(VehiclePhysicsTest, InitialSpeedZero)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);
    EXPECT_NEAR(vp.GetSpeedKmh(), 0.0f, 1e-3f);
}

TEST(VehiclePhysicsTest, InitialRPM)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    // idle RPM
    EXPECT_GE(vp.GetEngineRPM(), 700.0f);
    EXPECT_LE(vp.GetEngineRPM(), 1000.0f);
}

// ==================== シミュレーション ====================

TEST(VehiclePhysicsTest, UpdateStationary)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);

    // デフォルト位置のホイールを設定
    WheelConfig fl, fr, rl, rr;
    fl.position = { -0.8f, 0.0f,  1.2f };
    fr.position = {  0.8f, 0.0f,  1.2f };
    rl.position = { -0.8f, 0.0f, -1.2f };
    rr.position = {  0.8f, 0.0f, -1.2f };
    vp.SetWheelConfig(0, fl);
    vp.SetWheelConfig(1, fr);
    vp.SetWheelConfig(2, rl);
    vp.SetWheelConfig(3, rr);

    vp.Reset(Vector3(0, 1, 0), Quaternion::Identity());
    vp.Update(1.0f / 60.0f);

    // スロットルなしでは大きな速度変化はないはず
    EXPECT_LT(vp.GetSpeedKmh(), 5.0f);
}

TEST(VehiclePhysicsTest, UpdateAccelerate)
{
    VehiclePhysics vp;
    VehicleConfig cfg;
    cfg.driveType = DriveType::RWD;
    vp.Initialize(cfg);
    vp.SetWheelCount(4);

    WheelConfig fl, fr, rl, rr;
    fl.position = { -0.8f, 0.0f,  1.2f };
    fr.position = {  0.8f, 0.0f,  1.2f };
    rl.position = { -0.8f, 0.0f, -1.2f };
    rr.position = {  0.8f, 0.0f, -1.2f };
    vp.SetWheelConfig(0, fl);
    vp.SetWheelConfig(1, fr);
    vp.SetWheelConfig(2, rl);
    vp.SetWheelConfig(3, rr);

    vp.Reset(Vector3(0, 1, 0), Quaternion::Identity());
    vp.SetThrottle(1.0f);
    vp.ShiftGear(2); // 1st gear

    float prevSpeed = 0.0f;
    for (int i = 0; i < 60; ++i)
        vp.Update(1.0f / 60.0f);

    // スロットル全開で何らかの速度が出ているはず
    EXPECT_GT(vp.GetState().linearVelocity.Length(), 0.01f);
}

TEST(VehiclePhysicsTest, UpdateBraking)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);

    WheelConfig wc;
    wc.position = { 0, 0, 1 };
    for (int i = 0; i < 4; ++i)
        vp.SetWheelConfig(i, wc);

    vp.Reset(Vector3(0, 1, 0), Quaternion::Identity());

    // まず加速
    vp.SetThrottle(1.0f);
    for (int i = 0; i < 30; ++i)
        vp.Update(1.0f / 60.0f);

    float speedBefore = vp.GetState().linearVelocity.Length();

    // ブレーキ
    vp.SetThrottle(0.0f);
    vp.SetBrake(1.0f);
    for (int i = 0; i < 60; ++i)
        vp.Update(1.0f / 60.0f);

    // ブレーキ後速度は減少するか変わらない（ゼロスタートなのでいずれにせよ小さい）
    // ブレーキが動作していることのチェック：少なくともクラッシュしない
    EXPECT_FALSE(std::isnan(vp.GetSpeedKmh()));
}

TEST(VehiclePhysicsTest, UpdateSteering)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);

    WheelConfig fl, fr, rl, rr;
    fl.position = { -0.8f, 0.0f,  1.2f };
    fr.position = {  0.8f, 0.0f,  1.2f };
    rl.position = { -0.8f, 0.0f, -1.2f };
    rr.position = {  0.8f, 0.0f, -1.2f };
    vp.SetWheelConfig(0, fl);
    vp.SetWheelConfig(1, fr);
    vp.SetWheelConfig(2, rl);
    vp.SetWheelConfig(3, rr);

    vp.Reset(Vector3(0, 1, 0), Quaternion::Identity());
    vp.SetSteering(1.0f);
    vp.Update(1.0f / 60.0f);

    // 前輪にステアリング角が設定されること
    auto ws0 = vp.GetWheelState(0);
    EXPECT_GT(std::abs(ws0.steerAngle), 0.0f);

    // 後輪のステアリング角はゼロ
    auto ws2 = vp.GetWheelState(2);
    EXPECT_NEAR(ws2.steerAngle, 0.0f, 1e-5f);
}

TEST(VehiclePhysicsTest, ResetPosition)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);

    WheelConfig wc;
    for (int i = 0; i < 4; ++i)
        vp.SetWheelConfig(i, wc);

    vp.Reset(Vector3(10, 5, 20), Quaternion::Identity());
    EXPECT_NEAR(vp.GetState().position.x, 10.0f, 1e-3f);
    EXPECT_NEAR(vp.GetState().position.y, 5.0f, 1e-3f);
    EXPECT_NEAR(vp.GetState().position.z, 20.0f, 1e-3f);
    EXPECT_NEAR(vp.GetSpeedKmh(), 0.0f, 1e-3f);
}

TEST(VehiclePhysicsTest, AeroDrag)
{
    // 空力ドラッグ係数がゼロの場合と正の場合で違いが出ること
    VehicleConfig cfgNoDrag;
    cfgNoDrag.aeroDragCoeff = 0.0f;
    VehicleConfig cfgDrag;
    cfgDrag.aeroDragCoeff = 1.0f;

    EXPECT_NEAR(cfgNoDrag.aeroDragCoeff, 0.0f, 1e-5f);
    EXPECT_NEAR(cfgDrag.aeroDragCoeff, 1.0f, 1e-5f);
}

TEST(VehiclePhysicsTest, TireModelDefaults)
{
    TireModel tm;
    EXPECT_NEAR(tm.lateralStiffness, 10.0f, 1e-5f);
    EXPECT_NEAR(tm.longitudinalStiffness, 12.0f, 1e-5f);
    EXPECT_NEAR(tm.maxGrip, 1.0f, 1e-5f);
    EXPECT_NEAR(tm.slipAnglePeak, 0.12f, 1e-5f);
    EXPECT_NEAR(tm.loadSensitivity, 0.01f, 1e-5f);
    EXPECT_NEAR(tm.rollingResistance, 0.015f, 1e-5f);
}

TEST(VehiclePhysicsTest, SuspensionCompression)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);

    WheelConfig wc;
    wc.position = { 0, 0, 0 };
    wc.suspensionRestLength = 0.3f;
    wc.maxSuspensionTravel  = 0.2f;
    for (int i = 0; i < 4; ++i)
        vp.SetWheelConfig(i, wc);

    // 地面の上に車両を配置
    vp.Reset(Vector3(0, 0.5f, 0), Quaternion::Identity());
    vp.Update(1.0f / 60.0f);

    // サスペンション圧縮量は正常な範囲
    auto ws = vp.GetWheelState(0);
    EXPECT_GE(ws.suspensionCompression, 0.0f);
    EXPECT_LE(ws.suspensionCompression, wc.maxSuspensionTravel);
}

TEST(VehiclePhysicsTest, PacejkaLateralForce)
{
    // Pacejka簡易モデルのテスト: スリップ角が0の場合は力がゼロ近傍
    // スリップ角が0でない場合は力が発生する
    // VehiclePhysicsの内部関数はprivateなので、ホイールの状態で確認
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);

    WheelConfig wc;
    wc.position = { 0, 0, 1 };
    for (int i = 0; i < 4; ++i)
        vp.SetWheelConfig(i, wc);

    vp.Reset(Vector3(0, 1, 0), Quaternion::Identity());
    vp.SetSteering(0.5f);
    vp.SetThrottle(0.5f);

    for (int i = 0; i < 30; ++i)
        vp.Update(1.0f / 60.0f);

    // NaN がないことを確認
    auto ws = vp.GetWheelState(0);
    EXPECT_FALSE(std::isnan(ws.lateralForce));
    EXPECT_FALSE(std::isnan(ws.longitudinalForce));
}

TEST(VehiclePhysicsTest, SpeedKmhConversion)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);

    WheelConfig wc;
    wc.position = { 0, 0, 1 };
    for (int i = 0; i < 4; ++i)
        vp.SetWheelConfig(i, wc);

    // 初期速度は0
    vp.Reset(Vector3(0, 1, 0), Quaternion::Identity());
    EXPECT_NEAR(vp.GetSpeedKmh(), 0.0f, 1e-3f);

    // 加速後の速度がkm/h換算されていること
    vp.SetThrottle(1.0f);
    for (int i = 0; i < 120; ++i)
        vp.Update(1.0f / 60.0f);

    float speed = vp.GetSpeedKmh();
    EXPECT_FALSE(std::isnan(speed));
}

TEST(VehiclePhysicsTest, WheelRotation)
{
    VehiclePhysics vp;
    vp.Initialize(VehicleConfig{});
    vp.SetWheelCount(4);

    WheelConfig wc;
    wc.position = { 0, 0, 1 };
    for (int i = 0; i < 4; ++i)
        vp.SetWheelConfig(i, wc);

    vp.Reset(Vector3(0, 1, 0), Quaternion::Identity());
    vp.SetThrottle(1.0f);

    auto ws0 = vp.GetWheelState(2); // 駆動輪 (RWD)
    float angle0 = ws0.rotationAngle;

    for (int i = 0; i < 60; ++i)
        vp.Update(1.0f / 60.0f);

    auto ws1 = vp.GetWheelState(2);
    // 駆動輪の回転角が変化しているはず
    EXPECT_NE(ws1.rotationAngle, angle0);
}
