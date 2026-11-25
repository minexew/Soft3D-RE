#include "soft3d/Geometry.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

void CheckVec3(const soft3d::Vec3& actual, const soft3d::Vec3& expected, int line)
{
    Check(actual.x == expected.x && actual.y == expected.y && actual.z == expected.z, line);
}

#define CHECK_VEC3(actual, ...) CheckVec3((actual), soft3d::Vec3 __VA_ARGS__, __LINE__)

void CheckPoints(const soft3d::PointCloud& actual, const std::vector<soft3d::Vec3>& expected,
                 int line)
{
    Check(actual.GetPointCount() == expected.size(), line);
    for (std::size_t index = 0; index < expected.size(); ++index) {
        CheckVec3(actual.GetPoints()[index], expected[index], line);
    }
}

#define CHECK_POINTS(actual, ...) CheckPoints((actual), std::vector<soft3d::Vec3> __VA_ARGS__, __LINE__)

constexpr soft3d::Fixed one = soft3d::fixedOne;

void TestPlanesAndRays()
{
    const soft3d::Plane plane{{one, 0, 0}, -2 * one};
    CHECK(soft3d::PlaneDistance(plane, {3 * one, 0, 0}) == one);
    const soft3d::Plane fromPoint = soft3d::PlaneFromPointNormal({2 * one, 0, 0}, {one, 0, 0});
    CHECK_VEC3(fromPoint.normal, {one, 0, 0});
    CHECK(fromPoint.distance == -2 * one);

    soft3d::Ray ray{{one, 2 * one, 3 * one}, {3 * one, 4 * one, 0}};
    const soft3d::Ray normalized = soft3d::NormalizeRay(ray);
    CHECK_VEC3(normalized.origin, {one, 2 * one, 3 * one});
    CHECK_VEC3(normalized.direction, {39324, 52432, 0});
    const soft3d::Ray transformed = soft3d::TransformRay(
        soft3d::TranslationMatrix(one, 2 * one, 3 * one), {{0, 0, 0}, {one, 0, 0}});
    CHECK_VEC3(transformed.origin, {one, 2 * one, 3 * one});
    CHECK_VEC3(transformed.direction, {2 * one, 2 * one, 3 * one});
    CHECK(soft3d::DotDirection({{0, 0, 0}, {one, 0, 0}}, {2 * one, 0, 0}) == 2 * one);

    soft3d::Vec3 closest{1, 2, 3};
    const soft3d::Ray xAxis{{0, 0, 0}, {one, 0, 0}};
    CHECK(soft3d::DistanceToRay({0, 2 * one, 0}, xAxis, &closest) == 2 * one);
    CHECK_VEC3(closest, {0, 0, 0});
    CHECK(soft3d::DistanceToRay({2 * one, 0, 0}, xAxis, &closest) == 0);
    CHECK_VEC3(closest, {2 * one, 0, 0});
    CHECK(soft3d::DistanceToLine({0, 4 * one, 0}, xAxis) == 131072);

    soft3d::Vec3 projected;
    CHECK(soft3d::PlaneDistanceAndProject({3 * one, 0, 0}, plane, &projected) == one);
    CHECK_VEC3(projected, {2 * one, 0, 0});
    CHECK(soft3d::PlaneDistanceAndProject({0, 0, 0}, plane, &projected) == -2 * one);
    CHECK_VEC3(projected, {0, 0, 0});
    const soft3d::Plane thresholdPlane{{one, 0, 0}, 0};
    CHECK(soft3d::PlaneDistanceAndProject({0x28f, 0, 0}, thresholdPlane, &projected) == 0x28f);
    CHECK_VEC3(projected, {0x28f, 0, 0});
    CHECK(soft3d::PlaneDistanceAndProject({0x290, 0, 0}, thresholdPlane, &projected) == 0x290);
    CHECK_VEC3(projected, {0, 0, 0});

    soft3d::Vec3 intersection{1, 2, 3};
    CHECK(soft3d::PlaneIntersectRay({{3 * one, 0, 0}, {one, 0, 0}}, plane, &intersection) == 0);
    CHECK_VEC3(intersection, {2 * one, 0, 0});
    intersection = {1, 2, 3};
    CHECK(soft3d::PlaneIntersectRay({{3 * one, 0, 0}, {0, one, 0}}, plane, &intersection) == one);
    CHECK_VEC3(intersection, {1, 2, 3});
    CHECK(soft3d::PlaneIntersectRay({{0, 0, 0}, {0x28e, 0, 0}}, plane, &intersection) == -2 * one);
    CHECK(soft3d::PlaneIntersectRay({{0, 0, 0}, {0x28f, 0, 0}}, plane, &intersection) == 0);

    CHECK_VEC3(soft3d::Slide({one, 2 * one, 0}, {one, 0, 0}), {-655, 2 * one, 0});
    CHECK_VEC3(soft3d::Slide({0, 2 * one, 0}, {one, 0, 0}), {0, 2 * one, 0});
    CHECK(soft3d::SegmentIntersectionDistance({{0, 4 * one, 0}, {one, 0, 0}}, xAxis) == 131072);
    intersection = {0, 0, 0};
    CHECK(soft3d::SegmentIntersectionDistance({{0, 0, 0}, {one, 0, 0}},
                                              {{2 * one, -one, 0}, {0, one, 0}},
                                              &intersection) == 0);
    CHECK_VEC3(intersection, {one, 0, 0});
}

