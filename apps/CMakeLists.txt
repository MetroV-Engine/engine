# Final executables. Each one links only the module(s) it needs.
add_subdirectory(runtime)

if(ENGINE_BUILD_GUI)
  add_subdirectory(editor)
  add_subdirectory(examples)
endif()

# The playground game: needs the renderer; its executable also needs the editor.
if(ENGINE_BUILD_RENDERER)
  add_subdirectory(playground)
endif()
