cmake_minimum_required(VERSION 3.20)
project(Proto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL3
find_package(SDL3 REQUIRED CONFIG)

# Find Mono
find_package(PkgConfig REQUIRED)
pkg_check_modules(MONO REQUIRED mono-2)

# bgfx (GPU rendering for game window)
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
add_subdirectory(External/bgfx.cmake)
include(External/bgfx.cmake/cmake/bgfxToolUtils.cmake)

# Set ImGui directory
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/External/imgui" CACHE PATH "ImGui directory")

# Add ECS library
add_subdirectory(ECS)
add_subdirectory(GameStructure/Components)

# ImGui + SDL3 backend sources
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

add_library(ImGui_SDL3 STATIC ${IMGUI_SOURCES})
target_include_directories(ImGui_SDL3 PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)
target_link_libraries(ImGui_SDL3 PUBLIC SDL3::SDL3)

# Compile bgfx shaders
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")
set(BGFX_SHADER_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/External/bgfx.cmake/bgfx/src")
bgfx_compile_shaders(
    TYPE VERTEX
    SHADERS "${CMAKE_SOURCE_DIR}/shaders/src/vs_color.sc"
    VARYING_DEF "${CMAKE_SOURCE_DIR}/shaders/src/varying.def.sc"
    OUTPUT_DIR "${SHADER_OUTPUT_DIR}"
    INCLUDE_DIRS "${BGFX_SHADER_INCLUDE_DIR}"
    OUT_FILES_VAR VS_COLOR_OUTPUTS
)
bgfx_compile_shaders(
    TYPE FRAGMENT
    SHADERS "${CMAKE_SOURCE_DIR}/shaders/src/fs_color.sc"
    VARYING_DEF "${CMAKE_SOURCE_DIR}/shaders/src/varying.def.sc"
    OUTPUT_DIR "${SHADER_OUTPUT_DIR}"
    INCLUDE_DIRS "${BGFX_SHADER_INCLUDE_DIR}"
    OUT_FILES_VAR FS_COLOR_OUTPUTS
)
add_compile_definitions(SHADER_OUTPUT_DIR="${SHADER_OUTPUT_DIR}")

include_directories(
        "${CMAKE_SOURCE_DIR}/GameStructure/Components"
        "${CMAKE_SOURCE_DIR}/Editor"
        "${CMAKE_SOURCE_DIR}/Editor/Core"
        "${CMAKE_SOURCE_DIR}/External/imgui"
        "${CMAKE_SOURCE_DIR}/External/imgui/backends"
        "${CMAKE_SOURCE_DIR}/External"
        "${CMAKE_SOURCE_DIR}/ECS/Component"
)

# Recursively gather every .cpp/.hpp under Editor/UI
file(GLOB_RECURSE EDITOR_UI_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Editor/UI/*.cpp"
    "${CMAKE_SOURCE_DIR}/Editor/UI/*.hpp"
)

add_executable(Proto
        main.cpp

        # Game runtime
        GameStructure/Core/Core.cpp
        GameStructure/Core/Core.hpp
        GameStructure/Core/PlatformUtils.cpp
        GameStructure/Core/PlatformUtils.hpp

        # Game components / entities
        GameStructure/Components/Entity/Base/Position/Position.cpp
        GameStructure/Components/Entity/Base/Position/Position.hpp
        GameStructure/Components/Entity/Base/Velocity/Velocity.cpp
        GameStructure/Components/Entity/Base/Velocity/Velocity.hpp
        GameStructure/Components/Entity/Pong/Pad/Pad.cpp
        GameStructure/Components/Entity/Pong/Pad/Pad.hpp
        GameStructure/Components/Entity/Base/Bounds/Bounds.cpp
        GameStructure/Components/Entity/Base/Bounds/Bounds.hpp
        GameStructure/Components/Entity/Base/Controlable/Controlable.cpp
        GameStructure/Components/Entity/Base/Controlable/Controlable.hpp
        ECS/Component/Entity/Moves.hpp
        GameStructure/Components/System/Pong/ControlsSys/ControlsSys.cpp
        GameStructure/Components/System/Pong/ControlsSys/ControlsSys.hpp
        GameStructure/Components/System/Pong/MovementSys/MovementSys.cpp
        GameStructure/Components/System/Pong/MovementSys/MovementSys.hpp
        GameStructure/Components/System/Pong/DrawableSys/DrawableSys.cpp
        GameStructure/Components/System/Pong/DrawableSys/DrawableSys.hpp
        GameStructure/Components/Entity/Pong/Ball/Ball.cpp
        GameStructure/Components/Entity/Pong/Ball/Ball.hpp
        GameStructure/Components/System/Pong/BallSys/BallSys.cpp
        GameStructure/Components/System/Pong/BallSys/BallSys.hpp
        GameStructure/Components/System/Base/CollisionSys/CollisionSys.cpp
        GameStructure/Components/System/Base/CollisionSys/CollisionSys.hpp
        GameStructure/Components/Entity/GameEntities.hpp
        GameStructure/Components/System/GameSystems.hpp
        GameStructure/Components/Entity/Base/Script/ScriptComponent.hpp
        GameStructure/Components/Entity/Base/NativeScript/NativeScriptComponent.hpp
        GameStructure/Components/System/Base/NativeScriptSys/NativeScriptSystem.hpp
        GameStructure/Components/System/Base/NativeScriptSys/NativeScriptSystem.cpp

        # Editor UI tree (recursive)
        ${EDITOR_UI_SOURCES}

        # Editor non-UI utilities
        Editor/Core/ProjectContext.hpp
        Editor/Core/ProjectContext.cpp
        Editor/Core/ValueRenderer/ValueRenderer.hpp
        Editor/Core/ValueRenderer/ValueRenderer.cpp
        Editor/Core/Scripting/ScriptEngine.hpp
        Editor/Core/Scripting/ScriptEngine.cpp
        Editor/Core/Scripting/FileWatcher.hpp
        Editor/Core/Scripting/FileWatcher.cpp
        Editor/Save/SaveManager/SaveManager.cpp
        Editor/Save/SaveManager/SaveManager.hpp

        # ECS reflection
        ECS/Component/Reflection/FieldDescriptor.hpp
)
# Shader outputs as dependencies (triggers compilation before linking)
add_custom_target(compile_shaders DEPENDS ${VS_COLOR_OUTPUTS} ${FS_COLOR_OUTPUTS})
add_dependencies(Proto compile_shaders)

# Link libraries
target_link_libraries(Proto PRIVATE
        ECS
        Components
        ImGui_SDL3
        SDL3::SDL3
        bgfx
        bx
        bimg
        ${MONO_LIBRARIES}
)

target_include_directories(Proto PRIVATE
        ${ECS_INCLUDE_DIRS}
        ${MONO_INCLUDE_DIRS}
        "${CMAKE_SOURCE_DIR}/GameStructure/Core"
        "${CMAKE_SOURCE_DIR}"
)

add_compile_definitions(PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")
