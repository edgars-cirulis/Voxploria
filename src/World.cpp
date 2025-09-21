#include "World.hpp"
#include <cmath>
#include "ChunkMesher.hpp"

static inline ChunkCoord worldToChunk(int wx, int wz, int& lx, int& lz)
{
    ChunkCoord cc;
    cc.cx = (wx >= 0) ? wx / CHUNK_X : ((wx + 1) / CHUNK_X - 1);
    cc.cz = (wz >= 0) ? wz / CHUNK_Z : ((wz + 1) / CHUNK_Z - 1);
    lx = wx - cc.cx * CHUNK_X;
    lz = wz - cc.cz * CHUNK_Z;
    return cc;
}

Chunk* World::getOrCreate(ChunkCoord cc)
{
    auto it = chunks.find(cc);
    if (it != chunks.end())
        return &it->second;
    auto [insIt, _] = chunks.emplace(cc, Chunk{ cc });
    gen.buildChunk(insIt->second);
    return &insIt->second;
}

Voxel World::get(int wx, int wy, int wz)
{
    if (wy < 0 || wy >= CHUNK_Y)
        return AIR;
    int lx, lz;
    ChunkCoord cc = worldToChunk(wx, wz, lx, lz);
    return getOrCreate(cc)->getLocal(lx, wy, lz);
}

Voxel World::getFast(int wx, int wy, int wz) const
{
    if (wy < 0 || wy >= CHUNK_Y)
        return AIR;
    int lx, lz;
    ChunkCoord cc = worldToChunk(wx, wz, lx, lz);
    auto it = chunks.find(cc);
    if (it == chunks.end())
        return AIR;
    return it->second.getLocal(lx, wy, lz);
}

void World::set(int wx, int wy, int wz, Voxel v)
{
    if (wy < 0 || wy >= CHUNK_Y)
        return;
    int lx, lz;
    ChunkCoord cc = worldToChunk(wx, wz, lx, lz);
    getOrCreate(cc)->setLocal(lx, wy, lz, v);
    if (lx == 0)
        getOrCreate({ cc.cx - 1, cc.cz })->dirtyCPU = true;
    if (lx == CHUNK_X - 1)
        getOrCreate({ cc.cx + 1, cc.cz })->dirtyCPU = true;
    if (lz == 0)
        getOrCreate({ cc.cx, cc.cz - 1 })->dirtyCPU = true;
    if (lz == CHUNK_Z - 1)
        getOrCreate({ cc.cx, cc.cz + 1 })->dirtyCPU = true;
}

void World::update(const glm::vec3& camPos, int r)
{
    int cx = int(std::floor(camPos.x)) / CHUNK_X;
    int cz = int(std::floor(camPos.z)) / CHUNK_Z;
    for (int dz = -r; dz <= r; ++dz) {
        for (int dx = -r; dx <= r; ++dx) {
            ChunkCoord cc{ cx + dx, cz + dz };
            auto it = chunks.find(cc);
            if (it == chunks.end()) {
                auto [ins, _] = chunks.emplace(cc, Chunk{ cc });
                Chunk& ch = ins->second;
                gen.buildChunk(ch);
            }
        }
    }
}

void World::collectDirtyMeshes(std::vector<std::pair<ChunkCoord, Mesh>>& out)
{
    out.clear();
    for (auto& [cc, ch] : chunks) {
        if (!ch.dirtyCPU)
            continue;
        Mesh m;
        ChunkMesher::build(ch, *this, m);
        ch.dirtyCPU = false;
        ch.dirtyGPU = true;
        out.emplace_back(cc, std::move(m));
    }
}

void World::listLoaded(std::vector<ChunkCoord>& out) const
{
    out.clear();
    out.reserve(chunks.size());
    for (auto& kv : chunks)
        out.push_back(kv.first);
}
