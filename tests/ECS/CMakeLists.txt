# Every .cpp here is a test (or its support); main.cpp calls the run_* functions.
file(GLOB_RECURSE ECS_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(tests_ecs ${ECS_TEST_SOURCES})
target_link_libraries(tests_ecs PRIVATE engine_ecs Boost::ut)
target_compile_definitions(tests_ecs PRIVATE ENGINE_TESTING)
add_test(NAME tests_ecs COMMAND tests_ecs)
