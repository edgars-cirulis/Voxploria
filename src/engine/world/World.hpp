#pragma once
#include "Chunk.hpp"
#include "WorldGen.hpp"
#include "engine/gfx/Mesh.hpp"
#include "engine/world/VoxelTypes.hpp"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

struct World {
  WorldGen gen;
  explicit World(uint64_t seed) : gen(seed) {}

  Chunk* getOrCreate(ChunkCoord cc);
  Voxel get(int wx, int wy, int wz);
  void set(int wx, int wy, int wz, Voxel v);
  bool isSolid(int wx, int wy, int wz) {
    return get(wx, wy, wz) != AIR;
  }

  Voxel getFast(int wx, int wy, int wz) const;
  bool isSolidFast(int wx, int wy, int wz) const {
    return getFast(wx, wy, wz) != AIR;
  }

  void update(const glm::vec3& camPos, int radiusChunks);

  void collectDirtyMeshes(std::vector<std::pair<ChunkCoord, Mesh>>& out);

  void listLoaded(std::vector<ChunkCoord>& out) const;

private:
  struct ChunkKeyHashLocal {
    size_t operator()(const ChunkCoord& c) const {
      return (uint64_t(uint32_t(c.cx)) << 32) ^ uint32_t(c.cz);
    }
  };
  std::unordered_map<ChunkCoord, Chunk, ChunkKeyHashLocal> chunks;
};
