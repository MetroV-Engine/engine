# One folder per module, mirroring src/. One test executable per module.
find_package(ut CONFIG REQUIRED)

add_subdirectory(ECS)
if(ENGINE_BUILD_RENDERER)
  add_subdirectory(Renderer)
endif()
if(TARGET playground_game)
  add_subdirectory(Playground) # the game in apps/playground
endif()
# add_subdirectory(Engine)    # with src/Engine
if(ENGINE_BUILD_GUI)
  add_subdirectory(Editor)
endif()
