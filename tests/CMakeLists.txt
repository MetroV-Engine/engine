find_package(ut CONFIG REQUIRED)

set(ECS_TEST_FILES
	ECS/TestEntity.cpp
)

set(ENGINE_SOURCE_FILES
	# ADD ENGINE SOURCE FILES HERE
)

set(ENGINE_TEST_FILES
	# ADD ENGINE TEST FILES HERE
)

add_executable(tests_ecs
	${ECS_TEST_FILES}
)
target_link_libraries(tests_ecs PRIVATE engine_ecs Boost::ut)
add_test(NAME tests_ecs COMMAND tests_ecs)

if(ENGINE_TEST_FILES)
	add_executable(tests_engine
		${ENGINE_TEST_FILES}
		${ENGINE_SOURCE_FILES}
	)

	target_link_libraries(tests_engine PRIVATE engine_ecs Boost::ut)
	add_test(NAME tests_engine COMMAND tests_engine)
endif()
