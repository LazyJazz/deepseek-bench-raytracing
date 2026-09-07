file(GLOB RAYTRACING_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(raytracing_lib ${RAYTRACING_SOURCES})
target_include_directories(raytracing_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(raytracing_lib PUBLIC LongMarch)
target_compile_definitions(raytracing_lib PRIVATE SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

add_executable(simple_raytracer main.cpp)
target_include_directories(simple_raytracer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(simple_raytracer PRIVATE raytracing_lib)
