#include "material.hlsl"

struct HitRecord {
  Material material;
  float3 hit_position;
  float3 normal;
  float t_min;
  float t_max;
};
