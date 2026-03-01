#pragma once
/// @file SpringBone.h
/// @brief スプリングボーンシミュレーション（髪・布・尻尾等の揺れ物）
///
/// Verlet積分ベースの物理シミュレーションでボーンの二次運動を実現する。
/// 重力・風力・減衰・角度制限・コライダーによる衝突応答をサポート。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include <vector>
#include <functional>

namespace gx {

struct Skeleton;
struct TransformTRS;

/// @brief スプリングボーン1本分の設定
struct SpringBoneConfig
{
    int boneIndex       = -1;       ///< 対象ボーンインデックス
    float stiffness     = 300.0f;   ///< ばね定数
    float damping       = 10.0f;    ///< 減衰係数
    float mass          = 1.0f;     ///< 質量 (kg)
    float gravity       = -9.81f;   ///< 重力加速度
    float dragForce     = 0.1f;     ///< ドラッグ係数 (0-1)
    float maxAngle      = 60.0f;    ///< 親ボーンからの最大角度 (度)
    float radius        = 0.05f;    ///< コリジョン半径
    Vector3 gravityDirection = {0, -1, 0}; ///< 重力方向
};

/// @brief スプリングボーン用コライダー
struct SpringCollider
{
    enum Type { Sphere, Capsule };
    Type type        = Sphere;      ///< コライダータイプ
    Vector3 position;               ///< 中心位置 (ワールド)
    float radius     = 0.1f;        ///< 半径
    Vector3 endPosition;            ///< カプセル終端位置 (Capsuleの場合のみ)
};

/// @brief スプリングボーンシミュレーター
///
/// ボーンにばね・減衰系を適用し、重力や風力の影響下で揺れを生成する。
/// コライダーとの衝突応答もサポートし、キャラクターの髪・衣服・尻尾等の
/// セカンダリモーションに使用する。
class SpringBone
{
public:
    /// @brief スプリングボーンを追加する
    /// @param config ボーン設定
    void AddBone(const SpringBoneConfig& config);

    /// @brief ボーンチェーンを一括追加する
    /// @param boneIndices チェーンに含まれるボーンインデックス
    /// @param stiffness ばね定数
    /// @param damping 減衰係数
    void AddChain(const std::vector<int>& boneIndices, float stiffness, float damping);

    /// @brief コライダーを追加する
    /// @param collider コライダー定義
    void AddCollider(const SpringCollider& collider);

    /// @brief 指定ボーンを除去する
    /// @param boneIndex 除去するボーンインデックス
    void RemoveBone(int boneIndex);

    /// @brief 全ボーンを除去する
    void RemoveAllBones();

    /// @brief 全コライダーを除去する
    void ClearColliders();

    /// @brief 重力加速度を一括設定する
    /// @param gravity 重力値
    void SetGravity(float gravity);

    /// @brief 風力を設定する
    /// @param wind 風力ベクトル
    void SetWindForce(const Vector3& wind);

    /// @brief シミュレーションの有効/無効を切り替える
    /// @param enabled 有効にするか
    void SetEnabled(bool enabled);

    /// @brief シミュレーションが有効か
    /// @return 有効ならtrue
    bool IsEnabled() const;

    /// @brief 登録ボーン数を取得する
    /// @return ボーン数
    size_t GetBoneCount() const;

    /// @brief 登録コライダー数を取得する
    /// @return コライダー数
    size_t GetColliderCount() const;

    /// @brief シミュレーション状態をリセットする（初期姿勢に戻す）
    void Reset();

    /// @brief シミュレーションを1ステップ進める
    /// @param deltaTime フレーム経過時間 (秒)
    /// @param skeleton スケルトン参照
    /// @param localTransforms ローカルTRS姿勢配列 (入出力)
    /// @param globalTransforms グローバル変換行列配列 (入力)
    void Update(float deltaTime, const Skeleton& skeleton,
                std::vector<TransformTRS>& localTransforms,
                const std::vector<DirectX::XMFLOAT4X4>& globalTransforms);

private:
    /// @brief 内部ボーン状態
    struct BoneState
    {
        SpringBoneConfig config;
        Vector3 currentPosition;    ///< 現在のワールド位置
        Vector3 previousPosition;   ///< 前フレームのワールド位置
        Vector3 velocity;           ///< 現在の速度
        Quaternion initialRotation; ///< 初期ローカル回転
        bool initialized = false;   ///< 初期化済みか
    };

    std::vector<BoneState>     m_bones;
    std::vector<SpringCollider> m_colliders;
    Vector3 m_windForce = {0, 0, 0};
    bool    m_enabled   = true;
};

} // namespace gx
/// @}
