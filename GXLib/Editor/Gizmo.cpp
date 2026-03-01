/// @file Gizmo.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "Editor/Gizmo.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Core/Logger.h"

using namespace DirectX;

namespace gx
{

// ============================================================================
// 定数
// ============================================================================
namespace
{
    constexpr float k_AxisLength     = 1.5f;   // 各軸線の長さ（m_scale適用前）
    constexpr float k_HitThreshold   = 0.15f;  // レイ-軸ヒットテストの距離閾値
    constexpr float k_ArrowHeadSize  = 0.15f;  // 矢印の頭の長さ（移動モード）
    constexpr float k_ScaleBoxSize   = 0.08f;  // スケールギズモ先端ボックスの半サイズ
    constexpr uint32_t k_CircleSegs  = 32;     // 回転円のセグメント数
    constexpr float k_HighlightAlpha = 1.0f;
    constexpr float k_DimAlpha       = 0.6f;

    // 軸の色（RGBA）
    const XMFLOAT4 k_Red   = { 1.0f, 0.2f, 0.2f, k_HighlightAlpha };
    const XMFLOAT4 k_Green = { 0.2f, 1.0f, 0.2f, k_HighlightAlpha };
    const XMFLOAT4 k_Blue  = { 0.3f, 0.3f, 1.0f, k_HighlightAlpha };

    const XMFLOAT4 k_RedDim   = { 1.0f, 0.2f, 0.2f, k_DimAlpha };
    const XMFLOAT4 k_GreenDim = { 0.2f, 1.0f, 0.2f, k_DimAlpha };
    const XMFLOAT4 k_BlueDim  = { 0.3f, 0.3f, 1.0f, k_DimAlpha };

    const XMFLOAT4 k_Yellow = { 1.0f, 1.0f, 0.2f, 1.0f }; // アクティブ軸のハイライト色

    /// 指定軸の単位方向と色を返す
    void GetAxisInfo(GizmoAxis axis, XMFLOAT3& outDir, XMFLOAT4& outColor, XMFLOAT4& outColorDim)
    {
        switch (axis)
        {
        case GizmoAxis::X: outDir = { 1, 0, 0 }; outColor = k_Red;   outColorDim = k_RedDim;   break;
        case GizmoAxis::Y: outDir = { 0, 1, 0 }; outColor = k_Green; outColorDim = k_GreenDim; break;
        case GizmoAxis::Z: outDir = { 0, 0, 1 }; outColor = k_Blue;  outColorDim = k_BlueDim;  break;
        default:           outDir = { 0, 0, 0 }; outColor = {};      outColorDim = {};         break;
        }
    }

