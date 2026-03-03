/// @file ClothUpdate.hlsl
/// @brief Cloth simulation compute shader -- Verlet integration + PBD constraints
/// Reference: Mueller et al., "Position Based Dynamics" (2007)

struct ClothParticle
{
    float3 position;
    float  inverseMass;
    float3 prevPosition;
    float  _pad0;
    float3 acceleration;
    float  _pad1;
};

struct ClothConstraint
{
    uint   particleA;
    uint   particleB;
    float  restLength;
    float  stiffness;
};

struct ClothCollider
{
    uint   shape;        // 0=Sphere, 1=Capsule
    float3 position;
    float  radius;
    float3 capsuleEnd;
    float  capsuleRadius;
};

RWStructuredBuffer<ClothParticle>  gParticles   : register(u0);
StructuredBuffer<ClothConstraint>  gConstraints : register(t0);
StructuredBuffer<ClothCollider>    gColliders   : register(t1);

cbuffer ClothCB : register(b0)
{
    float3 gGravity;
    float  gDamping;
    float3 gWind;
    float  gDeltaTime;
    uint   gParticleCount;
    uint   gConstraintCount;
    uint   gSolverIterations;
    uint   gColliderCount;
};

// -----------------------------------------------------------------------
// Pass 1: Verlet Integration
// -----------------------------------------------------------------------
[numthreads(256, 1, 1)]
void CSVerlet(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= gParticleCount) return;

    ClothParticle p = gParticles[idx];
    if (p.inverseMass <= 0.0) return; // Pinned particle

    // Compute velocity from position difference
    float3 velocity = (p.position - p.prevPosition) * gDamping;

    // Store current position as previous
    p.prevPosition = p.position;

    // Update acceleration
    p.acceleration = gGravity + gWind;

    // Verlet integration: x(t+dt) = x(t) + v*dt + a*dt^2
    float dt2 = gDeltaTime * gDeltaTime;
    p.position += velocity + p.acceleration * dt2;

    gParticles[idx] = p;
}

// -----------------------------------------------------------------------
// Pass 2: PBD Distance Constraint Solving
// -----------------------------------------------------------------------
[numthreads(256, 1, 1)]
void CSConstraint(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= gConstraintCount) return;

    ClothConstraint c = gConstraints[idx];
    ClothParticle pA = gParticles[c.particleA];
    ClothParticle pB = gParticles[c.particleB];

    float3 delta = pB.position - pA.position;
    float currentLength = length(delta);
    if (currentLength < 0.0001) return; // Degenerate

    float correction = (currentLength - c.restLength) / currentLength * c.stiffness;
    float3 correctionVec = delta * correction;

    float totalInvMass = pA.inverseMass + pB.inverseMass;
    if (totalInvMass <= 0.0) return; // Both pinned

    if (pA.inverseMass > 0.0)
        gParticles[c.particleA].position += correctionVec * (pA.inverseMass / totalInvMass);
    if (pB.inverseMass > 0.0)
        gParticles[c.particleB].position -= correctionVec * (pB.inverseMass / totalInvMass);
}

// -----------------------------------------------------------------------
// Pass 3: Collision Handling
// -----------------------------------------------------------------------
[numthreads(256, 1, 1)]
void CSCollision(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.x;
    if (idx >= gParticleCount) return;

    ClothParticle p = gParticles[idx];
    if (p.inverseMass <= 0.0) return; // Pinned

    for (uint i = 0; i < gColliderCount; ++i)
    {
        ClothCollider col = gColliders[i];

        if (col.shape == 0) // Sphere
        {
            float3 delta = p.position - col.position;
            float dist = length(delta);
            if (dist < col.radius && dist > 0.0001)
            {
                p.position = col.position + normalize(delta) * col.radius;
            }
        }
        else // Capsule
        {
            float3 ab = col.capsuleEnd - col.position;
            float3 ap = p.position - col.position;
            float abLenSq = dot(ab, ab);
            float t = 0.0;
            if (abLenSq > 0.0001)
            {
                t = saturate(dot(ap, ab) / abLenSq);
            }
            float3 closest = col.position + ab * t;
            float3 delta = p.position - closest;
            float dist = length(delta);
            float capsuleR = col.capsuleRadius > 0.0 ? col.capsuleRadius : col.radius;
            if (dist < capsuleR && dist > 0.0001)
            {
                p.position = closest + normalize(delta) * capsuleR;
            }
        }
    }

    gParticles[idx] = p;
}
