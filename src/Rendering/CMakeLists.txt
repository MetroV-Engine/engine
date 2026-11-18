# engine_render: low-level rendering (meshes, textures, draw calls).
# It does NOT depend on the ECS: a RenderSystem in engine_core bridges the two.
#
# Not built yet (no sources). To enable it:
#   1. uncomment the block below and add its first .cpp;
#   2. uncomment add_subdirectory(src/Rendering) in the root CMakeLists.txt;
#   3. add the rendering dependencies to vcpkg.json if they are not there.

# find_package(SDL3 CONFIG REQUIRED)
#
# file(GLOB_RECURSE RENDER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# add_library(engine_render STATIC ${RENDER_SOURCES})
# target_include_directories(engine_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
# target_link_libraries(engine_render PUBLIC SDL3::SDL3)
