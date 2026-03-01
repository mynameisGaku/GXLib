/// @file ClothSimulator.cpp
/// @brief ClothSimulator の実装
#include "pch_common.h"
#include "Physics/ClothSimulator.h"
#include <cmath>

using namespace DirectX;

namespace gx
{

bool ClothSimulator::Initialize(const ClothDesc& desc)
{
    if (desc.width < 2 || desc.height < 2) return false;

    m_width     = desc.width;
    m_height    = desc.height;
    m_stiffness = desc.stiffness;
    m_damping   = desc.damping;
    m_gravity   = desc.gravity;

    const uint32_t vertexCount = m_width * m_height;
    const float totalMass = desc.mass > 0.0f ? desc.mass : 1.0f;
    m_invMass = static_cast<float>(vertexCount) / totalMass;

    // 頂点の初期配置（XZ平面、Y=0）
    m_positions.resize(vertexCount);
    m_prevPositions.resize(vertexCount);
    m_normals.resize(vertexCount, { 0.0f, 1.0f, 0.0f });
    m_pinned.resize(vertexCount, false);

    for (uint32_t y = 0; y < m_height; ++y)
    {
        for (uint32_t x = 0; x < m_width; ++x)
        {
            uint32_t idx = y * m_width + x;
            m_positions[idx] = { x * desc.spacing, 0.0f, y * desc.spacing };
            m_prevPositions[idx] = m_positions[idx];
        }
    }

    // 固定頂点
    for (uint32_t idx : desc.pinnedVertices)
    {
        if (idx < vertexCount)
            m_pinned[idx] = true;
    }

    // 拘束の生成（Structural + Shear + Bend）
    auto addConstraint = [&](uint32_t i0, uint32_t i1)
    {
        const auto& p0 = m_positions[i0];
        const auto& p1 = m_positions[i1];
        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        float dz = p1.z - p0.z;
        float restLen = std::sqrt(dx * dx + dy * dy + dz * dz);
        m_constraints.push_back({ i0, i1, restLen });
    };

    for (uint32_t y = 0; y < m_height; ++y)
    {
        for (uint32_t x = 0; x < m_width; ++x)
        {
            uint32_t idx = y * m_width + x;

            // Structural (水平・垂直)
            if (x + 1 < m_width)  addConstraint(idx, idx + 1);
            if (y + 1 < m_height) addConstraint(idx, idx + m_width);

            // Shear (対角)
            if (x + 1 < m_width && y + 1 < m_height)
            {
                addConstraint(idx, idx + m_width + 1);
                addConstraint(idx + 1, idx + m_width);
            }

            // Bend (2つ先)
            if (x + 2 < m_width)  addConstraint(idx, idx + 2);
            if (y + 2 < m_height) addConstraint(idx, idx + 2 * m_width);
        }
    }

    // インデックスバッファ（三角形ストリップ→三角形リスト）
    m_indices.clear();
    for (uint32_t y = 0; y < m_height - 1; ++y)
    {
        for (uint32_t x = 0; x < m_width - 1; ++x)
        {
            uint32_t tl = y * m_width + x;
            uint32_t tr = tl + 1;
            uint32_t bl = tl + m_width;
            uint32_t br = bl + 1;

            m_indices.push_back(tl);
            m_indices.push_back(bl);
            m_indices.push_back(tr);

            m_indices.push_back(tr);
            m_indices.push_back(bl);
            m_indices.push_back(br);
        }
    }

    m_externalForce = { 0.0f, 0.0f, 0.0f };
    return true;
}

void ClothSimulator::Update(float deltaTime, int substeps)
{
    if (m_positions.empty() || substeps <= 0) return;

    const float subDt = deltaTime / static_cast<float>(substeps);

    for (int step = 0; step < substeps; ++step)
    {
        // Verlet 積分
        for (size_t i = 0; i < m_positions.size(); ++i)
        {
            if (m_pinned[i]) continue;

            auto& pos  = m_positions[i];
            auto& prev = m_prevPositions[i];

            float vx = (pos.x - prev.x) * (1.0f - m_damping);
            float vy = (pos.y - prev.y) * (1.0f - m_damping);
            float vz = (pos.z - prev.z) * (1.0f - m_damping);

            prev = pos;

            float ax = (m_gravity.x + m_externalForce.x) * m_invMass;
            float ay = (m_gravity.y + m_externalForce.y) * m_invMass;
            float az = (m_gravity.z + m_externalForce.z) * m_invMass;

            pos.x += vx + ax * subDt * subDt;
            pos.y += vy + ay * subDt * subDt;
            pos.z += vz + az * subDt * subDt;
        }

        // 距離拘束を解く
        SolveConstraints();

        // コリジョン
        HandleCollisions();
    }

    // 外力リセット
    m_externalForce = { 0.0f, 0.0f, 0.0f };

    // 法線再計算
    RecalculateNormals();
}

void ClothSimulator::SolveConstraints()
{
    for (const auto& c : m_constraints)
    {
        auto& p0 = m_positions[c.i0];
        auto& p1 = m_positions[c.i1];

        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        float dz = p1.z - p0.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < 1e-7f) continue;

        float diff = (dist - c.restLength) / dist;
        float halfDiff = diff * 0.5f * m_stiffness;

        float cx = dx * halfDiff;
        float cy = dy * halfDiff;
        float cz = dz * halfDiff;

        if (!m_pinned[c.i0])
        {
            p0.x += cx;
            p0.y += cy;
            p0.z += cz;
        }
        if (!m_pinned[c.i1])
        {
            p1.x -= cx;
            p1.y -= cy;
            p1.z -= cz;
        }
    }
}

