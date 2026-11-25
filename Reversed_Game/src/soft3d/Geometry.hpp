#pragma once

#include "soft3d/Fixed.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace soft3d {

struct ObjectBounds {
    Vec3 minimum;
    Vec3 maximum;
};

inline ObjectBounds Translate(const ObjectBounds& bounds, const Vec3& offset)
{
    return {Add(bounds.minimum, offset), Add(bounds.maximum, offset)};
}

inline bool Contains(const ObjectBounds& bounds, const Vec3& point)
{
    return point.x >= bounds.minimum.x && point.y >= bounds.minimum.y &&
           point.z >= bounds.minimum.z && point.x <= bounds.maximum.x &&
           point.y <= bounds.maximum.y && point.z <= bounds.maximum.z;
}

inline bool Overlaps(const ObjectBounds& first, const ObjectBounds& second)
{
    return second.maximum.x >= first.minimum.x && second.maximum.y >= first.minimum.y &&
           second.maximum.z >= first.minimum.z && second.minimum.x <= first.maximum.x &&
           second.minimum.y <= first.maximum.y && second.minimum.z <= first.maximum.z;
}

inline bool SegmentHitsBounds(const Vec3& start, const Vec3& end, const ObjectBounds& bounds)
{
    const Vec3 center = Scale(Add(bounds.minimum, bounds.maximum), 0x8000);
    const Vec3 extents = Subtract(bounds.maximum, center);
    const Vec3 direction = Scale(Subtract(end, start), 0x8000);
    const Vec3 midpoint = Subtract(Add(start, direction), center);
    const Vec3 magnitude{AbsFixed(direction.x), AbsFixed(direction.y), AbsFixed(direction.z)};
    if (AbsFixed(midpoint.x) > AddFixed(extents.x, magnitude.x) ||
        AbsFixed(midpoint.y) > AddFixed(extents.y, magnitude.y) ||
        AbsFixed(midpoint.z) > AddFixed(extents.z, magnitude.z)) {
        return false;
    }
    const Vec3 cross = Cross(direction, midpoint);
    if (AbsFixed(cross.x) > AddFixed(FixedMultiply(extents.y, magnitude.z),
                                    FixedMultiply(extents.z, magnitude.y)) ||
        AbsFixed(cross.y) > AddFixed(FixedMultiply(extents.x, magnitude.z),
                                    FixedMultiply(extents.z, magnitude.x)) ||
        AbsFixed(cross.z) > AddFixed(FixedMultiply(extents.x, magnitude.y),
                                    FixedMultiply(extents.y, magnitude.x))) {
        return false;
    }
    return true;
}

inline void ResetBounds(ObjectBounds& bounds)
{
    bounds.minimum = {INT32_MAX, INT32_MAX, INT32_MAX};
    bounds.maximum = {INT32_MIN, INT32_MIN, INT32_MIN};
}

inline void Expand(ObjectBounds& bounds, const Vec3& point)
{
    bounds.minimum.x = std::min(bounds.minimum.x, point.x);
    bounds.minimum.y = std::min(bounds.minimum.y, point.y);
    bounds.minimum.z = std::min(bounds.minimum.z, point.z);
    bounds.maximum.x = std::max(bounds.maximum.x, point.x);
    bounds.maximum.y = std::max(bounds.maximum.y, point.y);
    bounds.maximum.z = std::max(bounds.maximum.z, point.z);
}

inline ObjectBounds Bounds(const Vec3& minimum, const Vec3& maximum)
{
    return {Vec3{std::min(minimum.x, maximum.x), std::min(minimum.y, maximum.y),
                 std::min(minimum.z, maximum.z)},
            Vec3{std::max(minimum.x, maximum.x), std::max(minimum.y, maximum.y),
                 std::max(minimum.z, maximum.z)}};
}

inline Vec3 Center(const ObjectBounds& bounds)
{
    return ShiftRight(Add(bounds.minimum, bounds.maximum), 1);
}

struct Plane {
    Vec3 normal;
    Fixed distance = 0;
};

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

Fixed PlaneDistance(const Plane& plane, const Vec3& point);
Plane PlaneFromPointNormal(const Vec3& point, const Vec3& normal);
Ray NormalizeRay(const Ray& ray);
Ray TransformRay(const Matrix4& matrix, const Ray& ray);
Fixed DotDirection(const Ray& ray, const Vec3& value);
Fixed DistanceToRay(const Vec3& point, const Ray& ray, Vec3* closestPoint = nullptr);
Fixed DistanceToLine(const Vec3& point, const Ray& ray, Vec3* closestPoint = nullptr);
Fixed PlaneDistanceAndProject(const Vec3& point, const Plane& plane, Vec3* projected = nullptr);
Fixed PlaneIntersectRay(const Ray& ray, const Plane& plane, Vec3* intersection = nullptr);
Fixed SegmentIntersectionDistance(const Ray& first, const Ray& second,
                                  Vec3* intersection = nullptr);
Vec3 Slide(const Vec3& value, const Vec3& normal);
bool OutsideFrustum(const std::array<Plane, 5>& planes, const Vec4& point);

enum class PlaneClassification : int32_t {
    Positive = 1,
    Negative = 2,
    Spanning = 3,
    OnPlane = 4,
};

enum class PointCloudValidation : int32_t {
    Valid = 0,
    TooFewPoints = 1,
    SmallArea = 2,
    CoordinateTooLarge = 3,
    ShortEdge = 4,
    NonPlanar = 5,
    Concave = 6,
};

class PointCloud {
public:
    PointCloud() = default;
    explicit PointCloud(const std::vector<Vec3>& inputPoints);

    static PointCloud FromPlane(const Plane& plane);

    void Allocate(std::size_t count);
    PointCloud Reversed() const;
    Plane GetPlane() const;
    Fixed Area() const;
    ObjectBounds Bounds() const;
    Vec3 Center() const;
    void Optimize();
    PlaneClassification Classify(const Plane& plane, Fixed epsilon) const;
    void ClipPositiveInPlace(const Plane& plane, Fixed epsilon);
    PointCloud ClippedPositive(const Plane& plane, Fixed epsilon) const;
    PointCloudValidation Validate() const;

    std::size_t GetPointCount() const;
    const std::vector<Vec3>& GetPoints() const;
    std::vector<Vec3>& GetPoints();

private:
    std::vector<Vec3> points;
};

} // namespace soft3d