    /// ヘルパー: XMFLOAT3 + XMFLOAT3 * スカラー
    XMFLOAT3 OffsetPoint(const XMFLOAT3& origin, const XMFLOAT3& dir, float t)
    {
        return { origin.x + dir.x * t, origin.y + dir.y * t, origin.z + dir.z * t };
    }
} // anonymous namespace

// ============================================================================
// Render
// ============================================================================
void Gizmo::Render(PrimitiveBatch3D& batch, const Transform3D& transform)
{
    if (!m_enabled) return;

    const XMFLOAT3 origin = transform.GetPosition();
    const float len = k_AxisLength * m_scale;

    auto drawAxisLine = [&](GizmoAxis axis)
    {
        XMFLOAT3 dir{};
        XMFLOAT4 color{}, colorDim{};
        GetAxisInfo(axis, dir, color, colorDim);

        // 現在ドラッグ中の軸をハイライト
        const XMFLOAT4& col = (m_dragging && m_dragAxis == axis) ? k_Yellow : color;

        XMFLOAT3 endPt = OffsetPoint(origin, dir, len);
        batch.DrawLine(origin, endPt, col);
    };

    switch (m_mode)
    {
    case GizmoMode::Translate:
    {
        // 矢印付き軸線を描画
        for (auto ax : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z })
        {
            XMFLOAT3 dir{};
            XMFLOAT4 color{}, colorDim{};
            GetAxisInfo(ax, dir, color, colorDim);
            const XMFLOAT4& col = (m_dragging && m_dragAxis == ax) ? k_Yellow : color;

            XMFLOAT3 endPt = OffsetPoint(origin, dir, len);
            batch.DrawLine(origin, endPt, col);

            // 矢印の頭: 端点から分岐する2本の短い線
            const float headLen = k_ArrowHeadSize * m_scale;

            // 矢印の頭用に'dir'に垂直な2方向を計算
            XMFLOAT3 perp1{}, perp2{};
            if (std::abs(dir.y) < 0.99f)
            {
                // cross(dir, up)
                perp1 = { dir.z, 0.0f, -dir.x };
            }
            else
            {
                // cross(dir, right)
                perp1 = { 0.0f, -dir.z, dir.y };
            }
            // perp1を正規化
            float pLen = std::sqrt(perp1.x * perp1.x + perp1.y * perp1.y + perp1.z * perp1.z);
            if (pLen > 1e-6f)
            {
                perp1.x /= pLen; perp1.y /= pLen; perp1.z /= pLen;
            }
            // perp2 = cross(dir, perp1)
            perp2 = {
                dir.y * perp1.z - dir.z * perp1.y,
                dir.z * perp1.x - dir.x * perp1.z,
                dir.x * perp1.y - dir.y * perp1.x
            };

            XMFLOAT3 arrowBase = OffsetPoint(endPt, dir, -headLen);
            batch.DrawLine(endPt, OffsetPoint(arrowBase, perp1,  headLen * 0.5f), col);
            batch.DrawLine(endPt, OffsetPoint(arrowBase, perp1, -headLen * 0.5f), col);
            batch.DrawLine(endPt, OffsetPoint(arrowBase, perp2,  headLen * 0.5f), col);
            batch.DrawLine(endPt, OffsetPoint(arrowBase, perp2, -headLen * 0.5f), col);
        }
        break;
    }
    case GizmoMode::Rotate:
    {
        // 各軸の周りに回転円を描画
        const float radius = len * 0.8f;
        for (auto ax : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z })
        {
            XMFLOAT3 dir{};
            XMFLOAT4 color{}, colorDim{};
            GetAxisInfo(ax, dir, color, colorDim);
            const XMFLOAT4& col = (m_dragging && m_dragAxis == ax) ? k_Yellow : color;

            // 法線が'dir'の円を描画
            XMFLOAT3 normal = dir;
            batch.DrawWireCircle(origin, normal, radius, col, k_CircleSegs);
        }
        break;
    }
    case GizmoMode::Scale:
    {
        // 先端にボックス付きの軸線を描画
        for (auto ax : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z })
        {
            XMFLOAT3 dir{};
            XMFLOAT4 color{}, colorDim{};
            GetAxisInfo(ax, dir, color, colorDim);
            const XMFLOAT4& col = (m_dragging && m_dragAxis == ax) ? k_Yellow : color;

            XMFLOAT3 endPt = OffsetPoint(origin, dir, len);
            batch.DrawLine(origin, endPt, col);

            // 先端のワイヤーフレームボックス
            XMFLOAT3 boxExtents = { k_ScaleBoxSize * m_scale,
                                     k_ScaleBoxSize * m_scale,
                                     k_ScaleBoxSize * m_scale };
            batch.DrawWireBox(endPt, boxExtents, col);
        }
        break;
    }
    } // switch
}

// ============================================================================
// HitTest
// ============================================================================
GizmoAxis Gizmo::HitTest(const XMFLOAT3& rayOrigin,
                           const XMFLOAT3& rayDir,
                           const Transform3D& transform) const
{
    if (!m_enabled) return GizmoAxis::None;

    const XMFLOAT3 center = transform.GetPosition();
    const float threshold = k_HitThreshold * m_scale;

    float bestDist = threshold;
    GizmoAxis bestAxis = GizmoAxis::None;

    for (auto ax : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z })
    {
        XMFLOAT3 dir{};
        XMFLOAT4 color{}, colorDim{};
        GetAxisInfo(ax, dir, color, colorDim);

        float dist = RayAxisDistance(rayOrigin, rayDir, center, dir);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestAxis = ax;
        }
    }

    return bestAxis;
}

// ============================================================================
// BeginDrag / UpdateDrag / EndDrag
// ============================================================================
void Gizmo::BeginDrag(GizmoAxis axis, const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir)
{
    if (axis == GizmoAxis::None) return;

    m_dragging        = true;
    m_dragInitialized = false;
    m_dragAxis        = axis;
    m_dragStartObjectPos = {};
    m_dragStartScale     = {};
    m_dragStartAxisT     = 0.0f;
}

