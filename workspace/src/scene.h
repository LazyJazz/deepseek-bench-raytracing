#pragma once

#include <vector>

#include "material.h"
#include "point_light.h"
#include "scene_settings.h"
#include "sphere.h"
#include "triangle.h"

class Scene {
 public:
  void AddTriangle(const Triangle &triangle, const Material &material);
  void AddSphere(const Sphere &sphere, const Material &material);
  void AddPointLight(const PointLight &point_light);
  void SetSceneSettings(const SceneSettings &scene_settings);
  [[nodiscard]] const std::vector<Triangle> &GetTriangleBuffer() const;
  [[nodiscard]] const std::vector<Material> &GetTriangleMaterialBuffer() const;
  [[nodiscard]] const std::vector<Sphere> &GetSphereBuffer() const;
  [[nodiscard]] const std::vector<Material> &GetSphereMaterialBuffer() const;
  [[nodiscard]] const std::vector<PointLight> &GetPointLightBuffer() const;
  [[nodiscard]] const SceneSettings &GetSceneSettings() const;

 private:
  SceneSettings scene_settings_{};
  std::vector<Triangle> triangles_;
  std::vector<Material> triangle_materials_;
  std::vector<Sphere> spheres_;
  std::vector<Material> sphere_materials_;
  std::vector<PointLight> point_lights_;
};
