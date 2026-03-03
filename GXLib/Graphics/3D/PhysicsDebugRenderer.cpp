/// @file PhysicsDebugRenderer.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Graphics/3D/PhysicsDebugRenderer.h"
#include "Graphics/3D/DebugDraw3D.h"
#include <cmath>

namespace gx
{

// クォータニオンでベクトルを回転する
static Vector3 RotateByQuaternion(const Quaternion& q, const Vector3& v)
{
    // q * v * q^-1 を展開した公式
    float qx = q.x, qy = q.y, qz = q.z, qw = q.w;

    float ix = qw * v.x + qy * v.z - qz * v.y;
    float iy = qw * v.y + qz * v.x - qx * v.z;
    float iz = qw * v.z + qx * v.y - qy * v.x;
    float iw = -qx * v.x - qy * v.y - qz * v.z;

    return Vector3(
        ix * qw + iw * (-qx) + iy * (-qz) - iz * (-qy),
        iy * qw + iw * (-qy) + iz * (-qx) - ix * (-qz),
        iz * qw + iw * (-qz) + ix * (-qy) - iy * (-qx)
    );
}

// ============================================================================
// メインレンダリング
// ============================================================================

void PhysicsDebugRenderer::Render(DebugDraw3D& debugDraw,
                                   const gx::Vector<BodyDebugInfo>& bodies,
                                   const gx::Vector<ContactDebugInfo>& contacts,
                                   const gx::Vector<ConstraintDebugInfo>& constraints)
{
    if (m_flags & PhysicsDebug_Colliders)
    {
        for (const auto& body : bodies)
        {
            RenderBody(debugDraw, body);
        }
    }

    // 速度ベクトル描画（Colliders フラグとは独立）
    if (m_flags & PhysicsDebug_Velocities)
    {
        for (const auto& body : bodies)
        {
            // Dynamic ボディのみ速度を描画
            if (body.motionType != 2) continue;

            float speed = std::sqrt(body.linearVelocity.x * body.linearVelocity.x +
                                    body.linearVelocity.y * body.linearVelocity.y +
                                    body.linearVelocity.z * body.linearVelocity.z);
            if (speed > 0.01f)
            {
                Vector3 end = body.position + body.linearVelocity;
                debugDraw.DrawArrow((body.position), (end),
                                    m_velocityColor, 0.05f);
            }
        }
    }

    if (m_flags & PhysicsDebug_Contacts)
    {
        for (const auto& contact : contacts)
        {
            RenderContact(debugDraw, contact);
        }
    }

    if (m_flags & PhysicsDebug_Constraints)
    {
        for (const auto& constraint : constraints)
        {
            RenderConstraint(debugDraw, constraint);
        }
    }
}

// ============================================================================
// ボディ描画
// ============================================================================

void PhysicsDebugRenderer::RenderBody(DebugDraw3D& debugDraw, const BodyDebugInfo& body)
{
    // モーションタイプに基づいて色を決定
    Color color;
    switch (body.motionType)
    {
    case 0: // Static
        color = m_staticColor;
        break;
    case 1: // Kinematic
        color = m_kinematicColor;
        break;
    case 2: // Dynamic
    default:
        color = body.isActive ? m_dynamicActiveColor : m_dynamicSleepColor;
        break;
    }

    switch (body.shapeType)
    {
    case DebugShapeType::Box:
    {
        // ボックスは回転を考慮してワイヤーボックスを描画
        // DebugDraw3D::DrawWireBox は軸整列なので、回転は手動で8頂点を計算
        Vector3 ext = body.shapeExtents;
        Vector3 corners[8] = {
            { -ext.x, -ext.y, -ext.z },
            {  ext.x, -ext.y, -ext.z },
            {  ext.x,  ext.y, -ext.z },
            { -ext.x,  ext.y, -ext.z },
            { -ext.x, -ext.y,  ext.z },
            {  ext.x, -ext.y,  ext.z },
            {  ext.x,  ext.y,  ext.z },
            { -ext.x,  ext.y,  ext.z },
        };

        // 回転 + 平行移動
        for (auto& c : corners)
        {
            c = RotateByQuaternion(body.rotation, c) + body.position;
        }

        // 12本の辺を描画
        // 底面
        debugDraw.DrawLine((corners[0]), (corners[1]), color);
        debugDraw.DrawLine((corners[1]), (corners[2]), color);
        debugDraw.DrawLine((corners[2]), (corners[3]), color);
        debugDraw.DrawLine((corners[3]), (corners[0]), color);
        // 上面
        debugDraw.DrawLine((corners[4]), (corners[5]), color);
        debugDraw.DrawLine((corners[5]), (corners[6]), color);
        debugDraw.DrawLine((corners[6]), (corners[7]), color);
        debugDraw.DrawLine((corners[7]), (corners[4]), color);
        // 縦辺
        debugDraw.DrawLine((corners[0]), (corners[4]), color);
        debugDraw.DrawLine((corners[1]), (corners[5]), color);
        debugDraw.DrawLine((corners[2]), (corners[6]), color);
        debugDraw.DrawLine((corners[3]), (corners[7]), color);
        break;
    }
    case DebugShapeType::Sphere:
    {
        debugDraw.DrawWireSphere((body.position), body.shapeRadius, color, 16);
        break;
    }
    case DebugShapeType::Capsule:
    {
        DrawWireCapsule(debugDraw, body.position, body.rotation,
                        body.capsuleHalfHeight, body.shapeRadius, color);
        break;
    }
    case DebugShapeType::Mesh:
    case DebugShapeType::ConvexHull:
    {
        // メッシュ/凸包は AABB で代替描画
        debugDraw.DrawWireBox((body.position), (body.shapeExtents), color);
        break;
    }
    }
}

// ============================================================================
// カプセル描画
// ============================================================================

void PhysicsDebugRenderer::DrawWireCapsule(DebugDraw3D& debugDraw,
                                            const Vector3& center,
                                            const Quaternion& rot,
                                            float halfHeight, float radius,
                                            const Color& color)
{
    constexpr int segments = 12;
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = PI * 2.0f;

    Vector3 up = RotateByQuaternion(rot, Vector3(0, 1, 0));
    Vector3 right = RotateByQuaternion(rot, Vector3(1, 0, 0));
    Vector3 fwd = RotateByQuaternion(rot, Vector3(0, 0, 1));

    Vector3 topCenter = center + up * halfHeight;
    Vector3 bottomCenter = center - up * halfHeight;

    // 側面の4本の直線（カプセルの筒部分）
    for (int i = 0; i < 4; ++i)
    {
        float angle = TWO_PI * i / 4.0f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);
        Vector3 offset = right * (cosA * radius) + fwd * (sinA * radius);
        debugDraw.DrawLine((topCenter + offset), (bottomCenter + offset), color);
    }

