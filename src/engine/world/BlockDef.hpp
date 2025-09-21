#pragma once
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

enum Face : uint8_t { FXN = 0, FXP = 1, FYN = 2, FYP = 3, FZN = 4, FZP = 5 };

constexpr int ATLAS_COLS = 4;
constexpr int ATLAS_ROWS = 4;

struct BlockDef {
  bool opaque{true};
  bool translucent{false};
  bool cutout{false};
  std::array<uint8_t, 6> tile{};
  float aoStrength{1.0f};
};

extern BlockDef gBlocks[256];

inline glm::vec4 tileRectGrid(int tileIndex) {
  int tx = tileIndex % ATLAS_COLS;
  int ty = tileIndex / ATLAS_COLS;
  float Tux = 1.0f / float(ATLAS_COLS);
  float Tuy = 1.0f / float(ATLAS_ROWS);
  const float e = 0.0005f;
  float u0 = tx * Tux + e, v0 = ty * Tuy + e;
  float u1 = (tx + 1) * Tux - e, v1 = (ty + 1) * Tuy - e;
  return {u0, v0, u1, v1};
}
