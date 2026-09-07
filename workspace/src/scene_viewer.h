#pragma once
#include "long_march.h"
#include "raytracing_lib.h"

using namespace long_march;

struct Vertex {
  Vector2<float> pos;
  Vector2<float> tex_coord;
};

class SceneViewer {
 public:
  SceneViewer(Scene *scene,
              const std::string &name,
              int width,
              int height,
              bool headless,
              const std::string &output_path =
                  "workspace/build/raytracing_result.png");
  void Run();

 private:
  void OnInit();
  void OnClose();
  void OnUpdate();
  void OnRender();

  Scene *scene_{};

  std::unique_ptr<graphics::Core> core_;
  std::unique_ptr<graphics::Window> window_;
  std::unique_ptr<graphics::Shader> vertex_shader_;
  std::unique_ptr<graphics::Shader> pixel_shader_;
  std::unique_ptr<graphics::Program> program_;
  std::unique_ptr<graphics::Buffer> vertex_buffer_;
  std::unique_ptr<graphics::Buffer> index_buffer_;
  std::unique_ptr<graphics::Buffer> camera_buffer_;
  std::unique_ptr<graphics::Buffer> triangle_buffer_;
  std::unique_ptr<graphics::Buffer> triangle_material_buffer_;
  std::unique_ptr<graphics::Buffer> sphere_buffer_;
  std::unique_ptr<graphics::Buffer> sphere_material_buffer_;
  std::unique_ptr<graphics::Buffer> point_light_buffer_;
  std::unique_ptr<graphics::Image> color_image_;

  int width_;
  int height_;
  bool headless_;

  std::string name_;
  std::string output_path_;
};
