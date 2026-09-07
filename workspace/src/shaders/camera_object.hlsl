struct CameraObject {
  float4x4 projection;
  float4x4 camera_to_world;
  float2 window_extent;
  uint num_triangle;
  uint num_sphere;
  float3 ambient_light;
  uint num_point_light;
};
