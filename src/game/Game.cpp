#include "Game.hpp"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

static bool g_mouseCaptured = true;

void Game::setMouseCaptured(GLFWwindow* win, bool capture, bool resetMouseState) {
  g_mouseCaptured = capture;
  glfwSetInputMode(win, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  if (capture) {
    int w, h;
    glfwGetWindowSize(win, &w, &h);
    glfwSetCursorPos(win, w * 0.5, h * 0.5);
    if (resetMouseState) firstMouse = true;
  }
}

void Game::bindCallbacks(GLFWwindow* window) {
  glfwSetWindowUserPointer(window, this);

  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int, int) {
    auto* self = reinterpret_cast<Game*>(glfwGetWindowUserPointer(w));
    if (self) self->renderer.vk.requestResize();
  });

  glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    auto* self = reinterpret_cast<Game*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    if (key == GLFW_KEY_ESCAPE) self->setMouseCaptured(w, !g_mouseCaptured, true);
  });

  glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
    auto* self = reinterpret_cast<Game*>(glfwGetWindowUserPointer(w));
    if (self) self->onMouse(x, y);
  });
}

void Game::init(GLFWwindow* win, int width, int height) {
  bindCallbacks(win);
  setMouseCaptured(win, true, true);

  renderer.init(win, width, height);
  player.pos = glm::vec3(64.f, 92.f, 64.f);
  renderer.cam.pos = player.pos + glm::vec3(0, player.height * 0.4f, 0);
  renderer.cam.yaw = 180.f;
  renderer.cam.pitch = -20.f;
  world.update(player.pos, chunkRadius);

  std::vector<std::pair<ChunkCoord, Mesh>> dirty;
  world.collectDirtyMeshes(dirty);
  std::vector<ChunkCoord> visible;
  world.listLoaded(visible);
  renderer.vk.syncChunks(visible, dirty);
}

void Game::shutdown() {
  renderer.shutdown();
}

void Game::onMouse(double x, double y) {
  if (!g_mouseCaptured) {
    firstMouse = true;
    return;
  }
  auto& cam = renderer.cam;
  if (firstMouse) {
    lastX = x;
    lastY = y;
    firstMouse = false;
  }
  float dx = float(x - lastX);
  float dy = float(y - lastY);
  lastX = x;
  lastY = y;
  cam.yaw += dx * mouseSensitivity;
  cam.pitch -= dy * mouseSensitivity;
  cam.pitch = std::clamp(cam.pitch, -89.f, 89.f);
}

void Game::updatePlayer(float dt) {
  auto& cam = renderer.cam;

  glm::vec3 fwdXZ(cosf(glm::radians(cam.yaw)), 0.0f, sinf(glm::radians(cam.yaw)));
  if (glm::length(fwdXZ) < 1e-4f) fwdXZ = {1, 0, 0};
  fwdXZ = glm::normalize(fwdXZ);
  glm::vec3 right = glm::normalize(glm::cross(fwdXZ, glm::vec3(0, 1, 0)));

  float speed = walkSpeed;
  if (glfwGetKey(renderer.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed = sprintSpeed;
  if (glfwGetKey(renderer.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed = sneakSpeed;

  glm::vec3 wish(0.0f);
  if (glfwGetKey(renderer.window, GLFW_KEY_W) == GLFW_PRESS) wish += fwdXZ;
  if (glfwGetKey(renderer.window, GLFW_KEY_S) == GLFW_PRESS) wish -= fwdXZ;
  if (glfwGetKey(renderer.window, GLFW_KEY_A) == GLFW_PRESS) wish -= right;
  if (glfwGetKey(renderer.window, GLFW_KEY_D) == GLFW_PRESS) wish += right;

  if (glm::length(wish) > 0.0f) wish = glm::normalize(wish) * speed;

  const float accelRate = (player.onGround ? 14.0f : 8.0f);
  const float alpha = 1.0f - std::exp(-accelRate * dt);
  player.vel.x += (wish.x - player.vel.x) * alpha;
  player.vel.z += (wish.z - player.vel.z) * alpha;

  if (glfwGetKey(renderer.window, GLFW_KEY_SPACE) == GLFW_PRESS && player.onGround) {
    player.vel.y = phys.jumpSpeed;
    player.onGround = false;
  }

  if (glfwGetKey(renderer.window, GLFW_KEY_Q) == GLFW_PRESS)
    renderer.cam.fov = std::max(30.f, renderer.cam.fov - 60.f * dt);
  if (glfwGetKey(renderer.window, GLFW_KEY_E) == GLFW_PRESS)
    renderer.cam.fov = std::min(90.f, renderer.cam.fov + 60.f * dt);
}

bool Game::tick(float dt) {
  updatePlayer(dt);

  auto solidQuery = [this](int x, int y, int z) { return world.isSolid(x, y, z); };
  simulate(player, dt, solidQuery, phys);

  renderer.cam.pos = player.pos + glm::vec3(0, player.height * 0.4f, 0);

  world.update(player.pos, chunkRadius);

  std::vector<std::pair<ChunkCoord, Mesh>> dirty;
  world.collectDirtyMeshes(dirty);
  std::vector<ChunkCoord> visible;
  world.listLoaded(visible);
  renderer.vk.syncChunks(visible, dirty);

  renderer.drawWorld(dt);
  return true; // keep running
}
