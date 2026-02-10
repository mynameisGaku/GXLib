#include "pch.h"
#include "Math/Random.h"

namespace GX {

Random::Random()
{
    std::random_device rd;
    m_engine.seed(rd());
}

Random::Random(uint32_t seed) : m_engine(seed)
{
}

void Random::SetSeed(uint32_t seed)
{
    m_engine.seed(seed);
}

int32_t Random::Int()
{
    return static_cast<int32_t>(m_engine());
}

int32_t Random::Int(int32_t min, int32_t max)
{
    std::uniform_int_distribution<int32_t> dist(min, max);
    return dist(m_engine);
}

float Random::Float()
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(m_engine);
}

float Random::Float(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_engine);
}

Vector2 Random::Vector2InRange(float minX, float maxX, float minY, float maxY)
{
    return { Float(minX, maxX), Float(minY, maxY) };
}

Vector3 Random::Vector3InRange(float minX, float maxX, float minY, float maxY, float minZ, float maxZ)
{
    return { Float(minX, maxX), Float(minY, maxY), Float(minZ, maxZ) };
}

Vector2 Random::PointInCircle(float radius)
{
    // 円内を一様に分布させる（リジェクションサンプリング）。
    // 四角からランダムに選んで、円の外ならやり直す。
    for (;;)
    {
        float x = Float(-1.0f, 1.0f);
        float y = Float(-1.0f, 1.0f);
        if (x * x + y * y <= 1.0f)
            return { x * radius, y * radius };
    }
}

Vector3 Random::PointInSphere(float radius)
{
    for (;;)
    {
        float x = Float(-1.0f, 1.0f);
        float y = Float(-1.0f, 1.0f);
        float z = Float(-1.0f, 1.0f);
        if (x * x + y * y + z * z <= 1.0f)
            return { x * radius, y * radius, z * radius };
    }
}

Vector2 Random::Direction2D()
{
    float angle = Float(0.0f, MathUtil::TAU);
    return { std::cos(angle), std::sin(angle) };
}

Vector3 Random::Direction3D()
{
    // 球面上の一様分布（Marsaglia法）。
    // 2Dの乱数から球面上の点に変換するテクニック。
    for (;;)
    {
        float x = Float(-1.0f, 1.0f);
        float y = Float(-1.0f, 1.0f);
        float s = x * x + y * y;
        if (s >= 1.0f) continue;
        float factor = 2.0f * std::sqrt(1.0f - s);
        return { x * factor, y * factor, 1.0f - 2.0f * s };
    }
}

Color Random::RandomColor(float alpha)
{
    return { Float(0.0f, 1.0f), Float(0.0f, 1.0f), Float(0.0f, 1.0f), alpha };
}

Random& Random::Global()
{
    static Random instance;
    return instance;
}

} // namespace GX
