#pragma once
/// @file BuoyancySystem.h
/// @brief 浮力・水面物理シミュレーション（CPU のみ）
///
/// Gerstner 波による波高計算と、アルキメデス原理に基づく浮力を
/// サンプルポイント方式で計算する。ボートや浮遊オブジェクトを
/// 波に揺らす演出に使い、GPU 不要で動作する。
/// @addtogroup grp_physics/// @{

#include "Math/Vector3.h"
#include "Math/Quaternion.h"

namespace gx {

/// @brief 水面構成パラメータ
struct WaterSurfaceConfig
{
    float waterLevel       = 0.0f;      ///< 基準水面高さ (m)
    float density          = 1000.0f;   ///< 水の密度 (kg/m^3)
    float waveAmplitude    = 0.5f;      ///< 波の振幅 (m)
    float waveFrequency    = 0.5f;      ///< 波の周波数 (rad/m)
    float waveSpeed        = 2.0f;      ///< 波の進行速度 (m/s)
    float dragCoefficient  = 0.5f;      ///< 線形ドラッグ係数
    float angularDragCoefficient = 0.3f; ///< 角度ドラッグ係数
};

/// @brief 浮力を受けるボディ
struct BuoyancyBody
{
    uint32_t    id = 0;                     ///< 一意識別子
    Vector3     position;                    ///< ワールド位置
    Quaternion  rotation;                    ///< ワールド回転
    Vector3     linearVelocity;              ///< 線速度 (m/s)
    Vector3     angularVelocity;             ///< 角速度 (rad/s)
    float       mass   = 1.0f;              ///< 質量 (kg)
    float       volume = 1.0f;              ///< 体積 (m^3)
    Vector3     centerOfBuoyancy;            ///< 浮力中心 (ローカル座標)
    float       submergedFraction = 0.0f;    ///< 浸水率 (0-1)
    std::vector<Vector3> samplePoints;       ///< サンプルポイント (ローカル座標)
};

/// @brief 浮力計算結果
struct BuoyancyForce
{
    Vector3 force;              ///< 合計力 (N)
    Vector3 torque;             ///< 合計トルク (Nm)
    float   submergedFraction = 0.0f; ///< 浸水率 (0-1)
};

/// @brief 浮力シミュレーションシステム
///
/// サンプルポイントごとに浸水深度を計算し、アルキメデスの原理に基づく
/// 浮力と水中ドラッグを適用する。波高はGerstner風のsin関数で計算。
class BuoyancySystem
{
public:
    BuoyancySystem() = default;
    ~BuoyancySystem() = default;

    /// @brief 水面構成を設定する
    void SetWaterConfig(const WaterSurfaceConfig& cfg) { m_waterConfig = cfg; }

    /// @brief 水面構成を取得する
    const WaterSurfaceConfig& GetWaterConfig() const { return m_waterConfig; }

    /// @brief 浮力ボディを登録する
    /// @param id 一意ID
    /// @param mass 質量 (kg)
    /// @param volume 体積 (m^3)
    /// @param samplePoints ローカル座標のサンプルポイント配列
    void AddBody(uint32_t id, float mass, float volume, const std::vector<Vector3>& samplePoints);

    /// @brief ボディを削除する
    /// @param id ボディID
    void RemoveBody(uint32_t id);

    /// @brief 登録ボディ数
    uint32_t GetBodyCount() const { return static_cast<uint32_t>(m_bodies.size()); }

    /// @brief ボディが登録されているか
    bool HasBody(uint32_t id) const;

    /// @brief ボディを取得する (存在しない場合nullptr)
    const BuoyancyBody* GetBody(uint32_t id) const;

    /// @brief ボディの位置・回転を設定する
    void SetBodyTransform(uint32_t id, const Vector3& position, const Quaternion& rotation);

    /// @brief ボディの速度を設定する
    void SetBodyVelocity(uint32_t id, const Vector3& linear, const Vector3& angular);

    /// @brief 指定座標での水面高さを取得する
    /// @param x ワールドX座標
    /// @param z ワールドZ座標
    /// @param time 現在時刻 (秒)
    /// @return 水面高さ
    float GetWaterHeight(float x, float z, float time) const;

    /// @brief 指定座標での水面法線を取得する
    /// @param x ワールドX座標
    /// @param z ワールドZ座標
    /// @param time 現在時刻 (秒)
    /// @return 水面法線ベクトル (正規化済み)
    Vector3 GetWaterNormal(float x, float z, float time) const;

    /// @brief 特定ボディの浮力を計算する
    /// @param id ボディID
    /// @param currentTime 現在時刻 (秒)
    /// @return 浮力計算結果
    BuoyancyForce ComputeBuoyancy(uint32_t id, float currentTime) const;

    /// @brief 全ボディの浮力を計算し積分する
    /// @param deltaTime タイムステップ (秒)
    /// @param currentTime 現在時刻 (秒)
    void Update(float deltaTime, float currentTime);

    /// @brief ボディの浸水率を取得する
    float GetSubmergedFraction(uint32_t id) const;

    /// @brief 全ボディを削除する
    void Clear() { m_bodies.clear(); }

    /// @brief 重力加速度を設定する
    void SetGravity(float g) { m_gravity = g; }

    /// @brief 重力加速度を取得する
    float GetGravity() const { return m_gravity; }

private:
    /// @brief ローカル座標をワールド座標に変換する
    Vector3 LocalToWorld(const BuoyancyBody& body, const Vector3& localPos) const;

    /// @brief ボディのインデックスを検索する (-1 = 未発見)
    int FindBodyIndex(uint32_t id) const;

    WaterSurfaceConfig      m_waterConfig;
    std::vector<BuoyancyBody> m_bodies;
    float                   m_gravity = 9.81f;
};

} // namespace gx
/// @}
