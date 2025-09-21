#pragma once
#include <vector>
#include "Types.hpp"

struct Chunk {
    ChunkCoord coord{};
    std::vector<Voxel> vox;
    bool dirtyCPU = true;
    bool dirtyGPU = false;

    Chunk(ChunkCoord c = {}) : coord(c), vox(CHUNK_X * CHUNK_Y * CHUNK_Z, AIR) {}

    static inline int idx(int x, int y, int z) { return (y * CHUNK_Z + z) * CHUNK_X + x; }
    Voxel getLocal(int x, int y, int z) const
    {
        if (x < 0 || y < 0 || z < 0 || x >= CHUNK_X || y >= CHUNK_Y || z >= CHUNK_Z)
            return AIR;
        return vox[idx(x, y, z)];
    }
    void setLocal(int x, int y, int z, Voxel v)
    {
        if (x < 0 || y < 0 || z < 0 || x >= CHUNK_X || y >= CHUNK_Y || z >= CHUNK_Z)
            return;
        vox[idx(x, y, z)] = v;
        dirtyCPU = true;
    }
};
