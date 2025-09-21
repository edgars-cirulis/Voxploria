#include "Noise.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

static inline float fade(float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}
static inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}
static inline float grad(int h, float x, float y) {
  int g = h & 3;
  switch (g) {
  case 0:
    return x + y;
  case 1:
    return -x + y;
  case 2:
    return x - y;
  default:
    return -x - y;
  }
}
Perlin::Perlin(uint64_t seed) {
  std::vector<int> perm(256);
  std::iota(perm.begin(), perm.end(), 0);
  std::mt19937_64 rng(seed);
  std::shuffle(perm.begin(), perm.end(), rng);
  for (int i = 0; i < 256; ++i) {
    p[i] = uint8_t(perm[i]);
    p[256 + i] = p[i];
  }
}
float Perlin::noise(float x, float y) const {
  int X = int(floor(x)) & 255, Y = int(floor(y)) & 255;
  x -= floor(x);
  y -= floor(y);
  float u = fade(x), v = fade(y);
  int aa = p[p[X] + Y], ab = p[p[X] + Y + 1], ba = p[p[X + 1] + Y], bb = p[p[X + 1] + Y + 1];
  float n00 = grad(aa, x, y), n10 = grad(ba, x - 1, y);
  float n01 = grad(ab, x, y - 1), n11 = grad(bb, x - 1, y - 1);
  float ix0 = lerp(n00, n10, u), ix1 = lerp(n01, n11, u);
  return lerp(ix0, ix1, v) * 0.7071f;
}
float Perlin::fbm(float x, float y, int oct, float freq, float gain) const {
  float a = 1.0f, f = freq, sum = 0.0f, norm = 0.0f;
  for (int i = 0; i < oct; ++i) {
    sum += a * noise(x * f, y * f);
    norm += a;
    a *= gain;
    f *= 2.0f;
  }
  return sum / (norm > 0 ? norm : 1.0f);
}
