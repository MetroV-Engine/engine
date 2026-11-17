# One folder per module, mirroring src/. One test executable per module.
find_package(ut CONFIG REQUIRED)

add_subdirectory(ECS)
# add_subdirectory(Renderer) # once the renderer has tests
# add_subdirectory(Engine)    # with src/Engine
if(ENGINE_BUILD_GUI)
  add_subdirectory(Editor)
endif()
