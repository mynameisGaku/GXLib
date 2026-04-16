/// @file DebugDraw3D.cpp
/// @brief 3Dデバッグ描画実装
#include "pch_graphics.h"
#include "Graphics/3D/DebugDraw3D.h"
#include "Graphics/3D/PrimitiveBatch3D.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"
#include <cmath>
#include <string>

using namespace DirectX;

namespace gx
{

DebugDraw3D& DebugDraw3D::Instance()
{
    static DebugDraw3D instance;
    return instance;
}

void DebugDraw3D::DrawLine(const Vector3& from, const Vector3& to, const Color& color)
{
    if (!m_enabled) return;
    m_lines.push_back({ from, to, color });
}

void DebugDraw3D::DrawWireBox(const Vector3& center, const Vector3& extent, const Color& color)
{
    if (!m_enabled) return;

    float x0 = center.x - extent.x, x1 = center.x + extent.x;
    float y0 = center.y - extent.y, y1 = center.y + extent.y;
    float z0 = center.z - extent.z, z1 = center.z + extent.z;

    // 底面
    DrawLine({ x0,y0,z0 }, { x1,y0,z0 }, color);
    DrawLine({ x1,y0,z0 }, { x1,y0,z1 }, color);
    DrawLine({ x1,y0,z1 }, { x0,y0,z1 }, color);
    DrawLine({ x0,y0,z1 }, { x0,y0,z0 }, color);
    // 上面
    DrawLine({ x0,y1,z0 }, { x1,y1,z0 }, color);
    DrawLine({ x1,y1,z0 }, { x1,y1,z1 }, color);
    DrawLine({ x1,y1,z1 }, { x0,y1,z1 }, color);
    DrawLine({ x0,y1,z1 }, { x0,y1,z0 }, color);
    // 柱
    DrawLine({ x0,y0,z0 }, { x0,y1,z0 }, color);
    DrawLine({ x1,y0,z0 }, { x1,y1,z0 }, color);
    DrawLine({ x1,y0,z1 }, { x1,y1,z1 }, color);
    DrawLine({ x0,y0,z1 }, { x0,y1,z1 }, color);
}

void DebugDraw3D::DrawWireSphere(const Vector3& center, float radius,
                                  const Color& color, int segments)
{
    if (!m_enabled) return;

    const float pi = 3.14159265358979f;
    const float step = 2.0f * pi / static_cast<float>(segments);

    // XY, XZ, YZ の3リング
    for (int i = 0; i < segments; ++i)
    {
        float a0 = step * static_cast<float>(i);
        float a1 = step * static_cast<float>(i + 1);
        float c0 = std::cos(a0) * radius, s0 = std::sin(a0) * radius;
        float c1 = std::cos(a1) * radius, s1 = std::sin(a1) * radius;

        // XZリング（水平）
        DrawLine({ center.x + c0, center.y, center.z + s0 },
                 { center.x + c1, center.y, center.z + s1 }, color);
        // XYリング
        DrawLine({ center.x + c0, center.y + s0, center.z },
                 { center.x + c1, center.y + s1, center.z }, color);
        // YZリング
        DrawLine({ center.x, center.y + c0, center.z + s0 },
                 { center.x, center.y + c1, center.z + s1 }, color);
    }
}

void DebugDraw3D::DrawArrow(const Vector3& from, const Vector3& to,
                             const Color& color, float headSize)
{
    if (!m_enabled) return;

    DrawLine(from, to, color);

    // 矢印ヘッド（簡易：方向に対して直交する2方向にオフセット）
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float dz = to.z - from.z;
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-6f) return;

    float invLen = 1.0f / len;
    dx *= invLen; dy *= invLen; dz *= invLen;

    // 直交ベクトルを求める
    Vector3 up = { 0, 1, 0 };
    if (std::abs(dy) > 0.99f) up = { 1, 0, 0 };

    float perpX = dy * up.z - dz * up.y;
    float perpY = dz * up.x - dx * up.z;
    float perpZ = dx * up.y - dy * up.x;
    float perpLen = std::sqrt(perpX * perpX + perpY * perpY + perpZ * perpZ);
    if (perpLen > 1e-6f)
    {
        perpX /= perpLen; perpY /= perpLen; perpZ /= perpLen;
    }

