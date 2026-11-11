# engine_core: the runtime library of a game. The main loop, the scenes and
# the systems bridging the ECS and the other modules. No editor, no ImGui.
#
# Not built yet (only a README). To enable it:
#   1. uncomment the block below and add its first .cpp (Engine.cpp);
#   2. uncomment add_subdirectory(src/Engine) in the root CMakeLists.txt;
#   3. link it from apps/runtime and from src/Editor instead of engine_ecs.
# Later, it becomes SHARED (plugins / hot-reload), see COMPILATION.md.

# file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# add_library(engine_core STATIC ${ENGINE_SOURCES})
# target_include_directories(engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
# target_link_libraries(engine_core PUBLIC engine_ecs)
# target_link_libraries(engine_core PRIVATE engine_render) # once Rendering is enabled
