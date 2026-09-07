cmake_minimum_required(VERSION 3.25)

set(LONGMARCH_DISABLE_CUDA ON CACHE BOOL "Build LongMarch without CUDA")
set(LONGMARCH_DISABLE_PYTHON ON CACHE BOOL "Do not build LongMarch Python bindings")
set(LONGMARCH_NO_DEMO ON CACHE BOOL "Do not build LongMarch demos")

if(NOT DEFINED VCPKG_PATH)
  message(FATAL_ERROR "Set VCPKG_PATH to a vcpkg checkout containing LongMarch dependencies")
endif()
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake"
    CACHE FILEPATH "vcpkg toolchain file")

project(simple_raytracer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/LongMarch "${CMAKE_BINARY_DIR}/longmarch")
add_subdirectory(lib)

add_executable(simple_raytracer gui/main.cpp)
target_include_directories(simple_raytracer PRIVATE lib)
target_link_libraries(simple_raytracer PRIVATE raytracing_lib)
