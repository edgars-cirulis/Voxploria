#include "Camera.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

static glm::vec3 dirFromAngles(float yawDeg, float pitchDeg) {
  const float yaw = glm::radians(yawDeg);
  const float pitch = glm::radians(pitchDeg);
  glm::vec3 d(cosf(pitch) * cosf(yaw), sinf(pitch), cosf(pitch) * sinf(yaw));
  return glm::normalize(d);
}

glm::mat4 Camera::view() const {
  glm::vec3 f = dirFromAngles(yaw, pitch);
  return glm::lookAt(pos, pos + f, glm::vec3(0, 1, 0));
}

static inline glm::mat4 fixForVulkan(glm::mat4 P) {
  P[1][1] *= -1.f;
  return P;
}

glm::mat4 Camera::proj() const {
  auto P = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f);
  return fixForVulkan(P);
}

glm::mat4 Camera::proj(float w, float h) const {
  auto P = glm::perspective(glm::radians(fov), w / h, 0.1f, 1000.0f);
  return fixForVulkan(P);
}

glm::mat4 Camera::viewProj() const {
  return proj() * view();
}
