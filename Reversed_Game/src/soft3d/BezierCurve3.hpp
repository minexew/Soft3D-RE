#pragma once

#include "soft3d/Fixed.hpp"
#include "soft3d/s3dCommon.hpp"
#include "soft3d/s3dFilePak.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace soft3d {

class BezierCurve3 {
public:
    BezierCurve3() = default;
    explicit BezierCurve3(const std::vector<Vec3>& points);

    void Reset();
    void Rebuild();
    Vec3 Evaluate(Fixed time) const;
    Vec3 EvaluateDerivative(Fixed time) const;
    Vec3 EvaluateSecondDerivative(Fixed time) const;

    int32_t GetSegmentCount() const;
    int32_t GetPointCount() const;
    const std::vector<Vec3>& GetPoints() const;
    const std::vector<Fixed>& GetCoefficientMatrix() const;
    const std::vector<Vec3>& GetFirstDifferences() const;
    const std::vector<Vec3>& GetSecondDifferences() const;

private:
    Vec3 Evaluate(const std::vector<Vec3>& points, int32_t degree, Fixed scale,
                  Fixed time) const;

    std::vector<Vec3> points;
    int32_t pointCount = 0;
    int32_t segmentCount = 0;
    std::vector<Fixed> coefficientMatrix;
    std::vector<Vec3> firstDifferences;
    std::vector<Vec3> secondDifferences;
};

class CameraPathCollection {
public:
    explicit CameraPathCollection(std::size_t capacity = 64);

    bool Append(const BezierCurve3& curve);
    void RebuildTiming();
    Vec3 EvaluatePosition(Fixed time) const;
    Vec3 EvaluateDerivative(Fixed time) const;
    Vec3 EvaluateSecondDerivative(Fixed time) const;

    std::size_t GetCapacity() const;
    std::size_t GetCount() const;
    const BezierCurve3& GetCurve(std::size_t index) const;
    const std::vector<Fixed>& GetPathLengths() const;
    const std::vector<Fixed>& GetPathEndTimes() const;

private:
    Vec3 Evaluate(Fixed time, int derivativeOrder) const;

    std::size_t capacity;
    std::vector<BezierCurve3> curves;
    std::vector<Fixed> pathLengths;
    std::vector<Fixed> pathEndTimes;
};

result ParseSplCameraPaths(const uint8_t* data, std::size_t size,
                           CameraPathCollection& collection);
result LoadSplCameraResource(CameraPathCollection& collection, const char16_t* fileName,
                             s3dFilePak* filePak);

} // namespace soft3d
