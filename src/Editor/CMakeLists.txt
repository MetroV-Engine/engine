# Editor GUI. Two ways to build it:
#  - from the engine root, with -DENGINE_BUILD_GUI=ON (see the root CMakeLists.txt);
#  - from this folder alone (cmake -S src/Editor -B build), handy when working
#    only on the editor: it then adds the ECS itself and uses the engine's
#    vcpkg manifest with the "gui" feature.
cmake_minimum_required(VERSION 3.24)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # vcpkg reads these at project(): the manifest is at the repository root.
  set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "vcpkg manifest of the engine")
  list(APPEND VCPKG_MANIFEST_FEATURES "gui")
  project(editor LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  add_subdirectory(../ECS ECS)
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Every .cpp under src/ is picked up automatically; CONFIGURE_DEPENDS re-runs
# the glob at build time so new files are detected without re-running cmake.
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# The editor itself, as a library: a program that has an ECS world (the
# engine, a game) links it to open the editor on that world:
#   target_link_libraries(my_game PRIVATE engine_editor)
add_library(engine_editor STATIC ${EDITOR_SOURCES})

# Public: the editor's headers include the ECS, SDL3 and Dear ImGui.
target_include_directories(engine_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(engine_editor PUBLIC
    engine_ecs
    SDL3::SDL3
    imgui::imgui
)

# The standalone editor: an empty ECS world, or the built-in demo (--demo).
add_executable(gui main.cpp)
target_link_libraries(gui PRIVATE engine_editor)

# A small game opening the editor on its own world: the reference for
# connecting game code to the editor.
add_executable(gui_ecs_example examples/EcsExample.cpp)
target_link_libraries(gui_ecs_example PRIVATE engine_editor)

# Next to the other executables, at the root of the build directory: ./build/gui
set_target_properties(gui gui_ecs_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# The editor's tests are with the engine's, in tests/Editor (tests_editor in
# tests/CMakeLists.txt): built from the engine root with -DENGINE_BUILD_TESTS=ON.
