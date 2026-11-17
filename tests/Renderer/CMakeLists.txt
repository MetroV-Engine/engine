# Tests of engine_render (Boost.ut). Not built by the "coverage" preset, which
# turns the renderer off. The offscreen tests draw with SDL's software renderer
# on a plain surface, so they need no window and no GPU.
find_package(SDL3 CONFIG REQUIRED)

file(GLOB_RECURSE RENDER_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(tests_render ${RENDER_TEST_SOURCES})
target_link_libraries(tests_render PRIVATE engine_render SDL3::SDL3 Boost::ut)
add_test(NAME tests_render COMMAND tests_render)
