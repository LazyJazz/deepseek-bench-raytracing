cmake_minimum_required(VERSION 3.25)

set(LONGMARCH_DISABLE_CUDA ON CACHE BOOL "Build LongMarch without CUDA")
set(LONGMARCH_DISABLE_PYTHON ON CACHE BOOL "Do not build LongMarch Python bindings")
set(LONGMARCH_NO_DEMO ON CACHE BOOL "Do not build LongMarch demos")

set(VCPKG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg"
    CACHE PATH "vcpkg checkout used for LongMarch dependencies" FORCE)
if(NOT EXISTS "${VCPKG_PATH}/vcpkg")
  if(UNIX)
    execute_process(
      COMMAND "${VCPKG_PATH}/bootstrap-vcpkg.sh" -disableMetrics
      WORKING_DIRECTORY "${VCPKG_PATH}")
  elseif(WIN32)
    execute_process(
      COMMAND "${VCPKG_PATH}/bootstrap-vcpkg.bat"
      WORKING_DIRECTORY "${VCPKG_PATH}")
  endif()
endif()
if(NOT EXISTS "${VCPKG_PATH}/vcpkg")
  message(FATAL_ERROR "Unable to bootstrap vcpkg under ${VCPKG_PATH}")
endif()
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake"
    CACHE FILEPATH "vcpkg toolchain file" FORCE)
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/LongMarch"
    CACHE PATH "LongMarch vcpkg manifest directory" FORCE)
set(VCPKG_INSTALLED_DIR "${VCPKG_MANIFEST_DIR}/vcpkg_installed"
    CACHE PATH "vcpkg installation directory" FORCE)

project(simple_raytracer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(PREPEND CMAKE_PREFIX_PATH
     "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

# LongMarch's BVH target includes GLM headers without propagating the GLM
# target's include directory, so expose the selected vcpkg include directory
# to all of its component targets.
include_directories("${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/glm")

add_subdirectory(external/LongMarch "${CMAKE_BINARY_DIR}/longmarch")
add_subdirectory(src)
