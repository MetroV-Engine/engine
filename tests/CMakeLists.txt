find_package(ut CONFIG REQUIRED)

set(ECS_TEST_FILES
	ECS/TestEntity.cpp
)

### ADD IT WHEN Engine tests are ready
# set(ENGINE_TEST_FILES
# 	Engine/TestEngine.cpp
# )

add_executable(tests_ecs ${ECS_TEST_FILES})
target_link_libraries(tests_ecs PRIVATE engine_ecs Boost::ut)
add_test(NAME tests_ecs COMMAND tests_ecs)

### ADD IT WHEN Engine tests are ready
# add_executable(tests_engine ${ENGINE_TEST_FILES})
# target_link_libraries(tests_engine PRIVATE Boost::ut)
# add_test(NAME tests_engine COMMAND tests_engine)
