/// @file VehiclePhysics.cpp
/// @brief VehiclePhysics の実装
#include "pch_common.h"

#include "Physics/VehiclePhysics.h"
#include "Math/MathUtil.h"

namespace gx {

// ============================================================================
// 定数
// ============================================================================
static constexpr float k_AirDensity   = 1.225f;  // kg/m^3 (海面)
static constexpr float k_Gravity      = 9.81f;
static constexpr float k_IdleRPM      = 800.0f;
static constexpr float k_MaxRPM       = 7000.0f;
static constexpr float k_MsToKmh      = 3.6f;
static constexpr float k_MinDt        = 1e-6f;

// ============================================================================
// コンストラクタ
// ============================================================================
VehiclePhysics::VehiclePhysics()
{
    m_externalForce  = Vector3::Zero();
    m_externalTorque = Vector3::Zero();
}

// ============================================================================
// Initialize
// ============================================================================
void VehiclePhysics::Initialize(const VehicleConfig& config)
{
    m_config = config;
    m_state  = {};
    m_state.currentGear = 2; // 1速
    m_state.engineRPM   = k_IdleRPM;
    m_throttle  = 0.0f;
    m_brake     = 0.0f;
    m_steering  = 0.0f;
    m_handbrake = false;
    m_externalForce  = Vector3::Zero();
    m_externalTorque = Vector3::Zero();

    // ホイール状態のリサイズ
    m_state.wheels.resize(m_wheelConfigs.size());
}

// ============================================================================
// SetWheelCount
// ============================================================================
void VehiclePhysics::SetWheelCount(int count)
{
    if (count < 0) count = 0;
    m_wheelConfigs.resize(static_cast<size_t>(count));
    m_state.wheels.resize(static_cast<size_t>(count));
}

// ============================================================================
// SetWheelConfig / GetWheelConfig / GetWheelState
// ============================================================================
void VehiclePhysics::SetWheelConfig(int index, const WheelConfig& cfg)
{
    if (index >= 0 && index < static_cast<int>(m_wheelConfigs.size()))
        m_wheelConfigs[static_cast<size_t>(index)] = cfg;
}

WheelConfig VehiclePhysics::GetWheelConfig(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_wheelConfigs.size()))
        return m_wheelConfigs[static_cast<size_t>(index)];
    return {};
}

WheelState VehiclePhysics::GetWheelState(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_state.wheels.size()))
        return m_state.wheels[static_cast<size_t>(index)];
    return {};
}

// ============================================================================
// 入力セッター
// ============================================================================
void VehiclePhysics::SetThrottle(float value)
{
    m_throttle = MathUtil::Clamp(value, 0.0f, 1.0f);
}

void VehiclePhysics::SetBrake(float value)
{
    m_brake = MathUtil::Clamp(value, 0.0f, 1.0f);
}

void VehiclePhysics::SetSteering(float value)
{
    m_steering = MathUtil::Clamp(value, -1.0f, 1.0f);
}

void VehiclePhysics::SetHandbrake(bool on)
{
    m_handbrake = on;
}

void VehiclePhysics::ShiftGear(int gear)
{
    if (gear >= 0 && gear < static_cast<int>(m_config.gearRatios.size()))
        m_state.currentGear = gear;
}

// ============================================================================
// Reset
// ============================================================================
void VehiclePhysics::Reset(const Vector3& position, const Quaternion& rotation)
{
    m_state.position        = position;
    m_state.rotation        = rotation;
    m_state.linearVelocity  = Vector3::Zero();
    m_state.angularVelocity = Vector3::Zero();
    m_state.speedKmh        = 0.0f;
    m_state.engineRPM       = k_IdleRPM;
    m_state.currentGear     = 2;
    m_throttle  = 0.0f;
    m_brake     = 0.0f;
    m_steering  = 0.0f;
    m_handbrake = false;
    m_externalForce  = Vector3::Zero();
    m_externalTorque = Vector3::Zero();

    for (auto& w : m_state.wheels)
    {
        w = {};
        w.isGrounded = true;
        w.contactNormal = Vector3::Up();
    }
}

// ============================================================================
// ApplyForce (外部力)
// ============================================================================
void VehiclePhysics::ApplyForce(const Vector3& force, const Vector3& worldPoint)
{
    m_externalForce += force;
    // トルク = (worldPoint - CoM) x force
    Vector3 comWorld = LocalToWorld(m_config.centerOfMass);
    Vector3 r = worldPoint - comWorld;
    m_externalTorque += r.Cross(force);
}

