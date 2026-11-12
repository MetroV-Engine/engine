# Tests of engine_editor. Every .cpp here is a test (or its support); main.cpp
# calls the run_* functions. The editor runs headless (SDL "dummy" video
# driver, "software" renderer) in a temporary working directory, where it
# writes its imgui.ini, shortcuts.ini and log.
file(GLOB_RECURSE EDITOR_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(tests_editor ${EDITOR_TEST_SOURCES})
target_include_directories(tests_editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests_editor PRIVATE engine_editor Boost::ut)
target_compile_definitions(tests_editor PRIVATE ENGINE_TESTING)
add_test(NAME tests_editor COMMAND tests_editor)
