#include "AppHost.hpp"

#include <algorithm>
#include <stdexcept>

void AppHost::initWindow(int w, int h, const char* title) {
  if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(w, h, title, nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }
}

void AppHost::shutdown() {
  if (window) {
    glfwDestroyWindow(window);
    window = nullptr;
  }
  glfwTerminate();
}

void AppHost::run(const std::function<bool(float)>& tick) {
  double prev = glfwGetTime();
  while (window && !glfwWindowShouldClose(window)) {
    glfwPollEvents();
    double now = glfwGetTime();
    float dt = std::min<float>(float(now - prev), 0.0333f);
    prev = now;
    if (!tick(dt)) break;
  }
}
