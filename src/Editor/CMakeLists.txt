# Editor GUI, built when ENGINE_BUILD_GUI is ON (see the root CMakeLists.txt).

find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Every .cpp under src/ is picked up automatically; CONFIGURE_DEPENDS re-runs
# the glob at build time so new files are detected without re-running cmake.
file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(gui
    main.cpp
    ${GUI_SOURCES}
)

target_include_directories(gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(gui PRIVATE
    engine_ecs
    SDL3::SDL3
    imgui::imgui
)

# Next to the other executables, at the root of the build directory: ./build/gui
set_target_properties(gui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
