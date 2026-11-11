# One folder per module, mirroring src/. One test executable per module.
find_package(ut CONFIG REQUIRED)

add_subdirectory(ECS)
# add_subdirectory(Rendering) # with src/Rendering
# add_subdirectory(Engine)    # with src/Engine
# add_subdirectory(Editor)    # with ENGINE_BUILD_GUI
