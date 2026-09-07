#pragma once
#include "glm/glm.hpp"

struct Sphere {
  explicit Sphere(const glm::vec4 &origin_radius_)
      : origin_radius(origin_radius_) {
  }
  Sphere(const glm::vec3 &origin, float radius)
      : origin_radius({origin, radius}) {
  }
  glm::vec4 origin_radius;
};
