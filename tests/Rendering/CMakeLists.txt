# Tests of engine_render. Not built yet: enable together with src/Rendering
# (and add_subdirectory(Rendering) in tests/CMakeLists.txt).

# file(GLOB_RECURSE RENDER_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# add_executable(tests_render ${RENDER_TEST_SOURCES})
# target_link_libraries(tests_render PRIVATE engine_render Boost::ut)
# add_test(NAME tests_render COMMAND tests_render)
