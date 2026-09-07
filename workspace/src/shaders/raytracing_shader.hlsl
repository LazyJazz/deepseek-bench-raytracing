#include "camera_object.hlsl"
#include "hit_record.hlsl"
#include "sphere.hlsl"
#include "triangle.hlsl"
#include "point_light.hlsl"

ConstantBuffer<CameraObject> camera_object : register(b0, space0);
StructuredBuffer<Triangle> triangles : register(t0, space1);
StructuredBuffer<Material> triangle_materials : register(t0, space2);
StructuredBuffer<Sphere> spheres : register(t0, space3);
StructuredBuffer<Material> sphere_materials : register(t0, space4);
StructuredBuffer<PointLight> point_lights : register(t0, space5);

struct VSInput {
  [[vk::location(0)]] float2 position : TEXCOORD0;
  [[vk::location(1)]] float2 tex_coord : TEXCOORD1;
};
struct PSInput {
  float4 position : SV_POSITION;
  [[vk::location(0)]] float2 tex_coord : TEXCOORD0;
};

bool IntersectSphere(float3 origin, float3 direction, Sphere sphere,
                     out float t, out float3 normal) {
  float3 offset = origin - sphere.origin;
  float a = dot(direction, direction);
  if (a <= 1e-8) { t = 0.0; normal = 0.0; return false; }
  float half_b = dot(offset, direction);
  float discriminant = half_b * half_b - a * (dot(offset, offset) - sphere.radius * sphere.radius);
  if (discriminant < 0.0) { t = 0.0; normal = 0.0; return false; }
  float root = (-half_b - sqrt(discriminant)) / a;
  if (root <= 1e-4) root = (-half_b + sqrt(discriminant)) / a;
  if (root <= 1e-4) { t = 0.0; normal = 0.0; return false; }
  t = root;
  float3 outward = normalize(origin + root * direction - sphere.origin);
  normal = dot(outward, direction) <= 0.0 ? outward : -outward;
  return true;
}

bool IntersectTriangle(float3 origin, float3 direction, Triangle tri,
                       out float t, out float3 normal) {
  float3 edge1 = tri.v1 - tri.v0;
  float3 edge2 = tri.v2 - tri.v0;
  float3 pvec = cross(direction, edge2);
  float determinant = dot(edge1, pvec);
  if (abs(determinant) < 1e-7) { t = 0.0; normal = 0.0; return false; }
  float inverse = 1.0 / determinant;
  float3 from_vertex = origin - tri.v0;
  float u = dot(from_vertex, pvec) * inverse;
  if (u < 0.0 || u > 1.0) { t = 0.0; normal = 0.0; return false; }
  float3 qvec = cross(from_vertex, edge1);
  float v = dot(direction, qvec) * inverse;
  if (v < 0.0 || u + v > 1.0) { t = 0.0; normal = 0.0; return false; }
  float distance = dot(edge2, qvec) * inverse;
  if (distance <= 1e-4) { t = 0.0; normal = 0.0; return false; }
  t = distance;
  float3 outward = normalize(cross(edge1, edge2));
  normal = dot(outward, direction) <= 0.0 ? outward : -outward;
  return true;
}

bool CastRay(float3 origin, float3 direction, out HitRecord hit_record) {
  direction = normalize(direction);
  bool found = false;
  float closest = 1e30;
  float candidate_t;
  float3 candidate_normal;
  for (uint i = 0; i < camera_object.num_sphere; ++i) {
    if (IntersectSphere(origin, direction, spheres[i], candidate_t, candidate_normal) &&
        candidate_t < closest) {
      found = true;
      closest = candidate_t;
      hit_record.hit_position = origin + candidate_t * direction;
      hit_record.normal = candidate_normal;
      hit_record.material = sphere_materials[i];
    }
  }
  for (uint i = 0; i < camera_object.num_triangle; ++i) {
    if (IntersectTriangle(origin, direction, triangles[i], candidate_t, candidate_normal) &&
        candidate_t < closest) {
      found = true;
      closest = candidate_t;
      hit_record.hit_position = origin + candidate_t * direction;
      hit_record.normal = candidate_normal;
      hit_record.material = triangle_materials[i];
    }
  }
  hit_record.t_min = closest;
  hit_record.t_max = closest;
  return found;
}

float3 CalculateLighting(float3 hit_point, float3 normal, Material material) {
  float3 result = camera_object.ambient_light;
  for (uint i = 0; i < camera_object.num_point_light; ++i) {
    float3 to_light = point_lights[i].position - hit_point;
    float distance_squared = dot(to_light, to_light);
    if (distance_squared <= 1e-8) continue;
    float distance = sqrt(distance_squared);
    float3 light_direction = to_light / distance;
    float cosine = max(dot(normal, light_direction), 0.0);
    if (cosine <= 0.0) continue;
    HitRecord blocker;
    if (CastRay(hit_point + 1e-4 * normal, light_direction, blocker) &&
        blocker.t_min < distance - 1e-4) continue;
    result += point_lights[i].power * (cosine / distance_squared);
  }
  return material.albedo_color * result;
}

float3 SampleRay(float3 origin, float3 direction) {
  direction = normalize(direction);
  float3 throughput = float3(1.0, 1.0, 1.0);
  for (int bounce = 0; bounce < 8; ++bounce) {
    HitRecord hit_record;
    if (!CastRay(origin, direction, hit_record))
      return throughput * camera_object.ambient_light;
    if (hit_record.material.material_type == MaterialDiffuse)
      return throughput * CalculateLighting(hit_record.hit_position, hit_record.normal,
                                            hit_record.material);
    throughput *= hit_record.material.albedo_color;
    origin = hit_record.hit_position + 1e-4 * hit_record.normal;
    direction = normalize(reflect(direction, hit_record.normal));
  }
  return float3(0.0, 0.0, 0.0);
}

PSInput VSMain(VSInput input) {
  PSInput output;
  output.position = float4(input.position, 0.0f, 1.0f);
  output.tex_coord = input.tex_coord;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
  float3 color = float3(0.0, 0.0, 0.0);
  #define SUPER_SAMPLE_X 4
  #define SUPER_SAMPLE_Y 4
  for (int x = 0; x < SUPER_SAMPLE_X; ++x) {
    for (int y = 0; y < SUPER_SAMPLE_Y; ++y) {
      float2 pixel_offset = float2((x + 0.5) / SUPER_SAMPLE_X,
                                   (y + 0.5) / SUPER_SAMPLE_Y);
      float4 direction = mul(camera_object.projection,
                             float4(((input.position.xy + pixel_offset) /
                                     camera_object.window_extent) * 2.0 - 1.0,
                                    1.0, 1.0));
      float3 subpixel_color = SampleRay(
          mul(camera_object.camera_to_world, float4(0.0, 0.0, 0.0, 1.0)).xyz,
          mul(camera_object.camera_to_world,
              float4(normalize(direction.xyz), 0.0)).xyz);
      color += clamp(subpixel_color, 0.0, 1.0);
    }
  }
  return float4(color / (SUPER_SAMPLE_X * SUPER_SAMPLE_Y), 1.0);
}
