cmake_minimum_required(VERSION 3.24)

# Manifest mode: use vcpkg when the submodule is present (clone with: git submodule update --init).
set(_ENGINE_VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
set(_ENGINE_VCPKG_TOOLCHAIN "${_ENGINE_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(EXISTS "${_ENGINE_VCPKG_TOOLCHAIN}")
  set(_engine_use_repo_vcpkg FALSE)
  if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(_engine_use_repo_vcpkg TRUE)
  else()
    if(CMAKE_TOOLCHAIN_FILE STREQUAL "")
      message(FATAL_ERROR
              "CMAKE_TOOLCHAIN_FILE is defined but empty. Please provide a valid toolchain file path.")
    elseif(EXISTS "${CMAKE_TOOLCHAIN_FILE}")
      file(REAL_PATH "${CMAKE_TOOLCHAIN_FILE}" _engine_resolved_toolchain
           BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
      if(_engine_resolved_toolchain STREQUAL "${_ENGINE_VCPKG_TOOLCHAIN}")
        set(_engine_use_repo_vcpkg TRUE)
      endif()
    else()
      message(FATAL_ERROR
              "CMAKE_TOOLCHAIN_FILE is defined but does not exist: ${CMAKE_TOOLCHAIN_FILE}")
    endif()
  endif()

  if(_engine_use_repo_vcpkg)
    if(NOT EXISTS "${_ENGINE_VCPKG_ROOT}/vcpkg" AND NOT EXISTS "${_ENGINE_VCPKG_ROOT}/vcpkg.exe")
      message(STATUS "vcpkg executable not found; running bootstrap (one-time)...")
      if(WIN32)
        execute_process(
          COMMAND "${_ENGINE_VCPKG_ROOT}/bootstrap-vcpkg.bat"
          WORKING_DIRECTORY "${_ENGINE_VCPKG_ROOT}"
          COMMAND_ERROR_IS_FATAL ANY)
      else()
        execute_process(
          COMMAND "${_ENGINE_VCPKG_ROOT}/bootstrap-vcpkg.sh"
          WORKING_DIRECTORY "${_ENGINE_VCPKG_ROOT}"
          COMMAND_ERROR_IS_FATAL ANY)
      endif()
    endif()
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
      set(CMAKE_TOOLCHAIN_FILE "${_ENGINE_VCPKG_TOOLCHAIN}"
          CACHE STRING "Vcpkg toolchain file")
    endif()
  endif()
endif()

project(engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENGINE_BUILD_TESTS "Build unit tests" OFF)
option(ENGINE_BUILD_APP "Build the engine executable" ON)
option(ENGINE_ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENGINE_ENABLE_COVERAGE AND NOT ENGINE_BUILD_TESTS)
  message(FATAL_ERROR
          "ENGINE_ENABLE_COVERAGE requires ENGINE_BUILD_TESTS=ON")
endif()

if(ENGINE_ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
  else()
    message(FATAL_ERROR "Coverage requires GCC or Clang")
  endif()
endif()

add_subdirectory(src/ECS)

if(ENGINE_BUILD_APP)
  add_executable(engine
    main.cpp)

  target_link_libraries(engine PRIVATE engine_ecs)
endif()

if(ENGINE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
