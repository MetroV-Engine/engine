# Every .cpp here is a test (or its support); main.cpp calls the run_* functions.
# Tests the header-only engine_common and engine_systems (src/Engine).
file(GLOB_RECURSE ENGINE_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(tests_engine ${ENGINE_TEST_SOURCES})
target_link_libraries(tests_engine PRIVATE engine_systems Boost::ut)
add_test(NAME tests_engine COMMAND tests_engine)
