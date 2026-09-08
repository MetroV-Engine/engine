cmake_minimum_required(VERSION 3.20)
project(ECS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(SDL3 REQUIRED CONFIG)

set(ECS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(ECS_COMPONENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Component)
set(ECS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Src)

set(ECS_SOURCES
        ${ECS_SRC_DIR}/registry.cpp
        Component/IComponent.hpp
        Component/IComponentFactory.hpp
        Component/ComponentRegistry.hpp
        Component/System/ISystem.hpp
        Component/Reflection/SceneSnapshot.cpp
        Component/Reflection/SceneSnapshot.hpp
        Component/SdlTypes.hpp
)

add_library(ECS STATIC ${ECS_SOURCES})

target_include_directories(ECS PUBLIC ${ECS_INCLUDE_DIR} ${ECS_COMPONENT_DIR})
target_link_libraries(ECS PUBLIC SDL3::SDL3)
