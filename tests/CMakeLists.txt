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
	Engine/Common/Math/TestScalar.cpp
	Engine/Common/Math/TestVector.cpp
	Engine/Common/Physic/TestKinematics.cpp
	Engine/Common/Physic/TestImpact.cpp
	Engine/Common/Physic/TestRotation.cpp
	Engine/Common/Physic/Forces/TestDynamics.cpp
	Engine/Common/Physic/Forces/TestGravity.cpp
	Engine/Common/Physic/Forces/TestFriction.cpp
	Engine/Common/Physic/Forces/TestSurface.cpp
	Engine/Common/Physic/Forces/TestSpring.cpp
	Engine/Common/Physic/Forces/TestRadial.cpp
	Engine/Common/Physic/Forces/TestDrag.cpp
	Engine/Common/Physic/Forces/TestBuoyancy.cpp
	Engine/Common/Shapes/TestRectangle.cpp
	Engine/Common/Shapes/TestCircle.cpp
	Engine/Common/Shapes/TestPolygon.cpp
	Engine/Common/Physic/Collision/TestOverlap.cpp
	Engine/Common/Physic/Collision/TestCollide.cpp
	Engine/Common/Physic/Collision/TestRaycast.cpp
	Engine/Common/Physic/Collision/TestSweep.cpp
	Engine/Common/Physic/Collision/TestPolygonQueries.cpp
	Engine/Common/Physic/Collision/TestManifold.cpp
	Engine/Component/Physics/TestCollider.cpp
	Engine/System/Physics/Gravity/TestGravitySystem.cpp
	Engine/System/Physics/Friction/TestFrictionSystem.cpp
	Engine/System/Physics/Spring/TestSpringSystem.cpp
	Engine/System/Physics/Integration/TestIntegrationSystem.cpp
	Engine/System/Physics/Radial/TestRadialFieldSystem.cpp
	Engine/System/Physics/Drag/TestDragSystem.cpp
	Engine/System/Physics/Buoyancy/TestBuoyancySystem.cpp
	Engine/System/Physics/Collision/TestCollisionSystem.cpp
	Engine/System/Physics/Collision/TestCollisionResponseSystem.cpp
	Engine/System/Physics/Character/TestCharacterControllerSystem.cpp
	Engine/System/Physics/TestPhysics.cpp
	Engine/main.cpp
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

	target_link_libraries(tests_engine PRIVATE engine_systems Boost::ut)
	add_test(NAME tests_engine COMMAND tests_engine)
endif()
