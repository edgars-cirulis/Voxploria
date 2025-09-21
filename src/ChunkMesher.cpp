#include "ChunkMesher.hpp"
#include <glm/glm.hpp>

static inline glm::vec4 tileRect(int id, int face)
{
    int tx = 0, ty = 0;
    if (id == GRASS) {
        if (face == 3) {
            tx = 2;
            ty = 0;
        } else if (face == 2) {
            tx = 1;
            ty = 0;
        } else {
            tx = 3;
            ty = 0;
        }
    } else if (id == DIRT) {
        tx = 1;
        ty = 0;
    } else if (id == SAND) {
        tx = 0;
        ty = 1;
    } else if (id == WATER) {
        tx = 1;
        ty = 1;
    } else {
        tx = 0;
        ty = 0;
    }
    const float Tux = 1.0f / 4.0f, Tuy = 1.0f / 4.0f, e = 0.0005f;
    float u0 = tx * Tux + e, v0 = ty * Tuy + e, u1 = (tx + 1) * Tux - e, v1 = (ty + 1) * Tuy - e;
    return { u0, v0, u1, v1 };
}

static inline void push(Mesh& m, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n, glm::vec4 rect)
{
    uint32_t base = (uint32_t)m.vertices.size();
    m.vertices.push_back({ a, n, { 0, 0 }, rect, { 1, 1, 1 } });
    m.vertices.push_back({ b, n, { 1, 0 }, rect, { 1, 1, 1 } });
    m.vertices.push_back({ c, n, { 1, 1 }, rect, { 1, 1, 1 } });
    m.vertices.push_back({ d, n, { 0, 1 }, rect, { 1, 1, 1 } });
    m.indices.insert(m.indices.end(), { base + 0, base + 1, base + 2, base + 0, base + 2, base + 3 });
}

void ChunkMesher::build(const Chunk& ch, const World& world, Mesh& out)
{
    out.clear();
    const int ox = ch.coord.cx * CHUNK_X;
    const int oz = ch.coord.cz * CHUNK_Z;
    auto solid = [&](int wx, int wy, int wz) { return world.isSolidFast(wx, wy, wz); };

    for (int z = 0; z < CHUNK_Z; ++z)
        for (int y = 0; y < CHUNK_Y; ++y)
            for (int x = 0; x < CHUNK_X; ++x) {
                Voxel id = ch.getLocal(x, y, z);
                if (id == AIR)
                    continue;
                int wx = ox + x, wy = y, wz = oz + z;

                if (!solid(wx - 1, wy, wz))
                    push(out, { wx, wy, wz }, { wx, wy, wz + 1 }, { wx, wy + 1, wz + 1 }, { wx, wy + 1, wz }, { -1, 0, 0 },
                         tileRect(id, 0));
                if (!solid(wx + 1, wy, wz))
                    push(out, { wx + 1, wy, wz + 1 }, { wx + 1, wy, wz }, { wx + 1, wy + 1, wz }, { wx + 1, wy + 1, wz + 1 }, { 1, 0, 0 },
                         tileRect(id, 1));
                if (!solid(wx, wy - 1, wz))
                    push(out, { wx, wy, wz }, { wx + 1, wy, wz }, { wx + 1, wy, wz + 1 }, { wx, wy, wz + 1 }, { 0, -1, 0 },
                         tileRect(id, 2));
                if (!solid(wx, wy + 1, wz))
                    push(out, { wx, wy + 1, wz + 1 }, { wx + 1, wy + 1, wz + 1 }, { wx + 1, wy + 1, wz }, { wx, wy + 1, wz }, { 0, 1, 0 },
                         tileRect(id, 3));
                if (!solid(wx, wy, wz - 1))
                    push(out, { wx + 1, wy, wz }, { wx, wy, wz }, { wx, wy + 1, wz }, { wx + 1, wy + 1, wz }, { 0, 0, -1 },
                         tileRect(id, 4));
                if (!solid(wx, wy, wz + 1))
                    push(out, { wx, wy, wz + 1 }, { wx + 1, wy, wz + 1 }, { wx + 1, wy + 1, wz + 1 }, { wx, wy + 1, wz + 1 }, { 0, 0, 1 },
                         tileRect(id, 5));
            }
}
