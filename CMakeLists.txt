cmake_minimum_required(VERSION 3.24)

project(GUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(gui
    main.cpp
    ${GUI_SOURCES}
)

target_include_directories(gui PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(gui PRIVATE
    SDL3::SDL3
    imgui::imgui
)
