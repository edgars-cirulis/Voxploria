#include "Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>

void Renderer::init(GLFWwindow* w, int width, int height)
{
    window = w;
    cam.aspect = float(width) / float(height);

    vk.init(window, width, height);

    vk.loadAtlasPNG("assets/atlas.png");
}

void Renderer::draw(float dt)
{
    auto ext = vk.getExtent();
    if (ext.height != 0)
        cam.aspect = float(ext.width) / float(ext.height);

    static float timeSec = 0.0f;
    timeSec += dt;

    glm::vec3 sunDir = glm::normalize(glm::vec3(-0.35f, -1.0f, -0.25f));

    Push pc;
    pc.uMVP = cam.viewProj();
    pc.uLight = glm::vec4(sunDir, timeSec);
    pc.uCamPos = glm::vec4(cam.pos, 0.0f);

    vk.drawFrame(&pc, sizeof(Push));
}

void Renderer::shutdown()
{
    vk.cleanup();
}
