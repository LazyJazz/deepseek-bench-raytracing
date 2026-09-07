#include "scene_viewer.h"

#include <iostream>

#include "glm/gtc/matrix_transform.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

SceneViewer::SceneViewer(Scene *scene,
                         const std::string &name,
                         int width,
                         int height,
                         bool headless,
                         const std::string &output_path)
    : name_(name),
      scene_(scene),
      width_(width),
      height_(height),
      headless_(headless),
      output_path_(output_path) {
  graphics::CreateCore(graphics::BACKEND_API_DEFAULT, {}, &core_);
  core_->InitializeLogicalDeviceAutoSelect(false);

  if (!headless) {
    core_->CreateWindowObject(width, height, name, false, false, &window_);
  }
}

void SceneViewer::Run() {
  OnInit();

  if (!headless_) {
    while (!window_->ShouldClose()) {
      OnUpdate();
      OnRender();
      glfwPollEvents();
    }
  } else {
    OnUpdate();
    OnRender();
  }

  OnClose();
}

void SceneViewer::OnInit() {
  // Create a fullscreen quad for raytracing
  std::vector<Vertex> vertices = {
      {{-1.0f, -1.0f}, {0.0f, 1.0f}},  // bottom-left
      {{1.0f, -1.0f}, {1.0f, 1.0f}},   // bottom-right
      {{-1.0f, 1.0f}, {0.0f, 0.0f}},   // top-left
      {{1.0f, 1.0f}, {1.0f, 0.0f}}     // top-right
  };

  std::vector<uint32_t> indices = {0, 1, 2, 2, 1, 3};

  // Create buffers
  core_->CreateBuffer(vertices.size() * sizeof(Vertex),
                      graphics::BUFFER_TYPE_STATIC, &vertex_buffer_);
  core_->CreateBuffer(indices.size() * sizeof(uint32_t),
                      graphics::BUFFER_TYPE_STATIC, &index_buffer_);
  vertex_buffer_->UploadData(vertices.data(), vertices.size() * sizeof(Vertex));
  index_buffer_->UploadData(indices.data(), indices.size() * sizeof(uint32_t));

  // Create scene data buffers
  core_->CreateBuffer(sizeof(CameraObject), graphics::BUFFER_TYPE_DYNAMIC,
                      &camera_buffer_);

  if (!scene_->GetTriangleBuffer().empty()) {
    core_->CreateBuffer(scene_->GetTriangleBuffer().size() * sizeof(::Triangle),
                        graphics::BUFFER_TYPE_STATIC, &triangle_buffer_);
    triangle_buffer_->UploadData(
        scene_->GetTriangleBuffer().data(),
        scene_->GetTriangleBuffer().size() * sizeof(::Triangle));
  }

  if (!scene_->GetTriangleMaterialBuffer().empty()) {
    core_->CreateBuffer(
        scene_->GetTriangleMaterialBuffer().size() * sizeof(Material),
        graphics::BUFFER_TYPE_STATIC, &triangle_material_buffer_);
    triangle_material_buffer_->UploadData(
        scene_->GetTriangleMaterialBuffer().data(),
        scene_->GetTriangleMaterialBuffer().size() * sizeof(Material));
  }

  if (!scene_->GetSphereBuffer().empty()) {
    core_->CreateBuffer(scene_->GetSphereBuffer().size() * sizeof(Sphere),
                        graphics::BUFFER_TYPE_STATIC, &sphere_buffer_);
    sphere_buffer_->UploadData(
        scene_->GetSphereBuffer().data(),
        scene_->GetSphereBuffer().size() * sizeof(Sphere));
  }

  if (!scene_->GetSphereMaterialBuffer().empty()) {
    core_->CreateBuffer(
        scene_->GetSphereMaterialBuffer().size() * sizeof(Material),
        graphics::BUFFER_TYPE_STATIC, &sphere_material_buffer_);
    sphere_material_buffer_->UploadData(
        scene_->GetSphereMaterialBuffer().data(),
        scene_->GetSphereMaterialBuffer().size() * sizeof(Material));
  }

  if (!scene_->GetPointLightBuffer().empty()) {
    core_->CreateBuffer(
        scene_->GetPointLightBuffer().size() * sizeof(PointLight),
        graphics::BUFFER_TYPE_STATIC, &point_light_buffer_);
    point_light_buffer_->UploadData(
        scene_->GetPointLightBuffer().data(),
        scene_->GetPointLightBuffer().size() * sizeof(PointLight));
  }

  // Create render target
  core_->CreateImage(width_, height_, graphics::IMAGE_FORMAT_R8G8B8A8_UNORM,
                     &color_image_);

  VirtualFileSystem shader_vfs = VirtualFileSystem::LoadDirectory(SHADER_DIR);

  // Create shaders and program
  core_->CreateShader(shader_vfs, "raytracing_shader.hlsl", "VSMain", "vs_6_0",
                      &vertex_shader_);
  core_->CreateShader(shader_vfs, "raytracing_shader.hlsl", "PSMain", "ps_6_0",
                      &pixel_shader_);
  core_->CreateProgram({graphics::IMAGE_FORMAT_R8G8B8A8_UNORM},
                       graphics::IMAGE_FORMAT_UNDEFINED, &program_);
  program_->BindShader(vertex_shader_.get(), graphics::SHADER_TYPE_VERTEX);
  program_->BindShader(pixel_shader_.get(), graphics::SHADER_TYPE_PIXEL);
  program_->AddInputBinding(sizeof(Vertex), false);
  program_->AddInputAttribute(0, graphics::INPUT_TYPE_FLOAT2,
                              offsetof(Vertex, pos));
  program_->AddInputAttribute(0, graphics::INPUT_TYPE_FLOAT2,
                              offsetof(Vertex, tex_coord));

  // Add resource bindings for raytracing data
  program_->AddResourceBinding(graphics::RESOURCE_TYPE_UNIFORM_BUFFER, 1);
  program_->AddResourceBinding(graphics::RESOURCE_TYPE_STORAGE_BUFFER, 1);
  program_->AddResourceBinding(graphics::RESOURCE_TYPE_STORAGE_BUFFER, 1);
  program_->AddResourceBinding(graphics::RESOURCE_TYPE_STORAGE_BUFFER, 1);
  program_->AddResourceBinding(graphics::RESOURCE_TYPE_STORAGE_BUFFER, 1);
  program_->AddResourceBinding(graphics::RESOURCE_TYPE_STORAGE_BUFFER, 1);
  program_->Finalize();
}