// ============================================================================
// LocalToWorld
// ============================================================================
Vector3 VehiclePhysics::LocalToWorld(const Vector3& localPos) const
{
    return m_state.position + m_state.rotation.RotateVector(localPos);
}

// ============================================================================
// GetEngineTorque — 簡易RPMカーブ
// ============================================================================
float VehiclePhysics::GetEngineTorque(float rpm) const
{
    // 単純なベルカーブ: ピーク付近で最大トルク
    float normalizedRPM = MathUtil::Clamp(rpm / k_MaxRPM, 0.0f, 1.0f);
    // torque = maxTorque * (1 - (2*norm - 1)^2) で概ねベルカーブ
    float x = 2.0f * normalizedRPM - 1.0f;
    float factor = MathUtil::Clamp(1.0f - x * x, 0.1f, 1.0f);
    return m_config.maxEngineTorque * factor * m_throttle;
}

// ============================================================================
// Pacejka 簡易 — 横力
// ============================================================================
float VehiclePhysics::ComputeLateralForce(const TireModel& tire, float slipAngle, float normalLoad) const
{
    if (normalLoad <= 0.0f) return 0.0f;
    // F = grip * normalLoad * sin(C * atan(B * slipAngle))
    float B = tire.lateralStiffness;
    float C = 1.9f; // shape factor
    float D = tire.maxGrip * normalLoad;
    float force = D * std::sin(C * std::atan(B * slipAngle));
    return force;
}

// ============================================================================
// Pacejka 簡易 — 縦力
// ============================================================================
float VehiclePhysics::ComputeLongitudinalForce(const TireModel& tire, float slipRatio, float normalLoad) const
{
    if (normalLoad <= 0.0f) return 0.0f;
    float B = tire.longitudinalStiffness;
    float C = 1.9f;
    float D = tire.maxGrip * normalLoad;
    float force = D * std::sin(C * std::atan(B * slipRatio));
    return force;
}

