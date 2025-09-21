#pragma once
#include <cmath>
#include <functional>
#include <glm/glm.hpp>

struct PlayerBody {
  glm::vec3 pos{0, 50, 0};
  glm::vec3 vel{0};
  float width = 0.6f;
  float height = 1.8f;
  bool onGround = false;
};

struct PhysParams {
  float gravity = 28.0f;
  float maxSpeed = 12.0f;
  float jumpSpeed = 10.0f;

  float dragGround = 6.0f;
  float dragAir = 1.0f;
};

inline bool aabbIntersectsSolid(const std::function<bool(int, int, int)>& solid, const glm::vec3& minB,
                                const glm::vec3& maxB) {
  int x0 = (int)std::floor(minB.x), x1 = (int)std::floor(maxB.x);
  int y0 = (int)std::floor(minB.y), y1 = (int)std::floor(maxB.y);
  int z0 = (int)std::floor(minB.z), z1 = (int)std::floor(maxB.z);
  for (int z = z0; z <= z1; ++z)
    for (int y = y0; y <= y1; ++y)
      for (int x = x0; x <= x1; ++x)
        if (solid(x, y, z)) return true;
  return false;
}

inline void moveAxis(PlayerBody& p, float dx, float dy, float dz, const std::function<bool(int, int, int)>& solid) {
  glm::vec3 half = {p.width * 0.5f, p.height * 0.5f, p.width * 0.5f};
  glm::vec3 minB = p.pos - half;
  glm::vec3 maxB = p.pos + half;

  if (dx != 0.0f) {
    minB.x += dx;
    maxB.x += dx;
    if (aabbIntersectsSolid(solid, minB, maxB)) {
      int blockX = (int)std::floor(dx > 0 ? maxB.x : minB.x);
      p.pos.x = dx > 0 ? (blockX - half.x) - 1e-4f : (blockX + 1 + half.x) + 1e-4f;
      p.vel.x = 0.0f;
    } else
      p.pos.x += dx;
    minB = p.pos - half;
    maxB = p.pos + half;
  }
  if (dy != 0.0f) {
    minB.y += dy;
    maxB.y += dy;
    if (aabbIntersectsSolid(solid, minB, maxB)) {
      int blockY = (int)std::floor(dy > 0 ? maxB.y : minB.y);
      p.pos.y = dy > 0 ? (blockY - half.y) - 1e-4f : (blockY + 1 + half.y) + 1e-4f;
      p.vel.y = 0.0f;
      p.onGround = dy < 0;
    } else {
      p.pos.y += dy;
      if (dy < 0) p.onGround = false;
    }
    minB = p.pos - half;
    maxB = p.pos + half;
  }
  if (dz != 0.0f) {
    minB.z += dz;
    maxB.z += dz;
    if (aabbIntersectsSolid(solid, minB, maxB)) {
      int blockZ = (int)std::floor(dz > 0 ? maxB.z : minB.z);
      p.pos.z = dz > 0 ? (blockZ - half.z) - 1e-4f : (blockZ + 1 + half.z) + 1e-4f;
      p.vel.z = 0.0f;
    } else
      p.pos.z += dz;
  }
}

inline void simulate(PlayerBody& p, float dt, const std::function<bool(int, int, int)>& solid, const PhysParams& par) {
  p.vel.y -= par.gravity * dt;

  glm::vec2 v2(p.vel.x, p.vel.z);
  float sp = glm::length(v2);
  if (sp > par.maxSpeed) v2 = (v2 / sp) * par.maxSpeed;
  p.vel.x = v2.x;
  p.vel.z = v2.y;

  moveAxis(p, p.vel.x * dt, 0, 0, solid);
  moveAxis(p, 0, p.vel.y * dt, 0, solid);
  moveAxis(p, 0, 0, p.vel.z * dt, solid);

  float drag = p.onGround ? par.dragGround : par.dragAir;
  float k = std::exp(-drag * dt);
  p.vel.x *= k;
  p.vel.z *= k;
}
