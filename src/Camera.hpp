#pragma once
#include <glm/glm.hpp>

struct Camera {
    glm::vec3 pos{ 0.f, 20.f, 60.f };
    float yaw{ -90.f };
    float pitch{ -15.f };

    float fov{ 70.f };
    float aspect{ 16.f / 9.f };

    glm::mat4 view() const;
    glm::mat4 proj() const;
    glm::mat4 proj(float w, float h) const;
    glm::mat4 viewProj() const;
};
