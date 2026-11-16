# A small game opening the editor on its own world: the reference for
# connecting game code to the editor.
add_executable(gui_ecs_example EcsExample.cpp)
target_link_libraries(gui_ecs_example PRIVATE engine_editor)