    // 上下の円
    for (int i = 0; i < segments; ++i)
    {
        float a0 = TWO_PI * i / segments;
        float a1 = TWO_PI * (i + 1) / segments;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        Vector3 p0 = right * (c0 * radius) + fwd * (s0 * radius);
        Vector3 p1 = right * (c1 * radius) + fwd * (s1 * radius);

        // 上の円
        debugDraw.DrawLine((topCenter + p0), (topCenter + p1), color);
        // 下の円
        debugDraw.DrawLine((bottomCenter + p0), (bottomCenter + p1), color);
    }

    // 上下の半球弧（right-up面 と fwd-up面 の2つの大円弧）
    constexpr int arcSegments = 8;
    for (int i = 0; i < arcSegments; ++i)
    {
        float a0 = PI * 0.5f * i / arcSegments;
        float a1 = PI * 0.5f * (i + 1) / arcSegments;

        // 上半球 (right-up 面)
        Vector3 tp0 = topCenter + up * (std::sin(a0) * radius) + right * (std::cos(a0) * radius);
        Vector3 tp1 = topCenter + up * (std::sin(a1) * radius) + right * (std::cos(a1) * radius);
        debugDraw.DrawLine((tp0), (tp1), color);
        // 反対側
        Vector3 tp0n = topCenter + up * (std::sin(a0) * radius) - right * (std::cos(a0) * radius);
        Vector3 tp1n = topCenter + up * (std::sin(a1) * radius) - right * (std::cos(a1) * radius);
        debugDraw.DrawLine((tp0n), (tp1n), color);

        // 上半球 (fwd-up 面)
        Vector3 fp0 = topCenter + up * (std::sin(a0) * radius) + fwd * (std::cos(a0) * radius);
        Vector3 fp1 = topCenter + up * (std::sin(a1) * radius) + fwd * (std::cos(a1) * radius);
        debugDraw.DrawLine((fp0), (fp1), color);
        Vector3 fp0n = topCenter + up * (std::sin(a0) * radius) - fwd * (std::cos(a0) * radius);
        Vector3 fp1n = topCenter + up * (std::sin(a1) * radius) - fwd * (std::cos(a1) * radius);
        debugDraw.DrawLine((fp0n), (fp1n), color);

        // 下半球 (right-up 面)
        Vector3 bp0 = bottomCenter - up * (std::sin(a0) * radius) + right * (std::cos(a0) * radius);
        Vector3 bp1 = bottomCenter - up * (std::sin(a1) * radius) + right * (std::cos(a1) * radius);
        debugDraw.DrawLine((bp0), (bp1), color);
        Vector3 bp0n = bottomCenter - up * (std::sin(a0) * radius) - right * (std::cos(a0) * radius);
        Vector3 bp1n = bottomCenter - up * (std::sin(a1) * radius) - right * (std::cos(a1) * radius);
        debugDraw.DrawLine((bp0n), (bp1n), color);

        // 下半球 (fwd-up 面)
        Vector3 bfp0 = bottomCenter - up * (std::sin(a0) * radius) + fwd * (std::cos(a0) * radius);
        Vector3 bfp1 = bottomCenter - up * (std::sin(a1) * radius) + fwd * (std::cos(a1) * radius);
        debugDraw.DrawLine((bfp0), (bfp1), color);
        Vector3 bfp0n = bottomCenter - up * (std::sin(a0) * radius) - fwd * (std::cos(a0) * radius);
        Vector3 bfp1n = bottomCenter - up * (std::sin(a1) * radius) - fwd * (std::cos(a1) * radius);
        debugDraw.DrawLine((bfp0n), (bfp1n), color);
    }
}

