# The engine executable, without the editor. Will link engine_core
# once src/Engine is enabled.
add_executable(engine main.cpp)
target_link_libraries(engine PRIVATE engine_ecs)
