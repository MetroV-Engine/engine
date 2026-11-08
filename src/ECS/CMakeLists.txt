add_library(engine_ecs INTERFACE)

target_include_directories(engine_ecs
	INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Sources are UTF-8: MSVC would read them in the Windows code page otherwise.
# Set here so every target of the project gets it (they all link the ECS).
target_compile_options(engine_ecs
	INTERFACE
	$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/utf-8>
)
