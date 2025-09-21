#pragma once
#include "engine/physics/Physics.hpp"
#include "engine/render/WorldRenderer.hpp"
#include "engine/world/World.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Game {
public:
  void init(GLFWwindow* win, int width, int height);
  void shutdown();

  bool tick(float dt);

  void setMouseCaptured(GLFWwindow* win, bool capture, bool resetMouseState);

private:
  WorldRenderer renderer{};
  World world{1337};

  PlayerBody player{};
  PhysParams phys{};

  bool firstMouse = true;
  double lastX = 0.0, lastY = 0.0;
  float mouseSensitivity = 0.12f;

  float walkSpeed = 6.3f;
  float sprintSpeed = 10.5f;
  float sneakSpeed = 3.2f;

  int chunkRadius = 8;

  void bindCallbacks(GLFWwindow* window);

  void onMouse(double x, double y);
  void updatePlayer(float dt);
};