// ============================================================================
// 接触点描画
// ============================================================================

void PhysicsDebugRenderer::RenderContact(DebugDraw3D& debugDraw, const ContactDebugInfo& contact)
{
    // 接触点AからBへの線
    debugDraw.DrawLine((contact.pointA), (contact.pointB), m_contactColor);

    // 接触法線を矢印で描画（pointA から法線方向に 0.3 単位）
    Vector3 normalEnd = contact.pointA + contact.normal * 0.3f;
    debugDraw.DrawArrow((contact.pointA), (normalEnd), m_contactColor, 0.03f);

    // 接触点に小さな十字を描画
    constexpr float crossSize = 0.02f;
    Vector3 p = contact.pointA;
    debugDraw.DrawLine(
        Vector3{p.x - crossSize, p.y, p.z},
        Vector3{p.x + crossSize, p.y, p.z},
        m_contactColor);
    debugDraw.DrawLine(
        Vector3{p.x, p.y - crossSize, p.z},
        Vector3{p.x, p.y + crossSize, p.z},
        m_contactColor);
    debugDraw.DrawLine(
        Vector3{p.x, p.y, p.z - crossSize},
        Vector3{p.x, p.y, p.z + crossSize},
        m_contactColor);
}

// ============================================================================
// コンストレイント描画
// ============================================================================

void PhysicsDebugRenderer::RenderConstraint(DebugDraw3D& debugDraw,
                                             const ConstraintDebugInfo& constraint)
{
    // アンカーA から アンカーB への線を描画
    debugDraw.DrawLine((constraint.anchorA), (constraint.anchorB),
                       m_constraintColor);

    // 各アンカー点に小さなダイヤモンド（菱形）を描画
    constexpr float size = 0.04f;
    auto drawDiamond = [&](const Vector3& p)
    {
        debugDraw.DrawLine(
            Vector3{p.x, p.y + size, p.z},
            Vector3{p.x + size, p.y, p.z},
            m_constraintColor);
        debugDraw.DrawLine(
            Vector3{p.x + size, p.y, p.z},
            Vector3{p.x, p.y - size, p.z},
            m_constraintColor);
        debugDraw.DrawLine(
            Vector3{p.x, p.y - size, p.z},
            Vector3{p.x - size, p.y, p.z},
            m_constraintColor);
        debugDraw.DrawLine(
            Vector3{p.x - size, p.y, p.z},
            Vector3{p.x, p.y + size, p.z},
            m_constraintColor);
    };

    drawDiamond(constraint.anchorA);
    drawDiamond(constraint.anchorB);
}

} // namespace gx
