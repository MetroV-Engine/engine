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

# Editor tests (src/Editor): built with the editor only, -DENGINE_BUILD_GUI=ON.
# Those that open a window use SDL's offscreen driver: no screen is needed.
set(EDITOR_TEST_FILES
	Editor/ecs/TestEcsBackend.cpp
	Editor/ecs/TestEcsScene.cpp
	Editor/ecs/TestEcsSimulation.cpp
	Editor/ecs/TestEditorOnEcs.cpp
	Editor/ecs/TestEntityDragDrop.cpp
	Editor/scene/TestDemoSceneRenderer.cpp
	Editor/main.cpp
)

if(ENGINE_BUILD_GUI)
	add_executable(tests_editor
		${EDITOR_TEST_FILES}
	)

	# The test helpers are included from tests/Editor: "Support/Logs.hpp".
	target_include_directories(tests_editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Editor)
	target_link_libraries(tests_editor PRIVATE engine_editor Boost::ut)
	add_test(NAME tests_editor COMMAND tests_editor)
	# One test runs the real editor: fail rather than hang if it never quits.
	set_tests_properties(tests_editor PROPERTIES TIMEOUT 120)
endif()
