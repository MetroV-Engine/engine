# Tests of engine_render (Boost.ut). Headless: SDL's software renderer on a plain
# surface, and windows on SDL's "dummy" video driver, so they need no display and no GPU.
find_package(SDL3 CONFIG REQUIRED)

file(GLOB_RECURSE RENDER_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(tests_render ${RENDER_TEST_SOURCES})
target_link_libraries(tests_render PRIVATE engine_render SDL3::SDL3 Boost::ut)
add_test(NAME tests_render COMMAND tests_render)
# Fixtures (the font, a sprite sheet) are read from the source tree, so any build directory finds them.
target_compile_definitions(tests_render PRIVATE RENDER_TEST_ASSETS_DIR="${PROJECT_SOURCE_DIR}/apps/playground/assets")
# The bgfx tests read bgfx's enums and state macros.
if(ENGINE_RENDERER_BGFX)
  target_link_libraries(tests_render PRIVATE bgfx)
endif()
