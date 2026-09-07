#pragma once
#include "glm/glm.hpp"

struct Triangle {
  Triangle(glm::vec3 v0_, glm::vec3 v1_, glm::vec3 v2_)
      : v0(v0_), v1(v1_), v2(v2_) {
  }
  glm::vec3 v0;
  glm::vec3 v1;
  glm::vec3 v2;
};
