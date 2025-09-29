#include "ChunkMesher.hpp"
#include "engine/world/BlockDef.hpp"

#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <vector>

static inline void pushQuadAO(Mesh& m, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n, glm::vec2 uv0,
                              glm::vec2 uv1, glm::vec4 rect, float ao00, float ao10, float ao11, float ao01,
                              float aoStrength) {
  uint32_t base = (uint32_t)m.vertices.size();
  m.vertices.push_back({a, n, {uv0.x, uv0.y}, rect, {ao00, aoStrength, 1}});
  m.vertices.push_back({b, n, {uv1.x, uv0.y}, rect, {ao10, aoStrength, 1}});
  m.vertices.push_back({c, n, {uv1.x, uv1.y}, rect, {ao11, aoStrength, 1}});
  m.vertices.push_back({d, n, {uv0.x, uv1.y}, rect, {ao01, aoStrength, 1}});

  float diag0 = ao00 + ao11;
  float diag1 = ao10 + ao01;
  if (diag0 < diag1) {
    m.indices.insert(m.indices.end(), {base + 0, base + 1, base + 2, base + 0, base + 2, base + 3});
  } else {
    m.indices.insert(m.indices.end(), {base + 0, base + 1, base + 3, base + 1, base + 2, base + 3});
  }
}

static inline float aoShade(bool side1, bool side2, bool corner) {
  int occ = int(side1) + int(side2);
  if (corner && (side1 || side2)) occ = 3;
  static const float lut[4] = {1.00f, 0.85f, 0.70f, 0.55f};
  return lut[occ];
}

namespace {
template <int AXIS>
void greedyAxis(const Chunk& ch, const World& world, Mesh& out) {
  constexpr int U = (AXIS == 0) ? 1 : 0;
  constexpr int V = (AXIS == 2) ? 1 : 2;

  const int dimA = (AXIS == 0) ? CHUNK_X : (AXIS == 1 ? CHUNK_Y : CHUNK_Z);
  const int dimU = (U == 0) ? CHUNK_X : (U == 1 ? CHUNK_Y : CHUNK_Z);
  const int dimV = (V == 0) ? CHUNK_X : (V == 1 ? CHUNK_Y : CHUNK_Z);

  const int ox = ch.coord.cx * CHUNK_X;
  const int oz = ch.coord.cz * CHUNK_Z;

  std::vector<uint8_t> mask(dimU * dimV);
  auto at = [&](int u, int v) -> uint8_t& { return mask[v * dimU + u]; };

  auto sampleVoxel = [&](int X, int Y, int Z) -> uint8_t {
    if (X >= 0 && X < CHUNK_X && Y >= 0 && Y < CHUNK_Y && Z >= 0 && Z < CHUNK_Z) return ch.getLocal(X, Y, Z);
    return world.getFast(ox + X, Y, oz + Z);
  };

  auto solidFast = [&](int X, int Y, int Z) -> bool {
    uint8_t id = sampleVoxel(X, Y, Z);
    return gBlocks[id].opaque;
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

        uint8_t id1 = (a > 0) ? sampleVoxel(x, y, z) : AIR;
        uint8_t id2 = (a < dimA) ? sampleVoxel(x2, y2, z2) : AIR;

        bool s1 = gBlocks[id1].opaque;
        bool s2 = gBlocks[id2].opaque;

        if (s1 == s2) {
          at(u, v) = 0;
          continue;
        }

        uint8_t id = s1 ? id1 : id2;
        if (s2) id |= 0x80;
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
          if (!stop) ++h;
        }

        const bool neg = (m0 & 0x80) != 0;
        const uint8_t idv = (m0 & 0x7F);
        const BlockDef& bd = gBlocks[idv];

        int face = (AXIS == 1) ? (neg ? 2 : 3) : (AXIS == 0 ? (neg ? 0 : 1) : (neg ? 4 : 5));
        glm::vec4 rect = tileRectGrid(bd.tile[face]);

        glm::vec3 O, eU, eV, nPlus;
        if constexpr (AXIS == 0) {
          O = {float(ox + a), float(u), float(oz + v)};
          eU = {0, 1, 0};
          eV = {0, 0, 1};
          nPlus = {1, 0, 0};
        } else if constexpr (AXIS == 1) {
          O = {float(ox + u), float(a), float(oz + v)};
          eU = {1, 0, 0};
          eV = {0, 0, 1};
          nPlus = {0, 1, 0};
        } else {
          O = {float(ox + u), float(v), float(oz + a)};
          eU = {1, 0, 0};
          eV = {0, 1, 0};
          nPlus = {0, 0, 1};
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
        if (neg) std::swap(B, D);

        auto aoCorner = [&](int baseX, int baseY, int baseZ, int duX, int duY, int duZ, int dvX, int dvY, int dvZ,
                            int nX, int nY, int nZ) {
          bool side1 = solidFast(baseX + duX, baseY + duY, baseZ + duZ);
          bool side2 = solidFast(baseX + dvX, baseY + dvY, baseZ + dvZ);
          bool corn = solidFast(baseX + duX + dvX, baseY + duY + dvY, baseZ + duZ + dvZ);
          return aoShade(side1, side2, corn);
        };

        auto toLocal = [&](glm::vec3 p) {
          return glm::ivec3(int(std::floor(p.x)) - ox, int(std::floor(p.y)), int(std::floor(p.z)) - oz);
        };

        glm::ivec3 iA = toLocal(A), iB = toLocal(B), iC = toLocal(C), iD = toLocal(D);
        glm::ivec3 N = glm::ivec3(n.x, n.y, n.z);
        glm::ivec3 dU = glm::ivec3(eU);
        glm::ivec3 dV = glm::ivec3(eV);

        auto aoA =
            aoCorner(iA.x - N.x, iA.y - N.y, iA.z - N.z, -dU.x, -dU.y, -dU.z, -dV.x, -dV.y, -dV.z, N.x, N.y, N.z);
        auto aoB = aoCorner(iB.x - N.x, iB.y - N.y, iB.z - N.z, dU.x, dU.y, dU.z, -dV.x, -dV.y, -dV.z, N.x, N.y, N.z);
        auto aoC = aoCorner(iC.x - N.x, iC.y - N.y, iC.z - N.z, dU.x, dU.y, dU.z, dV.x, dV.y, dV.z, N.x, N.y, N.z);
        auto aoD = aoCorner(iD.x - N.x, iD.y - N.y, iD.z - N.z, -dU.x, -dU.y, -dU.z, dV.x, dV.y, dV.z, N.x, N.y, N.z);

        glm::vec2 uv0{0.0f, 0.0f};
        glm::vec2 uvMax{float(w), float(h)};
        pushQuadAO(out, A, B, C, D, n, uv0, uvMax, rect, aoA, aoB, aoC, aoD, bd.aoStrength);

        for (int vv = 0; vv < h; ++vv)
          for (int uu = 0; uu < w; ++uu)
            at(u + uu, v + vv) = 0;

        u += w;
      }
    }
  }
}
} // namespace

void ChunkMesher::build(const Chunk& ch, const World& world, Mesh& out) {
  out.clear();
  greedyAxis<0>(ch, world, out);
  greedyAxis<1>(ch, world, out);
  greedyAxis<2>(ch, world, out);
}
