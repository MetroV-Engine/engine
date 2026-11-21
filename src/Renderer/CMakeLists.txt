# engine_render: the backend-agnostic 2D renderer (gfx::IRenderer), its SDL3
# backend and, with ENGINE_RENDERER_BGFX=ON, its bgfx backend. Built with
# ENGINE_BUILD_RENDERER=ON (the default); vcpkg then installs the "renderer"
# feature of the manifest (SDL3, stb). bgfx comes from cmake/Bgfx.cmake.
# It does NOT depend on the ECS: a RenderSystem in engine_core will bridge the two.
find_package(SDL3 CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Every .cpp below is picked up automatically, as in src/Editor.
file(GLOB_RECURSE RENDER_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# The bgfx units are left out unless the bgfx backend is enabled.
if(NOT ENGINE_RENDERER_BGFX)
  list(FILTER RENDER_SOURCES EXCLUDE REGEX "/(Bgfx[A-Za-z]*|SdlWindowBridge)/")
endif()

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

if(ENGINE_RENDERER_BGFX)
  include(${PROJECT_SOURCE_DIR}/cmake/Bgfx.cmake)

  # shaderc emits C headers at build time; as sources they rebuild on every .sc edit.
  set(RENDER_SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/shaders)
  bgfx_compile_shaders(TYPE VERTEX SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_sprite.sc
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc OUTPUT_DIR ${RENDER_SHADER_DIR}
    PROFILES ${ENGINE_BGFX_SHADER_PROFILES} AS_HEADERS OUT_FILES_VAR RENDER_VERTEX_SHADERS)
  bgfx_compile_shaders(TYPE FRAGMENT SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_sprite.sc
    VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc OUTPUT_DIR ${RENDER_SHADER_DIR}
    PROFILES ${ENGINE_BGFX_SHADER_PROFILES} AS_HEADERS OUT_FILES_VAR RENDER_FRAGMENT_SHADERS)
  target_sources(engine_render PRIVATE ${RENDER_VERTEX_SHADERS} ${RENDER_FRAGMENT_SHADERS})
  target_include_directories(engine_render PRIVATE ${CMAKE_CURRENT_BINARY_DIR}) # generated/shaders/<profile>/*.bin.h

  # Private: no bgfx type appears in the renderer's public headers.
  target_link_libraries(engine_render PRIVATE bgfx bx bimg)
  # Public: consumers (the editor, the tests) compile their bgfx parts only when it is built.
  target_compile_definitions(engine_render PUBLIC ENGINE_RENDERER_BGFX=1)
endif()
