#pragma once
#include <cstdint>
#include <glm/glm.hpp>

struct Perlin {
  explicit Perlin(uint64_t seed = 0);
  float noise(float x, float y) const;
  float fbm(float x, float y, int oct = 5, float freq = 0.003f, float gain = 0.5f) const;

private:
  uint8_t p[512];
};