void Gizmo::UpdateDrag(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir,
                         Transform3D& transform)
{
    if (!m_dragging || m_dragAxis == GizmoAxis::None) return;

    XMFLOAT3 axisDir{};
    XMFLOAT4 color{}, colorDim{};
    GetAxisInfo(m_dragAxis, axisDir, color, colorDim);

    const XMFLOAT3 axisOrigin = transform.GetPosition();

    // レイを軸に投影してカーソルが現在指している位置を求める
    float t = ProjectOntoAxis(rayOrigin, rayDir, axisOrigin, axisDir);

    // ドラッグの最初のフレームで開始値を記録
    if (!m_dragInitialized)
    {
        m_dragStartObjectPos = transform.GetPosition();
        m_dragStartScale     = transform.GetScale();
        m_dragStartAxisT     = t;
        m_dragInitialized    = true;
    }

    float delta = t - m_dragStartAxisT;

    switch (m_mode)
    {
    case GizmoMode::Translate:
    {
        XMFLOAT3 newPos = {
            m_dragStartObjectPos.x + axisDir.x * delta,
            m_dragStartObjectPos.y + axisDir.y * delta,
            m_dragStartObjectPos.z + axisDir.z * delta
        };
        transform.SetPosition(newPos);
        break;
    }
    case GizmoMode::Rotate:
    {
        // デルタ距離を回転角度にマッピング
        float angle = delta * 2.0f; // sensitivity factor
        XMFLOAT3 rot = transform.GetRotation();
        switch (m_dragAxis)
        {
        case GizmoAxis::X: rot.x = angle; break;
        case GizmoAxis::Y: rot.y = angle; break;
        case GizmoAxis::Z: rot.z = angle; break;
        default: break;
        }
        transform.SetRotation(rot);
        break;
    }
    case GizmoMode::Scale:
    {
        // スケール係数: 軸長さの単位あたり 1 + delta
        float scaleFactor = 1.0f + delta / (k_AxisLength * m_scale);
        if (scaleFactor < 0.01f) scaleFactor = 0.01f; // 負/ゼロスケールを防止

        XMFLOAT3 newScale = m_dragStartScale;
        switch (m_dragAxis)
        {
        case GizmoAxis::X: newScale.x *= scaleFactor; break;
        case GizmoAxis::Y: newScale.y *= scaleFactor; break;
        case GizmoAxis::Z: newScale.z *= scaleFactor; break;
        default: break;
        }
        transform.SetScale(newScale);
        break;
    }
    } // switch
}

void Gizmo::EndDrag()
{
    m_dragging        = false;
    m_dragInitialized = false;
    m_dragAxis        = GizmoAxis::None;
    m_dragStartPos       = {};
    m_dragStartObjectPos = {};
    m_dragStartScale     = {};
    m_dragStartAxisT     = 0.0f;
}

// ============================================================================
// プライベートヘルパー
// ============================================================================

