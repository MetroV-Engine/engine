# One folder per module, mirroring src/. One test executable per module.
find_package(ut CONFIG REQUIRED)

add_subdirectory(ECS)
# add_subdirectory(Rendering) # with src/Rendering
# add_subdirectory(Engine)    # with src/Engine
if(ENGINE_BUILD_GUI)
  add_subdirectory(Editor)
endif()

find_package(Threads REQUIRED)

# Logger tests compile Logger.cpp themselves with logging forced on, so they
# test the same thing whatever the build type.
set(LOGGER_TEST_FILES
	Logger/main.cpp
	Logger/TestSend.cpp
)

add_executable(tests_logger
	${LOGGER_TEST_FILES}
	${CMAKE_CURRENT_SOURCE_DIR}/../src/Core/Logger/Logger.cpp
)
target_include_directories(tests_logger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_compile_definitions(tests_logger PRIVATE ENGINE_LOG_ENABLED)
target_link_libraries(tests_logger PRIVATE Boost::ut Threads::Threads)
add_test(NAME tests_logger COMMAND tests_logger)
