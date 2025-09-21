#pragma once
#include "engine/gfx/Camera.hpp"
#include "engine/gfx/Mesh.hpp"
#include "engine/gfx/VulkanContext.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class WorldRenderer {
public:
  GLFWwindow* window{};
  VulkanContext vk{};
  Camera cam{};

  struct Push {
    glm::mat4 uMVP;
    glm::vec4 uLight;
    glm::vec4 uCamPos;
  };

  void init(GLFWwindow* w, int width, int height);
  void drawWorld(float dt);
  void shutdown();
};
