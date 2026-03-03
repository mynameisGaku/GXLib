#pragma once
/// @file ProceduralAnimation.h
/// @brief プロシージャルアニメーション — 歩行パターン、ボディモーション、LookAt
///
/// ゲイトパターンに基づくIKターゲット生成、スプリングダンパーによる
/// 滑らかなボディボブ/スウェイ、LookAtターゲット追従を行う。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"

namespace gx {

/// @brief プロシージャル歩行タイプ
enum class ProceduralGaitType
{
    Walk,    ///< 歩き
    Run,     ///< 走り
    Trot,    ///< 小走り
    Gallop,  ///< ギャロップ
    Custom   ///< カスタム
};

/// @brief 脚の構成
struct LegConfig
{
    int     hipJointIndex   = -1;   ///< 股関節ジョイントインデックス
    int     kneeJointIndex  = -1;   ///< 膝関節ジョイントインデックス
    int     footJointIndex  = -1;   ///< 足先ジョイントインデックス
    float   stepHeight      = 0.3f; ///< ステップの最大高さ (m)
    float   stepLength      = 0.6f; ///< ステップの長さ (m)
    Vector3 footOffset = { 0, 0, 0 }; ///< 足先オフセット (ローカル)
    float   legLength       = 1.0f; ///< 脚の全長 (m)
};

/// @brief 歩行パターン定義
struct GaitPattern
{
    ProceduralGaitType   type = ProceduralGaitType::Walk;
    gx::Vector<float>   legPhaseOffsets; ///< 各脚の位相オフセット (0-1, e.g. {0, 0.5, 0.25, 0.75})
    float                cycleTime   = 1.0f;  ///< 1サイクルの所要時間 (秒)
    float                dutyFactor  = 0.6f;   ///< サイクル中に足が地面にいる割合 (0-1)
};

/// @brief 脚の動的状態
struct LegState
{
    float   phase             = 0.0f; ///< 現在の位相 (0-1)
    bool    isGrounded        = true; ///< 接地しているか
    Vector3 footWorldPosition   = { 0, 0, 0 }; ///< 現在の足位置 (ワールド)
    Vector3 targetFootPosition  = { 0, 0, 0 }; ///< IKターゲット位置
    float   stepProgress      = 0.0f; ///< スイングフェーズの進行度 (0-1)
};

/// @brief ボディモーション構成
struct MotionConfig
{
    float bodyBobAmplitude  = 0.05f;  ///< ボディ上下振動の振幅 (m)
    float bodyBobFrequency  = 2.0f;   ///< ボディ上下振動の周波数 (Hz)
    float bodySwayAmplitude = 0.02f;  ///< ボディ左右揺れの振幅 (m)
    float bodyTiltFactor    = 0.1f;   ///< 地形適応の傾斜係数
    float smoothingFactor   = 0.15f;  ///< スムージングファクター
};

/// @brief LookAt構成
struct LookAtConfig
{
    int     headJointIndex  = -1;     ///< 頭部ジョイントインデックス
    int     neckJointIndex  = -1;     ///< 首ジョイントインデックス
    gx::Vector<int> eyeJointIndices; ///< 眼球ジョイントインデックス
    float   maxHeadAngle    = 60.0f;  ///< 頭部の最大角度 (度)
    float   maxEyeAngle     = 30.0f;  ///< 眼球の最大角度 (度)
    float   smoothSpeed     = 5.0f;   ///< 追従速度
};

/// @brief スプリングダンパー (1Dクリティカルダンプ)
struct SpringDamper
{
    float stiffness    = 100.0f;  ///< ばね定数
    float damping      = 10.0f;   ///< 減衰係数
    float currentValue = 0.0f;    ///< 現在値
    float velocity     = 0.0f;    ///< 現在の速度

    /// @brief ターゲット値に向かってスプリングを更新する
    /// @param target 目標値
    /// @param dt タイムステップ (秒)
    /// @return 更新後の値
    float Update(float target, float dt)
    {
        float error = target - currentValue;
        float accel = stiffness * error - damping * velocity;
        velocity     += accel * dt;
        currentValue += velocity * dt;
        return currentValue;
    }
};

/// @brief プロシージャルアニメーター
///
/// ゲイトパターンに従い脚のIKターゲットを生成し、ボディのボブ/スウェイを
/// スプリングダンパーで滑らかに制御する。LookAtターゲット追従もサポート。
class ProceduralAnimator
{
public:
    ProceduralAnimator() = default;
    ~ProceduralAnimator() = default;

