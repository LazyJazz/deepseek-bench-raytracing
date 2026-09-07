#pragma once
#include "glm/glm.hpp"

enum MaterialType : uint32_t { kLambertian = 0, kSpecular };

struct Material {
  glm::vec3 albedo_color;
  MaterialType material_type;
};
