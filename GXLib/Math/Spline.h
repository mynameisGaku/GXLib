#pragma once
#include "pch.h"
#include "Math/Vector3.h"

namespace GX {

/// Spline interpolation type
enum class SplineType { Linear, CatmullRom, CubicBezier };

/// A spline curve defined by control points
class Spline
{
public:
    Spline() = default;
    ~Spline() = default;

    /// Add a control point
    void AddPoint(const Vector3& point);

    /// Insert a control point at index
    void InsertPoint(int index, const Vector3& point);

    /// Remove a control point by index
    void RemovePoint(int index);

    /// Set a control point at index
    void SetPoint(int index, const Vector3& point);

    /// Get a control point
    const Vector3& GetPoint(int index) const;

    /// Number of control points
    int GetPointCount() const;

    /// Clear all points
    void Clear();

    /// Set interpolation type
    void SetType(SplineType type);
    SplineType GetType() const;

    /// Set whether the spline is closed (loops)
    void SetClosed(bool closed);
    bool IsClosed() const;

    /// Evaluate position at parameter t (0.0 to 1.0 over entire spline)
    Vector3 Evaluate(float t) const;

    /// Evaluate tangent (direction) at parameter t
    Vector3 EvaluateTangent(float t) const;

    /// Get approximate total length of the spline
    float GetTotalLength(int subdivisions = 64) const;

    /// Evaluate by arc-length distance from start
    Vector3 EvaluateByDistance(float distance, int subdivisions = 64) const;

    /// Get the closest point on the spline to a given point
    float FindClosestParameter(const Vector3& point, int subdivisions = 64) const;

private:
    std::vector<Vector3> m_points;
    SplineType m_type = SplineType::CatmullRom;
    bool m_closed = false;

    /// Linear interpolation between two points
    static Vector3 LerpPoints(const Vector3& a, const Vector3& b, float t);

    /// CatmullRom interpolation
    static Vector3 CatmullRom(const Vector3& p0, const Vector3& p1,
                                const Vector3& p2, const Vector3& p3, float t);

    /// Cubic Bezier interpolation (4 points per segment)
    static Vector3 CubicBezier(const Vector3& p0, const Vector3& p1,
                                 const Vector3& p2, const Vector3& p3, float t);

    /// Get segment index and local t from global t
    void GetSegment(float t, int& segIndex, float& localT) const;

    /// Get the 4 control points for a CatmullRom segment
    void GetCatmullRomPoints(int segIndex, Vector3& p0, Vector3& p1,
                              Vector3& p2, Vector3& p3) const;
};

} // namespace GX