void SceneViewer::OnClose() {
  core_->WaitGPU();
  std::vector<uint32_t> pixels(width_ * height_);
  color_image_->DownloadData(pixels.data());
  stbi_write_png(output_path_.c_str(), width_, height_, 4, pixels.data(),
                 width_ * sizeof(uint32_t));
}

void SceneViewer::OnUpdate() {
  auto looking_direction =
      glm::normalize(scene_->GetSceneSettings().look_at -
                     scene_->GetSceneSettings().camera_position);
  static float pitch = std::asin(looking_direction.y),
               yaw = std::atan2(looking_direction.x, looking_direction.z);

  CameraObject camera_object{};

  camera_object.window_extent = {static_cast<float>(width_),
                                 static_cast<float>(height_)};
  camera_object.projection =
      glm::inverse(glm::mat4{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                             0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f} *
                   glm::perspectiveLH_ZO(
                       scene_->GetSceneSettings().fov_y,
                       static_cast<float>(width_) / static_cast<float>(height_),
                       0.1f, 1.0f));
  auto camera_to_world = glm::inverse(
      glm::rotate(glm::mat4{1.0f}, pitch, glm::vec3{1.0f, 0.0f, 0.0f}) *
      glm::rotate(glm::mat4{1.0f}, yaw, glm::vec3{0.0f, -1.0f, 0.0f}) *
      glm::translate(glm::mat4{1.0f},
                     -scene_->GetSceneSettings().camera_position));
  camera_object.camera_to_world = camera_to_world;
  camera_object.num_triangle =
      static_cast<uint32_t>(scene_->GetTriangleBuffer().size());
  camera_object.num_sphere =
      static_cast<uint32_t>(scene_->GetSphereBuffer().size());
  camera_object.ambient_light = scene_->GetSceneSettings().ambient_color;
  camera_object.num_point_light =
      static_cast<uint32_t>(scene_->GetPointLightBuffer().size());

  // Update camera buffer
  camera_buffer_->UploadData(&camera_object, sizeof(camera_object));

  // Handle input if not headless
  if (!headless_ && window_) {
    // Calculate time duration of last frame using static timestamp
    static auto last_frame_time = std::chrono::high_resolution_clock::now();
    auto current_frame_time = std::chrono::high_resolution_clock::now();
    float delta_time =
        std::chrono::duration<float, std::chrono::seconds::period>(
            current_frame_time - last_frame_time)
            .count();
    last_frame_time = current_frame_time;

    if (window_) {
      auto scene_settings = scene_->GetSceneSettings();
      glm::vec3 move{0.0f};
      float speed = 30.0f;
      if (glfwGetKey(window_->GLFWWindow(), GLFW_KEY_LEFT_SHIFT)) {
        speed /= 10.0f;
      }
      if (glfwGetKey(window_->GLFWWindow(), GLFW_KEY_W)) {
        move.z += delta_time * speed;
      }
      if (glfwGetKey(window_->GLFWWindow(), GLFW_KEY_S)) {
        move.z -= delta_time * speed;
      }
      if (glfwGetKey(window_->GLFWWindow(), GLFW_KEY_A)) {
        move.x -= delta_time * speed;
      }
      if (glfwGetKey(window_->GLFWWindow(), GLFW_KEY_D)) {
        move.x += delta_time * speed;
      }
      if (glfwGetKey(window_->GLFWWindow(), GLFW_KEY_SPACE)) {
        move.y += delta_time * speed;
      }
      if (glfwGetKey(window_->GLFWWindow(), GLFW_KEY_LEFT_CONTROL)) {
        move.y -= delta_time * speed;
      }

      double current_cursor_x, current_cursor_y;
      glfwGetCursorPos(window_->GLFWWindow(), &current_cursor_x,
                       &current_cursor_y);
      static double last_cursor_x = current_cursor_x,
                    last_cursor_y = current_cursor_y;
      double diff_cursor_x = current_cursor_x - last_cursor_x;
      double diff_cursor_y = current_cursor_y - last_cursor_y;
      last_cursor_x = current_cursor_x;
      last_cursor_y = current_cursor_y;
      if (glfwGetMouseButton(window_->GLFWWindow(), GLFW_MOUSE_BUTTON_LEFT)) {
        pitch += glm::radians(100.0f) * 1e-3f * -float(diff_cursor_y);
        yaw += glm::radians(100.0f) * 1e-3f * float(diff_cursor_x);
        pitch = glm::clamp(pitch, -glm::pi<float>() * 0.5f,
                           glm::pi<float>() * 0.5f);
      }

      scene_settings.camera_position +=
          glm::vec3(camera_to_world * glm::vec4(move, 0.0f));
      scene_->SetSceneSettings(scene_settings);

      // Set FPS on window title
      static FPSCounter fps_counter;
      glfwSetWindowTitle(
          window_->GLFWWindow(),
          fmt::format("{} FPS: {}", name_, fps_counter.TickFPS()).c_str());
    }
  }
}

