# Tests of engine_render (Boost.ut). Not built yet: once there are tests,
# uncomment add_subdirectory(Renderer) in tests/CMakeLists.txt and remove
# ENGINE_BUILD_RENDERER=OFF from the "coverage" preset.

# file(GLOB_RECURSE RENDER_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# add_executable(tests_render ${RENDER_TEST_SOURCES})
# target_link_libraries(tests_render PRIVATE engine_render Boost::ut)
# add_test(NAME tests_render COMMAND tests_render)
