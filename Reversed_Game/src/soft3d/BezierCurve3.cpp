#include "soft3d/BezierCurve3.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>

namespace soft3d {
namespace {

uint32_t Read32(const uint8_t* input)
{
    return static_cast<uint32_t>(input[0]) |
           (static_cast<uint32_t>(input[1]) << 8) |
           (static_cast<uint32_t>(input[2]) << 16) |
           (static_cast<uint32_t>(input[3]) << 24);
}

float ReadFloat(const uint8_t* input)
{
    const uint32_t bits = Read32(input);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

bool ReadFixedPoint(const uint8_t* input, Fixed& output)
{
    const float value = ReadFloat(input);
    const float scaled = value * 65536.0f;
    if (!std::isfinite(scaled) ||
        static_cast<double>(scaled) < static_cast<double>(std::numeric_limits<int32_t>::min()) ||
        static_cast<double>(scaled) > static_cast<double>(std::numeric_limits<int32_t>::max())) {
        return false;
    }
    output = static_cast<Fixed>(scaled);
    return true;
}

Fixed BinomialCoefficient(int32_t degree, int32_t index)
{
    int32_t degreeFactorial = 1;
    int32_t differenceFactorial = 1;
    int32_t indexFactorial = 1;
    for (int32_t value = degree; value > 0; --value) {
        degreeFactorial = static_cast<int32_t>(static_cast<uint32_t>(degreeFactorial) *
                                               static_cast<uint32_t>(value));
    }
    for (int32_t value = degree - index; value > 0; --value) {
        differenceFactorial = static_cast<int32_t>(static_cast<uint32_t>(differenceFactorial) *
                                                   static_cast<uint32_t>(value));
    }
    for (int32_t value = index; value > 0; --value) {
        indexFactorial = static_cast<int32_t>(static_cast<uint32_t>(indexFactorial) *
                                              static_cast<uint32_t>(value));
    }
    const int32_t coefficient = degreeFactorial / (differenceFactorial * indexFactorial);
    return ShiftLeftFixed(coefficient, 16);
}

Fixed ClampTime(Fixed time)
{
    return std::clamp(time, 0, fixedOne);
}

} // namespace

BezierCurve3::BezierCurve3(const std::vector<Vec3>& controlPoints)
    : points(controlPoints)
    , segmentCount(static_cast<int32_t>(controlPoints.size()) - 1)
{
    Rebuild();
}

void BezierCurve3::Reset()
{
    points.clear();
    pointCount = 0;
    segmentCount = 0;
    coefficientMatrix.clear();
    firstDifferences.clear();
    secondDifferences.clear();
}

void BezierCurve3::Rebuild()
{
    pointCount = segmentCount + 1;
    coefficientMatrix.assign(static_cast<std::size_t>(pointCount) * pointCount, 0);
    for (int32_t degree = 0; degree <= segmentCount; ++degree) {
        coefficientMatrix[degree * pointCount] = fixedOne;
        coefficientMatrix[degree * pointCount + degree] = fixedOne;
        for (int32_t index = 1; index < degree; ++index) {
            coefficientMatrix[degree * pointCount + index] = BinomialCoefficient(degree, index);
        }
    }
    firstDifferences.clear();
    for (int32_t index = 0; index < segmentCount; ++index) {
        firstDifferences.push_back(Subtract(points[index + 1], points[index]));
    }
    secondDifferences.clear();
    for (int32_t index = 0; index + 1 < segmentCount; ++index) {
        secondDifferences.push_back(Subtract(firstDifferences[index + 1], firstDifferences[index]));
    }
}

Vec3 BezierCurve3::Evaluate(const std::vector<Vec3>& controlPoints, int32_t degree,
                            Fixed scale, Fixed time) const
{
    time = ClampTime(time);
    const Fixed oneMinusTime = SubtractFixed(fixedOne, time);
    Fixed power = time;
    Vec3 output = Scale(controlPoints[0], oneMinusTime);
    for (int32_t index = 1; index < degree; ++index) {
        const Fixed coefficient = FixedMultiply(
            coefficientMatrix[degree * pointCount + index], power);
        output = Add(output, Scale(controlPoints[index], coefficient));
        output = Scale(output, oneMinusTime);
        power = FixedMultiply(power, time);
    }
    output = Add(output, Scale(controlPoints[degree], power));
    return Scale(output, scale);
}

Vec3 BezierCurve3::Evaluate(Fixed time) const
{
    if (segmentCount < 1 || points.size() != static_cast<std::size_t>(pointCount)) {
        return {};
    }
    return Evaluate(points, segmentCount, fixedOne, time);
}

Vec3 BezierCurve3::EvaluateDerivative(Fixed time) const
{
    if (segmentCount < 1) {
        return {};
    }
    return Evaluate(firstDifferences, segmentCount - 1, ShiftLeftFixed(segmentCount, 16), time);
}

Vec3 BezierCurve3::EvaluateSecondDerivative(Fixed time) const
{
    if (segmentCount < 2) {
        return {};
    }
    const Fixed scale = ShiftLeftFixed(segmentCount * (segmentCount - 1), 16);
    return Evaluate(secondDifferences, segmentCount - 2, scale, time);
}

int32_t BezierCurve3::GetSegmentCount() const { return segmentCount; }
int32_t BezierCurve3::GetPointCount() const { return pointCount; }
const std::vector<Vec3>& BezierCurve3::GetPoints() const { return points; }
const std::vector<Fixed>& BezierCurve3::GetCoefficientMatrix() const { return coefficientMatrix; }
const std::vector<Vec3>& BezierCurve3::GetFirstDifferences() const { return firstDifferences; }
const std::vector<Vec3>& BezierCurve3::GetSecondDifferences() const { return secondDifferences; }

CameraPathCollection::CameraPathCollection(std::size_t curveCapacity)
    : capacity(curveCapacity)
{
    curves.reserve(capacity);
}

bool CameraPathCollection::Append(const BezierCurve3& curve)
{
    if (curves.size() >= capacity) {
        return false;
    }
    curves.push_back(curve);
    curves.back().Rebuild();
    RebuildTiming();
    return true;
}

void CameraPathCollection::RebuildTiming()
{
    pathLengths.assign(curves.size(), 0);
    pathEndTimes.assign(curves.size(), 0);
    Fixed totalLength = 0;
    for (std::size_t curveIndex = 0; curveIndex < curves.size(); ++curveIndex) {
        const auto& points = curves[curveIndex].GetPoints();
        for (std::size_t pointIndex = 1; pointIndex < points.size(); ++pointIndex) {
            pathLengths[curveIndex] = AddFixed(pathLengths[curveIndex],
                Length(Subtract(points[pointIndex], points[pointIndex - 1])));
        }
        totalLength = AddFixed(totalLength, pathLengths[curveIndex]);
    }
    for (std::size_t index = 0; index < curves.size(); ++index) {
        pathEndTimes[index] = FixedDivide(pathLengths[index], totalLength);
        if (index > 0) {
            pathEndTimes[index] = AddFixed(pathEndTimes[index], pathEndTimes[index - 1]);
        }
        if (pathEndTimes[index] > fixedOne) {
            pathEndTimes[index] = fixedOne;
        }
    }
    if (!pathEndTimes.empty()) {
        pathEndTimes.back() = fixedOne;
    }
}

Vec3 CameraPathCollection::Evaluate(Fixed time, int derivativeOrder) const
{
    time = ClampTime(time);
    if (curves.empty() || pathLengths.size() != curves.size() ||
        pathEndTimes.size() != curves.size()) {
        return {};
    }
    std::size_t curveIndex = 0;
    Fixed localTime = 0;
    for (; curveIndex < curves.size(); ++curveIndex) {
        if (time <= pathEndTimes[curveIndex]) {
            if (curveIndex == 0) {
                localTime = FixedDivide(time, pathEndTimes[0]);
            } else {
                const Fixed startTime = pathEndTimes[curveIndex - 1];
                localTime = FixedDivide(SubtractFixed(time, startTime),
                                        SubtractFixed(pathEndTimes[curveIndex], startTime));
            }
            break;
        }
    }
    if (curveIndex == curves.size()) {
        curveIndex = curves.size() - 1;
        localTime = fixedOne;
    }
    localTime = ClampTime(localTime);
    if (derivativeOrder == 1) {
        return curves[curveIndex].EvaluateDerivative(localTime);
    }
    if (derivativeOrder == 2) {
        return curves[curveIndex].EvaluateSecondDerivative(localTime);
    }
    return curves[curveIndex].Evaluate(localTime);
}

Vec3 CameraPathCollection::EvaluatePosition(Fixed time) const { return Evaluate(time, 0); }
Vec3 CameraPathCollection::EvaluateDerivative(Fixed time) const { return Evaluate(time, 1); }
Vec3 CameraPathCollection::EvaluateSecondDerivative(Fixed time) const { return Evaluate(time, 2); }
std::size_t CameraPathCollection::GetCapacity() const { return capacity; }
std::size_t CameraPathCollection::GetCount() const { return curves.size(); }
const BezierCurve3& CameraPathCollection::GetCurve(std::size_t index) const { return curves.at(index); }
const std::vector<Fixed>& CameraPathCollection::GetPathLengths() const { return pathLengths; }
const std::vector<Fixed>& CameraPathCollection::GetPathEndTimes() const { return pathEndTimes; }

result ParseSplCameraPaths(const uint8_t* data, std::size_t size,
                           CameraPathCollection& collection)
{
    if (data == nullptr || size < 4) {
        return S3D_FORMAT;
    }
    const int32_t recordCount = static_cast<int32_t>(Read32(data));
    if (recordCount < 0) {
        return S3D_FORMAT;
    }
    std::size_t offset = 4;
    for (int32_t record = 0; record < recordCount; ++record) {
        if (offset > size || size - offset < 8) {
            return S3D_FORMAT;
        }
        const int32_t segmentCount = static_cast<int32_t>(Read32(data + offset));
        const int32_t pointCount = static_cast<int32_t>(Read32(data + offset + 4));
        if (segmentCount < 1 || pointCount != segmentCount + 1) {
            return S3D_FORMAT;
        }
        const uint64_t matrixCount = static_cast<uint64_t>(pointCount) * pointCount;
        const uint64_t recordBytes = 8 + (matrixCount + 9ULL * segmentCount) * 4;
        if (recordBytes > size - offset) {
            return S3D_FORMAT;
        }
        std::size_t input = offset + 8;
        std::vector<Vec3> points(static_cast<std::size_t>(pointCount));
        for (Vec3& point : points) {
            if (!ReadFixedPoint(data + input, point.x) ||
                !ReadFixedPoint(data + input + 4, point.y) ||
                !ReadFixedPoint(data + input + 8, point.z)) {
                return S3D_FORMAT;
            }
            input += 12;
        }
        if (!collection.Append(BezierCurve3(points))) {
            return S3D_FORMAT;
        }
        offset += static_cast<std::size_t>(recordBytes);
    }
    return offset == size ? S3D_OK : S3D_FORMAT;
}

result LoadSplCameraResource(CameraPathCollection& collection, const char16_t* fileName,
                             s3dFilePak* filePak)
{
    if (filePak == nullptr || filePak->Select(fileName) != S3D_OK) {
        return S3D_FILE;
    }
    filePak->Seek(0, SEEK_END);
    const int bytes = filePak->Tell();
    filePak->Seek(0, SEEK_SET);
    if (bytes < 0) {
        return S3D_FILE;
    }
    std::vector<uint8_t> data(static_cast<std::size_t>(bytes));
    if (!data.empty() && filePak->Read(data.data(), bytes) != S3D_OK) {
        return S3D_FILE;
    }
    return ParseSplCameraPaths(data.data(), data.size(), collection);
}

} // namespace soft3d
