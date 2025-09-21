#include "Lighting.hpp"

#include <algorithm>
#include <queue>

void computeSkylight(LightVolume& L, int sx, int sy, int sz, const std::function<bool(int, int, int)>& solid) {
  L.sx = sx;
  L.sy = sy;
  L.sz = sz;
  L.skylight.assign(size_t(sx * sy * sz), 0);

  struct Node {
    int x, y, z;
  };
  std::queue<Node> q;

  for (int z = 0; z < sz; ++z) {
    for (int x = 0; x < sx; ++x) {
      int y = sy - 1;
      if (solid(x, y, z)) continue;
      L.skylight[L.idx(x, y, z)] = 15;
      q.push({x, y, z});
    }
  }

  auto tryPush = [&](int nx, int ny, int nz, int newLevel) {
    if (nx < 0 || ny < 0 || nz < 0 || nx >= sx || ny >= sy || nz >= sz) return;
    auto& dst = L.skylight[L.idx(nx, ny, nz)];
    if (newLevel > dst && !solid(nx, ny, nz)) {
      dst = (uint8_t)newLevel;
      q.push({nx, ny, nz});
    }
  };

  while (!q.empty()) {
    auto [x, y, z] = q.front();
    q.pop();
    int lv = L.skylight[L.idx(x, y, z)];
    if (lv <= 1) continue;
    tryPush(x + 1, y, z, lv - 1);
    tryPush(x - 1, y, z, lv - 1);
    tryPush(x, y + 1, z, lv - 1);
    tryPush(x, y - 1, z, lv - 1);
    tryPush(x, y, z + 1, lv - 1);
    tryPush(x, y, z - 1, lv - 1);

    if (y > 0 && L.get(x, y + 1, z) == 15) tryPush(x, y - 1, z, 14);
  }

  for (int z = 0; z < sz; ++z) {
    for (int x = 0; x < sx; ++x) {
      uint8_t lv = 15;
      for (int y = sy - 1; y >= 0; --y) {
        if (solid(x, y, z)) {
          lv = 0;
        } else {
          if (L.skylight[L.idx(x, y, z)] < lv) L.skylight[L.idx(x, y, z)] = lv;
          if (lv > 0) --lv;
        }
      }
    }
  }
}
