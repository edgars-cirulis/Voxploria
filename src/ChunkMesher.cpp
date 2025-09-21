#include "ChunkMesher.hpp"
#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <vector>

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

static inline void
pushQuad(Mesh& m, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n, glm::vec2 uv0, glm::vec2 uv1, glm::vec4 rect)
{
    uint32_t base = (uint32_t)m.vertices.size();
    m.vertices.push_back({ a, n, { uv0.x, uv0.y }, rect, { 1, 1, 1 } });
    m.vertices.push_back({ b, n, { uv1.x, uv0.y }, rect, { 1, 1, 1 } });
    m.vertices.push_back({ c, n, { uv1.x, uv1.y }, rect, { 1, 1, 1 } });
    m.vertices.push_back({ d, n, { uv0.x, uv1.y }, rect, { 1, 1, 1 } });
    m.indices.insert(m.indices.end(), { base + 0, base + 1, base + 2, base + 0, base + 2, base + 3 });
}

namespace {
template <int AXIS>
void greedyAxis(const Chunk& ch, const World& world, Mesh& out)
{
    constexpr int U = (AXIS == 0) ? 1 : 0;
    constexpr int V = (AXIS == 2) ? 1 : 2;

    const int dimA = (AXIS == 0) ? CHUNK_X : (AXIS == 1 ? CHUNK_Y : CHUNK_Z);
    const int dimU = (U == 0) ? CHUNK_X : (U == 1 ? CHUNK_Y : CHUNK_Z);
    const int dimV = (V == 0) ? CHUNK_X : (V == 1 ? CHUNK_Y : CHUNK_Z);

    const int ox = ch.coord.cx * CHUNK_X;
    const int oz = ch.coord.cz * CHUNK_Z;

    std::vector<uint8_t> mask(dimU * dimV);
    auto at = [&](int u, int v) -> uint8_t& { return mask[v * dimU + u]; };

    auto sample = [&](int X, int Y, int Z) -> uint8_t {
        if (X >= 0 && X < CHUNK_X && Y >= 0 && Y < CHUNK_Y && Z >= 0 && Z < CHUNK_Z)
            return ch.getLocal(X, Y, Z);

        return world.getFast(ox + X, Y, oz + Z);
    };

    for (int a = 0; a <= dimA; ++a) {
        std::fill(mask.begin(), mask.end(), 0);

        for (int v = 0; v < dimV; ++v)
            for (int u = 0; u < dimU; ++u) {
                int x = 0, y = 0, z = 0, x2 = 0, y2 = 0, z2 = 0;
                if constexpr (AXIS == 0) {
                    x = a - 1;
                    y = u;
                    z = v;
                    x2 = a;
                    y2 = u;
                    z2 = v;
                }
                if constexpr (AXIS == 1) {
                    x = u;
                    y = a - 1;
                    z = v;
                    x2 = u;
                    y2 = a;
                    z2 = v;
                }
                if constexpr (AXIS == 2) {
                    x = u;
                    y = v;
                    z = a - 1;
                    x2 = u;
                    y2 = v;
                    z2 = a;
                }

                uint8_t id1 = (a > 0) ? sample(x, y, z) : AIR;
                uint8_t id2 = (a < dimA) ? sample(x2, y2, z2) : AIR;

                bool s1 = id1 != AIR, s2 = id2 != AIR;
                if (s1 == s2) {
                    at(u, v) = 0;
                    continue;
                }

                uint8_t id = s1 ? id1 : id2;
                if (s2)
                    id |= 0x80;
                at(u, v) = id;
            }

        for (int v = 0; v < dimV; ++v) {
            for (int u = 0; u < dimU;) {
                uint8_t m0 = at(u, v);
                if (m0 == 0) {
                    ++u;
                    continue;
                }

                int w = 1;
                while (u + w < dimU && at(u + w, v) == m0)
                    ++w;
                int h = 1;
                bool stop = false;
                while (v + h < dimV && !stop) {
                    for (int k = 0; k < w; ++k) {
                        if (at(u + k, v + h) != m0) {
                            stop = true;
                            break;
                        }
                    }
                    if (!stop)
                        ++h;
                }

                const bool neg = (m0 & 0x80) != 0;
                const int idv = int(m0 & 0x7F);
                int face = (AXIS == 1) ? (neg ? 2 : 3) : (AXIS == 0 ? (neg ? 0 : 1) : (neg ? 4 : 5));
                glm::vec4 rect = tileRect(idv, face);

                glm::vec3 O, eU, eV, nPlus;
                if constexpr (AXIS == 0) {
                    O = glm::vec3(ox + a, float(u), oz + v);
                    eU = glm::vec3(0, 1, 0);
                    eV = glm::vec3(0, 0, 1);
                    nPlus = glm::vec3(1, 0, 0);
                } else if constexpr (AXIS == 1) {
                    O = glm::vec3(ox + u, float(a), oz + v);
                    eU = glm::vec3(1, 0, 0);
                    eV = glm::vec3(0, 0, 1);
                    nPlus = glm::vec3(0, 1, 0);
                } else {
                    O = glm::vec3(ox + u, float(v), oz + a);
                    eU = glm::vec3(1, 0, 0);
                    eV = glm::vec3(0, 1, 0);
                    nPlus = glm::vec3(0, 0, 1);
                }

                glm::vec3 P00 = O;
                glm::vec3 P10 = O + eU * float(w);
                glm::vec3 P01 = O + eV * float(h);
                glm::vec3 P11 = O + eU * float(w) + eV * float(h);

                glm::vec3 A, B, C, D;
                if constexpr (AXIS == 1) {
                    A = P00;
                    B = P01;
                    C = P11;
                    D = P10;
                } else {
                    A = P00;
                    B = P10;
                    C = P11;
                    D = P01;
                }

                glm::vec3 n = neg ? -nPlus : nPlus;
                if (neg)
                    std::swap(B, D);

                glm::vec2 uv0{ 0.0f, 0.0f };
                glm::vec2 uvMax{ float(w), float(h) };
                pushQuad(out, A, B, C, D, n, uv0, uvMax, rect);

                for (int vv = 0; vv < h; ++vv)
                    for (int uu = 0; uu < w; ++uu)
                        at(u + uu, v + vv) = 0;

                u += w;
            }
        }
    }
}
}  // namespace

void ChunkMesher::build(const Chunk& ch, const World& world, Mesh& out)
{
    out.clear();
    greedyAxis<0>(ch, world, out);
    greedyAxis<1>(ch, world, out);
    greedyAxis<2>(ch, world, out);
}
