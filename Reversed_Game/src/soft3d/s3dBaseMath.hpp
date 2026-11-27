#pragma once

#include <cstdint>

namespace soft3d {

int32_t GetPow2(uint32_t value);
uint32_t ConvPow2(uint32_t value);

using CompareFunction = int (*)(const void* left, const void* right);
void Qsort(void* base, int32_t count, int32_t width, CompareFunction compare);

class RandomGenerator {
public:
    void SetSeed(uint32_t seed);
    int32_t Next();

private:
    uint32_t seed;
};

} // namespace soft3d
