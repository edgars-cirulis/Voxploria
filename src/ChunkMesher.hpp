#pragma once
#include "Chunk.hpp"
#include "Mesh.hpp"
#include "World.hpp"

namespace ChunkMesher {
void build(const Chunk& c, const World& world, Mesh& out);
}
