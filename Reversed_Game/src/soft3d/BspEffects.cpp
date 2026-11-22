#include "soft3d/Bsp.hpp"

#include "soft3d/Texture.hpp"

#include <algorithm>
#include <cstring>
#include <functional>

namespace soft3d {
namespace {

Fixed DistanceToPlane(const BspPlane& plane, const Vec3& point)
{
    return plane.type < 3 ? AddFixed((&point.x)[plane.type], plane.distance)
                          : AddFixed(Dot(plane.normal, point), plane.distance);
}

uint32_t ReciprocalTable(uint16_t index)
{
    return index == 0 ? 0x7fffffffU : static_cast<uint32_t>(0x40000000ULL / index);
}

Fixed ReciprocalSigned(Fixed value)
{
    const uint32_t magnitude = value < 1 ? 0U - static_cast<uint32_t>(value)
                                         : static_cast<uint32_t>(value);
    uint32_t result;
    if (magnitude < 0x20000) {
        result = ReciprocalTable(static_cast<uint16_t>(magnitude >> 1)) << 1;
    } else if (magnitude < 0x80000) {
        result = ReciprocalTable(static_cast<uint16_t>(magnitude >> 3)) >> 1;
    } else if (magnitude < 0x200000) {
        result = ReciprocalTable(static_cast<uint16_t>(magnitude >> 5)) >> 3;
    } else if (magnitude < 0x800000) {
        result = ReciprocalTable(static_cast<uint16_t>(magnitude >> 7)) >> 5;
    } else {
        result = ReciprocalTable(static_cast<uint16_t>(magnitude >> 9)) >> 7;
    }
    return value < 1 ? static_cast<Fixed>(0U - result) : static_cast<Fixed>(result);
}

uint16_t BloodPaletteColor(const Texture* texture, uint8_t index)
{
    if (texture == nullptr || texture->GetPalette() == nullptr) {
        return 0;
    }
    const std::size_t offset = (static_cast<std::size_t>(index) * 32 + 15) * 2;
    if (offset + 2 > static_cast<std::size_t>(texture->GetPaletteSize()) * 64) {
        return 0;
    }
    uint16_t color = 0;
    std::memcpy(&color, texture->GetPalette() + offset, 2);
    return color;
}

uint16_t Blend50(uint16_t destination, uint16_t source)
{
    return static_cast<uint16_t>(((destination & 0xf7de) >> 1) +
                                 ((source & 0xf7de) >> 1));
}

} // namespace

void Bsp::BlendLightmapSplat(const Texture* texture, const Vec3& start, const Vec3& end,
                             bool flipU, bool flipV)
{
    if (texture == nullptr || texture->GetPixels() == nullptr || texture->GetWidth() == 0 ||
        texture->GetHeight() == 0 || nodes.empty()) {
        return;
    }

    std::function<int(int32_t, const Vec3&, const Vec3&)> visit =
        [&](int32_t nodeIndex, const Vec3& first, const Vec3& last) -> int {
        if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= nodes.size()) {
            return -1;
        }
        const BspNode& node = nodes[static_cast<std::size_t>(nodeIndex)];
        if (node.planeIndex >= planes.size()) {
            return -1;
        }
        const BspPlane& plane = planes[node.planeIndex];
        const Fixed firstDistance = DistanceToPlane(plane, first);
        const Fixed lastDistance = DistanceToPlane(plane, last);
        const uint32_t firstSide = static_cast<uint32_t>(firstDistance) >> 31;
        const uint32_t lastSide = static_cast<uint32_t>(lastDistance) >> 31;
        if (firstSide == lastSide) {
            return visit(node.children[firstSide], first, last);
        }

        Vec3 middle = first;
        if (plane.type < 3) {
            middle.y = NegateFixed(plane.distance);
        } else {
            const Fixed scale = ReciprocalSigned(SubtractFixed(firstDistance, lastDistance));
            const Fixed fraction = FixedMultiply(firstDistance, scale);
            middle.y = AddFixed(first.y,
                FixedMultiply(SubtractFixed(last.y, first.y), fraction));
        }
        int result = visit(node.children[firstSide], first, middle);
        if (result >= 0) {
            return result;
        }

        for (uint16_t offset = 0; offset < node.faceCount; ++offset) {
            const std::size_t faceIndex = static_cast<std::size_t>(node.firstFace) + offset;
            if (faceIndex >= faces.size()) {
                continue;
            }
            BspFace& face = faces[faceIndex];
            if (face.texInfoIndex < 0 ||
                static_cast<std::size_t>(face.texInfoIndex) >= texInfo.size() ||
                face.lightmapPixels.empty()) {
                continue;
            }
            const BspTexInfo& info = texInfo[static_cast<std::size_t>(face.texInfoIndex)];
            const Vec3 uAxis{info.textureMapping[0], info.textureMapping[1], info.textureMapping[2]};
            const Vec3 vAxis{info.textureMapping[4], info.textureMapping[5], info.textureMapping[6]};
            const int projectedU = AddFixed(Dot(middle, uAxis), info.textureMapping[3]) >> 16;
            const int projectedV = AddFixed(Dot(middle, vAxis), info.textureMapping[7]) >> 16;
            const int x = projectedU - face.lightmapMin[0];
            const int y = projectedV - face.lightmapMin[1];
            int leftWidth = static_cast<int>(texture->GetWidth()) >> 1;
            int rightWidth = leftWidth;
            int topHeight = static_cast<int>(texture->GetHeight()) >> 1;
            int bottomHeight = topHeight;
            int sourceX = 0;
            int sourceY = 0;
            const int clipLeft = leftWidth - x;
            if (clipLeft > 0) {
                leftWidth -= clipLeft;
                sourceX = clipLeft;
            }
            const int clipRight = x + rightWidth - face.lightmapPowerOfTwoWidth;
            if (clipRight > 0) {
                rightWidth -= clipRight;
            }
            if (x - leftWidth >= x + rightWidth || x - leftWidth >= face.lightmapSize[0] ||
                x + rightWidth <= 0) {
                continue;
            }
            const int clipTop = topHeight - y;
            if (clipTop > 0) {
                topHeight -= clipTop;
                sourceY = clipTop;
            }
            const int clipBottom = y + bottomHeight - face.lightmapSize[1];
            if (clipBottom > 0) {
                bottomHeight -= clipBottom;
            }
            if (y - topHeight >= y + bottomHeight || y - topHeight >= face.lightmapSize[1] ||
                y + bottomHeight <= 0) {
                continue;
            }
            if (flipU) {
                sourceX = static_cast<int>(texture->GetWidth()) - sourceX - 1;
            }
            if (flipV) {
                sourceY = static_cast<int>(texture->GetHeight()) - sourceY - 1;
            }
            const int startX = x - leftWidth;
            const int startY = y - topHeight;
            for (int dy = 0; dy < topHeight + bottomHeight; ++dy) {
                for (int dx = 0; dx < leftWidth + rightWidth; ++dx) {
                    const int textureX = sourceX + (flipU ? -dx : dx);
                    const int textureY = sourceY + (flipV ? -dy : dy);
                    if (textureX < 0 || textureY < 0 ||
                        textureX >= static_cast<int>(texture->GetWidth()) ||
                        textureY >= static_cast<int>(texture->GetHeight())) {
                        continue;
                    }
                    const uint8_t index = texture->GetPixels()[
                        static_cast<std::size_t>(textureY) * texture->GetWidth() + textureX];
                    if (index == texture->GetTransparentColor()) {
                        continue;
                    }
                    const int destinationX = startX + dx;
                    const int destinationY = startY + dy;
                    const int destination = destinationY * face.lightmapPowerOfTwoWidth + destinationX;
                    if (destinationX < 0 || destinationY < 0 || destination < 0 ||
                        static_cast<std::size_t>(destination) >= face.lightmapPixels.size()) {
                        continue;
                    }
                    uint16_t& pixel = face.lightmapPixels[static_cast<std::size_t>(destination)];
                    pixel = Blend50(pixel, BloodPaletteColor(texture, index));
                }
            }
            return 1;
        }
        return visit(node.children[1 - firstSide], middle, last);
    };
    visit(0, start, end);
}

} // namespace soft3d
