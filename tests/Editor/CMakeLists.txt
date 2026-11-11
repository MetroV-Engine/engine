# Tests of the editor on an ECS world:
#   ctest --test-dir build/editor -R tests_editor --output-on-failure
file(GLOB_RECURSE EDITOR_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(tests_editor ${EDITOR_TEST_SOURCES})
target_include_directories(tests_editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests_editor PRIVATE engine_editor Boost::ut)

add_test(NAME tests_editor COMMAND tests_editor)
# One test runs the real editor (offscreen): fail rather than hang if it
# never quits.
set_tests_properties(tests_editor PROPERTIES TIMEOUT 120)
