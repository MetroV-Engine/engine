# engine_editor: the editor GUI, as a library. Built with ENGINE_BUILD_GUI=ON
# (cmake --preset editor). A program that has an ECS world (the engine, a
# game) links it to open the editor on that world:
#   target_link_libraries(my_game PRIVATE engine_editor)
# The executables are in apps/editor and apps/examples, the tests in tests/Editor.
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Every .cpp under src/ is picked up automatically; CONFIGURE_DEPENDS re-runs
# the glob at build time so new files are detected without re-running cmake.
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(engine_editor STATIC ${EDITOR_SOURCES})

# Public: the editor's headers include the ECS, SDL3 and Dear ImGui.
# Will link engine_core instead of engine_ecs once src/Engine is enabled.
target_include_directories(engine_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(engine_editor PUBLIC
    engine_ecs
    SDL3::SDL3
    imgui::imgui
)
