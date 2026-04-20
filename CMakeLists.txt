cmake_minimum_required(VERSION 3.24)

# Manifest mode: use vcpkg when the submodule is present (clone with: git submodule update --init).
set(_ENGINE_VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
set(_ENGINE_VCPKG_TOOLCHAIN "${_ENGINE_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(EXISTS "${_ENGINE_VCPKG_TOOLCHAIN}")
  set(_engine_use_repo_vcpkg FALSE)
  if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(_engine_use_repo_vcpkg TRUE)
  else()
    file(REAL_PATH "${CMAKE_TOOLCHAIN_FILE}" _engine_resolved_toolchain
         BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    if(_engine_resolved_toolchain STREQUAL "${_ENGINE_VCPKG_TOOLCHAIN}")
      set(_engine_use_repo_vcpkg TRUE)
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
add_executable(engine
        main.cpp)
