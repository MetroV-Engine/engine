cmake_minimum_required(VERSION 3.20)
project(Components VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Collect all component source files recursively from Entity directory
file(GLOB_RECURSE COMPONENT_SOURCES
    "Entity/**/*.cpp"
)

# Collect all component header files recursively from Entity directory
file(GLOB_RECURSE COMPONENT_HEADERS
    "Entity/**/*.hpp"
)

# Create the static library
add_library(Components STATIC
    ${COMPONENT_SOURCES}
    ${COMPONENT_HEADERS}
    ComponentsInit.cpp
    ComponentsInit.hpp
        Entity/Base/Collider/Collider.cpp
        Entity/Base/Collider/Collider.hpp
        System/Base/CollisionSys/CollisionSys.cpp
        System/Base/CollisionSys/CollisionSys.hpp
)

# Include directories
target_include_directories(Components PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/ECS/Component
    ${CMAKE_SOURCE_DIR}/ECS/Include
)

# Link with ECS library
target_link_libraries(Components PUBLIC ECS)

