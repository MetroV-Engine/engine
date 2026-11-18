# Visual demos: small SDL3 programs to see engine code at work.
# Built only with -DENGINE_BUILD_DEMOS=ON, which selects the "gui" vcpkg feature (SDL3, Dear ImGui).
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Each demo is built when the engine code it shows exists on the current branch.
if(EXISTS "${PROJECT_SOURCE_DIR}/src/Engine/Common/Physic/Collision/Collision.hpp")
    add_executable(demo_collision Collision/CollisionDemo.cpp)
    target_link_libraries(demo_collision PRIVATE engine_common SDL3::SDL3 imgui::imgui)
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/src/Engine/System/Physics/Friction/FrictionSystem.hpp")
    add_executable(demo_friction Friction/FrictionDemo.cpp)
    target_link_libraries(demo_friction PRIVATE engine_systems SDL3::SDL3 imgui::imgui)
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/src/Engine/System/Physics/Spring/SpringSystem.hpp")
    add_executable(demo_spring Spring/SpringDemo.cpp)
    target_link_libraries(demo_spring PRIVATE engine_systems SDL3::SDL3 imgui::imgui)
endif()

# Playground uses the Engine/Physics API of an earlier branch.
if(EXISTS "${PROJECT_SOURCE_DIR}/src/Engine/Physics/Physics.hpp")
    add_executable(demo_playground Playground/PlaygroundDemo.cpp)
    target_link_libraries(demo_playground PRIVATE engine_systems SDL3::SDL3 imgui::imgui)
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/src/Engine/System/Physics/Physics.hpp")
    add_executable(demo_showcase Showcase/ShowcaseDemo.cpp)
    target_link_libraries(demo_showcase PRIVATE engine_systems SDL3::SDL3 imgui::imgui)
endif()
