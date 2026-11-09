# Engine common code (src/Engine/Common): math and physics formulas.
# Header-only for now, so an INTERFACE target that only carries the include
# path: link it and include "Engine/Common/Math/Math.hpp" or
# "Engine/Common/Physic/Physic.hpp".
add_library(engine_common INTERFACE)

target_include_directories(engine_common
	INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Engine components (src/Engine/Component) and ECS systems (src/Engine/System),
# built on the common formulas and the ECS. Header-only as well.
add_library(engine_systems INTERFACE)

target_link_libraries(engine_systems
	INTERFACE
	engine_common
	engine_ecs
)
