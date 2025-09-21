#pragma once
#include "Chunk.hpp"
#include "Noise.hpp"
#include "engine/world/VoxelTypes.hpp"

struct WorldGen {
  uint64_t seed{12345};
  Perlin perlin{seed};

  explicit WorldGen(uint64_t s) : seed(s), perlin(s) {}
  void buildChunk(Chunk& chunk) const;
};
