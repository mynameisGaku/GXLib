#include "pch.h"
#include "Math/Spline.h"

namespace GX {

// ============================================================================
// Control Point Management
// ============================================================================

void Spline::AddPoint(const Vector3& point)
{
    m_points.push_back(point);
}

void Spline::InsertPoint(int index, const Vector3& point)
{
    if (index < 0) index = 0;
    if (index > static_cast<int>(m_points.size())) index = static_cast<int>(m_points.size());
    m_points.insert(m_points.begin() + index, point);
}

void Spline::RemovePoint(int index)
{
    if (index < 0 || index >= static_cast<int>(m_points.size())) return;
    m_points.erase(m_points.begin() + index);
}

void Spline::SetPoint(int index, const Vector3& point)
{
    if (index < 0 || index >= static_cast<int>(m_points.size())) return;
    m_points[index] = point;
}

const Vector3& Spline::GetPoint(int index) const
{
    return m_points[index];
}

int Spline::GetPointCount() const
{
    return static_cast<int>(m_points.size());
}

void Spline::Clear()
{
    m_points.clear();
}

// ============================================================================
// Type and Closed State
// ============================================================================

void Spline::SetType(SplineType type)
{
    m_type = type;
}

SplineType Spline::GetType() const
{
    return m_type;
}

void Spline::SetClosed(bool closed)
{
    m_closed = closed;
}

bool Spline::IsClosed() const
{
    return m_closed;
}

// ============================================================================
// Static Interpolation Functions
// ============================================================================

Vector3 Spline::LerpPoints(const Vector3& a, const Vector3& b, float t)
{
    return a + (b - a) * t;
}

Vector3 Spline::CatmullRom(const Vector3& p0, const Vector3& p1,
                             const Vector3& p2, const Vector3& p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    // 0.5 * ((2*p1) + (-p0+p2)*t + (2*p0-5*p1+4*p2-p3)*t^2 + (-p0+3*p1-3*p2+p3)*t^3)
    Vector3 result =
        (p1 * 2.0f +
         (p2 - p0) * t +
         (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
         (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;

    return result;
}

Vector3 Spline::CubicBezier(const Vector3& p0, const Vector3& p1,
                              const Vector3& p2, const Vector3& p3, float t)
{
    float u  = 1.0f - t;
    float u2 = u * u;
    float u3 = u2 * u;
    float t2 = t * t;
    float t3 = t2 * t;

    // (1-t)^3*p0 + 3*(1-t)^2*t*p1 + 3*(1-t)*t^2*p2 + t^3*p3
    return p0 * u3 + p1 * (3.0f * u2 * t) + p2 * (3.0f * u * t2) + p3 * t3;
}

// ============================================================================
// Segment Helpers
// ============================================================================

void Spline::GetSegment(float t, int& segIndex, float& localT) const
{
    int n = static_cast<int>(m_points.size());
    if (n < 2)
    {
        segIndex = 0;
        localT = 0.0f;
        return;
    }

    int segCount = 0;

    if (m_type == SplineType::CubicBezier)
    {
        // Every 4 points form a segment (3n+1 total points).
        // Number of segments = (pointCount - 1) / 3
        segCount = (std::max)(1, (n - 1) / 3);
    }
    else
    {
        // Linear and CatmullRom: segments = pointCount - 1, or pointCount if closed
        segCount = m_closed ? n : n - 1;
    }

    // Clamp t to [0, 1]
    t = (std::max)(0.0f, (std::min)(1.0f, t));

    float scaled = t * static_cast<float>(segCount);
    segIndex = static_cast<int>(std::floor(scaled));

    // Clamp segment index to valid range
    if (segIndex >= segCount)
    {
        segIndex = segCount - 1;
        localT = 1.0f;
    }
    else
    {
        localT = scaled - static_cast<float>(segIndex);
    }
}

void Spline::GetCatmullRomPoints(int segIndex, Vector3& p0, Vector3& p1,
                                   Vector3& p2, Vector3& p3) const
{
    int n = static_cast<int>(m_points.size());

    if (m_closed)
    {
        // Wrap indices using modulo for closed spline
        int i0 = ((segIndex - 1) % n + n) % n;
        int i1 = segIndex % n;
        int i2 = (segIndex + 1) % n;
        int i3 = (segIndex + 2) % n;

        p0 = m_points[i0];
        p1 = m_points[i1];
        p2 = m_points[i2];
        p3 = m_points[i3];
    }
    else
    {
        // For open spline, duplicate first/last points as phantom points
        int i1 = segIndex;
        int i2 = segIndex + 1;
        int i0 = i1 - 1;
        int i3 = i2 + 1;

        // Clamp phantom point indices
        if (i0 < 0) i0 = 0;
        if (i3 >= n) i3 = n - 1;

        p0 = m_points[i0];
        p1 = m_points[i1];
        p2 = m_points[i2];
        p3 = m_points[i3];
    }
}

// ============================================================================
// Evaluation
// ============================================================================

Vector3 Spline::Evaluate(float t) const
{
    int n = static_cast<int>(m_points.size());
    if (n == 0) return Vector3::Zero();
    if (n == 1) return m_points[0];

    int segIndex = 0;
    float localT = 0.0f;
    GetSegment(t, segIndex, localT);

    switch (m_type)
    {
    case SplineType::Linear:
    {
        int i0, i1;
        if (m_closed)
        {
            i0 = segIndex % n;
            i1 = (segIndex + 1) % n;
        }
        else
        {
            i0 = segIndex;
            i1 = (std::min)(segIndex + 1, n - 1);
        }
        return LerpPoints(m_points[i0], m_points[i1], localT);
    }

    case SplineType::CatmullRom:
    {
        Vector3 p0, p1, p2, p3;
        GetCatmullRomPoints(segIndex, p0, p1, p2, p3);
        return CatmullRom(p0, p1, p2, p3, localT);
    }

    case SplineType::CubicBezier:
    {
        // Each segment uses 4 consecutive points: [seg*3, seg*3+1, seg*3+2, seg*3+3]
        int base = segIndex * 3;
        int maxIdx = n - 1;
        int i0 = (std::min)(base, maxIdx);
        int i1 = (std::min)(base + 1, maxIdx);
        int i2 = (std::min)(base + 2, maxIdx);
        int i3 = (std::min)(base + 3, maxIdx);
        return CubicBezier(m_points[i0], m_points[i1], m_points[i2], m_points[i3], localT);
    }

    default:
        return m_points[0];
    }
}

Vector3 Spline::EvaluateTangent(float t) const
{
    int n = static_cast<int>(m_points.size());
    if (n < 2) return Vector3::Zero();

    const float epsilon = 0.001f;
    float t0 = (std::max)(0.0f, t - epsilon);
    float t1 = (std::min)(1.0f, t + epsilon);

    // Avoid zero-length delta when clamped at boundaries
    if (t1 - t0 < 1e-6f) return Vector3::Zero();

    Vector3 a = Evaluate(t0);
    Vector3 b = Evaluate(t1);
    Vector3 tangent = b - a;

    float len = tangent.Length();
    if (len < 1e-8f) return Vector3::Zero();

    return tangent / len;
}

// ============================================================================
// Arc-Length Utilities
// ============================================================================

float Spline::GetTotalLength(int subdivisions) const
{
    if (static_cast<int>(m_points.size()) < 2) return 0.0f;
    if (subdivisions < 1) subdivisions = 1;

    float totalLength = 0.0f;
    Vector3 prev = Evaluate(0.0f);

    for (int i = 1; i <= subdivisions; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(subdivisions);
        Vector3 curr = Evaluate(t);
        totalLength += (curr - prev).Length();
        prev = curr;
    }

    return totalLength;
}

Vector3 Spline::EvaluateByDistance(float distance, int subdivisions) const
{
    if (static_cast<int>(m_points.size()) < 2) return Evaluate(0.0f);
    if (subdivisions < 1) subdivisions = 1;

    if (distance <= 0.0f) return Evaluate(0.0f);

    float accumulated = 0.0f;
    Vector3 prev = Evaluate(0.0f);

    for (int i = 1; i <= subdivisions; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(subdivisions);
        Vector3 curr = Evaluate(t);
        float segLen = (curr - prev).Length();

        if (accumulated + segLen >= distance)
        {
            // Interpolate within this subdivision segment
            float remaining = distance - accumulated;
            float frac = (segLen > 1e-8f) ? (remaining / segLen) : 0.0f;
            return LerpPoints(prev, curr, frac);
        }

        accumulated += segLen;
        prev = curr;
    }

    // Distance exceeds total length; return endpoint
    return Evaluate(1.0f);
}

float Spline::FindClosestParameter(const Vector3& point, int subdivisions) const
{
    if (static_cast<int>(m_points.size()) < 2) return 0.0f;
    if (subdivisions < 1) subdivisions = 1;

    float bestT = 0.0f;
    float bestDistSq = (std::numeric_limits<float>::max)();

    for (int i = 0; i <= subdivisions; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(subdivisions);
        Vector3 pos = Evaluate(t);
        float distSq = (pos - point).LengthSquared();

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestT = t;
        }
    }

    return bestT;
}

} // namespace GX
