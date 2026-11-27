#include "soft3d/s3dBaseMath.hpp"

namespace soft3d {

int32_t GetPow2(uint32_t value)
{
    if (value == 1) {
        return 0;
    }
    if (value > 0x80000000U) {
        return -1;
    }
    int32_t exponent = 1;
    uint32_t current = 2;
    while (current < value) {
        current <<= 1;
        ++exponent;
    }
    return current == value ? exponent : -1;
}

uint32_t ConvPow2(uint32_t value)
{
    if (((0U - value) & value) == value) {
        return value;
    }
    if (value > 0x80000000U) {
        return 0;
    }
    uint32_t result = 2;
    while (result < value) {
        result <<= 1;
    }
    return result;
}

namespace {

template <typename T>
void SwapAt(T* base, int32_t left, int32_t right)
{
    if (left != right) {
        const T temporary = base[left];
        base[left] = base[right];
        base[right] = temporary;
    }
}

template <typename T>
void SortSmall(T* base, int32_t first, int32_t last, CompareFunction compare)
{
    for (int32_t end = last; first < end; --end) {
        int32_t selected = first;
        for (int32_t current = first + 1; current <= end; ++current) {
            if (compare(base + current, base + selected) > 0) {
                selected = current;
            }
        }
        SwapAt(base, selected, end);
    }
}

template <typename T>
void QsortWidth(T* base, int32_t count, CompareFunction compare)
{
    int32_t pendingLeft[30];
    int32_t pendingRight[30];
    int32_t depth = 0;
    int32_t first = 0;
    int32_t last = count - 1;

    for (;;) {
        while (last - first + 1 < 9) {
            SortSmall(base, first, last, compare);
            --depth;
            if (depth < 0) {
                return;
            }
            first = pendingLeft[depth];
            last = pendingRight[depth];
        }

        int32_t median = first + ((last - first + 1) >> 1);
        if (compare(base + first, base + median) > 0) {
            SwapAt(base, first, median);
        }
        if (compare(base + first, base + last) > 0) {
            SwapAt(base, first, last);
        }
        if (compare(base + median, base + last) > 0) {
            SwapAt(base, median, last);
        }

        int32_t left = first;
        int32_t right = last;
        for (;;) {
            if (left < median) {
                do {
                    ++left;
                    if (median <= left) {
                        break;
                    }
                } while (compare(base + left, base + median) < 1);
            }
            if (median <= left) {
                do {
                    ++left;
                    if (last < left) {
                        break;
                    }
                } while (compare(base + left, base + median) < 1);
            }
            do {
                --right;
                if (right <= median) {
                    break;
                }
            } while (compare(base + right, base + median) > 0);
            if (left <= right) {
                SwapAt(base, left, right);
                if (median == right) {
                    median = left;
                }
                continue;
            }

            ++right;
            if (median < right) {
                do {
                    --right;
                    if (right <= median) {
                        break;
                    }
                } while (compare(base + right, base + median) == 0);
            }
            if (right <= median) {
                do {
                    --right;
                    if (right <= first) {
                        break;
                    }
                } while (compare(base + right, base + median) == 0);
            }
            break;
        }

        if (right - first < last - left) {
            if (left < last) {
                pendingLeft[depth] = left;
                pendingRight[depth] = last;
                ++depth;
            }
            if (right <= first) {
                --depth;
                if (depth < 0) {
                    return;
                }
                first = pendingLeft[depth];
                last = pendingRight[depth];
                continue;
            }
            last = right;
            continue;
        }

        if (first < right) {
            pendingLeft[depth] = first;
            pendingRight[depth] = right;
            ++depth;
        }
        if (last <= left) {
            --depth;
            if (depth < 0) {
                return;
            }
            first = pendingLeft[depth];
            last = pendingRight[depth];
            continue;
        }
        first = left;
    }
}

} // namespace

void Qsort(void* base, int32_t count, int32_t width, CompareFunction compare)
{
    if (count <= 1) {
        return;
    }
    if (width == 1) {
        QsortWidth(static_cast<uint8_t*>(base), count, compare);
    } else if (width == 2) {
        QsortWidth(static_cast<uint16_t*>(base), count, compare);
    } else if (width == 4) {
        QsortWidth(static_cast<uint32_t*>(base), count, compare);
    }
}

void RandomGenerator::SetSeed(uint32_t value)
{
    seed = value;
}

int32_t RandomGenerator::Next()
{
    seed = seed * 0x343fdU + 0x269ec3U;
    return static_cast<int32_t>((seed >> 16) & 0x7fffU);
}

} // namespace soft3d
