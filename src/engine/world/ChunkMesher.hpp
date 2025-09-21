#pragma once
#include "Chunk.hpp"
#include "World.hpp"
#include "engine/gfx/Mesh.hpp"

namespace ChunkMesher {
void build(const Chunk& c, const World& world, Mesh& out);
}
