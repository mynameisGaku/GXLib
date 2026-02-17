#include "pch.h"
#include "Physics/PhysicsWorld2D.h"

namespace GX {

PhysicsWorld2D::PhysicsWorld2D() = default;
PhysicsWorld2D::~PhysicsWorld2D() = default;

RigidBody2D* PhysicsWorld2D::AddBody()
{
    m_bodies.push_back(std::make_unique<RigidBody2D>());
    return m_bodies.back().get();
}

void PhysicsWorld2D::RemoveBody(RigidBody2D* body)
{
    auto it = std::remove_if(m_bodies.begin(), m_bodies.end(),
        [body](const std::unique_ptr<RigidBody2D>& b) { return b.get() == body; });
    m_bodies.erase(it, m_bodies.end());
}

void PhysicsWorld2D::Step(float deltaTime, int velocityIterations, int /*positionIterations*/)
{
    // 力の積分
    IntegrateBodies(deltaTime);

    // ブロードフェーズ
    std::vector<std::pair<RigidBody2D*, RigidBody2D*>> pairs;
    BroadPhase(pairs);

    // ナローフェーズ + 衝突解決
    for (int iter = 0; iter < velocityIterations; ++iter)
    {
        for (auto& [a, b] : pairs)
        {
            ContactInfo2D contact;
            if (NarrowPhase(a, b, contact))
            {
                if (a->isTrigger || b->isTrigger)
                {
                    if (onTriggerEnter) onTriggerEnter(a, b);
                }
                else
                {
                    ResolveCollision(contact);
                    if (onCollision) onCollision(contact);
                }
            }
        }
    }
}

void PhysicsWorld2D::IntegrateBodies(float dt)
{
    for (auto& body : m_bodies)
    {
        if (body->bodyType != BodyType2D::Dynamic) continue;

        // 重力を適用
        body->velocity += m_gravity * dt;

        // たまった力を反映
        body->velocity += body->m_forceAccum * (body->InverseMass() * dt);
        if (!body->fixedRotation)
            body->angularVelocity += body->m_torqueAccum * (body->InverseInertia() * dt);

        // 減衰（速度を少しずつ弱める）
        body->velocity *= (1.0f / (1.0f + body->linearDamping * dt));
        body->angularVelocity *= (1.0f / (1.0f + body->angularDamping * dt));

        // 位置の積分
        body->position += body->velocity * dt;
        body->rotation += body->angularVelocity * dt;

        // 力の蓄積をクリア
        body->m_forceAccum = Vector2::Zero();
        body->m_torqueAccum = 0.0f;
    }
}

AABB2D PhysicsWorld2D::GetBodyAABB(const RigidBody2D& body) const
{
    if (body.shape.type == ShapeType2D::Circle)
    {
        return {
            { body.position.x - body.shape.radius, body.position.y - body.shape.radius },
            { body.position.x + body.shape.radius, body.position.y + body.shape.radius }
        };
    }
    else
    {
        // 4隅をbody.rotationで回転してからAABBを算出
        float c = std::cos(body.rotation);
        float s = std::sin(body.rotation);
        float hx = body.shape.halfExtents.x;
        float hy = body.shape.halfExtents.y;

        // ローカル4隅: (+hx,+hy), (-hx,+hy), (-hx,-hy), (+hx,-hy)
        float cx0 =  hx * c - hy * s;
        float cy0 =  hx * s + hy * c;
        float cx1 = -hx * c - hy * s;
        float cy1 = -hx * s + hy * c;

        float maxX = (std::max)(std::abs(cx0), std::abs(cx1));
        float maxY = (std::max)(std::abs(cy0), std::abs(cy1));

        return {
            { body.position.x - maxX, body.position.y - maxY },
            { body.position.x + maxX, body.position.y + maxY }
        };
    }
}

Circle PhysicsWorld2D::GetBodyCircle(const RigidBody2D& body) const
{
    return { body.position, body.shape.radius };
}

void PhysicsWorld2D::BroadPhase(std::vector<std::pair<RigidBody2D*, RigidBody2D*>>& pairs)
{
    // 単純なO(n^2)ブロードフェーズ（小〜中規模向け）
    for (size_t i = 0; i < m_bodies.size(); ++i)
    {
        for (size_t j = i + 1; j < m_bodies.size(); ++j)
        {
            RigidBody2D* a = m_bodies[i].get();
            RigidBody2D* b = m_bodies[j].get();

            // 静的同士はスキップ
            if (a->bodyType == BodyType2D::Static && b->bodyType == BodyType2D::Static) continue;

            // レイヤーチェック
            if ((a->layer & b->layer) == 0) continue;

            AABB2D aabbA = GetBodyAABB(*a);
            AABB2D aabbB = GetBodyAABB(*b);

            if (Collision2D::TestAABBvsAABB(aabbA, aabbB))
                pairs.push_back({ a, b });
        }
    }
}

bool PhysicsWorld2D::NarrowPhase(RigidBody2D* a, RigidBody2D* b, ContactInfo2D& contact)
{
    HitResult2D hit;

    if (a->shape.type == ShapeType2D::Circle && b->shape.type == ShapeType2D::Circle)
    {
        hit = Collision2D::IntersectCirclevsCircle(GetBodyCircle(*a), GetBodyCircle(*b));
    }
    else if (a->shape.type == ShapeType2D::AABB && b->shape.type == ShapeType2D::AABB)
    {
        hit = Collision2D::IntersectAABBvsAABB(GetBodyAABB(*a), GetBodyAABB(*b));
    }
    else if (a->shape.type == ShapeType2D::AABB && b->shape.type == ShapeType2D::Circle)
    {
        hit = Collision2D::IntersectAABBvsCircle(GetBodyAABB(*a), GetBodyCircle(*b));
    }
    else // 円 vs AABB
    {
        hit = Collision2D::IntersectAABBvsCircle(GetBodyAABB(*b), GetBodyCircle(*a));
        hit.normal = -hit.normal;
    }

    if (!hit) return false;

    contact.bodyA = a;
    contact.bodyB = b;
    contact.point = hit.point;
    contact.normal = hit.normal;
    contact.depth = hit.depth;
    return true;
}

void PhysicsWorld2D::ResolveCollision(const ContactInfo2D& contact)
{
    RigidBody2D* a = contact.bodyA;
    RigidBody2D* b = contact.bodyB;

    float invMassA = a->InverseMass();
    float invMassB = b->InverseMass();
    float invMassSum = invMassA + invMassB;

    if (invMassSum <= 0.0f) return;

    // 位置補正（めり込み防止）
    const float percent = 0.8f;
    const float slop = 0.01f;
    float correctionMag = (std::max)(contact.depth - slop, 0.0f) * percent / invMassSum;
    Vector2 correction = contact.normal * correctionMag;

    a->position -= correction * invMassA;
    b->position += correction * invMassB;

    // 相対速度
    Vector2 relVel = b->velocity - a->velocity;
    float velAlongNormal = relVel.Dot(contact.normal);

    // 離れている場合は解決しない
    if (velAlongNormal > 0.0f) return;

    // 反発係数（小さい方を採用）
    // 初学者向け: 0ならほぼ跳ね返らず、1なら強く跳ね返ります。
    float e = (std::min)(a->restitution, b->restitution);

    // 衝撃量（インパルス）の大きさ
    float j = -(1.0f + e) * velAlongNormal / invMassSum;

    // インパルス適用
    Vector2 impulse = contact.normal * j;
    a->velocity -= impulse * invMassA;
    b->velocity += impulse * invMassB;

    // 角度インパルス適用
    if (!a->fixedRotation)
    {
        Vector2 rA = contact.point - a->position;
        a->angularVelocity -= rA.Cross(impulse) * a->InverseInertia();
    }
    if (!b->fixedRotation)
    {
        Vector2 rB = contact.point - b->position;
        b->angularVelocity += rB.Cross(impulse) * b->InverseInertia();
    }

    // 摩擦（接線方向の抵抗）
    // TECH-03: インパルス適用後の速度で相対速度を再計算
    relVel = b->velocity - a->velocity;
    velAlongNormal = relVel.Dot(contact.normal);
    Vector2 tangent = relVel - contact.normal * velAlongNormal;
    float tangentLen = tangent.Length();
    if (tangentLen > MathUtil::EPSILON)
    {
        tangent = tangent * (1.0f / tangentLen);
        float jt = -relVel.Dot(tangent) / invMassSum;

        float mu = std::sqrt(a->friction * b->friction);
        Vector2 frictionImpulse;
        if (std::abs(jt) < j * mu)
            frictionImpulse = tangent * jt;
        else
            frictionImpulse = tangent * (-j * mu);

        a->velocity -= frictionImpulse * invMassA;
        b->velocity += frictionImpulse * invMassB;
    }
}

bool PhysicsWorld2D::Raycast(const Vector2& origin, const Vector2& direction, float maxDistance,
                              RigidBody2D** outBody, Vector2* outPoint, Vector2* outNormal)
{
    float closestT = maxDistance;
    RigidBody2D* closestBody = nullptr;
    Vector2 closestNormal;

    for (auto& body : m_bodies)
    {
        float t;
        if (body->shape.type == ShapeType2D::Circle)
        {
            if (Collision2D::Raycast2D(origin, direction, GetBodyCircle(*body), t))
            {
                if (t < closestT)
                {
                    closestT = t;
                    closestBody = body.get();
                    Vector2 hitPt = origin + direction * t;
                    closestNormal = (hitPt - body->position).Normalized();
                }
            }
        }
        else
        {
            Vector2 normal;
            if (Collision2D::Raycast2D(origin, direction, GetBodyAABB(*body), t, &normal))
            {
                if (t < closestT)
                {
                    closestT = t;
                    closestBody = body.get();
                    closestNormal = normal;
                }
            }
        }
    }

    if (closestBody)
    {
        if (outBody) *outBody = closestBody;
        if (outPoint) *outPoint = origin + direction * closestT;
        if (outNormal) *outNormal = closestNormal;
        return true;
    }
    return false;
}

void PhysicsWorld2D::QueryAABB(const AABB2D& area, std::vector<RigidBody2D*>& results)
{
    for (auto& body : m_bodies)
    {
        if (Collision2D::TestAABBvsAABB(GetBodyAABB(*body), area))
            results.push_back(body.get());
    }
}

} // namespace GX
