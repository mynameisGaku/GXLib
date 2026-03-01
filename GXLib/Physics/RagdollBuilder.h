#pragma once
/// @file RagdollBuilder.h
/// @brief ラグドール（物理でぐにゃぐにゃ動く骨格）の構築・管理
///
/// 人型プリセットまたはカスタム骨格定義から、カプセルボディ群と
/// コーンコンストレイントを PhysicsWorld3D 上に生成する。
/// キャラクターが倒れる演出や死亡アニメーション代替に使う。
/// @addtogroup grp_physics/// @{

#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "Physics/PhysicsWorld3D.h"
#include "Physics/PhysicsConstraint.h"
#include <vector>
#include <string>

namespace gx {

/// @brief ラグドールの1本の骨情報
struct RagdollBone {
    std::string name;                   ///< ボーン名
    PhysicsBodyID bodyID;               ///< 物理ボディID
    int parentIndex = -1;               ///< 親ボーンインデックス (-1 = ルート)
    Vector3 offset;                     ///< 親からのオフセット
    float length = 0.3f;               ///< カプセルの半高 (円柱部分)
    float radius = 0.05f;              ///< カプセルの半径
    float mass = 5.0f;                 ///< 質量 (kg)
};

/// @brief ラグドールのジョイント（ボーン間の制約）情報
struct RagdollJoint {
    int boneA = -1;                    ///< 接続先ボーンA
    int boneB = -1;                    ///< 接続先ボーンB
    PhysicsConstraintID constraintID;  ///< コンストレイントID
    float coneAngle = 30.0f;           ///< コーン制約の半角 (度)
};

/// @brief ラグドールプリセット
enum class RagdollPreset {
    Custom,     ///< カスタム (手動構築)
    Humanoid    ///< 人型 (15ボーン)
};

/// @brief ラグドール構築・管理クラス
class RagdollBuilder {
public:
    RagdollBuilder();
    ~RagdollBuilder();

    /// @brief プリセットからラグドールを構築する
    /// @param world 物理ワールド
    /// @param preset プリセット種別
    /// @param position ルートの初期位置
    /// @param scale 全体のスケール
    /// @return 構築に成功した場合true
    bool Build(PhysicsWorld3D* world, RagdollPreset preset, const Vector3& position = {}, float scale = 1.0f);

    /// @brief カスタムボーン構成からラグドールを構築する
    /// @param world 物理ワールド
    /// @param bones ボーン定義配列
    /// @param connections ボーン間接続ペア (boneA, boneB)
    /// @param position ルートの初期位置
    /// @return 構築に成功した場合true
    bool BuildCustom(PhysicsWorld3D* world, const std::vector<RagdollBone>& bones,
                     const std::vector<std::pair<int,int>>& connections, const Vector3& position);

    /// @brief ラグドールを破棄する
    /// @param world 物理ワールド
    void Destroy(PhysicsWorld3D* world);

    /// @brief ラグドールの有効/無効を切り替える
    /// @param world 物理ワールド
    /// @param enabled trueで有効化
    void SetEnabled(PhysicsWorld3D* world, bool enabled);

    /// @brief 特定のボーンにインパルスを適用する
    /// @param world 物理ワールド
    /// @param boneIndex ボーンインデックス
    /// @param impulse インパルスベクトル
    void ApplyImpulse(PhysicsWorld3D* world, int boneIndex, const Vector3& impulse);

    /// @brief 全ボーンのワールド変換行列を取得する
    /// @param world 物理ワールド
    /// @return 各ボーンの4x4変換行列
    std::vector<Matrix4x4> GetBoneTransforms(PhysicsWorld3D* world) const;

    /// @brief ボーン配列を取得する
    const std::vector<RagdollBone>& GetBones() const { return m_bones; }

    /// @brief ジョイント配列を取得する
    const std::vector<RagdollJoint>& GetJoints() const { return m_joints; }

    /// @brief ボーン数を取得する
    uint32_t GetBoneCount() const { return static_cast<uint32_t>(m_bones.size()); }

    /// @brief 構築済みかどうか判定する
    bool IsBuilt() const { return m_built; }

private:
    void BuildHumanoid(PhysicsWorld3D* world, const Vector3& position, float scale);

    std::vector<RagdollBone> m_bones;
    std::vector<RagdollJoint> m_joints;
    bool m_built = false;
};

} // namespace gx
/// @}
