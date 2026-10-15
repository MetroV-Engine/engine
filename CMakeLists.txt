cmake_minimum_required(VERSION 3.24)

project(GUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(gui
    main.cpp
    src/GuiContext.cpp
)

target_link_libraries(gui PRIVATE
    SDL3::SDL3
    imgui::imgui
)
