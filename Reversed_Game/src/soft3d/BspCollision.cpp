#include "soft3d/Bsp.hpp"

#include <algorithm>
#include <limits>

namespace soft3d {
namespace {

uint32_t ReciprocalTable(uint32_t index)
{
    return index == 0 ? 0x7fffffffU : static_cast<uint32_t>(0x40000000ULL / index);
}

Fixed ReciprocalMagnitude(uint32_t value)
{
    if (value < 0x20000) {
        return static_cast<Fixed>(ReciprocalTable(value >> 1) << 1);
    }
    if (value < 0x80000) {
        return static_cast<Fixed>(ReciprocalTable(value >> 3) >> 1);
    }
    if (value < 0x200000) {
        return static_cast<Fixed>(ReciprocalTable(value >> 5) >> 3);
    }
    if (value < 0x800000) {
        return static_cast<Fixed>(ReciprocalTable(value >> 7) >> 5);
    }
    if (value < 0x2000000) {
        return static_cast<Fixed>(ReciprocalTable(value >> 9) >> 7);
    }
    return static_cast<Fixed>(ReciprocalTable(value >> 11) >> 9);
}

Fixed TraceReciprocal(Fixed value)
{
    if (value < 1) {
        return NegateFixed(ReciprocalMagnitude(static_cast<uint32_t>(0U -
            static_cast<uint32_t>(value))));
    }
    return ReciprocalMagnitude(static_cast<uint32_t>(value));
}

Fixed Component(const Vec3& value, unsigned axis)
{
    return (&value.x)[axis];
}

Fixed PlaneDistance(const BspPlane& plane, const Vec3& position)
{
    if (plane.type < 3) {
        return AddFixed(Component(position, plane.type), plane.distance);
    }
    return AddFixed(Dot(position, plane.normal), plane.distance);
}

Vec3 Lerp(const Vec3& start, const Vec3& end, Fixed fraction)
{
    return Add(start, Scale(Subtract(end, start), fraction));
}

Fixed ClampFraction(Fixed value)
{
    return std::max<Fixed>(0, std::min<Fixed>(fixedOne, value));
}

struct TraceContext {
    explicit TraceContext(const Bsp& map) : bsp(map) {}

