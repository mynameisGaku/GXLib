#pragma once
/// @file PhysicsDebugRenderer.h
/// @brief 物理デバッグ可視化レンダラー
///
/// PhysicsDebugInfo のデータを DebugDraw3D を使ってワイヤーフレーム描画する。
/// コライダー形状、接触点、コンストレイント、速度ベクトルの可視化に対応。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Physics/PhysicsDebugInfo.h"
#include "Math/Color.h"

namespace gx
{

class DebugDraw3D;

/// @brief 物理デバッグ描画フラグ
enum PhysicsDebugFlags : uint32_t
{
    PhysicsDebug_Colliders   = 1 << 0,  ///< コライダー形状を描画
    PhysicsDebug_Contacts    = 1 << 1,  ///< 接触点を描画
    PhysicsDebug_Constraints = 1 << 2,  ///< コンストレイントを描画
    PhysicsDebug_Velocities  = 1 << 3,  ///< 速度ベクトルを描画
    PhysicsDebug_All         = 0xFFFFFFFF, ///< すべて描画
};

/// @brief 物理デバッグ可視化レンダラー
class PhysicsDebugRenderer
{
public:
    /// @brief 物理デバッグ情報を描画する
    /// @param debugDraw DebugDraw3Dインスタンス
    /// @param bodies ボディのデバッグ情報配列
    /// @param contacts 接触点のデバッグ情報配列
    /// @param constraints コンストレイントのデバッグ情報配列
    void Render(DebugDraw3D& debugDraw,
                const gx::Vector<BodyDebugInfo>& bodies,
                const gx::Vector<ContactDebugInfo>& contacts,
                const gx::Vector<ConstraintDebugInfo>& constraints);

    /// @brief 描画フラグを設定する
    void SetFlags(uint32_t flags) { m_flags = flags; }

    /// @brief 描画フラグを取得する
    uint32_t GetFlags() const { return m_flags; }

    /// @brief 静的ボディの描画色を設定する
    void SetStaticColor(const Color& c) { m_staticColor = c; }

    /// @brief キネマティックボディの描画色を設定する
    void SetKinematicColor(const Color& c) { m_kinematicColor = c; }

    /// @brief アクティブな動的ボディの描画色を設定する
    void SetDynamicActiveColor(const Color& c) { m_dynamicActiveColor = c; }

    /// @brief スリープ中の動的ボディの描画色を設定する
    void SetDynamicSleepColor(const Color& c) { m_dynamicSleepColor = c; }

private:
    void RenderBody(DebugDraw3D& debugDraw, const BodyDebugInfo& body);
    void RenderContact(DebugDraw3D& debugDraw, const ContactDebugInfo& contact);
    void RenderConstraint(DebugDraw3D& debugDraw, const ConstraintDebugInfo& constraint);
    void DrawWireCapsule(DebugDraw3D& debugDraw, const Vector3& center, const Quaternion& rot,
                         float halfHeight, float radius, const Color& color);

    uint32_t m_flags = PhysicsDebug_All;
    Color m_staticColor       = Color{0.5f, 0.5f, 0.5f, 1.0f};
    Color m_kinematicColor    = Color{0.2f, 0.6f, 1.0f, 1.0f};
    Color m_dynamicActiveColor = Color{0.0f, 1.0f, 0.0f, 1.0f};
    Color m_dynamicSleepColor = Color{0.3f, 0.3f, 0.0f, 1.0f};
    Color m_contactColor      = Color{1.0f, 0.0f, 0.0f, 1.0f};
    Color m_constraintColor   = Color{1.0f, 1.0f, 0.0f, 1.0f};
    Color m_velocityColor     = Color{0.0f, 0.5f, 1.0f, 1.0f};
};

} // namespace gx
/// @}