    Vector3 base = { to.x - dx * headSize, to.y - dy * headSize, to.z - dz * headSize };
    DrawLine(to, { base.x + perpX * headSize * 0.5f,
                   base.y + perpY * headSize * 0.5f,
                   base.z + perpZ * headSize * 0.5f }, color);
    DrawLine(to, { base.x - perpX * headSize * 0.5f,
                   base.y - perpY * headSize * 0.5f,
                   base.z - perpZ * headSize * 0.5f }, color);
}

void DebugDraw3D::DrawText3D(const Vector3& worldPos, const gx::String& text,
                              const Color& color)
{
    if (!m_enabled) return;
    m_texts.push_back({ worldPos, text, color });
}

void DebugDraw3D::SetViewProjection(const Matrix4x4& viewProj)
{
    m_viewProj = viewProj;
}

void DebugDraw3D::RenderText(float screenWidth, float screenHeight)
{
    if (!m_enabled || m_texts.empty()) return;

    XMMATRIX vp = XMLoadFloat4x4(XM(&m_viewProj));

    for (const auto& entry : m_texts)
    {
        // ワールド座標をクリップ空間に変換
        XMVECTOR worldPos = XMLoadFloat3(XM(&entry.worldPos));
        worldPos = XMVectorSetW(worldPos, 1.0f);
        XMVECTOR clipPos = XMVector4Transform(worldPos, vp);

        float w = XMVectorGetW(clipPos);

        // カメラの背後ならスキップ
        if (w < 0.0001f) continue;

        // 透視除算でNDCに変換
        float ndcX = XMVectorGetX(clipPos) / w;
        float ndcY = XMVectorGetY(clipPos) / w;

        // NDC範囲外ならスキップ
        if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) continue;

        // NDCからスクリーン座標に変換
        float screenX = (ndcX + 1.0f) * 0.5f * screenWidth;
        float screenY = (1.0f - ndcY) * 0.5f * screenHeight;

        GX_LOG_INFO("DebugDraw3D::Text [%.0f, %.0f] \"%s\"",
                    screenX, screenY, entry.text.c_str());
    }
}

void DebugDraw3D::DrawLinePersistent(const Vector3& from, const Vector3& to,
                                      const Color& color, float duration)
{
    if (!m_enabled) return;
    m_persistentLines.push_back({ from, to, color, duration });
}

void DebugDraw3D::DrawWireBoxPersistent(const Vector3& center, const Vector3& extent,
                                         const Color& color, float duration)
{
    if (!m_enabled) return;

    float x0 = center.x - extent.x, x1 = center.x + extent.x;
    float y0 = center.y - extent.y, y1 = center.y + extent.y;
    float z0 = center.z - extent.z, z1 = center.z + extent.z;

    auto add = [&](const Vector3& a, const Vector3& b)
    {
        m_persistentLines.push_back({ a, b, color, duration });
    };

    add({ x0,y0,z0 }, { x1,y0,z0 }); add({ x1,y0,z0 }, { x1,y0,z1 });
    add({ x1,y0,z1 }, { x0,y0,z1 }); add({ x0,y0,z1 }, { x0,y0,z0 });
    add({ x0,y1,z0 }, { x1,y1,z0 }); add({ x1,y1,z0 }, { x1,y1,z1 });
    add({ x1,y1,z1 }, { x0,y1,z1 }); add({ x0,y1,z1 }, { x0,y1,z0 });
    add({ x0,y0,z0 }, { x0,y1,z0 }); add({ x1,y0,z0 }, { x1,y1,z0 });
    add({ x1,y0,z1 }, { x1,y1,z1 }); add({ x0,y0,z1 }, { x0,y1,z1 });
}

void DebugDraw3D::Render(PrimitiveBatch3D& batch, float deltaTime)
{
    if (!m_enabled) return;

    auto toVec4 = [](const Color& c) -> Vector4 {
        return { c.r, c.g, c.b, c.a };
    };

    // 即時描画
    for (const auto& line : m_lines)
    {
        batch.DrawLine(line.from, line.to, toVec4(line.color));
    }

    // 永続描画
    for (auto it = m_persistentLines.begin(); it != m_persistentLines.end(); )
    {
        batch.DrawLine(it->from, it->to, toVec4(it->color));
        it->remainingTime -= deltaTime;
        if (it->remainingTime <= 0.0f)
            it = m_persistentLines.erase(it);
        else
            ++it;
    }
}

void DebugDraw3D::Clear()
{
    m_lines.clear();
    m_texts.clear();
}

} // namespace gx
