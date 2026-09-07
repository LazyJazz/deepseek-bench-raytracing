#pragma once

#include "glm/glm.hpp"

struct CameraObject {
  glm::mat4 projection;
  glm::mat4 camera_to_world;
  glm::vec2 window_extent;
  uint32_t num_triangle;
  uint32_t num_sphere;
  glm::vec3 ambient_light;
  uint32_t num_point_light;
};
