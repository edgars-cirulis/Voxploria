#include "WorldGen.hpp"
#include <algorithm>

static inline int worldX(const Chunk& c, int lx)
{
    return c.coord.cx * CHUNK_X + lx;
}
static inline int worldZ(const Chunk& c, int lz)
{
    return c.coord.cz * CHUNK_Z + lz;
}

void WorldGen::buildChunk(Chunk& c) const
{
    for (int z = 0; z < CHUNK_Z; ++z) {
        for (int x = 0; x < CHUNK_X; ++x) {
            int wx = worldX(c, x), wz = worldZ(c, z);

            float hBase = perlin.fbm(wx, wz, 5, 0.0035f, 0.5f);
            float hills = perlin.fbm(wx + 1000, wz - 1000, 3, 0.008f, 0.5f);
            float h = 48.0f + hBase * 22.0f + hills * 8.0f;
            int H = std::clamp(int(std::round(h)), 4, CHUNK_Y - 2);

            for (int y = 0; y < CHUNK_Y; ++y) {
                Voxel id = AIR;
                if (y <= H - 4)
                    id = STONE;
                else if (y <= H - 1)
                    id = DIRT;
                else if (y == H)
                    id = GRASS;
                if (y < 38)
                    id = (y <= 35 ? STONE : SAND);
                c.vox[Chunk::idx(x, y, z)] = id;
            }

            const int SEA = 40;
            for (int y = H + 1; y < SEA && y < CHUNK_Y; ++y)
                c.vox[Chunk::idx(x, y, z)] = WATER;
        }
    }
    c.dirtyCPU = true;
}