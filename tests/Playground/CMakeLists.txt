# Tests of the playground game's library (apps/playground). Headless: the
# systems run on a plain ECS world, the rendering on a NullRenderer.
file(GLOB_RECURSE PLAYGROUND_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(tests_playground ${PLAYGROUND_TEST_SOURCES})
target_include_directories(tests_playground PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
find_package(SDL3 CONFIG REQUIRED)
# SDL only writes a test sheet; the game library itself has no SDL.
target_link_libraries(tests_playground PRIVATE playground_game SDL3::SDL3 Boost::ut)
target_compile_definitions(tests_playground PRIVATE
    PLAYGROUND_TEST_FONT_PATH="${PROJECT_SOURCE_DIR}/apps/playground/assets/DejaVuSans.ttf")
add_test(NAME tests_playground COMMAND tests_playground)
