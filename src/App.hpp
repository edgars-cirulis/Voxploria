#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include "Physics.hpp"
#include "Renderer.hpp"
#include "World.hpp"

struct App {
    GLFWwindow* window{};
    Renderer renderer{};

    bool firstMouse = true;
    double lastX = 0.0, lastY = 0.0;
    float mouseSensitivity = 0.12f;

    float walkSpeed = 6.3f;
    float sprintSpeed = 10.5f;
    float sneakSpeed = 3.2f;

    PlayerBody player{};
    PhysParams phys{};

    uint64_t worldSeed = 1337;
    World world{ worldSeed };
    std::vector<Mesh> pendingMeshes;

    int chunkRadius = 8;

    void init(GLFWwindow* win, int w, int h);
    void run();
    void cleanup();

    void onMouse(double x, double y);
    void updatePlayer(float dt);
};
