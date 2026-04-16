#pragma once
/// @file RootMotionLocomotion.h
/// @brief ルートモーションに基づくキャラクター移動制御
///
/// Animatorからルートモーション差分を取得し、位置・回転スケーリング、
/// 垂直ロック、地面スナッピング等の加工を行ってキャラクター移動に適用する。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include <functional>

namespace gx {

class Animator; // forward

/// @brief ルートモーション設定
struct RootMotionConfig
{
    bool  applyPosition     = true;     ///< 位置差分を適用するか
    bool  applyRotation     = true;     ///< 回転差分を適用するか
    bool  lockVertical      = false;    ///< Y成分をゼロにするか
    float positionScale     = 1.0f;     ///< 位置スケール倍率
    float rotationScale     = 1.0f;     ///< 回転スケール倍率
    bool  useGroundSnapping = false;    ///< 地面スナッピングを使うか
};

/// @brief ルートモーション移動コントローラー
///
/// Animatorのルートモーション抽出結果を加工し、キャラクターの位置・回転に
/// 適用するための差分を提供する。地面スナッピングや垂直ロックにも対応。
class RootMotionLocomotion
{
public:
    /// @brief 設定を適用する
    /// @param config ルートモーション設定
    void SetConfig(const RootMotionConfig& config);

    /// @brief 現在の設定を取得する
    /// @return 設定への参照
    const RootMotionConfig& GetConfig() const;

    /// @brief Animatorを関連付ける
    /// @param animator Animatorインスタンス（nullptrで解除）
    void SetAnimator(Animator* animator);

    /// @brief 関連付けられたAnimatorを取得する
    /// @return Animatorポインタ
    Animator* GetAnimator() const;

    /// @brief 毎フレーム呼び出してルートモーション差分を計算する
    /// @param deltaTime フレーム経過時間 (秒)
    void Update(float deltaTime);

    /// @brief 今フレームの位置差分を取得する
    /// @return 位置差分ベクトル
    Vector3 GetPositionDelta() const;

    /// @brief 今フレームの回転差分を取得する
    /// @return 回転差分クォータニオン
    Quaternion GetRotationDelta() const;

    /// @brief 累積位置を取得する（リセット以降の総移動量）
    /// @return 累積位置ベクトル
    Vector3 GetAccumulatedPosition() const;

    /// @brief 現在の移動速度を取得する (m/s)
    /// @return 速度
    float GetSpeed() const;

    /// @brief 地面高さ取得関数
    using GroundHeightFunc = std::function<float(float x, float z)>;

    /// @brief 地面高さ取得関数を設定する
    /// @param func 地面高さ関数 (x, z) -> y
    void SetGroundHeightFunction(GroundHeightFunc func);

    /// @brief 状態をリセットする
    void Reset();

private:
    Animator*        m_animator = nullptr;
    RootMotionConfig m_config;
    Vector3          m_positionDelta;
    Quaternion       m_rotationDelta = {0, 0, 0, 1};
    Vector3          m_accumulatedPosition;
    GroundHeightFunc m_groundFunc;
    float            m_speed = 0.0f;
};

} // namespace gx
/// @}
