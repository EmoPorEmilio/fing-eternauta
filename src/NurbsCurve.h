#pragma once
#include <glm/glm.hpp>
#include <vector>

// Simple cubic B-spline curve (degree 3) for smooth camera paths
// Uses uniform knot vector for simplicity
class NurbsCurve {
public:
    NurbsCurve() = default;

    // Set control points for the curve
    void setControlPoints(const std::vector<glm::vec3>& points) {
        m_controlPoints = points;
    }

    // Add a single control point
    void addControlPoint(const glm::vec3& point) {
        m_controlPoints.push_back(point);
    }

    // Clear all control points
    void clear() {
        m_controlPoints.clear();
    }

    // Evaluate the curve at parameter t (0 to 1)
    // Returns interpolated position along the curve
    glm::vec3 evaluate(float t) const {
        if (m_controlPoints.size() < 2) {
            return m_controlPoints.empty() ? glm::vec3(0.0f) : m_controlPoints[0];
        }

        // Clamp t to valid range
        t = glm::clamp(t, 0.0f, 1.0f);

        // For 2-3 points, use simple linear/quadratic interpolation
        if (m_controlPoints.size() == 2) {
            return glm::mix(m_controlPoints[0], m_controlPoints[1], t);
        }

        if (m_controlPoints.size() == 3) {
            // Quadratic Bezier
            float u = 1.0f - t;
            return u * u * m_controlPoints[0] +
                   2.0f * u * t * m_controlPoints[1] +
                   t * t * m_controlPoints[2];
        }

        // For 4+ points, use Catmull-Rom spline (passes through all points)
        // This is more intuitive for camera paths than B-spline
        return evaluateCatmullRom(t);
    }

    // Get the tangent (direction) at parameter t
    glm::vec3 tangent(float t) const {
        const float delta = 0.001f;
        float t0 = glm::max(0.0f, t - delta);
        float t1 = glm::min(1.0f, t + delta);
        return glm::normalize(evaluate(t1) - evaluate(t0));
    }

    size_t numControlPoints() const { return m_controlPoints.size(); }

    const std::vector<glm::vec3>& controlPoints() const { return m_controlPoints; }

private:
    std::vector<glm::vec3> m_controlPoints;

    // Catmull-Rom spline evaluation - passes through all control points
    glm::vec3 evaluateCatmullRom(float t) const {
        size_t n = m_controlPoints.size();
        if (n < 2) return m_controlPoints.empty() ? glm::vec3(0.0f) : m_controlPoints[0];

        // Ensure exact endpoints
        if (t <= 0.0f) return m_controlPoints[0];
        if (t >= 1.0f) return m_controlPoints[n - 1];

        // Scale t to the number of segments
        float scaledT = t * (n - 1);
        size_t segment = static_cast<size_t>(scaledT);
        float localT = scaledT - segment;

        // Clamp segment to valid range
        if (segment >= n - 1) {
            segment = n - 2;
            localT = 1.0f;
        }

        // Get 4 control points for this segment (with clamping at boundaries)
        glm::vec3 p0 = m_controlPoints[segment > 0 ? segment - 1 : 0];
        glm::vec3 p1 = m_controlPoints[segment];
        glm::vec3 p2 = m_controlPoints[segment + 1];
        glm::vec3 p3 = m_controlPoints[segment + 2 < n ? segment + 2 : n - 1];

        // Catmull-Rom basis functions
        float t2 = localT * localT;
        float t3 = t2 * localT;

        // Catmull-Rom matrix coefficients (tension = 0.5)
        glm::vec3 result = 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * localT +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
        );

        return result;
    }
};
