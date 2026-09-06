cmake_minimum_required(VERSION 3.16)
project(raytracing_image_evaluation LANGUAGES CXX)
include(CTest)
find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WORKSPACE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../workspace")
set(EXTERNAL_DIR "${WORKSPACE_DIR}/external")
set(CANDIDATE_BINARY "" CACHE FILEPATH "Path to the freshly built workspace renderer")
if(NOT CANDIDATE_BINARY OR NOT EXISTS "${CANDIDATE_BINARY}")
  message(FATAL_ERROR "CANDIDATE_BINARY must point to a built workspace renderer")
endif()

add_executable(compare_png compare_png.cpp)
target_include_directories(compare_png PRIVATE "${EXTERNAL_DIR}/stb")

foreach(scene primitives reflections enclosure)
  add_test(NAME ray.image.${scene} COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_LIST_DIR}/run_case.py"
    "${CANDIDATE_BINARY}"
    "$<TARGET_FILE:compare_png>"
    "${WORKSPACE_DIR}/scenes/${scene}.json"
    "${WORKSPACE_DIR}/reference/${scene}.png"
    "${CMAKE_CURRENT_BINARY_DIR}/${scene}.png"
  )
  set_tests_properties(ray.image.${scene} PROPERTIES TIMEOUT 120)
endforeach()
