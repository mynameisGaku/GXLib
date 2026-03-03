#pragma once
/// @file MotionMatching.h
/// @brief モーションマッチングシステム
///
/// アニメーションクリップのデータベースから、現在の状態（速度・向き・足位置等）に
/// 最も近いポーズを高速検索し、シームレスなアニメーション遷移を実現する。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include <vector>
#include <string>

namespace gx {

class AnimationClip;
class Animator;
struct Skeleton;

/// @brief モーションマッチングで使用する特徴量
///
/// 各ポーズフレームから抽出される特徴ベクトル。
/// 位置・速度・足位置・向き等を含み、コスト関数による比較に使用する。
struct MotionFeature
{
    Vector3 velocity;           ///< ルートボーンの速度
    Vector3 futurePosition;     ///< 将来の予測位置
    Vector3 facingDirection;    ///< 正面方向
    Vector3 leftFootPosition;   ///< 左足のローカル位置
    Vector3 rightFootPosition;  ///< 右足のローカル位置
    float   leftFootVelocity  = 0.0f; ///< 左足の速さ
    float   rightFootVelocity = 0.0f; ///< 右足の速さ
};

/// @brief モーションデータベース内の1ポーズ
struct MotionPose
{
    int   clipIndex = -1;       ///< クリップインデックス
    float time      = 0.0f;     ///< クリップ内の時刻 (秒)
    MotionFeature feature;      ///< 特徴量
};

/// @brief モーションマッチングの設定
struct MotionMatchingConfig
{
    float positionWeight   = 1.0f;  ///< 位置コストの重み
    float velocityWeight   = 1.0f;  ///< 速度コストの重み
    float footWeight       = 1.0f;  ///< 足位置コストの重み
    float facingWeight     = 1.0f;  ///< 向きコストの重み
    float minSwitchInterval = 0.2f; ///< 最小切り替え間隔 (秒)
    float sampleRate       = 30.0f; ///< サンプリングレート (fps)
    int   leftFootBone     = -1;    ///< 左足ボーンインデックス
    int   rightFootBone    = -1;    ///< 右足ボーンインデックス
    float futureTime       = 0.2f;  ///< 将来予測の先読み時間 (秒)
};

/// @brief モーションデータベース
///
/// 複数のAnimationClipからポーズ特徴量を事前計算し、
/// クエリに対して最良のマッチを線形検索する。
class MotionDatabase
{
public:
    MotionDatabase() = default;
    ~MotionDatabase() = default;

    /// @brief クリップをデータベースに追加する
    /// @param clip アニメーションクリップ
    void AddClip(const AnimationClip* clip);

    /// @brief 登録されたクリップ数を取得する
    /// @return クリップ数
    size_t GetClipCount() const;

    /// @brief データベースを構築する（全ポーズの特徴量を事前計算）
    /// @param skeleton スケルトン参照
    /// @param config マッチング設定
    void Build(const Skeleton& skeleton, const MotionMatchingConfig& config);

    /// @brief クエリ特徴量に最も近いポーズを検索する
    /// @param query クエリ特徴量
    /// @param config マッチング設定
    /// @param outCost 最良マッチのコスト (出力)
    /// @return 最良マッチのポーズ
    MotionPose FindBestMatch(const MotionFeature& query,
                              const MotionMatchingConfig& config,
                              float& outCost) const;

    /// @brief データベース内の総ポーズ数を取得する
    /// @return ポーズ数
    size_t GetTotalPoseCount() const;

    /// @brief 指定インデックスのポーズを取得する
    /// @param index ポーズインデックス
    /// @return ポーズデータ
    const MotionPose& GetPose(size_t index) const;

    /// @brief 登録されたクリップを取得する
    /// @param index クリップインデックス
    /// @return クリップポインタ
    const AnimationClip* GetClip(size_t index) const;

private:
    /// @brief 2つの特徴量間のコストを計算する
    static float ComputeCost(const MotionFeature& a, const MotionFeature& b,
                              const MotionMatchingConfig& config);

    gx::Vector<const AnimationClip*> m_clips;
    gx::Vector<MotionPose>           m_poses;
};

/// @brief モーションマッチャー
///
/// MotionDatabaseとAnimatorを接続し、毎フレームの状態に基づいて
/// 最適なアニメーションポーズを自動選択・遷移する。
class MotionMatcher
{
public:
    MotionMatcher() = default;
    ~MotionMatcher() = default;

    /// @brief データベースを設定する
    /// @param database モーションデータベース
    void SetDatabase(const MotionDatabase* database);

    /// @brief データベースを取得する
    /// @return データベースポインタ
    const MotionDatabase* GetDatabase() const;

    /// @brief 設定を適用する
    /// @param config マッチング設定
    void SetConfig(const MotionMatchingConfig& config);

    /// @brief 現在の設定を取得する
    /// @return 設定への参照
    const MotionMatchingConfig& GetConfig() const;

    /// @brief Animatorを関連付ける
    /// @param animator Animatorインスタンス
    void SetAnimator(Animator* animator);

    /// @brief 希望移動速度を設定する
    /// @param velocity 希望速度ベクトル
    void SetDesiredVelocity(const Vector3& velocity);

    /// @brief 希望移動速度を取得する
    /// @return 希望速度ベクトル
    Vector3 GetDesiredVelocity() const;

    /// @brief 希望正面方向を設定する
    /// @param direction 希望方向ベクトル
    void SetDesiredFacing(const Vector3& direction);

    /// @brief 希望正面方向を取得する
    /// @return 希望方向ベクトル
    Vector3 GetDesiredFacing() const;

    /// @brief 毎フレーム呼び出してマッチングを実行する
    /// @param deltaTime フレーム経過時間 (秒)
    void Update(float deltaTime);

    /// @brief 直近のマッチングコストを取得する
    /// @return コスト値
    float GetLastMatchCost() const;

    /// @brief 現在再生中のクリップインデックスを取得する
    /// @return クリップインデックス (-1 = 未設定)
    int GetCurrentClipIndex() const;

    /// @brief 現在のフレーム時刻を取得する
    /// @return フレーム時刻 (秒)
    float GetCurrentTime() const;

    /// @brief 状態をリセットする
    void Reset();

private:
    const MotionDatabase*  m_database = nullptr;
    Animator*              m_animator = nullptr;
    MotionMatchingConfig   m_config;
    Vector3                m_desiredVelocity;
    Vector3                m_desiredFacing = {0, 0, 1};
    float                  m_timeSinceSwitch = 0.0f;
    float                  m_lastCost = 0.0f;
    int                    m_currentClipIndex = -1;
    float                  m_currentTime = 0.0f;
};

} // namespace gx
/// @}
