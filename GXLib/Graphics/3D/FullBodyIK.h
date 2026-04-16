#pragma once
/// @file FullBodyIK.h
/// @brief 全身IKシステム — 脊柱・腕・脚の複数チェーンに関節制約付きIKを適用
///
/// ヒンジ（膝/肘）・ボール&ソケット（肩/股）・ツイスト（前腕）の3種類の関節制約を持つ。
/// Spine・LeftArm・RightArm・LeftLeg・RightLeg の5チェーンを個別に設定し、
/// IKターゲット位置に向けてキャラクターの全身姿勢を計算する。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace gx
{

/// @brief 関節制約タイプ
enum class JointConstraintType
{
    None,           ///< 制約なし
    Hinge,          ///< ヒンジ (膝/肘) — 1軸回転
    BallAndSocket,  ///< ボール&ソケット (肩/股) — 自由回転(角度制限付き)
    Twist           ///< ツイスト (前腕) — 回転範囲制限
};

/// @brief IKチェーンID
enum class IKChainId
{
    Spine,
    LeftArm,
    RightArm,
    LeftLeg,
    RightLeg,
    Count
};

/// @brief IKChainId用ハッシュ関数
struct IKChainIdHash
{
    std::size_t operator()(IKChainId id) const noexcept
    {
        return static_cast<std::size_t>(id);
    }
};

/// @brief 関節制約
struct JointConstraint
{
    JointConstraintType type = JointConstraintType::None;
    float minAngle = -180.0f;  ///< 最小角度 (度)
    float maxAngle = 180.0f;   ///< 最大角度 (度)
    Vector3 axis = { 1.0f, 0.0f, 0.0f }; ///< ヒンジ軸
};

/// @brief IKチェーン定義
struct FullBodyIKChain
{
    IKChainId id = IKChainId::Spine;                ///< チェーンID
    gx::Vector<int> boneIndices;                    ///< ボーンインデックス（ルート→エフェクタ順）
    Vector3 target = { 0, 0, 0 };        ///< IKターゲット位置
    float weight = 1.0f;                             ///< FK/IKブレンドウェイト（0=FK, 1=IK）
    uint32_t maxIterations = 20;                     ///< 最大反復回数
    float tolerance = 0.001f;                        ///< 収束許容誤差（ワールド単位）
    gx::Vector<JointConstraint> constraints;        ///< ボーンごとの関節制約
};

/// @brief 全身IKソルバー
class FullBodyIK
{
public:
    FullBodyIK() = default;
    ~FullBodyIK() = default;

    /// @brief チェーンを設定
    void SetupChain(IKChainId id, const gx::Vector<int>& boneIndices);

    /// @brief チェーンが設定済みか
    bool HasChain(IKChainId id) const;

    /// @brief チェーン数
    uint32_t GetChainCount() const { return static_cast<uint32_t>(m_chains.size()); }

    /// @brief ターゲット位置を設定
    void SetTarget(IKChainId id, const Vector3& target);

    /// @brief ターゲット位置を取得
    Vector3 GetTarget(IKChainId id) const;

    /// @brief FK/IKブレンドウェイト設定 (0=FK, 1=IK)
    void SetWeight(IKChainId id, float weight);

    /// @brief ウェイト取得
    float GetWeight(IKChainId id) const;

    /// @brief 関節制約を設定
    void SetConstraint(IKChainId chainId, uint32_t jointIndex, const JointConstraint& constraint);

    /// @brief 制約を取得
    JointConstraint GetConstraint(IKChainId chainId, uint32_t jointIndex) const;

    /// @brief LookAtターゲット設定
    void SetLookAtTarget(const Vector3& target);

    /// @brief LookAtが有効か
    bool HasLookAtTarget() const { return m_hasLookAt; }

    /// @brief LookAtターゲット取得
    Vector3 GetLookAtTarget() const { return m_lookAtTarget; }

    /// @brief LookAtをクリア
    void ClearLookAtTarget();

    /// @brief Foot Placement設定
    void SetFootPlacement(bool enabled, float groundHeight = 0.0f);

    /// @brief Foot Placementが有効か
    bool IsFootPlacementEnabled() const { return m_footPlacement; }

    /// @brief 地面高さ取得
    float GetGroundHeight() const { return m_groundHeight; }

    /// @brief 全チェーンをソルブ (Spine→Leg→Arm順)
    void Solve();

    /// @brief ソルブ順序を取得
    gx::Vector<IKChainId> GetSolveOrder() const;

    /// @brief 最大反復回数の設定
    void SetMaxIterations(IKChainId id, uint32_t iterations);

    /// @brief 収束許容誤差の設定
    void SetTolerance(IKChainId id, float tolerance);

    /// @brief 全チェーンをリセット
    void Reset();

private:
    void SolveChain(FullBodyIKChain& chain);
    float ClampAngle(float angle, float minAngle, float maxAngle) const;

    gx::HashMap<IKChainId, FullBodyIKChain, IKChainIdHash> m_chains;  ///< IKチェーンマップ

    Vector3 m_lookAtTarget = { 0, 0, 1 };  ///< LookAtターゲット位置
    bool m_hasLookAt = false;                          ///< LookAtが設定されているか

    bool m_footPlacement = false;   ///< Foot Placement有効フラグ
    float m_groundHeight = 0.0f;    ///< 地面の高さ（ワールドY座標）
};

} // namespace gx
/// @}
