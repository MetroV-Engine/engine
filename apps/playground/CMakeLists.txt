# The playground game: renderer-wrapper's level and bull, built from ECS
# components and systems, and drawn through gfx::IRenderer.
#  - playground_game: the game itself (components, systems, rendering, level),
#    with no editor, no ImGui and no SDL: tests/Playground links it headless;
#  - playground: the executable opening the editor on the game's world (GUI only).
file(GLOB_RECURSE PLAYGROUND_GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Components/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Input/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Systems/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Level/*.cpp)

add_library(playground_game STATIC ${PLAYGROUND_GAME_SOURCES})
target_include_directories(playground_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(playground_game PUBLIC engine_ecs engine_render)

function(playground_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()
playground_warnings(playground_game)

# The executable needs the editor: built with ENGINE_BUILD_GUI.
if(ENGINE_BUILD_GUI)
  add_executable(playground main.cpp ViewportView/ViewportView.cpp)
  target_link_libraries(playground PRIVATE playground_game engine_editor)
  playground_warnings(playground)
  # Assets live next to the binary: the code resolves them from there, never from the working directory.
  add_custom_command(TARGET playground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:playground>/assets)
endif()
