#include "BlockDef.hpp"
#include "engine/world/VoxelTypes.hpp"

BlockDef gBlocks[256];

static void setCube(BlockDef& b, uint8_t allTile) {
  b.tile = {allTile, allTile, allTile, allTile, allTile, allTile};
}

static void setGrass(BlockDef& b, uint8_t side, uint8_t bottom, uint8_t top) {
  b.tile = {side, side, bottom, top, side, side};
}

__attribute__((constructor)) static void initBlocks() {
  for (auto& b : gBlocks) {
    b = {};
    setCube(b, 0);
  }

  gBlocks[AIR].opaque = false;

  setCube(gBlocks[STONE], /*allTile*/ 0);
  gBlocks[STONE].aoStrength = 1.00f;

  setCube(gBlocks[DIRT], 1);
  gBlocks[DIRT].aoStrength = 0.95f;

  setGrass(gBlocks[GRASS], /*side*/ 3, /*bottom*/ 1, /*top*/ 2);
  gBlocks[GRASS].aoStrength = 0.90f;

  setCube(gBlocks[SAND], 4);

  setCube(gBlocks[WATER], 5);
  gBlocks[WATER].opaque = false;
  gBlocks[WATER].translucent = true;

  setCube(gBlocks[SNOW], 6);
}