    const Bsp& bsp;
    BspTraceResult result;
    ObjectBounds bounds;
    Vec3 start;
    Vec3 end;
    Vec3 extents;
    std::vector<uint32_t> brushMarks;
    uint32_t traceNumber = 0;
};

void ClipBrush(TraceContext& context, uint32_t brushIndex, bool pointOnly)
{
    const BspBrush& brush = context.bsp.GetBrushes()[brushIndex];
    Fixed enter = -fixedOne;
    Fixed leave = fixedOne;
    bool startsOut = false;
    bool allOut = true;
    BspPlane enterPlane{};
    int32_t enterSide = -1;

    for (uint16_t index = 0; index < brush.sideCount; ++index) {
        const uint32_t sideValue = context.bsp.GetBrushSides()[brush.firstSide + index];
        const uint32_t sideIndex = sideValue & 0xffffU;
        if (sideIndex >= context.bsp.GetPlanes().size()) {
            continue;
        }
        const BspPlane& plane = context.bsp.GetPlanes()[sideIndex];
        Fixed distance = plane.distance;
        if (!pointOnly) {
            Vec3 offset;
            offset.x = plane.normal.x < 0 ? context.bounds.maximum.x : context.bounds.minimum.x;
            offset.y = plane.normal.y < 0 ? context.bounds.maximum.y : context.bounds.minimum.y;
            offset.z = plane.normal.z < 0 ? context.bounds.maximum.z : context.bounds.minimum.z;
            distance = AddFixed(Dot(offset, plane.normal), distance);
        }
        const Fixed first = AddFixed(Dot(context.start, plane.normal), distance);
        const Fixed last = AddFixed(Dot(context.end, plane.normal), distance);
        if (last > 0) {
            allOut = false;
        }
        if (first > 0) {
            startsOut = true;
        }
        if (first > 0 && (last >= 0 || last >= first)) {
            return;
        }
        if (first <= 0 && last <= 0) {
            continue;
        }
        const Fixed fraction = FixedMultiply(first, TraceReciprocal(SubtractFixed(first, last)));
        if (first > last) {
            if (fraction > enter) {
                enter = fraction;
                enterPlane = plane;
                enterSide = static_cast<int32_t>(sideValue >> 16);
            }
        } else if (fraction < leave) {
            leave = fraction;
        }
    }

    if (!startsOut) {
        context.result.brushStartSolid = true;
        if (allOut) {
            context.result.allSolid = true;
        }
        return;
    }
    if (enter < leave && enter > -fixedOne && enter < context.result.fraction) {
        context.result.fraction = std::max<Fixed>(0, enter);
        context.result.planeNormal = enterPlane.normal;
        context.result.planeDistance = enterPlane.distance;
        context.result.sideIndex = enterSide;
    }
}

void ClipLeaf(TraceContext& context, int32_t leafIndex)
{
    const BspLeaf& leaf = context.bsp.GetLeaves()[leafIndex];
    for (uint16_t index = 0; index < leaf.brushCount; ++index) {
        const uint32_t brushIndex = context.bsp.GetLeafBrushes()[leaf.firstBrush + index];
        if (brushIndex >= context.brushMarks.size() ||
            context.brushMarks[brushIndex] == context.traceNumber) {
            continue;
        }
        context.brushMarks[brushIndex] = context.traceNumber;
        ClipBrush(context, brushIndex, false);
        if (context.result.fraction == 0) {
            return;
        }
    }
}

void TraceNode(TraceContext& context, int32_t nodeIndex, Fixed firstFraction,
               Fixed lastFraction, const Vec3& start, const Vec3& end)
{
    if (context.result.fraction <= firstFraction) {
        return;
    }
    if (nodeIndex < 0) {
        const int32_t leafIndex = -nodeIndex;
        if (leafIndex >= 0 && static_cast<std::size_t>(leafIndex) < context.bsp.GetLeaves().size() &&
            (context.bsp.GetLeaves()[leafIndex].contents & 1) != 0) {
            ClipLeaf(context, leafIndex);
        }
        return;
    }
    if (static_cast<std::size_t>(nodeIndex) >= context.bsp.GetNodes().size()) {
        return;
    }

    const BspNode& node = context.bsp.GetNodes()[nodeIndex];
    const BspPlane& plane = context.bsp.GetPlanes()[node.planeIndex];
    const Fixed first = PlaneDistance(plane, start);
    const Fixed last = PlaneDistance(plane, end);
    Fixed offset;
    if (plane.type < 3) {
        offset = Component(context.extents, plane.type);
    } else {
        offset = AddFixed(AddFixed(AddFixed(
            AbsFixed(FixedMultiply(context.extents.x, plane.normal.x)),
            AbsFixed(FixedMultiply(context.extents.y, plane.normal.y))),
            AbsFixed(FixedMultiply(context.extents.z, plane.normal.z))), 0x50000);
    }

    if (first >= offset && last >= offset) {
        TraceNode(context, node.children[0], firstFraction, lastFraction, start, end);
        return;
    }
    if (first < -offset && last < -offset) {
        TraceNode(context, node.children[1], firstFraction, lastFraction, start, end);
        return;
    }

    Fixed enter;
    Fixed leave;
    uint32_t side;
    const Fixed difference = SubtractFixed(first, last);
    if (difference < -0xff) {
        side = 1;
        const Fixed reciprocal = TraceReciprocal(difference);
        enter = FixedMultiply(AddFixed(first, offset), reciprocal);
        leave = FixedMultiply(SubtractFixed(first, offset), reciprocal);
    } else if (difference < 0x100) {
        side = 0;
        enter = fixedOne;
        leave = 0;
    } else {
        side = 0;
        const Fixed reciprocal = TraceReciprocal(difference);
        enter = FixedMultiply(SubtractFixed(first, offset), reciprocal);
        leave = FixedMultiply(AddFixed(first, offset), reciprocal);
    }
    enter = ClampFraction(enter);
    leave = ClampFraction(leave);

    const Vec3 enterPoint = Lerp(start, end, leave);
    const Fixed enterFraction = AddFixed(firstFraction,
        FixedMultiply(SubtractFixed(lastFraction, firstFraction), leave));
    TraceNode(context, node.children[side], firstFraction, enterFraction, start, enterPoint);

    const Vec3 leavePoint = Lerp(start, end, enter);
    const Fixed leaveFraction = AddFixed(firstFraction,
        FixedMultiply(SubtractFixed(lastFraction, firstFraction), enter));
    TraceNode(context, node.children[side ^ 1], leaveFraction, lastFraction, leavePoint, end);
}

int ClassifyPoint(const BspPlane& plane, const Vec3& first, const Vec3& second)
{
    const Fixed firstDistance = PlaneDistance(plane, first);
    if (firstDistance >= 0) {
        return 1;
    }
    return PlaneDistance(plane, second) <= 0 ? 2 : 3;
}

bool TracePointNode(TraceContext& context, int32_t nodeIndex, const Vec3& point)
{
    if (nodeIndex >= 0) {
        const BspNode& node = context.bsp.GetNodes()[nodeIndex];
        const int side = ClassifyPoint(context.bsp.GetPlanes()[node.planeIndex], point, point);
        if (side == 1) {
            return TracePointNode(context, node.children[0], point);
        }
        if (side == 2) {
            return TracePointNode(context, node.children[1], point);
        }
        if (TracePointNode(context, node.children[0], point)) {
            return true;
        }
        return TracePointNode(context, node.children[1], point);
    }
    const int32_t leafIndex = -nodeIndex;
    if (leafIndex < 0 || static_cast<std::size_t>(leafIndex) >= context.bsp.GetLeaves().size()) {
        return false;
    }
    if ((context.bsp.GetLeaves()[leafIndex].contents & 1) != 0) {
        context.result.startSolid = true;
        context.result.brushStartSolid = true;
        context.result.allSolid = true;
        return true;
    }
    return false;
}

int SampleLightFace(const Bsp& bsp, int32_t nodeIndex, const Vec3& start,
                    const Vec3& end, uint8_t& sample)
{
    if (nodeIndex >= 0) {
        if (static_cast<std::size_t>(nodeIndex) >= bsp.GetNodes().size()) {
            return -1;
        }
        const BspNode& node = bsp.GetNodes()[nodeIndex];
        const BspPlane& plane = bsp.GetPlanes()[node.planeIndex];
        const Fixed first = PlaneDistance(plane, start);
        const Fixed last = PlaneDistance(plane, end);
        const int firstSide = first < 0 ? 1 : 0;
        if ((last < 0 ? 1 : 0) == firstSide) {
            return SampleLightFace(bsp, node.children[firstSide], start, end, sample);
        }

        Vec3 middle = start;
        if (plane.type < 3) {
            middle.y = NegateFixed(plane.distance);
        } else {
            const Fixed factor = FixedMultiply(first, TraceReciprocal(SubtractFixed(first, last)));
            middle.y = AddFixed(start.y, FixedMultiply(SubtractFixed(end.y, start.y), factor));
        }
        int face = SampleLightFace(bsp, node.children[firstSide], start, middle, sample);
        if (face >= 0) {
            return face;
        }

        for (uint16_t index = 0; index < node.faceCount; ++index) {
            const uint32_t faceIndex = node.firstFace + index;
            if (faceIndex >= bsp.GetFaces().size()) {
                continue;
            }
            const BspFace& candidate = bsp.GetFaces()[faceIndex];
            if (candidate.lightmapOffset < 0 || candidate.texInfoIndex < 0) {
                continue;
            }
            const BspTexInfo& info = bsp.GetTexInfo()[candidate.texInfoIndex];
            const Vec3 uAxis{info.textureMapping[0], info.textureMapping[1], info.textureMapping[2]};
            const Vec3 vAxis{info.textureMapping[4], info.textureMapping[5], info.textureMapping[6]};
            const int32_t u = AddFixed(Dot(middle, uAxis), info.textureMapping[3]) >> 16;
            const int32_t v = AddFixed(Dot(middle, vAxis), info.textureMapping[7]) >> 16;
            const int32_t du = u - candidate.lightmapMin[0];
            const int32_t dv = v - candidate.lightmapMin[1];
            if (du < 0 || dv < 0 || du > candidate.lightmapSize[0] ||
                dv > candidate.lightmapSize[1]) {
                continue;
            }
            const int32_t width = (candidate.lightmapSize[0] >> 4) + 1;
            const int32_t texel = (dv >> 4) * width + (du >> 4);
            const int32_t offset = candidate.lightmapOffset + texel;
            if (offset < 0 || static_cast<std::size_t>(offset) >= bsp.GetLightmapData().size()) {
                continue;
            }
            uint32_t value = bsp.GetLightmapData()[offset];
            for (std::size_t styleIndex = 1; styleIndex < candidate.styles.size(); ++styleIndex) {
                const uint8_t styleNumber = candidate.styles[styleIndex];
                if (styleNumber == 0) {
                    break;
                }
                const int32_t styleOffset = offset + static_cast<int32_t>(styleIndex) *
                    candidate.lightmapSampleCount;
                if (styleOffset < 0 ||
                    static_cast<std::size_t>(styleOffset) >= bsp.GetLightmapData().size()) {
                    continue;
                }
                const BspLightStyle& style = bsp.GetLightStyles()[styleNumber];
                const uint32_t styled = bsp.GetLightmapData()[styleOffset];
                if (style.light >= 8) {
                    value += styled << ((style.light - 8) & 0xff);
                } else {
                    value += styled >> ((8 - style.light) & 0xff);
                }
            }
            sample = static_cast<uint8_t>(value);
            return static_cast<int>(faceIndex);
        }
        return SampleLightFace(bsp, node.children[firstSide ^ 1], middle, end, sample);
    }
    return -1;
}

} // namespace

BspTraceResult Bsp::Trace(const Vec3& startPosition, const Vec3& endPosition,
                          const ObjectBounds& traceBounds) const
{
    TraceContext context{*this};
    context.result.endPosition = startPosition;
    context.bounds = traceBounds;
    context.start = startPosition;
    context.end = endPosition;
    context.brushMarks.assign(brushes.size(), 0);

    if (startPosition.x == endPosition.x && startPosition.y == endPosition.y &&
        startPosition.z == endPosition.z) {
        const Vec3 point = Add(startPosition, endPosition);
        if (!nodes.empty()) {
            TracePointNode(context, 0, point);
        } else if (!leaves.empty() && (leaves[0].contents & 1) != 0) {
            context.result.startSolid = true;
            context.result.brushStartSolid = true;
            context.result.allSolid = true;
        }
        return context.result;
    }

    context.traceNumber = 1;
    context.result.endPosition = endPosition;
    context.extents = {
        std::max(NegateFixed(traceBounds.minimum.x), traceBounds.maximum.x),
        std::max(NegateFixed(traceBounds.minimum.y), traceBounds.maximum.y),
        std::max(NegateFixed(traceBounds.minimum.z), traceBounds.maximum.z)
    };
    if (!nodes.empty()) {
        TraceNode(context, 0, 0, fixedOne, startPosition, endPosition);
    } else if (!leaves.empty() && (leaves[0].contents & 1) != 0) {
        ClipLeaf(context, 0);
    }
    if (context.result.allSolid) {
        context.result.fraction = 0;
    }
    if (context.result.fraction < fixedOne) {
        context.result.startSolid = true;
        context.result.endPosition = Add(Lerp(startPosition, endPosition, context.result.fraction),
                                         Scale(context.result.planeNormal, 0x51e));
    }
    return context.result;
}

const BspPathNode* Bsp::FindNearestPathNode(const Vec3& position, const ObjectBounds& bounds,
                                            const BspPathNode* excluded) const
{
    Fixed best = 0x7fff0000;
    const BspPathNode* result = nullptr;
    for (const BspPathNode& node : pathNodes) {
        if (&node == excluded) {
            continue;
        }
        const Fixed distance = SquaredLength(Subtract(position, node.position));
        if (distance >= 0 && distance < best && !Trace(position, node.position, bounds).startSolid) {
            best = distance;
            result = &node;
        }
    }
    return result;
}

const BspPathNode* Bsp::GetPathNode(uint16_t index) const
{
    if (index == 0xffff || index >= pathNodes.size()) {
        return nullptr;
    }
    return &pathNodes[index];
}

Fixed Bsp::GetLightIntensity(const Vec3& position) const
{
    if (lightmapData.empty()) {
        return fixedOne;
    }
    Vec3 end = position;
    end.y = SubtractFixed(end.y, 0x640000);
    uint8_t sample = 0;
    if (nodes.empty() || SampleLightFace(*this, 0, position, end, sample) < 0) {
        return 0;
    }
    Fixed intensity = sample;
    return intensity >= 0xff0000 ? fixedOne : intensity >> 8;
}

} // namespace soft3d