void TestFrustum()
{
    for (std::size_t tested = 0; tested < 5; ++tested) {
        std::array<soft3d::Plane, 5> planes{};
        for (soft3d::Plane& plane : planes) {
            plane = {{one, 0, 0}, one};
        }
        planes[tested].distance = 0;
        CHECK(soft3d::OutsideFrustum(planes, {-one, 0, 0, 0}));
        CHECK(!soft3d::OutsideFrustum(planes, {-one, 0, 0, one}));
    }
    std::array<soft3d::Plane, 5> boundary{};
    for (soft3d::Plane& plane : boundary) {
        plane = {{one, 0, 0}, 0};
    }
    CHECK(!soft3d::OutsideFrustum(boundary, {0, 0, 0, 0}));
}

void TestPointCloudConstruction()
{
    constexpr soft3d::Fixed extent = 0x20000000;
    const soft3d::PointCloud xCloud = soft3d::PointCloud::FromPlane({{one, 0, 0}, 2 * one});
    CHECK_POINTS(xCloud, {{{2 * one, extent, -extent},
                           {2 * one, extent, extent},
                           {2 * one, -extent, extent},
                           {2 * one, -extent, -extent}}});
    CHECK_VEC3(xCloud.Center(), {2 * one, 0, 0});
    const soft3d::PointCloud yCloud = soft3d::PointCloud::FromPlane({{0, one, 0}, 2 * one});
    CHECK_POINTS(yCloud, {{{-extent, 2 * one, extent},
                           {extent, 2 * one, extent},
                           {extent, 2 * one, -extent},
                           {-extent, 2 * one, -extent}}});
    const soft3d::PointCloud zCloud = soft3d::PointCloud::FromPlane({{0, 0, one}, 2 * one});
    CHECK_POINTS(zCloud, {{{extent, extent, 2 * one},
                           {-extent, extent, 2 * one},
                           {-extent, -extent, 2 * one},
                           {extent, -extent, 2 * one}}});

    soft3d::PointCloud original({{one, 2 * one, 3 * one}, {4 * one, 5 * one, 6 * one}});
    soft3d::PointCloud copy = original;
    copy.GetPoints()[0].x = 99;
    CHECK(original.GetPoints()[0].x == one);
    const soft3d::PointCloud reversed = original.Reversed();
    CHECK_POINTS(reversed, {{{4 * one, 5 * one, 6 * one}, {one, 2 * one, 3 * one}}});
    original.Allocate(3);
    CHECK(original.GetPointCount() == 3);
    CHECK_VEC3(original.GetPoints()[2], {0, 0, 0});
    original.Allocate(2);
    CHECK(original.GetPointCount() == 2);

    const soft3d::PointCloud triangle({{0, 0, 0}, {2 * one, 0, 0}, {0, 3 * one, 0}});
    CHECK_VEC3(triangle.GetPlane().normal, {0, 0, 65520});
    CHECK(triangle.GetPlane().distance == 0);
    CHECK(triangle.Area() == 3 * one);
    CHECK_VEC3(triangle.Reversed().GetPlane().normal, {0, 0, -65520});
    const soft3d::PointCloud degenerate({{0, 0, 0}, {one, 0, 0}, {2 * one, 0, 0}});
    CHECK(degenerate.Area() == 0);
    CHECK_VEC3(degenerate.GetPlane().normal, {0, 0, 0});

    const soft3d::ObjectBounds bounds = triangle.Bounds();
    CHECK_VEC3(bounds.minimum, {0, 0, 0});
    CHECK_VEC3(bounds.maximum, {2 * one, 3 * one, 0});
    const soft3d::PointCloud empty;
    const soft3d::ObjectBounds emptyBounds = empty.Bounds();
    CHECK(emptyBounds.minimum.x == std::numeric_limits<int32_t>::max());
    CHECK(emptyBounds.maximum.x == std::numeric_limits<int32_t>::min());
    CHECK_VEC3(empty.Center(), {0, 0, 0});
    CHECK_VEC3(empty.GetPlane().normal, {0, 0, 0});
    const soft3d::PointCloud average({{one, 0, 0}, {0, 0, 0}, {0, 0, 0}});
    CHECK_VEC3(average.Center(), {0x5555, 0, 0});
}

