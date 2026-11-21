# bgfx, bx and bimg through bgfx.cmake, plus bgfx_compile_shaders(). Included
# by src/Renderer when ENGINE_RENDERER_BGFX is ON. The first build compiles
# bgfx's shader compiler (shaderc, with tint/glslang/spirv-opt): it is slow.
include(FetchContent)

# Only bgfx, bx, bimg and shaderc: no examples, no other tools, no install.
set(BGFX_BUILD_EXAMPLES OFF)
set(BGFX_BUILD_TESTS OFF)
set(BGFX_INSTALL OFF)
set(BGFX_CUSTOM_TARGETS OFF)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF)
set(BGFX_BUILD_TOOLS_TEXTURE OFF)
set(BGFX_BUILD_TOOLS_BIN2C OFF)   # shaderc --bin2c covers the embedded headers

FetchContent_Declare(
  bgfx
  GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
  GIT_TAG        v1.161.9495-578  # pinned: newer releases changed the init API
  GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(bgfx)

# bgfx.cmake force-enables example-common along with its tools; nothing links it.
if(TARGET example-common)
  set_target_properties(example-common PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

# Only set by bgfx.cmake's installed config, not under FetchContent.
set(BGFX_SHADER_INCLUDE_PATH ${bgfx_SOURCE_DIR}/bgfx/src)
include(${bgfx_SOURCE_DIR}/cmake/bgfxToolUtils.cmake)

# Each host compiles shaders for its platform's default renderer: D3D11 shaders
# need the Windows compiler, Metal ones run on macOS only.
if(WIN32)
  set(ENGINE_BGFX_SHADER_PROFILES s_5_0 spirv)
elseif(APPLE)
  set(ENGINE_BGFX_SHADER_PROFILES metal)
else()
  set(ENGINE_BGFX_SHADER_PROFILES 430 spirv)
endif()
