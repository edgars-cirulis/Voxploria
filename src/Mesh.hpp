#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tile;
    glm::vec3 color;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void clear()
    {
        vertices.clear();
        indices.clear();
    }
};
