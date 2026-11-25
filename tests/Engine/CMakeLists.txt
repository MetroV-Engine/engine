# Tests of engine_core. Every .cpp here is a test (or its support); main.cpp
# calls the run_* functions. Headless: no window, no editor.
file(GLOB_RECURSE ENGINE_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(tests_engine ${ENGINE_TEST_SOURCES})
target_include_directories(tests_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# nlohmann_json: the serialization tests read the text back as JSON to check its structure.
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(tests_engine PRIVATE engine_core nlohmann_json::nlohmann_json Boost::ut)
target_compile_definitions(tests_engine PRIVATE ENGINE_TESTING)
add_test(NAME tests_engine COMMAND tests_engine)
