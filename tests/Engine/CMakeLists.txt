# Tests of engine_core. Every .cpp here is a test (or its support); main.cpp
# calls the run_* functions. Headless: no window, no editor.
file(GLOB_RECURSE ENGINE_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(tests_engine ${ENGINE_TEST_SOURCES})
target_include_directories(tests_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests_engine PRIVATE engine_core Boost::ut)
target_compile_definitions(tests_engine PRIVATE ENGINE_TESTING)
add_test(NAME tests_engine COMMAND tests_engine)
