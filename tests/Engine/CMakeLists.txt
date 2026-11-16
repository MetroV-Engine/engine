# Tests of engine_core. Not built yet: enable together with src/Engine
# (and add_subdirectory(Engine) in tests/CMakeLists.txt).

# file(GLOB_RECURSE ENGINE_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# add_executable(tests_engine ${ENGINE_TEST_SOURCES})
# target_link_libraries(tests_engine PRIVATE engine_core Boost::ut)
# add_test(NAME tests_engine COMMAND tests_engine)
