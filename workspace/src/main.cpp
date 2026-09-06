#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using json = nlohmann::json;

namespace {
enum class MaterialType { Lambertian, Specular };
struct Material { glm::vec3 albedo; MaterialType type; };
struct Sphere { glm::vec3 center; float radius; Material material; };
struct Triangle { glm::vec3 a, b, c; Material material; };
struct Light { glm::vec3 position, power; };
struct Ray { glm::vec3 origin, direction; };
struct Hit { float t; glm::vec3 point, normal; Material material; };
struct Scene {
  int width, height, samples, max_bounces;
  float fov_degrees, epsilon;
  glm::vec3 eye, target, up, ambient;
  std::vector<Sphere> spheres;
  std::vector<Triangle> triangles;
  std::vector<Light> lights;
};

glm::vec3 Vec3(const json &value) {
  if (!value.is_array() || value.size() != 3) throw std::runtime_error("expected vec3");
  return {value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>()};
}

Scene LoadScene(const std::string &path) {
  std::ifstream stream(path);
  if (!stream) throw std::runtime_error("cannot open scene");
  json root;
  stream >> root;
  if (root.at("version").get<int>() != 1) throw std::runtime_error("unsupported version");
  Scene scene;
  const auto &image = root.at("image");
  scene.width = image.at("width").get<int>();
  scene.height = image.at("height").get<int>();
  scene.samples = image.at("samples_per_axis").get<int>();
  const auto &camera = root.at("camera");
  scene.eye = Vec3(camera.at("eye"));
  scene.target = Vec3(camera.at("target"));
  scene.up = Vec3(camera.at("up"));
  scene.fov_degrees = camera.at("vertical_fov_degrees").get<float>();
  const auto &render = root.at("render");
  scene.max_bounces = render.at("max_bounces").get<int>();
  scene.epsilon = render.at("ray_epsilon").get<float>();
  scene.ambient = Vec3(root.at("ambient"));

  std::unordered_map<std::string, Material> materials;
  for (auto item = root.at("materials").begin(); item != root.at("materials").end(); ++item) {
    const std::string type = item.value().at("type").get<std::string>();
    materials.emplace(item.key(), Material{Vec3(item.value().at("albedo")),
        type == "lambertian" ? MaterialType::Lambertian : MaterialType::Specular});
  }
  for (const auto &value : root.at("spheres")) {
    scene.spheres.push_back({Vec3(value.at("center")), value.at("radius").get<float>(),
                             materials.at(value.at("material").get<std::string>())});
  }
  for (const auto &value : root.at("triangles")) {
    const auto &vertices = value.at("vertices");
    scene.triangles.push_back({Vec3(vertices.at(0)), Vec3(vertices.at(1)), Vec3(vertices.at(2)),
                               materials.at(value.at("material").get<std::string>())});
  }
  for (const auto &value : root.at("lights")) {
    scene.lights.push_back({Vec3(value.at("position")), Vec3(value.at("power"))});
  }
  return scene;
}

glm::vec3 FaceForward(glm::vec3 outward, glm::vec3 direction) {
  return glm::dot(outward, direction) <= 0.0f ? outward : -outward;
}

bool HitSphere(const Ray &input, const Sphere &sphere, float t_min, float t_max, Hit &hit) {
  const Ray ray{input.origin, glm::normalize(input.direction)};
  const glm::vec3 offset = ray.origin - sphere.center;
  const float half_b = glm::dot(offset, ray.direction);
  const float discriminant = half_b * half_b - glm::dot(offset, offset) +
                             sphere.radius * sphere.radius;
  if (discriminant < 0.0f) return false;
  const float root_term = std::sqrt(discriminant);
  float root = -half_b - root_term;
  if (root < t_min || root > t_max) {
    root = -half_b + root_term;
    if (root < t_min || root > t_max) return false;
  }
  hit.t = root;
  hit.point = ray.origin + root * ray.direction;
  hit.normal = FaceForward(glm::normalize(hit.point - sphere.center), ray.direction);
  hit.material = sphere.material;
  return true;
}

bool HitTriangle(const Ray &input, const Triangle &triangle, float t_min, float t_max, Hit &hit) {
  const Ray ray{input.origin, glm::normalize(input.direction)};
  const glm::vec3 edge1 = triangle.b - triangle.a;
  const glm::vec3 edge2 = triangle.c - triangle.a;
  const glm::vec3 p = glm::cross(ray.direction, edge2);
  const float determinant = glm::dot(edge1, p);
  if (std::abs(determinant) < 1e-7f) return false;
  const float inverse = 1.0f / determinant;
  const glm::vec3 from_vertex = ray.origin - triangle.a;
  const float u = inverse * glm::dot(from_vertex, p);
  if (u < 0.0f || u > 1.0f) return false;
  const glm::vec3 q = glm::cross(from_vertex, edge1);
  const float v = inverse * glm::dot(ray.direction, q);
  if (v < 0.0f || u + v > 1.0f) return false;
  const float distance = inverse * glm::dot(edge2, q);
  if (distance < t_min || distance > t_max) return false;
  hit.t = distance;
  hit.point = ray.origin + distance * ray.direction;
  hit.normal = FaceForward(glm::normalize(glm::cross(edge1, edge2)), ray.direction);
  hit.material = triangle.material;
  return true;
}

bool Cast(const Scene &scene, const Ray &ray, float t_min, float t_max, Hit &hit) {
  bool found = false;
  float closest = t_max;
  Hit candidate;
  for (const Sphere &sphere : scene.spheres) {
    if (HitSphere(ray, sphere, t_min, closest, candidate)) {
      found = true; closest = candidate.t; hit = candidate;
    }
  }
  for (const Triangle &triangle : scene.triangles) {
    if (HitTriangle(ray, triangle, t_min, closest, candidate)) {
      found = true; closest = candidate.t; hit = candidate;
    }
  }
  return found;
}

glm::vec3 Lighting(const Scene &scene, const Hit &hit) {
  glm::vec3 illumination = scene.ambient;
  for (const Light &light : scene.lights) {
    const glm::vec3 delta = light.position - hit.point;
    const float distance = glm::length(delta);
    if (distance <= scene.epsilon) continue;
    const glm::vec3 direction = delta / distance;
    const float cosine = std::max(0.0f, glm::dot(hit.normal, direction));
    if (cosine == 0.0f) continue;
    Hit blocker;
    if (Cast(scene, {hit.point + scene.epsilon * hit.normal, direction},
             scene.epsilon, distance - scene.epsilon, blocker)) continue;
    illumination += light.power * (cosine / (distance * distance));
  }
  return hit.material.albedo * illumination;
}

glm::vec3 Sample(const Scene &scene, Ray ray) {
  glm::vec3 throughput(1.0f);
  ray.direction = glm::normalize(ray.direction);
  for (int bounce = 0; bounce < scene.max_bounces; ++bounce) {
    Hit hit;
    if (!Cast(scene, ray, scene.epsilon, std::numeric_limits<float>::infinity(), hit))
      return throughput * scene.ambient;
    if (hit.material.type == MaterialType::Lambertian)
      return throughput * Lighting(scene, hit);
    throughput *= hit.material.albedo;
    ray = {hit.point + scene.epsilon * hit.normal,
           glm::normalize(glm::reflect(ray.direction, hit.normal))};
  }
  return glm::vec3(0.0f);
}

std::vector<std::uint8_t> Render(const Scene &scene) {
  const glm::vec3 forward = glm::normalize(scene.target - scene.eye);
  const glm::vec3 right = glm::normalize(glm::cross(forward, scene.up));
  const glm::vec3 camera_up = glm::normalize(glm::cross(right, forward));
  const float pi = std::acos(-1.0f);
  const float scale = std::tan(scene.fov_degrees * pi / 360.0f);
  const float aspect = static_cast<float>(scene.width) / scene.height;
  const float divisor = static_cast<float>(scene.samples * scene.samples);
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(scene.width) * scene.height * 4, 255);
  for (int y = 0; y < scene.height; ++y) {
    for (int x = 0; x < scene.width; ++x) {
      glm::vec3 color(0.0f);
      for (int sy = 0; sy < scene.samples; ++sy) {
        for (int sx = 0; sx < scene.samples; ++sx) {
          const float sample_x = x + (sx + 0.5f) / scene.samples;
          const float sample_y = y + (sy + 0.5f) / scene.samples;
          const float ndc_x = (2.0f * sample_x / scene.width - 1.0f) * aspect * scale;
          const float ndc_y = (2.0f * sample_y / scene.height - 1.0f) * scale;
          color += Sample(scene, {scene.eye,
              glm::normalize(forward + right * ndc_x - camera_up * ndc_y)});
        }
      }
      color /= divisor;
      const std::size_t index = (static_cast<std::size_t>(y) * scene.width + x) * 4;
      pixels[index] = static_cast<std::uint8_t>(std::lround(std::clamp(color.r, 0.0f, 1.0f) * 255.0f));
      pixels[index + 1] = static_cast<std::uint8_t>(std::lround(std::clamp(color.g, 0.0f, 1.0f) * 255.0f));
      pixels[index + 2] = static_cast<std::uint8_t>(std::lround(std::clamp(color.b, 0.0f, 1.0f) * 255.0f));
    }
  }
  return pixels;
}
}  // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "usage: raytracer_submission <scene.json> <output.png>\n";
    return 2;
  }
  try {
    const Scene scene = LoadScene(argv[1]);
    const std::vector<std::uint8_t> pixels = Render(scene);
    if (!stbi_write_png(argv[2], scene.width, scene.height, 4, pixels.data(), scene.width * 4)) {
      throw std::runtime_error("cannot write PNG");
    }
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