float Gizmo::RayAxisDistance(const XMFLOAT3& rayOrigin,
                              const XMFLOAT3& rayDir,
                              const XMFLOAT3& axisOrigin,
                              const XMFLOAT3& axisDir) const
{
    // 2直線間の最短距離:
    //   Line1: P = rayOrigin + t * rayDir
    //   Line2: Q = axisOrigin + s * axisDir   (s clamped to [0, axisLength*m_scale])
    //
    // w = rayOrigin - axisOrigin
    // a = dot(rayDir, rayDir) = 1  (normalised)
    // b = dot(rayDir, axisDir)
    // c = dot(axisDir, axisDir) = 1
    // d = dot(rayDir, w)
    // e = dot(axisDir, w)
    // denom = a*c - b*b

    XMFLOAT3 w = { rayOrigin.x - axisOrigin.x,
                    rayOrigin.y - axisOrigin.y,
                    rayOrigin.z - axisOrigin.z };

    float a = rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z; // ~1
    float b = rayDir.x * axisDir.x + rayDir.y * axisDir.y + rayDir.z * axisDir.z;
    float c = axisDir.x * axisDir.x + axisDir.y * axisDir.y + axisDir.z * axisDir.z; // ~1
    float d = rayDir.x * w.x + rayDir.y * w.y + rayDir.z * w.z;
    float e = axisDir.x * w.x + axisDir.y * w.y + axisDir.z * w.z;

    float denom = a * c - b * b;
    float axisLen = k_AxisLength * m_scale;

    float sc, tc; // それぞれレイと軸上のパラメータ
    if (denom < 1e-6f)
    {
        // 直線がほぼ平行
        sc = 0.0f;
        tc = d / a; // approximate
    }
    else
    {
        sc = (b * d - a * e) / denom;
        tc = (c * d - b * e) / denom;
    }

    // sc（軸パラメータ）を[0, axisLen]にクランプ
    sc = (sc < 0.0f) ? 0.0f : (sc > axisLen ? axisLen : sc);

    // tc（レイパラメータ）を>= 0にクランプ（レイの前方のみ）
    if (tc < 0.0f) tc = 0.0f;

    // 最近接点
    XMFLOAT3 pOnAxis = OffsetPoint(axisOrigin, axisDir, sc);
    XMFLOAT3 pOnRay  = OffsetPoint(rayOrigin, rayDir, tc);

    float dx = pOnAxis.x - pOnRay.x;
    float dy = pOnAxis.y - pOnRay.y;
    float dz = pOnAxis.z - pOnRay.z;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

XMFLOAT3 Gizmo::RayPlaneIntersect(const XMFLOAT3& rayOrigin,
                                    const XMFLOAT3& rayDir,
                                    const XMFLOAT3& planePoint,
                                    const XMFLOAT3& planeNormal) const
{
    // t = dot(planePoint - rayOrigin, planeNormal) / dot(rayDir, planeNormal)
    float denom = rayDir.x * planeNormal.x + rayDir.y * planeNormal.y + rayDir.z * planeNormal.z;
    if (std::abs(denom) < 1e-8f)
    {
        // レイが平面に平行; フォールバックとしてレイ原点を返す
        return rayOrigin;
    }

    XMFLOAT3 diff = { planePoint.x - rayOrigin.x,
                       planePoint.y - rayOrigin.y,
                       planePoint.z - rayOrigin.z };
    float t = (diff.x * planeNormal.x + diff.y * planeNormal.y + diff.z * planeNormal.z) / denom;

    return OffsetPoint(rayOrigin, rayDir, t);
}

float Gizmo::ProjectOntoAxis(const XMFLOAT3& rayOrigin,
                               const XMFLOAT3& rayDir,
                               const XMFLOAT3& axisOrigin,
                               const XMFLOAT3& axisDir) const
{
    // 軸方向を含み、ビューレイに最も垂直な平面を求める。
    // 法線が cross(axisDir, cross(rayDir, axisDir)) の平面を使用する。
    // これにより axisDirを含み、カメラレイに最も「正面」を向く平面が得られる。

    // cross(rayDir, axisDir)
    XMFLOAT3 rxa = {
        rayDir.y * axisDir.z - rayDir.z * axisDir.y,
        rayDir.z * axisDir.x - rayDir.x * axisDir.z,
        rayDir.x * axisDir.y - rayDir.y * axisDir.x
    };

    // planeNormal = cross(axisDir, rxa)
    XMFLOAT3 planeN = {
        axisDir.y * rxa.z - axisDir.z * rxa.y,
        axisDir.z * rxa.x - axisDir.x * rxa.z,
        axisDir.x * rxa.y - axisDir.y * rxa.x
    };

    // planeNを正規化
    float lenN = std::sqrt(planeN.x * planeN.x + planeN.y * planeN.y + planeN.z * planeN.z);
    if (lenN < 1e-8f)
    {
        return 0.0f;
    }
    planeN.x /= lenN; planeN.y /= lenN; planeN.z /= lenN;

    // レイとこの平面の交点を求める
    XMFLOAT3 hitPt = RayPlaneIntersect(rayOrigin, rayDir, axisOrigin, planeN);

    // ヒットポイントを軸に投影する
    XMFLOAT3 diff = { hitPt.x - axisOrigin.x,
                       hitPt.y - axisOrigin.y,
                       hitPt.z - axisOrigin.z };
    float t = diff.x * axisDir.x + diff.y * axisDir.y + diff.z * axisDir.z;
    return t;
}

} // namespace gx