void ClothSimulator::HandleCollisions()
{
    for (const auto& col : m_colliders)
    {
        for (size_t i = 0; i < m_positions.size(); ++i)
        {
            if (m_pinned[i]) continue;

            auto& pos = m_positions[i];
            float dx = pos.x - col.center.x;
            float dy = pos.y - col.center.y;
            float dz = pos.z - col.center.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float rSq = col.radius * col.radius;

            if (distSq < rSq && distSq > 1e-10f)
            {
                float dist = std::sqrt(distSq);
                float invDist = 1.0f / dist;
                pos.x = col.center.x + dx * invDist * col.radius;
                pos.y = col.center.y + dy * invDist * col.radius;
                pos.z = col.center.z + dz * invDist * col.radius;
            }
        }
    }
}

void ClothSimulator::RecalculateNormals()
{
    // ゼロクリア
    for (auto& n : m_normals) n = { 0.0f, 0.0f, 0.0f };

    // 三角形ごとの面法線を頂点に蓄積
    for (size_t i = 0; i + 2 < m_indices.size(); i += 3)
    {
        uint32_t i0 = m_indices[i];
        uint32_t i1 = m_indices[i + 1];
        uint32_t i2 = m_indices[i + 2];

        const auto& p0 = m_positions[i0];
        const auto& p1 = m_positions[i1];
        const auto& p2 = m_positions[i2];

        float e1x = p1.x - p0.x, e1y = p1.y - p0.y, e1z = p1.z - p0.z;
        float e2x = p2.x - p0.x, e2y = p2.y - p0.y, e2z = p2.z - p0.z;

        float nx = e1y * e2z - e1z * e2y;
        float ny = e1z * e2x - e1x * e2z;
        float nz = e1x * e2y - e1y * e2x;

        m_normals[i0].x += nx; m_normals[i0].y += ny; m_normals[i0].z += nz;
        m_normals[i1].x += nx; m_normals[i1].y += ny; m_normals[i1].z += nz;
        m_normals[i2].x += nx; m_normals[i2].y += ny; m_normals[i2].z += nz;
    }

    // 正規化
    for (auto& n : m_normals)
    {
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 1e-7f)
        {
            n.x /= len;
            n.y /= len;
            n.z /= len;
        }
        else
        {
            n = { 0.0f, 1.0f, 0.0f };
        }
    }
}

void ClothSimulator::AddSphereCollider(const XMFLOAT3& center, float radius)
{
    m_colliders.push_back({ center, radius });
}

void ClothSimulator::ClearColliders()
{
    m_colliders.clear();
}

void ClothSimulator::PinVertex(uint32_t index)
{
    if (index < m_pinned.size())
        m_pinned[index] = true;
}

void ClothSimulator::UnpinVertex(uint32_t index)
{
    if (index < m_pinned.size())
        m_pinned[index] = false;
}

void ClothSimulator::ApplyForce(const XMFLOAT3& force)
{
    m_externalForce.x += force.x;
    m_externalForce.y += force.y;
    m_externalForce.z += force.z;
}

} // namespace gx