void TestPointCloudEditing()
{
    soft3d::PointCloud cloud({{0, 0, 0}, {one, 0, 0}, {2 * one, 0, 0}, {2 * one, one, 0}});
    cloud.Optimize();
    CHECK_POINTS(cloud, {{{0, 0, 0}, {2 * one, 0, 0}, {2 * one, one, 0}}});
    cloud.Optimize();
    CHECK(cloud.GetPointCount() == 3);
    soft3d::PointCloud empty;
    empty.Optimize();
    CHECK(empty.GetPointCount() == 0);

    const soft3d::Plane plane{{one, 0, 0}, 0};
    CHECK(soft3d::PointCloud().Classify(plane, 0x100) == soft3d::PlaneClassification::OnPlane);
    CHECK(soft3d::PointCloud({{one, 0, 0}}).Classify(plane, 0x100) ==
          soft3d::PlaneClassification::Positive);
    CHECK(soft3d::PointCloud({{-one, 0, 0}}).Classify(plane, 0x100) ==
          soft3d::PlaneClassification::Negative);
    CHECK(soft3d::PointCloud({{0x100, 0, 0}, {-0x100, 0, 0}}).Classify(plane, 0x100) ==
          soft3d::PlaneClassification::OnPlane);
    CHECK(soft3d::PointCloud({{one, 0, 0}, {-one, 0, 0}}).Classify(plane, 0x100) ==
          soft3d::PlaneClassification::Spanning);

    const soft3d::PointCloud square({{-one, -one, 0}, {3 * one, -one, 0},
                                     {3 * one, one, 0}, {-one, one, 0}});
    const soft3d::Plane clipPlane{{one, 0, 0}, -one};
    const std::vector<soft3d::Vec3> expected = {
        {one, -one, 0}, {3 * one, -one, 0}, {3 * one, one, 0}, {one, one, 0}};
    CheckPoints(square.ClippedPositive(clipPlane, 0), expected, __LINE__);
    soft3d::PointCloud inPlace = square;
    inPlace.ClipPositiveInPlace(clipPlane, 0);
    CheckPoints(inPlace, expected, __LINE__);
    CHECK(square.GetPointCount() == 4);
    CheckPoints(square.ClippedPositive({{one, 0, 0}, 10 * one}, 0),
                square.GetPoints(), __LINE__);
    CHECK(square.ClippedPositive({{one, 0, 0}, -10 * one}, 0).GetPointCount() == 0);
    const soft3d::PointCloud onPlane({{0, 0, 0}, {0, one, 0}, {0, 2 * one, 0}});
    CheckPoints(onPlane.ClippedPositive(plane, 0), onPlane.GetPoints(), __LINE__);
    CHECK(empty.ClippedPositive(plane, 0).GetPointCount() == 0);
}

void TestPointCloudValidation()
{
    const std::vector<soft3d::Vec3> squarePoints = {
        {0, 0, 0}, {2 * one, 0, 0}, {2 * one, 2 * one, 0}, {0, 2 * one, 0}};
    CHECK(soft3d::PointCloud(squarePoints).Validate() == soft3d::PointCloudValidation::Valid);
    CHECK(soft3d::PointCloud({{0, 0, 0}, {one, 0, 0}}).Validate() ==
          soft3d::PointCloudValidation::TooFewPoints);
    CHECK(soft3d::PointCloud({{0, 0, 0}, {one, 0, 0}, {2 * one, 0, 0}}).Validate() ==
          soft3d::PointCloudValidation::SmallArea);

    constexpr soft3d::Fixed large = 0x20000001;
    CHECK(soft3d::PointCloud({{large, 0, 0}, {large + 2 * one, 0, 0},
                              {large + 2 * one, 2 * one, 0}, {large, 2 * one, 0}}).Validate() ==
          soft3d::PointCloudValidation::CoordinateTooLarge);
    CHECK(soft3d::PointCloud({{0, 0, 0}, {0, 0, 0}, {2 * one, 0, 0},
                              {2 * one, 2 * one, 0}, {0, 2 * one, 0}}).Validate() ==
          soft3d::PointCloudValidation::ShortEdge);
    CHECK(soft3d::PointCloud({{0, 0, 0}, {2 * one, 0, 0}, {0, 2 * one, 0},
                              {0, 0, 0x199a}}).Validate() ==
          soft3d::PointCloudValidation::NonPlanar);
    CHECK(soft3d::PointCloud({{0, 0, 0}, {3 * one, 0, 0}, {one, one, 0},
                              {3 * one, 3 * one, 0}, {0, 3 * one, 0}}).Validate() ==
          soft3d::PointCloudValidation::Concave);
}

} // namespace

int main()
{
    try {
        TestPlanesAndRays();
        TestFrustum();
        TestPointCloudConstruction();
        TestPointCloudEditing();
        TestPointCloudValidation();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "geometry and point-cloud API tests passed\n";
    return 0;
}
