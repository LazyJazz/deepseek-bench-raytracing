cmake_minimum_required(VERSION 3.16)
project(raytracing_image_evaluation LANGUAGES CXX)
include(CTest)
find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REPOSITORY_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
set(WORKSPACE_DIR "${REPOSITORY_DIR}/workspace")
set(EXTERNAL_DIR "${REPOSITORY_DIR}/external")
file(GLOB_RECURSE CANDIDATE_SOURCES CONFIGURE_DEPENDS "${WORKSPACE_DIR}/src/*.cpp")
if(NOT CANDIDATE_SOURCES)
  message(FATAL_ERROR "No candidate .cpp files found under workspace/src")
endif()

add_executable(raytracer_submission ${CANDIDATE_SOURCES})
target_include_directories(raytracer_submission PRIVATE
  "${WORKSPACE_DIR}/include"
  "${EXTERNAL_DIR}/glm"
  "${EXTERNAL_DIR}/stb"
  "${EXTERNAL_DIR}/json/include"
)

add_executable(compare_png compare_png.cpp)
target_include_directories(compare_png PRIVATE "${EXTERNAL_DIR}/stb")

foreach(scene primitives reflections enclosure)
  add_test(NAME ray.image.${scene} COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_LIST_DIR}/run_case.py"
    "$<TARGET_FILE:raytracer_submission>"
    "$<TARGET_FILE:compare_png>"
    "${CMAKE_CURRENT_LIST_DIR}/scenes/${scene}.json"
    "${CMAKE_CURRENT_LIST_DIR}/reference/${scene}.png"
    "${CMAKE_CURRENT_BINARY_DIR}/${scene}.png"
  )
  set_tests_properties(ray.image.${scene} PROPERTIES TIMEOUT 120)
endforeach()
