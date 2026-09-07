#pragma once

#include "glm/glm.hpp"

struct SceneSettings {
  glm::vec3 camera_position{10.0f, 10.0f, 10.0f};
  glm::vec3 look_at{0.0f, 0.0f, 0.0f};
  float fov_y{glm::radians(60.0f)};
  glm::vec3 ambient_color{0.5f};
};
