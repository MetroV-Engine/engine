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
	Logger/TestFilter.cpp
	Logger/TestRingBuffer.cpp
	Logger/TestCapacity.cpp
	Logger/TestOutput.cpp
	Logger/TestConcurrency.cpp
	Logger/TestWriteFailure.cpp
	Logger/TestOutOfMemory.cpp
	Logger/TestMacro.cpp
)

add_executable(tests_logger
	${LOGGER_TEST_FILES}
	${CMAKE_CURRENT_SOURCE_DIR}/../src/Core/Logger/Logger.cpp
)
target_include_directories(tests_logger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_compile_definitions(tests_logger PRIVATE ENGINE_LOG_ENABLED)
target_link_libraries(tests_logger PRIVATE Boost::ut Threads::Threads)
add_test(NAME tests_logger COMMAND tests_logger)

# The same Logger.cpp compiled without ENGINE_LOG_ENABLED: what a Release build gets.
add_executable(tests_logger_disabled
	LoggerDisabled/main.cpp
	LoggerDisabled/TestDisabled.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/Core/Logger/Logger.cpp
)
target_include_directories(tests_logger_disabled PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(tests_logger_disabled PRIVATE Boost::ut)
add_test(NAME tests_logger_disabled COMMAND tests_logger_disabled)

# Links the real engine_logger, so it sees the definition every engine target gets.
add_executable(tests_logger_switch
	LoggerSwitch/TestSwitch.cpp
)
target_link_libraries(tests_logger_switch PRIVATE engine_logger Boost::ut)
target_compile_definitions(tests_logger_switch PRIVATE ENGINE_EXPECT_LOG_ENABLED=$<IF:$<CONFIG:Debug>,1,0>)
add_test(NAME tests_logger_switch COMMAND tests_logger_switch)

# Logs from a global destructor at exit. Needs Valgrind to see a use-after-free.
add_executable(tests_logger_shutdown
	LoggerShutdown/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/Core/Logger/Logger.cpp
)
target_include_directories(tests_logger_shutdown PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_compile_definitions(tests_logger_shutdown PRIVATE ENGINE_LOG_ENABLED)

find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
	add_test(NAME tests_logger_shutdown
		COMMAND ${VALGRIND_EXECUTABLE} --error-exitcode=1 $<TARGET_FILE:tests_logger_shutdown>)
else()
	message(STATUS "Valgrind not found: tests_logger_shutdown is built but not run")
endif()

# Compile-time check, logging off: the same call must build with a text message
# and must fail to build with an int, exactly as in Debug.
foreach(_logger_case valid invalid)
	add_executable(logger_type_check_${_logger_case} EXCLUDE_FROM_ALL LoggerDisabled/TypeCheck.cpp)
	target_include_directories(logger_type_check_${_logger_case} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
	add_test(NAME tests_logger_type_check_${_logger_case}
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target logger_type_check_${_logger_case} --config $<CONFIG>)
endforeach()
target_compile_definitions(logger_type_check_valid PRIVATE LOGGER_VALID_ARGUMENT)
set_tests_properties(tests_logger_type_check_invalid PROPERTIES WILL_FAIL TRUE)
