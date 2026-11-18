cmake_minimum_required(VERSION 3.24)

# Single entry point: configure from here, then build the target you need
# (cmake --build <dir> --target <name>). See CMakePresets.json.
option(ENGINE_BUILD_GUI        "Build the editor GUI (needs SDL3 and Dear ImGui)" ON)
option(ENGINE_BUILD_APP        "Build the executables (apps/)"                    ON)
option(ENGINE_BUILD_TESTS      "Build unit tests"                                 OFF)
option(ENGINE_BUILD_BENCHMARKS "Build benchmarks (use a Release build)"           OFF)
option(ENGINE_ENABLE_COVERAGE  "Enable code coverage"                             OFF)

include(cmake/Vcpkg.cmake) # before project(): vcpkg reads its settings there
project(engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/Coverage.cmake)

# Every executable (engine, editor, examples, tests) lands at the root of the
# build directory: ./build/<preset>/editor
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Modules: one CMakeLists.txt, one target, only the dependencies it needs.
add_subdirectory(src/ECS)
# add_subdirectory(src/Rendering) # engine_render: enable with its first .cpp
# add_subdirectory(src/Engine)    # engine_core:   enable with its first .cpp
if(ENGINE_BUILD_GUI)
  add_subdirectory(src/Editor)
endif()

if(ENGINE_BUILD_APP)
  add_subdirectory(apps)
endif()

if(ENGINE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ENGINE_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
