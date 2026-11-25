# runtime: runs a scene file with the engine, without the editor.
#   ./build/<preset>/runtime <scene file> [--fps N]
# Its main names no game: it installs every game of the GameCatalog.
add_executable(runtime main.cpp)
target_link_libraries(runtime PRIVATE engine_core)

# Which games the runtime contains is chosen here only. Until plugins exist,
# a game is linked into the executable (its registrations run before main);
# a plugin loader will later load it at run time instead.
if(TARGET demo_game)
  target_link_libraries(runtime PRIVATE demo_game)
endif()
