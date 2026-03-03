/// @file ClothSimulation.cpp
/// @brief GPU布シミュレーション実装（Verlet積分 + PBD制約ソルバー）

#include "pch_graphics.h"
#include "Graphics/3D/ClothSimulation.h"

#include <algorithm>
#include <cmath>
#include <cassert>

namespace gx
{

// -----------------------------------------------------------------------
// ヘルパー: Vector3 演算
// -----------------------------------------------------------------------
namespace
{

inline Vector3 Add(const Vector3& a, const Vector3& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vector3 Sub(const Vector3& a, const Vector3& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline Vector3 Mul(const Vector3& v, float s)
{
    return { v.x * s, v.y * s, v.z * s };
}

inline float Length(const Vector3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline Vector3 Normalize(const Vector3& v)
{
    float len = Length(v);
    if (len < 1e-8f) return { 0.0f, 0.0f, 0.0f };
    return Mul(v, 1.0f / len);
}

inline float Distance(const Vector3& a, const Vector3& b)
{
    return Length(Sub(a, b));
}

} // anonymous namespace

// -----------------------------------------------------------------------
// Initialize / Shutdown
// -----------------------------------------------------------------------

bool ClothSimulation::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    if (m_initialized) Shutdown();

    m_width  = width;
    m_height = height;

    if (!device)
    {
        // CPU-only mode — no GPU resources
        m_cpuMode     = true;
        m_initialized = true;
        return true;
    }

    // GPU mode
    m_cpuMode = false;

    // TODO: Create compute pipeline state objects for Verlet integration and constraint solving
    // For now, fall back to CPU mode even with a device to allow basic functionality
    // GPU resources: m_particleBuffer, m_constraintBuffer, m_rootSignature, m_verletPSO, m_constraintPSO
    // These will be created when shaders are compiled

    m_initialized = true;
    return true;
}

void ClothSimulation::Shutdown()
{
    m_particles.clear();
    m_constraints.clear();
    m_colliders.clear();

    m_particleBuffer.Reset();
    m_constraintBuffer.Reset();
    m_rootSignature.Reset();
    m_verletPSO.Reset();
    m_constraintPSO.Reset();

    m_width        = 0;
    m_height       = 0;
    m_cpuMode      = true;
    m_initialized  = false;
    m_nextColliderId = 0;
}

// -----------------------------------------------------------------------
// Grid creation
// -----------------------------------------------------------------------

void ClothSimulation::CreateGrid(uint32_t width, uint32_t height, float spacing)
{
    m_width  = width;
    m_height = height;

    m_particles.clear();
    m_constraints.clear();

    // Generate particles in a grid layout (XZ plane at Y=0)
    m_particles.resize(static_cast<size_t>(width) * height);
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t idx = y * width + x;
            ClothParticle& p = m_particles[idx];
            p.position     = { static_cast<float>(x) * spacing, 0.0f, static_cast<float>(y) * spacing };
            p.prevPosition = p.position;
            p.acceleration = { 0.0f, 0.0f, 0.0f };
            p.inverseMass  = 1.0f;
        }
    }

    // Helper lambda to compute rest length between two particles
    auto addConstraint = [&](uint32_t a, uint32_t b, ClothConstraintType type, float stiffness)
    {
        ClothConstraint c;
        c.type       = type;
        c.particleA  = a;
        c.particleB  = b;
        c.restLength = Distance(m_particles[a].position, m_particles[b].position);
        c.stiffness  = stiffness;
        m_constraints.push_back(c);
    };

    // Stretch constraints (horizontal + vertical neighbors)
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t idx = y * width + x;
            // Horizontal neighbor
            if (x + 1 < width)
                addConstraint(idx, idx + 1, ClothConstraintType::Stretch, 1.0f);
            // Vertical neighbor
            if (y + 1 < height)
                addConstraint(idx, idx + width, ClothConstraintType::Stretch, 1.0f);
        }
    }

    // Shear constraints (diagonal neighbors)
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t idx = y * width + x;
            // Diagonal (\)
            if (x + 1 < width && y + 1 < height)
                addConstraint(idx, (y + 1) * width + (x + 1), ClothConstraintType::Shear, 1.0f);
            // Diagonal (/)
            if (x > 0 && y + 1 < height)
                addConstraint(idx, (y + 1) * width + (x - 1), ClothConstraintType::Shear, 1.0f);
        }
    }

    // Bend constraints (skip-one horizontal + vertical)
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t idx = y * width + x;
            // Horizontal skip-one
            if (x + 2 < width)
                addConstraint(idx, idx + 2, ClothConstraintType::Bend, 0.5f);
            // Vertical skip-one
            if (y + 2 < height)
                addConstraint(idx, (y + 2) * width + x, ClothConstraintType::Bend, 0.5f);
        }
    }
}

// -----------------------------------------------------------------------
// Pin / Unpin
// -----------------------------------------------------------------------

void ClothSimulation::PinParticle(uint32_t index)
{
    assert(index < m_particles.size());
    m_particles[index].inverseMass = 0.0f;
}

void ClothSimulation::UnpinParticle(uint32_t index, float mass)
{
    assert(index < m_particles.size());
    m_particles[index].inverseMass = (mass > 0.0f) ? (1.0f / mass) : 1.0f;
}

