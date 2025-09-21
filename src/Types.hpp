#pragma once
#include <cstdint>

constexpr int CHUNK_X = 16;
constexpr int CHUNK_Z = 16;
constexpr int CHUNK_Y = 128;

using Voxel = uint8_t;
enum Block : Voxel { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3, SAND = 4, WATER = 5, SNOW = 6 };

struct ChunkCoord {
    int cx{ 0 }, cz{ 0 };
};
inline bool operator==(ChunkCoord a, ChunkCoord b)
{
    return a.cx == b.cx && a.cz == b.cz;
}

struct ChunkKeyHash {
    size_t operator()(const ChunkCoord& c) const { return (uint64_t(uint32_t(c.cx)) << 32) ^ uint32_t(c.cz); }
};

inline int64_t packKey(ChunkCoord c)
{
    return (int64_t(uint32_t(c.cx)) << 32) | uint32_t(c.cz);
}
struct PackedKeyHash {
    size_t operator()(int64_t k) const { return std::hash<int64_t>{}(k); }
};
