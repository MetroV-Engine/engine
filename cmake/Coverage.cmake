# Code coverage (GCC / Clang only). Included by the root CMakeLists.txt after
# project(), so that the flags apply to every target defined afterwards.
if(ENGINE_ENABLE_COVERAGE AND NOT ENGINE_BUILD_TESTS)
  message(FATAL_ERROR "ENGINE_ENABLE_COVERAGE requires ENGINE_BUILD_TESTS=ON")
endif()

if(ENGINE_ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
  else()
    message(FATAL_ERROR "Coverage requires GCC or Clang")
  endif()
endif()
