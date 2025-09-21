#pragma once
#include <GLFW/glfw3.h>
#include <functional>

struct AppHost {
  GLFWwindow* window{};

  void initWindow(int w, int h, const char* title);
  void shutdown();

  void run(const std::function<bool(float)>& tick);

  GLFWwindow* getWindow() const {
    return window;
  }
};
