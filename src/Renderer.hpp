#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Camera.hpp"
#include "Mesh.hpp"
#include "VulkanContext.hpp"

struct Renderer {
    GLFWwindow* window{};
    VulkanContext vk{};
    Camera cam{};
    Mesh mesh{};

    struct Push {
        glm::mat4 uMVP;
        glm::vec4 uLight;
        glm::vec4 uCamPos;
    };

    void init(GLFWwindow* w, int width, int height);
    void draw(float dt);
    void shutdown();
};
