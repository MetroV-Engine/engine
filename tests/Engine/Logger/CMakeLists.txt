# Tests of engine_logger. The logger behaves differently with logging on and
# off, so it has several small executables instead of one:
#   Enabled/   tests_logger           Logger.cpp, logging forced on
#   Disabled/  tests_logger_disabled  LoggerDisabled.cpp, logging forced off
#   Switch/    tests_logger_switch    the real engine_logger: on in Debug only
#   Shutdown/  tests_logger_shutdown  logs from a global destructor, under Valgrind
# plus a compile-time check that a wrong argument fails to build with logging off.
# Enabled/ and Disabled/ compile the logger source themselves, so they test the
# same thing whatever the build type.
find_package(Threads REQUIRED)

set(LOGGER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/Engine/Logger)

file(GLOB LOGGER_ENABLED_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Enabled/*.cpp)
add_executable(tests_logger
	${LOGGER_ENABLED_TEST_SOURCES}
	${LOGGER_SOURCE_DIR}/Logger.cpp
)
target_include_directories(tests_logger PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(tests_logger PRIVATE ENGINE_LOG_ENABLED)
target_link_libraries(tests_logger PRIVATE Boost::ut Threads::Threads)
add_test(NAME tests_logger COMMAND tests_logger)

add_executable(tests_logger_disabled
	Disabled/main.cpp
	Disabled/TestDisabled.cpp
	${LOGGER_SOURCE_DIR}/LoggerDisabled.cpp
)
target_include_directories(tests_logger_disabled PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tests_logger_disabled PRIVATE Boost::ut)
add_test(NAME tests_logger_disabled COMMAND tests_logger_disabled)

# Links the real engine_logger, so it sees the definition every engine target gets.
add_executable(tests_logger_switch Switch/TestSwitch.cpp)
target_link_libraries(tests_logger_switch PRIVATE engine_logger Boost::ut)
target_compile_definitions(tests_logger_switch PRIVATE ENGINE_EXPECT_LOG_ENABLED=$<IF:$<CONFIG:Debug>,1,0>)
add_test(NAME tests_logger_switch COMMAND tests_logger_switch)

# Logs from a global destructor at exit. Needs Valgrind to see a use-after-free.
add_executable(tests_logger_shutdown
	Shutdown/main.cpp
	${LOGGER_SOURCE_DIR}/Logger.cpp
)
target_include_directories(tests_logger_shutdown PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(tests_logger_shutdown PRIVATE ENGINE_LOG_ENABLED)
target_link_libraries(tests_logger_shutdown PRIVATE Threads::Threads)

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
	add_executable(logger_type_check_${_logger_case} EXCLUDE_FROM_ALL Disabled/TypeCheck.cpp)
	target_include_directories(logger_type_check_${_logger_case} PRIVATE ${PROJECT_SOURCE_DIR}/src)
	add_test(NAME tests_logger_type_check_${_logger_case}
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target logger_type_check_${_logger_case} --config $<CONFIG>)
endforeach()
target_compile_definitions(logger_type_check_valid PRIVATE LOGGER_VALID_ARGUMENT)
set_tests_properties(tests_logger_type_check_invalid PROPERTIES WILL_FAIL TRUE)
