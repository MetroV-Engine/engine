# Final executables. Each one links only the module(s) it needs.
# The demo game first: the runtime links it when it exists (until plugins).
if(ENGINE_BUILD_DEMO)
  add_subdirectory(demo)
endif()
add_subdirectory(runtime)

if(ENGINE_BUILD_GUI)
  add_subdirectory(editor)
  add_subdirectory(examples)
endif()
