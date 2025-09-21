#include <GLFW/glfw3.h>
#include <iostream>
#include "App.hpp"

int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    int w = 1280, h = 720;
    GLFWwindow* win = glfwCreateWindow(w, h, "Voxploria", nullptr, nullptr);
    if (!win) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    int fbw, fbh;
    glfwGetFramebufferSize(win, &fbw, &fbh);
    if (fbw > 0 && fbh > 0) {
        w = fbw;
        h = fbh;
    }

    App app;
    try {
        app.init(win, w, h);
        app.run();
        app.cleanup();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
