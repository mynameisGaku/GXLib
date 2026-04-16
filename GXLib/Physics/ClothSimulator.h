#pragma once
/// @file ClothSimulator.h
/// @brief 布シミュレーション — Verlet 積分と距離拘束による揺れ物演算
///
/// グリッド頂点に重力と距離拘束を繰り返し適用して布の動きを計算する。
/// 旗・マント・カーテンなどの揺れ表現に使い、
/// 球コライダーとの衝突や固定頂点の設定も行える。
/// @addtogroup grp_physics/// @{

#include "pch_common.h"
#include "Math/Vector3.h"

namespace gx
{

/// @brief 布シミュレーションの初期設定
struct ClothDesc
{
    uint32_t width   = 10;    ///< 横方向の頂点数
    uint32_t height  = 10;    ///< 縦方向の頂点数
    float spacing    = 0.1f;  ///< 頂点間隔
    float mass       = 1.0f;  ///< 総質量
    float stiffness  = 0.8f;  ///< 拘束の剛性 (0..1)
    float damping    = 0.02f; ///< 速度減衰
    Vector3 gravity = { 0.0f, -9.8f, 0.0f }; ///< 重力
    gx::Vector<uint32_t> pinnedVertices; ///< 初期固定頂点インデックス
};

/// @brief 布シミュレーター
class ClothSimulator
{
public:
    /// @brief 布を初期化する
    /// @param desc 初期設定
    /// @return 初期化成功時 true
    bool Initialize(const ClothDesc& desc);

    /// @brief シミュレーションを1ステップ進める
    /// @param deltaTime 経過時間（秒）
    /// @param substeps サブステップ数（デフォルト: 4）
    void Update(float deltaTime, int substeps = 4);

    /// @brief 球コライダーを追加する
    /// @param center 球の中心座標
    /// @param radius 球の半径
    void AddSphereCollider(const Vector3& center, float radius);

    /// @brief 全球コライダーをクリアする
    void ClearColliders();

    /// @brief 頂点を固定する
    /// @param index 頂点インデックス
    void PinVertex(uint32_t index);

    /// @brief 頂点の固定を解除する
    /// @param index 頂点インデックス
    void UnpinVertex(uint32_t index);

    /// @brief 全頂点に外力を印加する
    /// @param force 力ベクトル
    void ApplyForce(const Vector3& force);

    /// @brief 頂点位置を取得する
    const gx::Vector<Vector3>& GetPositions() const { return m_positions; }

    /// @brief 頂点法線を取得する
    const gx::Vector<Vector3>& GetNormals() const { return m_normals; }

    /// @brief インデックスバッファを取得する
    const gx::Vector<uint32_t>& GetIndices() const { return m_indices; }

    /// @brief 頂点数を取得する
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_positions.size()); }

    /// @brief 横方向の頂点数
    uint32_t GetWidth() const { return m_width; }

    /// @brief 縦方向の頂点数
    uint32_t GetHeight() const { return m_height; }

private:
    struct Constraint
    {
        uint32_t i0, i1;
        float restLength;
    };

    struct SphereCollider
    {
        Vector3 center;
        float radius;
    };

    void SolveConstraints();
    void HandleCollisions();
    void RecalculateNormals();

    uint32_t m_width  = 0;
    uint32_t m_height = 0;
    float m_stiffness = 0.8f;
    float m_damping   = 0.02f;
    float m_invMass   = 1.0f;
    Vector3 m_gravity = { 0.0f, -9.8f, 0.0f };
    Vector3 m_externalForce = { 0.0f, 0.0f, 0.0f };

    gx::Vector<Vector3> m_positions;
    gx::Vector<Vector3> m_prevPositions;
    gx::Vector<Vector3> m_normals;
    gx::Vector<uint32_t>  m_indices;
    gx::Vector<bool>      m_pinned;
    gx::Vector<Constraint> m_constraints;
    gx::Vector<SphereCollider> m_colliders;
};

} // namespace gx
/// @}
