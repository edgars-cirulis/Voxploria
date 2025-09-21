#pragma once
#include <cstdint>
#include <functional>
#include <vector>

struct LightVolume {
    int sx = 0, sy = 0, sz = 0;
    std::vector<uint8_t> skylight;
    inline int idx(int x, int y, int z) const { return (y * sz + z) * sx + x; }
    uint8_t get(int x, int y, int z) const
    {
        if (x < 0 || y < 0 || z < 0 || x >= sx || y >= sy || z >= sz)
            return 0;
        return skylight[idx(x, y, z)];
    }
};

void computeSkylight(LightVolume& L, int sx, int sy, int sz, const std::function<bool(int, int, int)>& solid);
