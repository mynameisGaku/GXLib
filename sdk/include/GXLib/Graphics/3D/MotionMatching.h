#pragma once
/// @file MotionMatching.h
/// @brief モーションマッチングシステム
///
/// アニメーションクリップのデータベースから、現在の状態（速度・向き・足位置等）に
/// 最も近いポーズを高速検索し、シームレスなアニメーション遷移を実現する。
///
/// KD-Tree 高速化:
///   Build() 完了時に特徴量ベクトル (17次元) の KD-Tree を自動構築し、
///   FindBestMatch() を O(n) 線形探索から O(log n) 近傍探索に置き換える。
///   KD-Tree 未構築時は従来の線形探索にフォールバックする。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"

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
/// KD-Tree による O(log n) 近傍探索でクエリに対する最良マッチを返す。
/// KD-Tree 未構築時は線形探索にフォールバックする。
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

    /// @brief データベースを構築する（全ポーズの特徴量を事前計算 + KD-Tree 構築）
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

    /// @brief KD-Tree が構築済みかどうか
    bool IsKDTreeBuilt() const { return m_kdBuilt; }

private:
    /// @brief 2つの特徴量間のコストを計算する
    static float ComputeCost(const MotionFeature& a, const MotionFeature& b,
                              const MotionMatchingConfig& config);

    // ---------------------------------------------------------------
    // KD-Tree (O(log n) nearest neighbor search)
    // ---------------------------------------------------------------

    /// @brief 特徴量ベクトルの次元数
    ///
    /// velocity(3) + futurePosition(3) + facingDirection(3)
    /// + leftFootPosition(3) + rightFootPosition(3)
    /// + leftFootVelocity(1) + rightFootVelocity(1) = 17
    static constexpr int k_FeatureDims = 17;

    /// @brief KD-Tree ノード
    struct KDNode
    {
        int   poseIndex  = -1;    ///< リーフ: ポーズインデックス (-1 = 内部ノード)
        int   splitAxis  = 0;     ///< 分割軸 (特徴ベクトル内のインデックス)
        float splitValue = 0.0f;  ///< 分割値
        int   left       = -1;    ///< 左子ノードインデックス
        int   right      = -1;    ///< 右子ノードインデックス
    };

    gx::Vector<KDNode> m_kdNodes;
    int  m_kdRoot  = -1;
    bool m_kdBuilt = false;

    /// @brief KD-Tree を構築する (Build() から呼ばれる)
    void BuildKDTree();

    /// @brief KD-Tree を再帰的に構築する
    /// @param indices ポーズインデックス配列 (ソートされる)
    /// @param begin 対象範囲の先頭
    /// @param end   対象範囲の末尾 (exclusive)
    /// @param depth 現在の深さ
    /// @return 作成したノードのインデックス
    int BuildKDTreeRecursive(gx::Vector<int>& indices, int begin, int end, int depth);

    /// @brief 指定ポーズの特徴量ベクトルから axis 番目の要素を取得する
    float FeatureValue(int poseIndex, int axis) const;

    /// @brief MotionFeature を平坦な float 配列に変換する
    static void FlattenFeature(const MotionFeature& f, float out[k_FeatureDims]);

    /// @brief KD-Tree による最近傍探索 (再帰)
    void KDSearch(int nodeIdx, const float query[k_FeatureDims],
                  int& bestIdx, float& bestDist) const;

    /// @brief 平坦化された特徴量同士の L2 距離の二乗を計算する
    static float FeatureDistSq(const float a[k_FeatureDims], const float b[k_FeatureDims]);

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
