#pragma once
/// @file VehiclePhysics.h
/// @brief 車両物理シミュレーション — タイヤ・サスペンション・エンジン・操舵を統合
///
/// Pacejka 簡易タイヤモデルと Ackermann 操舵を CPU 純粋計算で実装する。
/// Jolt Physics などの外部エンジンに依存しないため、
/// 任意の物理ワールドと組み合わせて車の動きを再現できる。
/// @addtogroup grp_physics/// @{

#include "Math/Vector3.h"
#include "Math/Quaternion.h"

namespace gx {

/// @brief 駆動方式
enum class DriveType
{
    FWD,  ///< 前輪駆動
    RWD,  ///< 後輪駆動
    AWD   ///< 四輪駆動
};

/// @brief タイヤ力学モデルパラメータ（Pacejka簡易モデル）
struct TireModel
{
    float lateralStiffness    = 10.0f;   ///< 横力剛性 (B factor)
    float longitudinalStiffness = 12.0f; ///< 縦力剛性 (B factor)
    float maxGrip             = 1.0f;    ///< 最大グリップ係数
    float slipAnglePeak       = 0.12f;   ///< ピークスリップ角 (ラジアン ~7度)
    float loadSensitivity     = 0.01f;   ///< 荷重感度
    float rollingResistance   = 0.015f;  ///< 転がり抵抗係数
};

/// @brief ホイール構成
struct WheelConfig
{
    float   radius              = 0.35f;      ///< タイヤ半径 (m)
    float   width               = 0.2f;       ///< タイヤ幅 (m)
    float   suspensionRestLength = 0.3f;      ///< サスペンション自然長 (m)
    float   suspensionStiffness = 35000.0f;   ///< ばね定数 (N/m)
    float   suspensionDamping   = 4500.0f;    ///< 減衰係数 (Ns/m)
    float   maxSuspensionTravel = 0.2f;       ///< サスペンション最大ストローク (m)
    Vector3 position;                          ///< ローカル空間でのホイール位置
    TireModel tireModel;                       ///< タイヤモデル
};

/// @brief ホイールの動的状態
struct WheelState
{
    float   suspensionCompression = 0.0f;  ///< サスペンション圧縮量 (m)
    float   steerAngle            = 0.0f;  ///< 操舵角 (ラジアン)
    float   rotationAngle         = 0.0f;  ///< ホイール回転角 (ラジアン)
    float   angularVelocity       = 0.0f;  ///< ホイール角速度 (rad/s)
    float   slipRatio             = 0.0f;  ///< スリップ率 (縦)
    float   slipAngle             = 0.0f;  ///< スリップ角 (横, ラジアン)
    float   lateralForce          = 0.0f;  ///< 横力 (N)
    float   longitudinalForce     = 0.0f;  ///< 縦力 (N)
    bool    isGrounded            = true;   ///< 接地しているか
    Vector3 contactNormal         = Vector3::Up();   ///< 接触法線
    Vector3 contactPoint;                            ///< 接触点 (ワールド)
};

/// @brief 車両構成パラメータ
struct VehicleConfig
{
    float   mass              = 1500.0f;     ///< 車両質量 (kg)
    Vector3 centerOfMass      = { 0.0f, -0.3f, 0.0f }; ///< 重心オフセット (ローカル)
    float   maxSteerAngle     = 35.0f;       ///< 最大操舵角 (度)
    float   maxEngineTorque   = 400.0f;      ///< 最大エンジントルク (Nm)
    float   maxBrakeTorque    = 3000.0f;     ///< 最大ブレーキトルク (Nm)
    DriveType driveType       = DriveType::RWD; ///< 駆動方式
    float   differentialRatio = 3.42f;       ///< デフ比
    gx::Vector<float> gearRatios = { -3.5f, 0.0f, 3.8f, 2.2f, 1.5f, 1.0f, 0.75f }; ///< ギア比 (R, N, 1, 2, 3, 4, 5)
    float   aeroDragCoeff     = 0.35f;       ///< 空気抵抗係数 Cd
    float   aeroLiftCoeff     = -0.1f;       ///< 揚力係数 (負=ダウンフォース)
    float   frontalArea       = 2.2f;        ///< 前面投影面積 (m^2)
};

/// @brief 車両の動的状態
struct VehicleState
{
    Vector3     position;                     ///< ワールド位置
    Quaternion  rotation;                     ///< ワールド回転
    Vector3     linearVelocity;               ///< 線速度 (m/s)
    Vector3     angularVelocity;              ///< 角速度 (rad/s)
    float       speedKmh       = 0.0f;       ///< 速度 (km/h)
    int         currentGear    = 2;           ///< 現在のギア (0=R, 1=N, 2=1st, ...)
    float       engineRPM      = 800.0f;     ///< エンジン回転数
    gx::Vector<WheelState> wheels;           ///< 各ホイールの状態
};

/// @brief 車両物理シミュレーション
///
/// Pacejka簡易タイヤモデル、質量-ばね-ダンパーサスペンション、
/// エンジン→ギアボックス→デファレンシャル駆動系、Ackermann操舵、
/// 空力ドラッグ/ダウンフォースを統合した車両シミュレータ。
class VehiclePhysics
{
public:
    VehiclePhysics();
    ~VehiclePhysics() = default;

