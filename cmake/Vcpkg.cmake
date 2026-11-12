# vcpkg setup. Must be included BEFORE project(): the toolchain and the
# manifest features are read at that point. Only the root CMakeLists.txt
# includes this file; modules just call find_package().
#
# CMake presets already pass CMAKE_TOOLCHAIN_FILE; without a preset, the vcpkg
# submodule is used automatically (git submodule update --init).

# The editor's dependencies (SDL3, Dear ImGui) are the "gui" manifest feature.
if(ENGINE_BUILD_GUI)
  list(APPEND VCPKG_MANIFEST_FEATURES "gui")
endif()

# Linux: overlay triplet working around libsystemd failing on recent headers.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND NOT DEFINED VCPKG_OVERLAY_TRIPLETS)
  set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/triplets")
endif()

set(_ENGINE_VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../vcpkg")
cmake_path(NORMAL_PATH _ENGINE_VCPKG_ROOT)
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
           BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
      file(REAL_PATH "${_ENGINE_VCPKG_TOOLCHAIN}" _engine_repo_toolchain)
      if(_engine_resolved_toolchain STREQUAL _engine_repo_toolchain)
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