void SceneViewer::OnRender() {
  std::unique_ptr<graphics::CommandContext> cmd_ctx;
  core_->CreateCommandContext(&cmd_ctx);
  cmd_ctx->CmdClearImage(color_image_.get(), {{0.0f, 0.0f, 0.0f, 1.0f}});
  cmd_ctx->CmdBeginRendering({color_image_.get()}, nullptr);
  cmd_ctx->CmdBindProgram(program_.get());
  cmd_ctx->CmdBindVertexBuffers(0, {vertex_buffer_.get()}, {0});
  cmd_ctx->CmdBindIndexBuffer(index_buffer_.get(), 0);

  // Bind resources - camera buffer and scene buffers
  cmd_ctx->CmdBindResources(0, {camera_buffer_.get()});
  cmd_ctx->CmdBindResources(1, {triangle_buffer_.get()});
  cmd_ctx->CmdBindResources(2, {triangle_material_buffer_.get()});
  cmd_ctx->CmdBindResources(3, {sphere_buffer_.get()});
  cmd_ctx->CmdBindResources(4, {sphere_material_buffer_.get()});
  cmd_ctx->CmdBindResources(5, {point_light_buffer_.get()});

  cmd_ctx->CmdSetPrimitiveTopology(graphics::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  graphics::Scissor scissor{0, 0, static_cast<uint32_t>(width_),
                            static_cast<uint32_t>(height_)};
  graphics::Viewport viewport{0, 0, static_cast<float>(width_),
                              static_cast<float>(height_)};
  cmd_ctx->CmdSetScissor(scissor);
  cmd_ctx->CmdSetViewport(viewport);
  cmd_ctx->CmdDrawIndexed(6, 1, 0, 0, 0);

  cmd_ctx->CmdEndRendering();

  if (!headless_) {
    cmd_ctx->CmdPresent(window_.get(), color_image_.get());
  }

  core_->SubmitCommandContext(cmd_ctx.get());
}
