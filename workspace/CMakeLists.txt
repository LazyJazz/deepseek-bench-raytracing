cmake_minimum_required(VERSION 3.16)
project(raytracing_submission LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE RAYTRACER_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
if(NOT RAYTRACER_SOURCES)
  message(FATAL_ERROR "No renderer source files found under workspace/src")
endif()

add_executable(raytracer_submission ${RAYTRACER_SOURCES})
target_include_directories(raytracer_submission PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/glm"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/stb"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/json/include"
)
