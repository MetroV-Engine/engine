find_package(ut CONFIG REQUIRED)

set(ECS_TEST_FILES
	ECS/Core/TestRegistry.cpp
	ECS/Core/TestCommandBuffer.cpp
	ECS/Core/Entity/TestEntity.cpp
	ECS/Core/Entity/TestEntityManager.cpp
	ECS/Core/Entity/TestEntityList.cpp
	ECS/Core/Component/TestComponentId.cpp
	ECS/Core/Component/TestComponentStorage.cpp
	ECS/Core/Component/TestComponentPool.cpp
	ECS/Core/Component/TestComponentOps.cpp
	ECS/Core/Component/TestSignatureTable.cpp
	ECS/Core/Component/TestComponentStore.cpp
	ECS/Core/Resource/TestResourceId.cpp
	ECS/Core/Resource/TestResourceHolder.cpp
	ECS/Core/Resource/TestResourceStore.cpp
	ECS/Core/Query/TestZipper.cpp
	ECS/Core/Query/TestZipperDriver.cpp
	ECS/Core/Query/TestView.cpp
	ECS/Core/System/TestSystemManager.cpp
	ECS/Reflection/TestDescriptor.cpp
	ECS/Reflection/TestTypeRegistry.cpp
	ECS/Reflection/TestContainers.cpp
	ECS/Reflection/TestPrettyPrint.cpp
	ECS/Reflection/TestRegistryInspector.cpp
	ECS/Reflection/TestComponentCatalog.cpp
	ECS/Reflection/Unknown/Treasure.cpp
	ECS/Reflection/Unknown/TestUnknownType.cpp
	main.cpp
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
target_compile_definitions(tests_ecs PRIVATE ENGINE_TESTING)
add_test(NAME tests_ecs COMMAND tests_ecs)

if(ENGINE_TEST_FILES)
	add_executable(tests_engine
		${ENGINE_TEST_FILES}
		${ENGINE_SOURCE_FILES}
	)

	target_link_libraries(tests_engine PRIVATE engine_ecs Boost::ut)
	add_test(NAME tests_engine COMMAND tests_engine)
endif()
