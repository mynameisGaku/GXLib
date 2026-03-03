/// @file BuoyancySystem.cpp
/// @brief BuoyancySystem の実装
#include "pch_common.h"

#include "Physics/BuoyancySystem.h"
#include "Math/MathUtil.h"

namespace gx {

// ============================================================================
// AddBody
// ============================================================================
void BuoyancySystem::AddBody(uint32_t id, float mass, float volume,
                             const gx::Vector<Vector3>& samplePoints)
{
    // 既存IDは上書き
    int idx = FindBodyIndex(id);
    if (idx >= 0)
    {
        m_bodies[static_cast<size_t>(idx)].mass         = mass;
        m_bodies[static_cast<size_t>(idx)].volume       = volume;
        m_bodies[static_cast<size_t>(idx)].samplePoints = samplePoints;
        return;
    }

    BuoyancyBody body;
    body.id           = id;
    body.mass         = mass;
    body.volume       = volume;
    body.samplePoints = samplePoints;
    m_bodies.push_back(std::move(body));
}

// ============================================================================
// RemoveBody
// ============================================================================
void BuoyancySystem::RemoveBody(uint32_t id)
{
    int idx = FindBodyIndex(id);
    if (idx >= 0)
        m_bodies.erase(m_bodies.begin() + idx);
}

// ============================================================================
// HasBody / GetBody
// ============================================================================
bool BuoyancySystem::HasBody(uint32_t id) const
{
    return FindBodyIndex(id) >= 0;
}

const BuoyancyBody* BuoyancySystem::GetBody(uint32_t id) const
{
    int idx = FindBodyIndex(id);
    if (idx >= 0)
        return &m_bodies[static_cast<size_t>(idx)];
    return nullptr;
}

// ============================================================================
// SetBodyTransform / SetBodyVelocity
// ============================================================================
void BuoyancySystem::SetBodyTransform(uint32_t id, const Vector3& position, const Quaternion& rotation)
{
    int idx = FindBodyIndex(id);
    if (idx < 0) return;
    m_bodies[static_cast<size_t>(idx)].position = position;
    m_bodies[static_cast<size_t>(idx)].rotation = rotation;
}

void BuoyancySystem::SetBodyVelocity(uint32_t id, const Vector3& linear, const Vector3& angular)
{
    int idx = FindBodyIndex(id);
    if (idx < 0) return;
    m_bodies[static_cast<size_t>(idx)].linearVelocity  = linear;
    m_bodies[static_cast<size_t>(idx)].angularVelocity = angular;
}

// ============================================================================
// GetWaterHeight — Gerstner風波高
// ============================================================================
float BuoyancySystem::GetWaterHeight(float x, float z, float time) const
{
    float level = m_waterConfig.waterLevel;
    float amp   = m_waterConfig.waveAmplitude;
    float freq  = m_waterConfig.waveFrequency;
    float speed = m_waterConfig.waveSpeed;

    // waterLevel + amplitude * sin(freq*x + speed*time) + amplitude*0.5*sin(freq*1.3*z + speed*0.7*time)
    float h = level
            + amp * std::sin(freq * x + speed * time)
            + amp * 0.5f * std::sin(freq * 1.3f * z + speed * 0.7f * time);
    return h;
}

// ============================================================================
// GetWaterNormal — 波高関数の解析微分
// ============================================================================
Vector3 BuoyancySystem::GetWaterNormal(float x, float z, float time) const
{
    float amp   = m_waterConfig.waveAmplitude;
    float freq  = m_waterConfig.waveFrequency;
    float speed = m_waterConfig.waveSpeed;

    // dH/dx = amp * freq * cos(freq*x + speed*time)
    float dhdx = amp * freq * std::cos(freq * x + speed * time);

    // dH/dz = amp * 0.5 * freq * 1.3 * cos(freq*1.3*z + speed*0.7*time)
    float dhdz = amp * 0.5f * freq * 1.3f * std::cos(freq * 1.3f * z + speed * 0.7f * time);

    // 法線 = (-dH/dx, 1, -dH/dz) を正規化
    Vector3 normal(-dhdx, 1.0f, -dhdz);
    return normal.Normalized();
}

// ============================================================================
// ComputeBuoyancy
// ============================================================================
BuoyancyForce BuoyancySystem::ComputeBuoyancy(uint32_t id, float currentTime) const
{
    BuoyancyForce result;
    result.force  = Vector3::Zero();
    result.torque = Vector3::Zero();
    result.submergedFraction = 0.0f;

    int idx = FindBodyIndex(id);
    if (idx < 0) return result;

    const BuoyancyBody& body = m_bodies[static_cast<size_t>(idx)];
    const int sampleCount = static_cast<int>(body.samplePoints.size());
    if (sampleCount == 0)
    {
        // サンプルポイントなし → ボディ中心で単一計算
        Vector3 worldCenter = body.position + body.rotation.RotateVector(body.centerOfBuoyancy);
        float waterH = GetWaterHeight(worldCenter.x, worldCenter.z, currentTime);
        float depth  = waterH - worldCenter.y;
        if (depth > 0.0f)
        {
            float fraction = MathUtil::Clamp(depth / 1.0f, 0.0f, 1.0f);
            result.submergedFraction = fraction;
            // buoyancy = rho * g * V * fraction * up
            float buoyancyMag = m_waterConfig.density * m_gravity * body.volume * fraction;
            result.force = Vector3(0.0f, buoyancyMag, 0.0f);

            // ドラッグ
            float dragMag = m_waterConfig.dragCoefficient * fraction;
            Vector3 dragForce = body.linearVelocity * (-dragMag * body.linearVelocity.Length());
            result.force += dragForce;
        }
        return result;
    }

    // サンプルポイントごとに計算
    float volumePerPoint = body.volume / static_cast<float>(sampleCount);
    int submergedCount = 0;
    Vector3 totalForce  = Vector3::Zero();
    Vector3 totalTorque = Vector3::Zero();

    for (int i = 0; i < sampleCount; ++i)
    {
        Vector3 worldPt = LocalToWorld(body, body.samplePoints[static_cast<size_t>(i)]);
        float waterH = GetWaterHeight(worldPt.x, worldPt.z, currentTime);
        float depth  = waterH - worldPt.y;

        if (depth <= 0.0f) continue;

        ++submergedCount;
        float depthFraction = MathUtil::Clamp(depth / (body.volume > 0.0f ? std::cbrt(body.volume) : 1.0f), 0.0f, 1.0f);

        // 浮力: rho * g * Vpoint * depthFraction * up
        float buoyancyMag = m_waterConfig.density * m_gravity * volumePerPoint * depthFraction;
        Vector3 buoyancyForce(0.0f, buoyancyMag, 0.0f);
        totalForce += buoyancyForce;

        // トルク: r x F (rはボディ中心からサンプル点までのオフセット)
        Vector3 r = worldPt - body.position;
        totalTorque += r.Cross(buoyancyForce);
    }

    // ドラッグ（浸水率に比例）
    float fraction = static_cast<float>(submergedCount) / static_cast<float>(sampleCount);
    if (fraction > 0.0f)
    {
        float speed = body.linearVelocity.Length();
        if (speed > 1e-4f)
        {
            Vector3 dragForce = body.linearVelocity * (-m_waterConfig.dragCoefficient * fraction * speed);
            totalForce += dragForce;
        }

        // 角度ドラッグ
        float angSpeed = body.angularVelocity.Length();
        if (angSpeed > 1e-4f)
        {
            Vector3 angDrag = body.angularVelocity * (-m_waterConfig.angularDragCoefficient * fraction * angSpeed);
            totalTorque += angDrag;
        }
    }

    result.force             = totalForce;
    result.torque            = totalTorque;
    result.submergedFraction = fraction;

    return result;
}

// ============================================================================
// Update — 全ボディの浮力を計算して積分する
// ============================================================================
void BuoyancySystem::Update(float deltaTime, float currentTime)
{
    if (deltaTime <= 0.0f) return;

    for (auto& body : m_bodies)
    {
        const int sampleCount = static_cast<int>(body.samplePoints.size());
        float volumePerPoint = (sampleCount > 0)
            ? (body.volume / static_cast<float>(sampleCount))
            : body.volume;

        Vector3 totalForce  = Vector3::Zero();
        Vector3 totalTorque = Vector3::Zero();
        int submergedCount = 0;

        if (sampleCount == 0)
        {
            // 単一中心点
            Vector3 worldCenter = body.position + body.rotation.RotateVector(body.centerOfBuoyancy);
            float waterH = GetWaterHeight(worldCenter.x, worldCenter.z, currentTime);
            float depth  = waterH - worldCenter.y;
            if (depth > 0.0f)
            {
                float fraction = MathUtil::Clamp(depth / 1.0f, 0.0f, 1.0f);
                float buoyancyMag = m_waterConfig.density * m_gravity * body.volume * fraction;
                totalForce += Vector3(0.0f, buoyancyMag, 0.0f);
                submergedCount = 1;
            }
        }
        else
        {
            for (int i = 0; i < sampleCount; ++i)
            {
                Vector3 worldPt = LocalToWorld(body, body.samplePoints[static_cast<size_t>(i)]);
                float waterH = GetWaterHeight(worldPt.x, worldPt.z, currentTime);
                float depth  = waterH - worldPt.y;

                if (depth <= 0.0f) continue;
                ++submergedCount;

                float depthFraction = MathUtil::Clamp(
                    depth / (body.volume > 0.0f ? std::cbrt(body.volume) : 1.0f), 0.0f, 1.0f);
                float buoyancyMag = m_waterConfig.density * m_gravity * volumePerPoint * depthFraction;
                Vector3 buoyancyForce(0.0f, buoyancyMag, 0.0f);
                totalForce += buoyancyForce;

                Vector3 r = worldPt - body.position;
                totalTorque += r.Cross(buoyancyForce);
            }
        }

        // 浸水率
        float fraction = 0.0f;
        if (sampleCount > 0)
            fraction = static_cast<float>(submergedCount) / static_cast<float>(sampleCount);
        else
            fraction = (submergedCount > 0) ? 1.0f : 0.0f;

        body.submergedFraction = fraction;

        // ドラッグ
        if (fraction > 0.0f)
        {
            float speed = body.linearVelocity.Length();
            if (speed > 1e-4f)
                totalForce += body.linearVelocity * (-m_waterConfig.dragCoefficient * fraction * speed);

            float angSpeed = body.angularVelocity.Length();
            if (angSpeed > 1e-4f)
                totalTorque += body.angularVelocity * (-m_waterConfig.angularDragCoefficient * fraction * angSpeed);
        }

        // 重力
        totalForce += Vector3(0.0f, -m_gravity * body.mass, 0.0f);

        // Semi-Implicit Euler 積分
        float invMass = (body.mass > 0.0f) ? (1.0f / body.mass) : 0.0f;
        // 簡易慣性モーメント
        float inertia    = body.mass * 0.4f;
        float invInertia = (inertia > 0.0f) ? (1.0f / inertia) : 0.0f;

        body.linearVelocity  += totalForce * invMass * deltaTime;
        body.angularVelocity += totalTorque * invInertia * deltaTime;

        body.position += body.linearVelocity * deltaTime;

        // 回転の積分
        float angVelLen = body.angularVelocity.Length();
        if (angVelLen > 1e-6f)
        {
            Vector3 axis = body.angularVelocity * (1.0f / angVelLen);
            float angle  = angVelLen * deltaTime;
            Quaternion dq = Quaternion::FromAxisAngle(axis, angle);
            body.rotation = dq * body.rotation;
            body.rotation.Normalize();
        }
    }
}

// ============================================================================
// GetSubmergedFraction
// ============================================================================
float BuoyancySystem::GetSubmergedFraction(uint32_t id) const
{
    int idx = FindBodyIndex(id);
    if (idx < 0) return 0.0f;
    return m_bodies[static_cast<size_t>(idx)].submergedFraction;
}

// ============================================================================
// LocalToWorld
// ============================================================================
Vector3 BuoyancySystem::LocalToWorld(const BuoyancyBody& body, const Vector3& localPos) const
{
    return body.position + body.rotation.RotateVector(localPos);
}

// ============================================================================
// FindBodyIndex
// ============================================================================
int BuoyancySystem::FindBodyIndex(uint32_t id) const
{
    for (size_t i = 0; i < m_bodies.size(); ++i)
    {
        if (m_bodies[i].id == id)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace gx
