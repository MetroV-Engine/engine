# engine_core: the runtime library of a game. The main loop, the scenes and
# the systems bridging the ECS and the other modules. No editor, no ImGui.
#
# Linked by apps/runtime and by src/Editor. Later, it becomes SHARED
# (plugins / hot-reload), see COMPILATION.md. Once Rendering is enabled, add
#   target_link_libraries(engine_core PRIVATE engine_render)

# Every .cpp under this folder is picked up automatically; CONFIGURE_DEPENDS
# re-runs the glob at build time so new files are detected without re-running cmake.
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(engine_core STATIC ${ENGINE_SOURCES})

# Includes are written from src/: #include "Engine/Engine.hpp".
target_include_directories(engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
# Public: Scene.hpp exposes the ECS types.
# Private: nlohmann-json is the format behind Serialization/; its public
# header exposes no JSON type, so the library can change freely.
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(engine_core
  PUBLIC engine_ecs
  PRIVATE nlohmann_json::nlohmann_json)