bool ClothSimulation::IsParticlePinned(uint32_t index) const
{
    assert(index < m_particles.size());
    return m_particles[index].inverseMass <= 0.0f;
}

// -----------------------------------------------------------------------
// Colliders
// -----------------------------------------------------------------------

int ClothSimulation::AddCollider(const ClothCollider& collider)
{
    m_colliders.push_back(collider);
    return m_nextColliderId++;
}

void ClothSimulation::RemoveCollider(int id)
{
    if (id >= 0 && id < static_cast<int>(m_colliders.size()))
    {
        m_colliders.erase(m_colliders.begin() + id);
    }
}

// -----------------------------------------------------------------------
// Update (main simulation step)
// -----------------------------------------------------------------------

void ClothSimulation::Update(float deltaTime)
{
    if (!m_initialized || m_particles.empty()) return;

    if (m_cpuMode)
    {
        // CPU simulation
        float dt = (deltaTime > 0.0f) ? deltaTime : m_settings.timeStep;

        // Apply wind to acceleration
        ApplyWind();

        // Verlet integration
        VerletIntegrate(dt);

        // Solve constraints (multiple iterations)
        for (uint32_t iter = 0; iter < m_settings.solverIterations; ++iter)
        {
            SolveConstraints();
        }

        // Handle collisions
        HandleCollisions();
    }
    else
    {
        // GPU dispatch path (requires command list)
        // TODO: Record compute shader dispatches
        // For now, fall back to CPU simulation
        float dt = (deltaTime > 0.0f) ? deltaTime : m_settings.timeStep;
        ApplyWind();
        VerletIntegrate(dt);
        for (uint32_t iter = 0; iter < m_settings.solverIterations; ++iter)
            SolveConstraints();
        HandleCollisions();
    }
}

// -----------------------------------------------------------------------
// CPU simulation methods
// -----------------------------------------------------------------------

void ClothSimulation::ApplyWind()
{
    for (auto& p : m_particles)
    {
        if (p.inverseMass > 0.0f)
        {
            p.acceleration = Add(m_settings.gravity, m_settings.windForce);
        }
    }
}

void ClothSimulation::VerletIntegrate(float dt)
{
    for (auto& p : m_particles)
    {
        if (p.inverseMass <= 0.0f) continue; // Pinned — do not move

        // velocity = (position - prevPosition) * damping
        Vector3 velocity = Mul(Sub(p.position, p.prevPosition), m_settings.damping);

        // prevPosition = position
        p.prevPosition = p.position;

        // position += velocity + acceleration * dt^2
        float dt2 = dt * dt;
        p.position = Add(p.position, Add(velocity, Mul(p.acceleration, dt2)));
    }
}

void ClothSimulation::SolveConstraints()
{
    for (auto& c : m_constraints)
    {
        ClothParticle& pA = m_particles[c.particleA];
        ClothParticle& pB = m_particles[c.particleB];

        Vector3 delta = Sub(pB.position, pA.position);
        float currentLength = Length(delta);

        if (currentLength < 1e-8f) continue; // Degenerate — skip

        float diff = (currentLength - c.restLength) / currentLength;
        Vector3 correction = Mul(delta, diff * c.stiffness);

        float totalInvMass = pA.inverseMass + pB.inverseMass;
        if (totalInvMass <= 0.0f) continue; // Both pinned

        if (pA.inverseMass > 0.0f)
        {
            float weightA = pA.inverseMass / totalInvMass;
            pA.position = Add(pA.position, Mul(correction, weightA));
        }

        if (pB.inverseMass > 0.0f)
        {
            float weightB = pB.inverseMass / totalInvMass;
            pB.position = Sub(pB.position, Mul(correction, weightB));
        }
    }
}

void ClothSimulation::HandleCollisions()
{
    for (auto& p : m_particles)
    {
        if (p.inverseMass <= 0.0f) continue; // Pinned — skip

        for (const auto& collider : m_colliders)
        {
            if (collider.shape == ClothColliderShape::Sphere)
            {
                Vector3 delta = Sub(p.position, collider.position);
                float dist = Length(delta);

                if (dist < collider.radius && dist > 1e-8f)
                {
                    // Push particle to sphere surface
                    Vector3 normal = Normalize(delta);
                    p.position = Add(collider.position, Mul(normal, collider.radius));
                }
            }
            else if (collider.shape == ClothColliderShape::Capsule)
            {
                // Project particle onto capsule axis segment
                Vector3 ab = Sub(collider.capsuleEnd, collider.position);
                Vector3 ap = Sub(p.position, collider.position);
                float abLenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;

                float t = 0.0f;
                if (abLenSq > 1e-8f)
                {
                    t = (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / abLenSq;
                    t = std::clamp(t, 0.0f, 1.0f);
                }

                Vector3 closestOnAxis = Add(collider.position, Mul(ab, t));
                Vector3 delta = Sub(p.position, closestOnAxis);
                float dist = Length(delta);

                float capsuleR = collider.capsuleRadius > 0.0f ? collider.capsuleRadius : collider.radius;
                if (dist < capsuleR && dist > 1e-8f)
                {
                    Vector3 normal = Normalize(delta);
                    p.position = Add(closestOnAxis, Mul(normal, capsuleR));
                }
            }
        }
    }
}

} // namespace gx