    /// @brief 車両を初期化する
    /// @param config 車両構成
    void Initialize(const VehicleConfig& config);

    /// @brief 車両構成を取得する
    const VehicleConfig& GetConfig() const { return m_config; }

    /// @brief ホイール数を設定する（通常4）
    void SetWheelCount(int count);

    /// @brief ホイール数を取得する
    int GetWheelCount() const { return static_cast<int>(m_wheelConfigs.size()); }

    /// @brief ホイール構成を設定する
    void SetWheelConfig(int index, const WheelConfig& cfg);

    /// @brief ホイール構成を取得する
    WheelConfig GetWheelConfig(int index) const;

    /// @brief ホイール状態を取得する
    WheelState GetWheelState(int index) const;

    /// @brief スロットル入力 (0-1)
    void SetThrottle(float value);
    float GetThrottle() const { return m_throttle; }

    /// @brief ブレーキ入力 (0-1)
    void SetBrake(float value);
    float GetBrake() const { return m_brake; }

    /// @brief ステアリング入力 (-1 left ~ +1 right)
    void SetSteering(float value);
    float GetSteering() const { return m_steering; }

    /// @brief ハンドブレーキ
    void SetHandbrake(bool on);
    bool GetHandbrake() const { return m_handbrake; }

    /// @brief ギアシフト
    void ShiftGear(int gear);
    int GetCurrentGear() const { return m_state.currentGear; }

    /// @brief 1シミュレーションステップ実行
    void Update(float deltaTime);

    /// @brief 現在の車両状態を取得
    const VehicleState& GetState() const { return m_state; }

    /// @brief 速度 (km/h)
    float GetSpeedKmh() const { return m_state.speedKmh; }

    /// @brief エンジンRPM
    float GetEngineRPM() const { return m_state.engineRPM; }

    /// @brief 車両をリセットする
    void Reset(const Vector3& position, const Quaternion& rotation);

    /// @brief 外部力を加える (ワールド座標)
    void ApplyForce(const Vector3& force, const Vector3& worldPoint);

private:
    /// @brief Pacejka簡易モデルで横力を計算する
    float ComputeLateralForce(const TireModel& tire, float slipAngle, float normalLoad) const;

    /// @brief Pacejka簡易モデルで縦力を計算する
    float ComputeLongitudinalForce(const TireModel& tire, float slipRatio, float normalLoad) const;

    /// @brief エンジントルクカーブ (RPMに応じた出力)
    float GetEngineTorque(float rpm) const;

    /// @brief ローカル座標をワールド座標に変換する
    Vector3 LocalToWorld(const Vector3& localPos) const;

    VehicleConfig               m_config;
    gx::Vector<WheelConfig>    m_wheelConfigs;
    VehicleState                m_state;

    float m_throttle   = 0.0f;
    float m_brake      = 0.0f;
    float m_steering   = 0.0f;
    bool  m_handbrake  = false;

    Vector3 m_externalForce;
    Vector3 m_externalTorque;
};

} // namespace gx
/// @}
