#include "soft3d/Geometry.hpp"

#include <algorithm>
#include <utility>

namespace soft3d {
namespace {

PlaneClassification ClassifyPoint(Fixed distance, Fixed epsilon)
{
    if (distance > epsilon) {
        return PlaneClassification::Positive;
    }
    if (distance < NegateFixed(epsilon)) {
        return PlaneClassification::Negative;
    }
    return PlaneClassification::OnPlane;
}

Vec3 EdgeIntersection(const Vec3& current, const Vec3& next, Fixed currentDistance,
                      Fixed nextDistance, const Plane& plane)
{
    const Fixed fraction = FixedDivide(currentDistance,
                                       SubtractFixed(currentDistance, nextDistance));
    const auto interpolate = [&](Fixed first, Fixed last, Fixed normal) {
        if (normal == fixedOne) {
            return NegateFixed(plane.distance);
        }
        if (normal == NegateFixed(fixedOne)) {
            return plane.distance;
        }
        return AddFixed(first, FixedMultiply(fraction, SubtractFixed(last, first)));
    };
    return {interpolate(current.x, next.x, plane.normal.x),
            interpolate(current.y, next.y, plane.normal.y),
            interpolate(current.z, next.z, plane.normal.z)};
}

std::vector<Vec3> ClipPositive(const std::vector<Vec3>& points, const Plane& plane, Fixed epsilon)
{
    if (points.empty()) {
        return {};
    }
    std::vector<Fixed> distances(points.size());
    std::vector<PlaneClassification> classifications(points.size());
    for (std::size_t index = 0; index < points.size(); ++index) {
        distances[index] = PlaneDistance(plane, points[index]);
        classifications[index] = ClassifyPoint(distances[index], epsilon);
    }

    std::vector<Vec3> result;
    result.reserve(points.size() + 4);
    for (std::size_t index = 0; index < points.size(); ++index) {
        const std::size_t nextIndex = (index + 1) % points.size();
        const PlaneClassification currentClass = classifications[index];
        const PlaneClassification nextClass = classifications[nextIndex];
        if (currentClass == PlaneClassification::Positive ||
            currentClass == PlaneClassification::OnPlane) {
            result.push_back(points[index]);
        }
        if (nextClass != PlaneClassification::OnPlane && nextClass != currentClass) {
            result.push_back(EdgeIntersection(points[index], points[nextIndex], distances[index],
                                              distances[nextIndex], plane));
        }
    }
    return result;
}

} // namespace

Fixed PlaneDistance(const Plane& plane, const Vec3& point)
{
    return AddFixed(Dot(plane.normal, point), plane.distance);
}

Plane PlaneFromPointNormal(const Vec3& point, const Vec3& normal)
{
    return {normal, NegateFixed(Dot(point, normal))};
}

Ray NormalizeRay(const Ray& ray)
{
    return {ray.origin, Normalize(ray.direction)};
}

Ray TransformRay(const Matrix4& matrix, const Ray& ray)
{
    return {Transform(matrix, ray.origin), Transform(matrix, ray.direction)};
}

Fixed DotDirection(const Ray& ray, const Vec3& value)
{
    return Dot(ray.direction, value);
}

Fixed DistanceToRay(const Vec3& point, const Ray& ray, Vec3* closestPoint)
{
    const Vec3 delta = Subtract(point, ray.origin);
    const Fixed projection = DotDirection(ray, delta);
    const Vec3 perpendicular = Subtract(delta, Scale(ray.direction, projection));
    const Fixed distance = Length(perpendicular);
    if (closestPoint != nullptr) {
        *closestPoint = distance <= 0x28f ? point : Add(ray.origin, Scale(ray.direction, projection));
    }
    return distance;
}

Fixed DistanceToLine(const Vec3& point, const Ray& ray, Vec3* closestPoint)
{
    return FixedSqrt(DistanceToRay(point, ray, closestPoint));
}

Fixed PlaneDistanceAndProject(const Vec3& point, const Plane& plane, Vec3* projected)
{
    const Fixed distance = PlaneDistance(plane, point);
    if (projected != nullptr) {
        *projected = distance <= 0x28f ? point : Subtract(point, Scale(plane.normal, distance));
    }
    return distance;
}

Fixed PlaneIntersectRay(const Ray& ray, const Plane& plane, Vec3* intersection)
{
    const Fixed denominator = Dot(plane.normal, ray.direction);
    if (AbsFixed(denominator) < 0x28f) {
        return PlaneDistanceAndProject(ray.origin, plane);
    }
    if (intersection != nullptr) {
        const Fixed distance = PlaneDistance(plane, ray.origin);
        *intersection = Subtract(ray.origin, Scale(ray.direction, FixedDivide(distance, denominator)));
    }
    return 0;
}

Fixed SegmentIntersectionDistance(const Ray& first, const Ray& second, Vec3* intersection)
{
    const Fixed directionDot = Dot(first.direction, second.direction);
    if (SubtractFixed(fixedOne, AbsFixed(directionDot)) <= 0x28f) {
        return DistanceToLine(first.origin, second);
    }
    const Vec3 cross = Cross(first.direction, second.direction);
    const Plane plane = PlaneFromPointNormal(second.origin, cross);
    const Fixed distance = PlaneIntersectRay(first, plane, intersection);
    if (intersection != nullptr && distance <= 0x28f) {
        const Fixed scale = FixedDivide(fixedOne, SquaredLength(cross));
        *intersection = Add(first.origin, Scale(first.direction, scale));
    }
    return distance;
}

Vec3 Slide(const Vec3& value, const Vec3& normal)
{
    const Fixed projection = FixedMultiply(Dot(value, normal), 0x1028f);
    return Subtract(value, Scale(normal, projection));
}

bool OutsideFrustum(const std::array<Plane, 5>& planes, const Vec4& point)
{
    const Vec3 position{point.x, point.y, point.z};
    for (const Plane& plane : planes) {
        if (PlaneDistance(plane, position) < NegateFixed(point.w)) {
            return true;
        }
    }
    return false;
}

PointCloud::PointCloud(const std::vector<Vec3>& inputPoints)
    : points(inputPoints)
{
}

PointCloud PointCloud::FromPlane(const Plane& plane)
{
    const Fixed ax = AbsFixed(plane.normal.x);
    const Fixed ay = AbsFixed(plane.normal.y);
    const Fixed az = AbsFixed(plane.normal.z);
    unsigned dominant = 2;
    if (ax >= ay && ax >= az) {
        dominant = 0;
    } else if (ay >= ax && ay >= az) {
        dominant = 1;
    }
    const Vec3 seed = dominant == 1 ? Vec3{0, 0, fixedOne} : Vec3{0, fixedOne, 0};
    Vec3 tangent = Subtract(seed, Scale(plane.normal, Dot(seed, plane.normal)));
    Vec3 bitangent = Cross(plane.normal, tangent);
    const Vec3 center = Scale(plane.normal, plane.distance);
    tangent = Scale(tangent, 0x20000000);
    bitangent = Scale(bitangent, 0x20000000);
    return PointCloud({Add(Subtract(center, bitangent), tangent),
                       Add(Add(center, bitangent), tangent),
                       Subtract(Add(center, bitangent), tangent),
                       Subtract(Subtract(center, bitangent), tangent)});
}

void PointCloud::Allocate(std::size_t count)
{
    points.assign(count, Vec3{});
}

PointCloud PointCloud::Reversed() const
{
    return PointCloud(std::vector<Vec3>(points.rbegin(), points.rend()));
}

Plane PointCloud::GetPlane() const
{
    if (points.size() < 3) {
        return {};
    }
    const Vec3 normal = Normalize(Cross(Subtract(points[1], points[0]),
                                        Subtract(points[2], points[0])));
    return {normal, NegateFixed(Dot(points[0], normal))};
}

Fixed PointCloud::Area() const
{
    Fixed area = 0;
    for (std::size_t index = 2; index < points.size(); ++index) {
        const Vec3 first = Subtract(points[index - 1], points[0]);
        const Vec3 second = Subtract(points[index], points[0]);
        area = AddFixed(area, FixedMultiply(0x8000, Length(Cross(first, second))));
    }
    return area;
}

ObjectBounds PointCloud::Bounds() const
{
    ObjectBounds result;
    ResetBounds(result);
    for (const Vec3& point : points) {
        Expand(result, point);
    }
    return result;
}

Vec3 PointCloud::Center() const
{
    if (points.empty()) {
        return {};
    }
    Vec3 result;
    for (const Vec3& point : points) {
        result = Add(result, point);
    }
    return Scale(result, FixedDivide(fixedOne, ShiftLeftFixed(static_cast<Fixed>(points.size()), 16)));
}

void PointCloud::Optimize()
{
    if (points.empty()) {
        return;
    }
    std::vector<Vec3> optimized;
    optimized.reserve(points.size());
    for (std::size_t index = 0; index < points.size(); ++index) {
        const std::size_t previous = (points.size() + index - 1) % points.size();
        const std::size_t next = (index + 1) % points.size();
        const Vec3 incoming = Normalize(Subtract(points[index], points[previous]));
        const Vec3 outgoing = Normalize(Subtract(points[next], points[index]));
        if (Dot(incoming, outgoing) < 0xffbe) {
            optimized.push_back(points[index]);
        }
    }
    if (optimized.size() != points.size()) {
        points = std::move(optimized);
    }
}

PlaneClassification PointCloud::Classify(const Plane& plane, Fixed epsilon) const
{
    bool positive = false;
    bool negative = false;
    for (const Vec3& point : points) {
        const PlaneClassification current = ClassifyPoint(PlaneDistance(plane, point), epsilon);
        if (current == PlaneClassification::Positive) {
            if (negative) {
                return PlaneClassification::Spanning;
            }
            positive = true;
        } else if (current == PlaneClassification::Negative) {
            if (positive) {
                return PlaneClassification::Spanning;
            }
            negative = true;
        }
    }
    if (positive) {
        return PlaneClassification::Positive;
    }
    if (negative) {
        return PlaneClassification::Negative;
    }
    return PlaneClassification::OnPlane;
}

void PointCloud::ClipPositiveInPlace(const Plane& plane, Fixed epsilon)
{
    points = ClipPositive(points, plane, epsilon);
}

PointCloud PointCloud::ClippedPositive(const Plane& plane, Fixed epsilon) const
{
    return PointCloud(ClipPositive(points, plane, epsilon));
}

PointCloudValidation PointCloud::Validate() const
{
    if (points.size() < 3) {
        return PointCloudValidation::TooFewPoints;
    }
    if (Area() < fixedOne) {
        return PointCloudValidation::SmallArea;
    }
    const Plane plane = GetPlane();
    for (std::size_t index = 0; index < points.size(); ++index) {
        const Vec3& current = points[index];
        if (AbsFixed(current.x) > 0x20000000 || AbsFixed(current.y) > 0x20000000 ||
            AbsFixed(current.z) > 0x20000000) {
            return PointCloudValidation::CoordinateTooLarge;
        }
        if (AbsFixed(PlaneDistance(plane, current)) > 0x1999) {
            return PointCloudValidation::NonPlanar;
        }
        const std::size_t nextIndex = (index + 1) % points.size();
        const Vec3& next = points[nextIndex];
        const Vec3 edge = Subtract(next, current);
        if (Length(edge) < 0x28f) {
            return PointCloudValidation::ShortEdge;
        }
        const Vec3 edgeNormal = Normalize(Cross(plane.normal, edge));
        const Plane edgePlane{edgeNormal, NegateFixed(Dot(current, edgeNormal))};
        for (std::size_t other = 0; other < points.size(); ++other) {
            if (other != index && other != nextIndex &&
                PlaneDistance(edgePlane, points[other]) < -0x28f) {
                return PointCloudValidation::Concave;
            }
        }
    }
    return PointCloudValidation::Valid;
}

std::size_t PointCloud::GetPointCount() const { return points.size(); }
const std::vector<Vec3>& PointCloud::GetPoints() const { return points; }
std::vector<Vec3>& PointCloud::GetPoints() { return points; }

} // namespace soft3d
