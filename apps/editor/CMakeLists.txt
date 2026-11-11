# The standalone editor: an empty ECS world, or the built-in demo (--demo).
add_executable(editor main.cpp)
target_link_libraries(editor PRIVATE engine_editor)