// ============================================================================
// Update — メインシミュレーションステップ
// ============================================================================
void VehiclePhysics::Update(float deltaTime)
{
    if (deltaTime < k_MinDt) return;
    const int wheelCount = static_cast<int>(m_wheelConfigs.size());
    if (wheelCount == 0) return;

    const float mass    = m_config.mass;
    const float invMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    // 簡易慣性モーメント (均質な箱体: I = m/12 * (w^2 + h^2) 近似)
    const float inertiaFactor = mass * 0.4f; // 簡易
    const float invInertia    = (inertiaFactor > 0.0f) ? (1.0f / inertiaFactor) : 0.0f;

    // ------- 1. 操舵角計算 (Ackermann簡易) -------
    float maxSteerRad = MathUtil::DegreesToRadians(m_config.maxSteerAngle);
    float steerAngle  = m_steering * maxSteerRad;

    // ------- 2. エンジン → ギア → ホイールトルク -------
    float gearRatio = 0.0f;
    if (m_state.currentGear >= 0 && m_state.currentGear < static_cast<int>(m_config.gearRatios.size()))
        gearRatio = m_config.gearRatios[static_cast<size_t>(m_state.currentGear)];

    float engineTorque = GetEngineTorque(m_state.engineRPM);
    float drivelineTorque = engineTorque * gearRatio * m_config.differentialRatio;

    // 駆動輪にトルクを分配
    // 前2輪=index 0,1、後2輪=index 2,3 と仮定
    auto isDriveWheel = [&](int i) -> bool {
        switch (m_config.driveType) {
        case DriveType::FWD: return i < 2;
        case DriveType::RWD: return i >= 2;
        case DriveType::AWD: return true;
        }
        return false;
    };

    int driveWheelCount = 0;
    for (int i = 0; i < wheelCount; ++i)
        if (isDriveWheel(i)) ++driveWheelCount;

    float wheelTorquePerDrive = (driveWheelCount > 0) ? (drivelineTorque / static_cast<float>(driveWheelCount)) : 0.0f;

    // ------- 3. 前方/右方ベクトル -------
    Vector3 forward = m_state.rotation.RotateVector(Vector3::Forward());
    Vector3 right   = m_state.rotation.RotateVector(Vector3::Right());
    Vector3 up      = m_state.rotation.RotateVector(Vector3::Up());

    // ------- 4. 車体の前方速度成分 -------
    float forwardSpeed = m_state.linearVelocity.Dot(forward);

    // ------- 5. ホイールごとの力計算 -------
    Vector3 totalForce  = Vector3::Zero();
    Vector3 totalTorque = Vector3::Zero();

    float weightPerWheel = (mass * k_Gravity) / static_cast<float>(wheelCount);

    for (int i = 0; i < wheelCount; ++i)
    {
        auto& wCfg = m_wheelConfigs[static_cast<size_t>(i)];
        auto& wState = m_state.wheels[static_cast<size_t>(i)];

        // --- 操舵 (前輪のみ: index 0,1) ---
        wState.steerAngle = (i < 2) ? steerAngle : 0.0f;

        // --- ホイールのワールド位置 ---
        Vector3 wheelWorldPos = LocalToWorld(wCfg.position);

        // --- サスペンション (簡易: 地面はy=0と仮定) ---
        float groundY    = 0.0f;
        float wheelBottomY = wheelWorldPos.y - wCfg.suspensionRestLength - wCfg.radius;
        float compression = groundY - wheelBottomY;
        compression = MathUtil::Clamp(compression, 0.0f, wCfg.maxSuspensionTravel);
        wState.suspensionCompression = compression;
        wState.isGrounded = (compression > 0.0f) || (wheelWorldPos.y - wCfg.radius <= groundY + wCfg.suspensionRestLength + 0.01f);

        if (!wState.isGrounded)
        {
            wState.lateralForce = 0.0f;
            wState.longitudinalForce = 0.0f;
            continue;
        }

        // isGrounded をデフォルト true にして簡易化
        wState.isGrounded = true;
        wState.contactNormal = Vector3::Up();
        wState.contactPoint  = Vector3(wheelWorldPos.x, groundY, wheelWorldPos.z);

        // --- サスペンション力 (Spring + Damper) ---
        float suspensionVelocity = -m_state.linearVelocity.Dot(up); // 上方向成分のマイナス → 圧縮速度
        float suspensionForce    = wCfg.suspensionStiffness * compression
                                 + wCfg.suspensionDamping * suspensionVelocity;
        suspensionForce = MathUtil::Max(suspensionForce, 0.0f);

        float normalLoad = suspensionForce;

        // --- ホイール進行方向（操舵考慮） ---
        float sinSteer = std::sin(wState.steerAngle);
        float cosSteer = std::cos(wState.steerAngle);
        Vector3 wheelForward = forward * cosSteer + right * sinSteer;
        Vector3 wheelRight   = right * cosSteer - forward * sinSteer;

        // --- スリップ角 (lateral) ---
        float wheelForwardSpeed = m_state.linearVelocity.Dot(wheelForward);
        float wheelLateralSpeed = m_state.linearVelocity.Dot(wheelRight);
        wState.slipAngle = 0.0f;
        if (MathUtil::Abs(wheelForwardSpeed) > 0.5f)
            wState.slipAngle = std::atan2(wheelLateralSpeed, MathUtil::Abs(wheelForwardSpeed));

        // --- スリップ率 (longitudinal) ---
        float wheelLinearSpeed = wheelForwardSpeed;
        float wheelSpinSpeed   = wState.angularVelocity * wCfg.radius;
        wState.slipRatio = 0.0f;
        float denomSpeed = MathUtil::Max(MathUtil::Abs(wheelLinearSpeed), MathUtil::Abs(wheelSpinSpeed));
        if (denomSpeed > 0.5f)
            wState.slipRatio = (wheelSpinSpeed - wheelLinearSpeed) / denomSpeed;

        // --- タイヤ力 (Pacejka簡易) ---
        float latForce  = ComputeLateralForce(wCfg.tireModel, wState.slipAngle, normalLoad);
        float longForce = ComputeLongitudinalForce(wCfg.tireModel, wState.slipRatio, normalLoad);

        wState.lateralForce      = latForce;
        wState.longitudinalForce = longForce;

        // --- 駆動トルク & ブレーキ ---
        float driveTorque = isDriveWheel(i) ? wheelTorquePerDrive : 0.0f;
        float brakeTorque = m_brake * m_config.maxBrakeTorque;
        if (m_handbrake && i >= 2) // ハンドブレーキは後輪
            brakeTorque += m_config.maxBrakeTorque;

        // ホイール角加速度
        float wheelInertia = 0.5f * 10.0f * wCfg.radius * wCfg.radius; // 簡易: m_wheel * r^2 / 2
        if (wheelInertia < 0.01f) wheelInertia = 0.01f;
        float netTorque = driveTorque - longForce * wCfg.radius
                        - brakeTorque * MathUtil::Sign(wState.angularVelocity)
                        - wCfg.tireModel.rollingResistance * normalLoad * wCfg.radius * MathUtil::Sign(wState.angularVelocity);
        wState.angularVelocity += (netTorque / wheelInertia) * deltaTime;

        // ブレーキで停止
        if (m_brake > 0.5f && MathUtil::Abs(wState.angularVelocity) < 1.0f && MathUtil::Abs(wheelLinearSpeed) < 1.0f)
            wState.angularVelocity = 0.0f;

        // ホイール回転角
        wState.rotationAngle += wState.angularVelocity * deltaTime;

        // --- 力をワールドに合算 ---
        Vector3 tireForceWorld = wheelForward * longForce - wheelRight * latForce;
        Vector3 suspForce      = up * suspensionForce;

        Vector3 wheelForce = tireForceWorld + suspForce;
        totalForce += wheelForce;

        // トルク (ホイール位置からCoMまでのモーメント)
        Vector3 comWorld = LocalToWorld(m_config.centerOfMass);
        Vector3 r = wState.contactPoint - comWorld;
        totalTorque += r.Cross(wheelForce);
    }

    // ------- 6. 空力 -------
    float speed = m_state.linearVelocity.Length();
    if (speed > 0.1f)
    {
        Vector3 velDir = m_state.linearVelocity * (1.0f / speed);
        float dragMag = 0.5f * k_AirDensity * m_config.aeroDragCoeff * m_config.frontalArea * speed * speed;
        totalForce += velDir * (-dragMag);

        // ダウンフォース
        float liftMag = 0.5f * k_AirDensity * m_config.aeroLiftCoeff * m_config.frontalArea * speed * speed;
        totalForce += up * liftMag;
    }

    // ------- 7. 重力 -------
    totalForce += Vector3(0.0f, -k_Gravity * mass, 0.0f);

    // ------- 8. 外部力 -------
    totalForce  += m_externalForce;
    totalTorque += m_externalTorque;
    m_externalForce  = Vector3::Zero();
    m_externalTorque = Vector3::Zero();

    // ------- 9. 積分 (Semi-Implicit Euler) -------
    Vector3 linearAccel  = totalForce * invMass;
    Vector3 angularAccel = totalTorque * invInertia;

    m_state.linearVelocity  += linearAccel * deltaTime;
    m_state.angularVelocity += angularAccel * deltaTime;

    m_state.position += m_state.linearVelocity * deltaTime;

    // 回転の積分（角速度→クォータニオン微分）
    float angVelLen = m_state.angularVelocity.Length();
    if (angVelLen > 1e-6f)
    {
        Vector3 axis = m_state.angularVelocity * (1.0f / angVelLen);
        float angle  = angVelLen * deltaTime;
        Quaternion dq = Quaternion::FromAxisAngle(axis, angle);
        m_state.rotation = dq * m_state.rotation;
        m_state.rotation.Normalize();
    }

    // ------- 10. 地面制約 (y >= 0 簡易) -------
    if (m_state.position.y < 0.0f)
    {
        m_state.position.y = 0.0f;
        if (m_state.linearVelocity.y < 0.0f)
            m_state.linearVelocity.y = 0.0f;
    }

    // ------- 11. 速度/RPM更新 -------
    m_state.speedKmh = m_state.linearVelocity.Length() * k_MsToKmh;

    // エンジンRPM: ホイール回転からフィードバック
    float avgWheelRPM = 0.0f;
    int driveCount = 0;
    for (int i = 0; i < wheelCount; ++i)
    {
        if (isDriveWheel(i))
        {
            float wheelRPM = MathUtil::Abs(m_state.wheels[static_cast<size_t>(i)].angularVelocity)
                           * 60.0f / (MathUtil::TAU);
            avgWheelRPM += wheelRPM;
            ++driveCount;
        }
    }
    if (driveCount > 0) avgWheelRPM /= static_cast<float>(driveCount);

    float effectiveRatio = MathUtil::Abs(gearRatio) * m_config.differentialRatio;
    float computedRPM = avgWheelRPM * effectiveRatio;
    m_state.engineRPM = MathUtil::Clamp(computedRPM, k_IdleRPM, k_MaxRPM);

    // ------- 12. 角速度の減衰 (安定性) -------
    m_state.angularVelocity *= (1.0f - 0.05f * deltaTime);
}

} // namespace gx
