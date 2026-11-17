# engine_render: the backend-agnostic 2D renderer (gfx::IRenderer) and its
# SDL3 backend. Built with ENGINE_BUILD_RENDERER=ON (the default); vcpkg then
# installs the "renderer" feature of the manifest (SDL3, stb).
# It does NOT depend on the ECS: a RenderSystem in engine_core will bridge the two.
# The bgfx backend is not built yet; see src/Renderer/README.md.
find_package(SDL3 CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Every .cpp below is picked up automatically, as in src/Editor.
file(GLOB_RECURSE RENDER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(engine_render STATIC ${RENDER_SOURCES})

# Includes are written from src/: #include "Renderer/IRenderer.hpp".
target_include_directories(engine_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
# SYSTEM keeps stb's own warnings out of the renderer's warning-clean build.
target_include_directories(engine_render SYSTEM PRIVATE ${Stb_INCLUDE_DIR})
# Private: the public headers only forward-declare SDL types, they include no SDL header.
target_link_libraries(engine_render PRIVATE SDL3::SDL3)

if(MSVC)
  target_compile_options(engine_render PRIVATE /W4)
else()
  target_compile_options(engine_render PRIVATE -Wall -Wextra -Wpedantic)
endif()