    /// @brief 歩行パターンを設定する
    void SetGait(const GaitPattern& pattern) { m_gait = pattern; }

    /// @brief 歩行パターンを取得する
    const GaitPattern& GetGait() const { return m_gait; }

    /// @brief 脚を追加する
    /// @param config 脚の構成
    /// @return 脚インデックス
    int AddLeg(const LegConfig& config);

    /// @brief 脚の数を取得する
    int GetLegCount() const { return static_cast<int>(m_legs.size()); }

    /// @brief 脚の状態を取得する
    LegState GetLegState(int index) const;

    /// @brief ボディモーション構成を設定する
    void SetMotionConfig(const MotionConfig& cfg) { m_motionConfig = cfg; }

    /// @brief ボディモーション構成を取得する
    const MotionConfig& GetMotionConfig() const { return m_motionConfig; }

    /// @brief LookAt構成を設定する
    void SetLookAtConfig(const LookAtConfig& cfg) { m_lookAtConfig = cfg; }

    /// @brief LookAt構成を取得する
    const LookAtConfig& GetLookAtConfig() const { return m_lookAtConfig; }

    /// @brief 移動方向を設定する (正規化ベクトル)
    void SetMovementDirection(const Vector3& dir) { m_movementDir = dir; }

    /// @brief 移動速度を設定する
    void SetMovementSpeed(float speed) { m_movementSpeed = speed; }

    /// @brief 移動速度を取得する
    float GetMovementSpeed() const { return m_movementSpeed; }

    /// @brief 地面の高さを設定する (フラットな地面の仮定)
    void SetGroundHeight(float h) { m_groundHeight = h; }

    /// @brief 地面の高さを取得する
    float GetGroundHeight() const { return m_groundHeight; }

    /// @brief LookAtターゲットを設定する
    void SetLookAtTarget(const Vector3& target) { m_lookAtTarget = target; m_hasLookAt = true; }

    /// @brief LookAtターゲットを取得する
    Vector3 GetLookAtTarget() const { return m_lookAtTarget; }

    /// @brief LookAtターゲットをクリアする
    void ClearLookAt() { m_hasLookAt = false; }

    /// @brief LookAtターゲットが設定されているか
    bool HasLookAt() const { return m_hasLookAt; }

    /// @brief シミュレーションを1ステップ進める
    /// @param deltaTime 経過時間 (秒)
    void Update(float deltaTime);

    /// @brief ボディオフセット (ボブ + スウェイ)
    Vector3 GetBodyOffset() const { return m_bodyOffset; }

    /// @brief ボディの傾き (オイラー角: pitch, yaw, roll)
    Vector3 GetBodyTilt() const { return m_bodyTilt; }

    /// @brief 指定脚のIKターゲット (ワールド座標)
    Vector3 GetFootTarget(int legIndex) const;

    /// @brief 全状態をリセットする
    void Reset();

    /// @brief 有効/無効を設定する
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief 有効かどうか取得する
    bool IsEnabled() const { return m_enabled; }

private:
    struct LegData
    {
        LegConfig config;
        LegState  state;
        Vector3 liftoffPos = { 0, 0, 0 };
        Vector3 landingPos = { 0, 0, 0 };
    };

    GaitPattern     m_gait;
    MotionConfig    m_motionConfig;
    LookAtConfig    m_lookAtConfig;
    gx::Vector<LegData> m_legs;

    Vector3 m_movementDir   = { 0, 0, 1 };
    float              m_movementSpeed = 0.0f;
    float              m_groundHeight  = 0.0f;
    float              m_globalPhase   = 0.0f; ///< グローバルサイクル位相 (0-1)

    Vector3 m_lookAtTarget  = { 0, 0, 1 };
    bool               m_hasLookAt    = false;

    Vector3 m_bodyOffset = { 0, 0, 0 };
    Vector3 m_bodyTilt   = { 0, 0, 0 };

    SpringDamper m_bobSpring;
    SpringDamper m_swaySpring;

    bool m_enabled = true;
};

} // namespace gx
/// @}
