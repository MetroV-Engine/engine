# Renderer

The engine's 2D renderer: a backend-agnostic interface, `gfx::IRenderer`, and its backends. Engine and
editor code draws through the interface, so the backend can be swapped without touching them. The
backend is picked once, at startup, with `gfx::makeRenderer` (`RendererFactory`).

It comes from [MetroV-Engine/renderer-wrapper](https://github.com/MetroV-Engine/renderer-wrapper)
(commit `7235a70`), where its history stays.

## Backends

| Backend | State |
| --- | --- |
| `Backend::Sdl3Cpu` (`SdlRenderer`) | Built. Draws through an `SDL_Renderer` it creates on the window, or offscreen (see below). |
| `Backend::Null` (`NullRenderer`) | Built. Records the draw calls without a window: for tests. |
| `Backend::Bgfx` (`BgfxRenderer`) | Built with `ENGINE_RENDERER_BGFX` (see Build). Draws through bgfx on the window, or offscreen. Without the switch, `makeRenderer` logs an error and returns `nullptr`. |

## Offscreen rendering

`gfx::makeOffscreenRenderer(backend, sdlRenderer, width, height)` returns an `IOffscreenRenderer`. It draws
into a texture on a renderer that someone else owns, such as the editor's, instead of presenting to a
window. `outputTexture()` gives that texture as an opaque handle (an `SDL_Texture*` for SDL), ready for
`ImGui::Image`. Each frame puts the borrowed renderer's target, blend mode and texture address mode back
as they were.

`gfx::makeOffscreenRenderer(bgfxContext, width, height)` is the bgfx equivalent. It draws into a bgfx
framebuffer of that size, recreated on `resize`, on a view of its own. `outputTexture()` gives the
framebuffer's colour texture in the id encoding of `BgfxContext/BgfxTextureId.hpp` (`idx + 1`, so 0 means
none), the one the editor's bgfx ImGui backend decodes.

### One bgfx per process

bgfx is global, so one `BgfxContext` owns it: `bgfx::init` on an SDL window, `reset` on resize, `frame`,
`shutdown`, and the view ids (game views from 0 up, `BgfxContext::UiView` = 255 for the UI). Only one can
exist at a time. In window mode (`makeRenderer(Backend::Bgfx, ...)`) the renderer owns its context and calls
`frame()` itself. An offscreen renderer borrows the context of whoever owns it, such as the editor's UI,
and never calls `frame()`. Under SDL's `dummy` video driver, or with no window, the context starts bgfx's
Noop device: it draws nothing, which is how the tests run bgfx without a GPU.

## Build

`engine_render` is built when `ENGINE_BUILD_RENDERER` is ON (the default). vcpkg then installs the
`renderer` feature of the manifest (SDL3 and stb). Like the ECS, it is included from `src/`:
`#include "Renderer/IRenderer.hpp"`. It does not depend on the ECS.

`ENGINE_RENDERER_BGFX` (OFF by default) adds the bgfx backend. It fetches bgfx.cmake (pinned in
`cmake/Bgfx.cmake`) and builds bgfx's shader compiler, which takes a long time on the first build. The
sprite shaders in `shaders/` are compiled for each platform's default renderer: GLSL 430 and SPIR-V on
Linux, Direct3D 11 and SPIR-V on Windows, Metal on macOS. Use the `bgfx` and `bgfx-tests` presets. With the
switch OFF nothing about the build changes.

`./build/bgfx/playground --backend=bgfx` runs the playground with both the editor UI and the game drawn by
bgfx. The manual workflow `.github/workflows/bgfx-compile.yml` compiles and tests this build on Linux,
Windows and macOS: start it from the Actions tab, or push to the `bgfx-check` branch.

Its tests are in `tests/Renderer` (`tests_render`). They run headless (SDL's software renderer and its `dummy`
video driver) and count towards the 90 % line coverage CI requires over `src/`. The bgfx files and
`SdlWindowBridge` are only built with `ENGINE_RENDERER_BGFX=ON`, so the `coverage` preset does not measure them.

## Layout

Each unit with a `.cpp` lives in a folder of its own name with its header
(`Font/Font.hpp`, `Font/Font.cpp`). Header-only files (`IRenderer.hpp`, `RendererTypes.hpp`) stay at the
root of the module. New `.cpp` files are picked up by CMake automatically. Public headers include no SDL
or bgfx header: `IRenderer.hpp` is backend-free, and `RendererFactory.hpp` / `SdlRenderer.hpp` only
forward-declare the SDL and bgfx types they take (`SDL_Window`, `BgfxContext`...).
