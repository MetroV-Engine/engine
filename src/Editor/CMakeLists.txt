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
file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(gui
    main.cpp
    ${GUI_SOURCES}
)

target_include_directories(gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(gui PRIVATE
    engine_ecs
    SDL3::SDL3
    imgui::imgui
)

# Next to the other executables, at the root of the build directory: ./build/gui
set_target_properties(gui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
