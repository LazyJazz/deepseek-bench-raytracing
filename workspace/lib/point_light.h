#pragma once

#include "glm/glm.hpp"

struct PointLight {
  PointLight(const glm::vec3 &position_, float power_)
      : position(position_), power(power_) {
  }

  PointLight(const glm::vec3 &position_, const glm::vec3 &power_)
      : position(position_), power(power_) {
  }

  glm::vec3 position;
  glm::vec3 power;
};
