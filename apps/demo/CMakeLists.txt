# demo_game: a small Sokoban in the terminal, to show how the engine runs a
# game made of data (a scene file) and systems. See README.md.
#
# curses is a prerequisite of this demo only, not a dependency of the
# project: without it, the demo is skipped and the rest builds.
find_package(Curses QUIET)
if(NOT CURSES_FOUND)
  message(STATUS "Demo skipped: curses not found (see apps/demo/README.md)")
  return()
endif()

# An OBJECT library: every object file is linked into the program that uses
# it, none is dropped. Demo.cpp compiles the game's registrations (E_REFLECT,
# E_SYSTEM, E_GAME), which nothing in that program references by name.
# Later, the same sources become a shared library loaded as a plugin.
add_library(demo_game OBJECT Demo.cpp)
# Includes are written from apps/: #include "demo/Demo.hpp".
target_include_directories(demo_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CURSES_INCLUDE_DIRS})
target_link_libraries(demo_game PUBLIC engine_core ${CURSES_LIBRARIES})
